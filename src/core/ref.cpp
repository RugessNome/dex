// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/core/ref.h"

#include <script/castbuilder.h>
#include <script/classtemplate.h>
#include <script/classtemplateinstancebuilder.h>
#include <script/constructorbuilder.h>
#include <script/conversions.h>
#include <script/destructorbuilder.h>
#include <script/engine.h>
#include <script/initialization.h>
#include <script/interpreter/executioncontext.h>
#include <script/functionbuilder.h>
#include <script/functiontemplate.h>
#include <script/namespace.h>
#include <script/operatorbuilder.h>
#include <script/overloadresolution.h>
#include <script/templateargumentdeduction.h>
#include <script/templatebuilder.h>
#include <script/userdata.h>
#include <script/private/engine_p.h>
#include <script/private/value_p.h>

namespace dex
{

ValuePtr::ValuePtr()
  : value(nullptr)
{

}

ValuePtr::ValuePtr(const script::Value& val)
  : value(val.impl())
{
  if (value)
    value->ref += 1;
}

ValuePtr::ValuePtr(const ValuePtr& other)
  : value(other.value)
{
  if (value)
    value->ref += 1;
}

ValuePtr::~ValuePtr()
{
  reset();
}

void ValuePtr::reset()
{
  if (value)
  {
    value->ref -= 1;
    if (value->ref == 0)
      value->engine->destroy(script::Value(value));
    value = nullptr;
  }
}

ValuePtr& ValuePtr::operator=(const ValuePtr& other)
{
  if (other.value == this->value)
  {
    return *this;
  }

  if (value)
  {
    value->ref -= 1;
    if (value->ref == 0)
      value->engine->destroy(script::Value(value));
  }

  value = other.value;
  if (value)
    value->ref += 1;

  return *this;
}

ValuePtr& ValuePtr::operator=(script::ValueImpl* ptr)
{
  if (ptr == this->value)
  {
    return *this;
  }

  if (value)
  {
    value->ref -= 1;
    if (value->ref == 0)
      value->engine->destroy(script::Value(value));
  }

  value = ptr;
  if (value)
    value->ref += 1;

  return *this;
}

bool ValuePtr::operator==(nullptr_t) const
{
  return value == nullptr;
}

bool ValuePtr::operator!=(nullptr_t) const
{
  return value != nullptr;
}

bool operator==(const ValuePtr& lhs, const ValuePtr& rhs)
{
  return lhs.value == rhs.value;
}

namespace callbacks
{

script::Value default_ctor(script::FunctionCall *c)
{
  c->thisObject().init<ValuePtr>();
  return c->thisObject();
}

script::Value copy_ctor(script::FunctionCall *c)
{
  script::Value other = c->arg(1);
  c->thisObject().init<ValuePtr>(script::get<ValuePtr>(other));
  return c->thisObject();
}

script::Value dtor(script::FunctionCall *c)
{
  c->thisObject().destroy<ValuePtr>();
  return c->thisObject();
}

script::Value get(script::FunctionCall *c)
{
  ValuePtr& ptr = script::get<ValuePtr>(c->thisObject());
  if (ptr.value == nullptr)
    throw std::runtime_error{ "Call to get() on empty Ref" };

  auto ret = script::Value{ ptr.value };
  return ret;
}

script::Value is_null(script::FunctionCall *c)
{
  return c->engine()->newBool(script::get<ValuePtr>(c->thisObject()) == nullptr);
}

script::Value reset(script::FunctionCall* c)
{
  ValuePtr& ptr = script::get<ValuePtr>(c->thisObject());
  ptr.reset();
  return script::Value::Void;
}

script::Value is_valid(script::FunctionCall *c)
{
  return c->engine()->newBool(script::get<ValuePtr>(c->thisObject()) != nullptr);
}

script::Value assign(script::FunctionCall *c)
{
  script::get<ValuePtr>(c->thisObject()) = script::get<ValuePtr>(c->arg(1));
  return c->thisObject();
}

script::Value assign_null(script::FunctionCall* c)
{
  script::get<ValuePtr>(c->thisObject()).reset();
  return c->thisObject();
}

script::Value eq(script::FunctionCall *c)
{
  const bool result = (script::get<ValuePtr>(c->thisObject()) == script::get<ValuePtr>(c->arg(1)));
  return c->engine()->newBool(result);
}

script::Value cast(script::FunctionCall *c)
{
  ValuePtr& self = script::get<ValuePtr>(c->thisObject());
  /// TODO: check that cast is correct !!
  script::Value ret = c->engine()->construct(c->callee().returnType(), {});
  script::get<ValuePtr>(ret) = self;
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
  ref.newConversion(get_ref_instance(src).id(), callbacks::cast).create();
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

  builder.returns(builder.proto_.at(0).baseType());
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
  std::vector<script::Conversion> conversions;
  std::vector<script::Type> types;
};

namespace callbacks
{

static script::Value make_ref_template(script::FunctionCall *c)
{
  using namespace script;
  auto data = std::static_pointer_cast<MakeRefTemplateData>(c->callee().data());
  Value content = c->engine()->allocate(data->target.memberOf().id());
  std::vector<Value> args;
  args.push_back(content);
  args.insert(args.end(), c->args().begin(), c->args().end());
  c->engine()->applyConversions(args, data->conversions);
  c->engine()->invoke(data->target, args);
  script::Value result = c->engine()->construct(c->callee().returnType(), {});
  script::get<ValuePtr>(result) = content.impl();
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
  types.insert(types.begin(), Type::ref(target_type.id()));

  OverloadResolution resol = OverloadResolution::New(make_ref.engine());
  if (!resol.process(target_type.constructors(), types))
    throw TemplateInstantiationError{ "Ref<T>::make() : Could not find valid constructor" };

  data->target = resol.selectedOverload();
  for (const auto init : resol.initializations())
    data->conversions.push_back(init.conversion());
  data->types = data->target.prototype().parameters();

  return { callbacks::make_ref_template, data };
}

void register_make_ref_template(script::Class ref_instance)
{
  using namespace script;

  Symbol{ ref_instance }.newFunctionTemplate("make")
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
  Value val{ script::get<ValuePtr>(c->thisObject()).value };
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

  Symbol{ ref_instance }.newFunctionTemplate("is")
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
  Value val{ script::get<ValuePtr>(c->thisObject()).value };
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

  Value ret = c->engine()->construct(c->callee().returnType(), {});
  script::get<ValuePtr>(ret) = val.impl();
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

  Symbol{ ref_instance }.newFunctionTemplate("as")
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

  instance.newConstructor(callbacks::default_ctor).create();
  instance.newConstructor(callbacks::copy_ctor).params(Type::cref(instance.id())).create();

  instance.newDestructor(callbacks::dtor).create();

  instance.newMethod("get", callbacks::get)
    .setConst()
    .returns(Type::ref(cla.id()))
    .create();

  instance.newMethod("isNull", callbacks::is_null)
    .setConst()
    .returns(Type::Boolean)
    .create();

  instance.newMethod("isValid", callbacks::is_valid)
    .setConst()
    .returns(Type::Boolean)
    .create();

  instance.newMethod("reset", callbacks::reset)
    .create();

  instance.newOperator(AssignmentOperator, callbacks::assign)
    .returns(Type::ref(instance.id()))
    .params(Type::cref(instance.id())).create();

  instance.newOperator(AssignmentOperator, callbacks::assign_null)
    .returns(Type::ref(instance.id()))
    .params(Type::cref(Type::NullType)).create();

  instance.newOperator(EqualOperator, callbacks::eq)
    .setConst()
    .returns(Type::Boolean)
    .params(Type::cref(instance.id())).create();

  instance.newConversion(Type::ref(cla.id()), callbacks::get)
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

  ClassTemplate ref = Symbol{ ns }.newClassTemplate("Ref")
    .params(TemplateParameter{ TemplateParameter::TypeParameter{}, "T" })
    .setScope(Scope{ ns })
    .setCallback(ref_template)
    .get();
  
  ns.engine()->implementation()->templates.ref_template = ref;
}


} // namespace dex