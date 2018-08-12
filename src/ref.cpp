// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/ref.h"

#include <script/classtemplate.h>
#include <script/classtemplateinstancebuilder.h>
#include <script/conversions.h>
#include <script/engine.h>
#include <script/interpreter/executioncontext.h>
#include <script/functionbuilder.h>
#include <script/functiontemplate.h>
#include <script/namespace.h>
#include <script/overloadresolution.h>
#include <script/templateargumentdeduction.h>
#include <script/templatebuilder.h>
#include <script/userdata.h>
#include <script/private/engine_p.h>
#include <script/private/value_p.h>

namespace dex
{

namespace callbacks
{

script::Value default_ctor(script::FunctionCall *c)
{
  c->thisObject().impl()->set_ref(nullptr);
  return c->thisObject();
}

script::Value copy_ctor(script::FunctionCall *c)
{
  script::Value other = c->arg(0);
  c->thisObject().impl()->set_ref(other.impl()->data.builtin.ref);
  return c->thisObject();
}

script::Value dtor(script::FunctionCall *c)
{
  c->thisObject().impl()->set_ref(nullptr);
  c->thisObject().impl()->clear();
  return c->thisObject();
}

script::Value get(script::FunctionCall *c)
{
  if (c->thisObject().impl()->data.builtin.ref == nullptr)
    throw std::runtime_error{ "Call to get() on empty Ref" };

  auto self = c->thisObject();
  auto ret = script::Value{ self.impl()->data.builtin.ref };
  return ret;
}

script::Value is_null(script::FunctionCall *c)
{
  return c->engine()->newBool(c->thisObject().impl()->data.builtin.ref == nullptr);
}

script::Value is_valid(script::FunctionCall *c)
{
  return c->engine()->newBool(c->thisObject().impl()->data.builtin.ref != nullptr);
}

script::Value assign(script::FunctionCall *c)
{
  script::Value other = c->arg(1);
  c->thisObject().impl()->set_ref(other.impl()->data.builtin.ref);
  return c->thisObject();
}

script::Value eq(script::FunctionCall *c)
{
  script::Value other = c->arg(1);
  const bool result = c->thisObject().impl()->data.builtin.ref == other.impl()->data.builtin.ref;
  return c->engine()->newBool(result);
}

script::Value cast(script::FunctionCall *c)
{
  script::ValueImpl *ptr = c->thisObject().impl()->data.builtin.ref;
  script::Value ret = c->engine()->construct(c->callee().returnType(), {});
  ret.impl()->set_ref(ptr);
  return ret;
}

}

static script::Class get_ref_instance(const script::Class & type)
{
  using namespace script;

  ClassTemplate ct = type.engine()->getTemplate(Engine::RefTemplate);
  std::vector<TemplateArgument> args;
  args.push_back(TemplateArgument{ Type{ type.id() } });
  return ct.getInstance(args);
}

static void inject_conversions(script::Class & ref, const script::Class & src)
{
  using namespace script;

  if (src.id() == ref.arguments().front().type.baseType().data())
    return;

  // we add a conversion from Ref<T> to Ref<Base>
  ref.Conversion(get_ref_instance(src).id(), callbacks::cast).create();
}

static void inject_conversions_recursively(script::Class & ref, const script::Class & src)
{
  inject_conversions(ref, src);

  if (src.parent().isNull())
    return;

  inject_conversions_recursively(ref, src.parent());
}



void make_ref_deduce(script::TemplateArgumentDeduction &deduc, const script::FunctionTemplate & make_ref, const std::vector<script::TemplateArgument> & targs, const std::vector<script::Type> & itypes)
{
  using namespace script;

  if (targs.size() != 0)
    return deduc.fail();

  std::vector<TemplateArgument> args;
  for (const auto & t : itypes)
  {
    if (t.isConst())
      args.push_back(TemplateArgument{ Type::cref(t.baseType()) });
    else
      args.push_back(TemplateArgument{ Type::ref(t.baseType()) });
  }

  deduc.record_deduction(0, TemplateArgument{ std::move(args) });
}

void make_ref_substitute(script::FunctionBuilder & builder, script::FunctionTemplate make_ref, const std::vector<script::TemplateArgument> & targs)
{
  using namespace script;

  builder.returns(builder.proto.at(0).baseType());
  builder.setStatic();

  for (const auto & p : targs.back().pack->args())
  {
    builder.addParam(p.type);
  }
}


class MakeRefTemplateData : public script::UserData
{
public:
  ~MakeRefTemplateData() = default;

  script::Function target;
  std::vector<script::ConversionSequence> conversions;
  std::vector<script::Type> types;
};

namespace callbacks
{

static script::Value make_ref_template(script::FunctionCall *c)
{
  using namespace script;
  auto data = std::static_pointer_cast<MakeRefTemplateData>(c->callee().data());
  std::vector<Value> args{ c->args().begin(), c->args().end() };
  c->engine()->applyConversions(args, data->types, data->conversions);
  Value node = c->engine()->invoke(data->target, args);
  script::Value result = c->engine()->construct(c->callee().returnType(), {});
  result.impl()->set_ref(node.impl());
  return result;
}

} // namespace callbacks

std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> make_ref_instantitate(script::FunctionTemplate make_ref, script::Function instance)
{
  using namespace script;

  auto data = std::make_shared<MakeRefTemplateData>();

  Class ref_type = make_ref.engine()->getClass(instance.returnType());
  Class target_type = make_ref.engine()->getClass(ref_type.arguments().front().type);

  std::vector<Type> types = instance.prototype().parameters();

  OverloadResolution resol = OverloadResolution::New(make_ref.engine());
  if (!resol.process(target_type.constructors(), types))
    throw TemplateInstantiationError{ "Ref<T>::make() : Could not find valid constructor" };

  data->target = resol.selectedOverload();
  data->conversions = resol.conversionSequence();
  data->types = data->target.prototype().parameters();

  return { callbacks::make_ref_template, data };
}

void register_make_ref_template(script::Class ref_instance)
{
  using namespace script;

  Symbol{ ref_instance }.FunctionTemplate("make")
    .params(TemplateParameter{ TemplateParameter::TypeParameter{}, TemplateParameter::ParameterPack{}, "Args" })
    .setScope(Scope{ ref_instance })
    .deduce(make_ref_deduce)
    .substitute(make_ref_substitute)
    .instantiate(make_ref_instantitate)
    .create();
}



void is_template_deduce(script::TemplateArgumentDeduction &deduc, const script::FunctionTemplate & is_template, const std::vector<script::TemplateArgument> & targs, const std::vector<script::Type> & itypes)
{
  using namespace script;

  if (targs.size() != 1)
    return deduc.fail();
}

void is_template_substitute(script::FunctionBuilder & builder, script::FunctionTemplate is_template, const std::vector<script::TemplateArgument> & targs)
{
  using namespace script;

  builder.returns(Type::Boolean);
  builder.setConst();
}

namespace callbacks
{

static script::Value is_template(script::FunctionCall *c)
{
  using namespace script;
  Value val{ c->thisObject().impl()->data.builtin.ref };
  if(val.isNull())
    return c->engine()->newBool(false);

  if (!val.type().isObjectType())
  {
    return c->engine()->newBool(c->callee().arguments().front().type == val.type());
  }
  
  Class cla = c->engine()->getClass(val.type());
  Class T = c->engine()->getClass(c->callee().arguments().front().type);
  return c->engine()->newBool(cla.inherits(T));
}

} // namespace callbacks

std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> is_template_instantitate(script::FunctionTemplate is_template, script::Function instance)
{
  using namespace script;
  return { callbacks::is_template, nullptr };
}

void register_is_template(script::Class ref_instance)
{
  using namespace script;

  Symbol{ ref_instance }.FunctionTemplate("is")
    .params(TemplateParameter{ TemplateParameter::TypeParameter{}, "T" })
    .setScope(Scope{ ref_instance })
    .deduce(is_template_deduce)
    .substitute(is_template_substitute)
    .instantiate(is_template_instantitate)
    .create();
}



void as_template_deduce(script::TemplateArgumentDeduction &deduc, const script::FunctionTemplate & as_template, const std::vector<script::TemplateArgument> & targs, const std::vector<script::Type> & itypes)
{
  using namespace script;

  if (targs.size() != 1)
    return deduc.fail();
}

void as_template_substitute(script::FunctionBuilder & builder, script::FunctionTemplate as_template, const std::vector<script::TemplateArgument> & targs)
{
  using namespace script;

  ClassTemplate ref_template = as_template.engine()->getTemplate(Engine::RefTemplate);
  Class ref_instance = ref_template.getInstance(targs);
  builder.returns(ref_instance.id());
  builder.setConst();
}

namespace callbacks
{

static script::Value as_template(script::FunctionCall *c)
{
  using namespace script;
  Value val{ c->thisObject().impl()->data.builtin.ref };
  if (!val.type().isObjectType() || !c->callee().arguments().front().type.isObjectType())
  {
    return c->engine()->construct(c->callee().returnType(), {});
  }

  Class cla = c->engine()->getClass(val.type());
  Class T = c->engine()->getClass(c->callee().arguments().front().type);
  if (!cla.inherits(T))
  {
    return c->engine()->construct(c->callee().returnType(), {});
  }

  Value ret = c->engine()->uninitialized(c->callee().returnType());
  ret.impl()->set_ref(val.impl());
  return ret;
}

} // namespace callbacks

std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> as_template_instantitate(script::FunctionTemplate is_ref, script::Function instance)
{
  using namespace script;
  return { callbacks::as_template, nullptr };
}

void register_as_template(script::Class ref_instance)
{
  using namespace script;

  Symbol{ ref_instance }.FunctionTemplate("as")
    .params(TemplateParameter{ TemplateParameter::TypeParameter{}, "T" })
    .setScope(Scope{ ref_instance })
    .deduce(as_template_deduce)
    .substitute(as_template_substitute)
    .instantiate(as_template_instantitate)
    .create();
}


static void fill_ref_instance(script::Class & instance, const script::Class & cla)
{
  using namespace script;

  instance.Constructor(callbacks::default_ctor).create();
  instance.Constructor(callbacks::copy_ctor).params(Type::cref(instance.id())).create();

  instance.newDestructor(callbacks::dtor);

  instance.Method("get", callbacks::get)
    .setConst()
    .returns(Type::ref(cla.id()))
    .create();

  instance.Method("isNull", callbacks::is_null)
    .setConst()
    .returns(Type::Boolean)
    .create();

  instance.Method("isValid", callbacks::is_valid)
    .setConst()
    .returns(Type::Boolean)
    .create();

  instance.Operation(AssignmentOperator, callbacks::assign)
    .returns(Type::ref(instance.id()))
    .params(Type::cref(instance.id())).create();

  instance.Operation(EqualOperator, callbacks::eq)
    .setConst()
    .returns(Type::Boolean)
    .params(Type::cref(instance.id())).create();

  instance.Conversion(Type::ref(cla.id()), callbacks::get)
    .setConst()
    .create();

  register_is_template(instance);
  register_as_template(instance);
  register_make_ref_template(instance);

  inject_conversions_recursively(instance, cla);
}


script::Class ref_template(script::ClassTemplateInstanceBuilder & builder)
{
  /// TODO: should we throw on failure

  using namespace script;

  if (builder.arguments().size() != 1 || builder.arguments().at(0).kind != TemplateArgument::TypeArgument)
    return Class{};

  Type T = builder.arguments().front().type;
  if (!T.isObjectType() || T.isReference() || T.isRefRef())
    return Class{};

  Engine *e = builder.getTemplate().engine();

  Class cla = e->getClass(T);

  Class result = builder.get();

  fill_ref_instance(result, cla);

  return result;
}

void register_ref_template(script::Namespace ns)
{
  using namespace script;

  ClassTemplate ref = Symbol{ ns }.ClassTemplate("Ref")
    .params(TemplateParameter{ TemplateParameter::TypeParameter{}, "T" })
    .setScope(Scope{ ns })
    .setCallback(ref_template)
    .get();
  
  ns.engine()->implementation()->ref_template_ = ref;
}


} // namespace dex