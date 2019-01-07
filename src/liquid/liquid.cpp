// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/liquid/liquid.h"

#include <script/class.h>
#include <script/classbuilder.h>
#include <script/constructorbuilder.h>
#include <script/destructorbuilder.h>
#include <script/engine.h>
#include <script/function.h>
#include <script/functionbuilder.h>
#include <script/functiontemplate.h>
#include <script/interpreter/executioncontext.h>
#include <script/namespace.h>
#include <script/symbol.h>
#include <script/templateargumentdeduction.h>
#include <script/templatebuilder.h>

namespace script
{

static Type LiquidTemplate = Type{};

static dex::liquid::Context* get_liquid_context(const script::Value & val)
{
  return static_cast<dex::liquid::Context*>(val.getPtr());
}

static dex::liquid::Context* get_liquid_context(script::FunctionCall *c)
{
  return get_liquid_context(c->thisObject());
}

static dex::liquid::Template* get_liquid_template(const script::Value & val)
{
  return static_cast<dex::liquid::Template*>(val.getPtr());
}

static dex::liquid::Template* get_liquid_template(script::FunctionCall *c)
{
  return get_liquid_template(c->thisObject());
}

namespace callbacks
{

/* Context */

// Context();
script::Value liquid_context_ctor(script::FunctionCall *c)
{
  c->thisObject().setPtr(new dex::liquid::Context{ c->engine() });
  return c->thisObject();
}

// ~Context();
script::Value liquid_context_dtor(script::FunctionCall *c)
{
  delete static_cast<dex::liquid::Context*>(c->thisObject().getPtr());
  c->thisObject().setPtr(nullptr);
  return script::Value::Void;
}

// template<typename T> void expose(const String & name, const T & val);
script::Value liquid_context_expose(script::FunctionCall *c)
{
  const QString & name = c->arg(1).toString();
  const dex::Value val = dex::Value{ c->arg(2), script::ParameterPolicy::Copy };

  get_liquid_context(c)->variables()[name] = val;

  return script::Value::Void;
}

// int loadFilters(const String & namespace);
script::Value liquid_context_loadfilters(script::FunctionCall *c)
{
  const QString & name = c->arg(1).toString();

  const int result = get_liquid_context(c)->loadFilters(name);

  return c->engine()->newInt(result);
}

// int loadStringConverters(const String & namespace);
script::Value liquid_context_loadstringconverters(script::FunctionCall *c)
{
  const QString & name = c->arg(1).toString();

  const int result = get_liquid_context(c)->loadStringConverters(name);

  return c->engine()->newInt(result);
}


/* Template */

// Template();
script::Value liquid_template_ctor(script::FunctionCall *c)
{
  c->thisObject().setPtr(new dex::liquid::Template{ });
  return c->thisObject();
}

// Template();
script::Value liquid_template_copyctor(script::FunctionCall *c)
{
  const dex::liquid::Template *other = get_liquid_template(c->arg(1));
  c->thisObject().setPtr(new dex::liquid::Template{ *other });
  return c->thisObject();
}

// ~Template();
script::Value liquid_template_dtor(script::FunctionCall *c)
{
  delete static_cast<dex::liquid::Template*>(c->thisObject().getPtr());
  c->thisObject().setPtr(nullptr);
  return script::Value::Void;
}

// String render(Context & context);
script::Value liquid_template_render(script::FunctionCall *c)
{
  dex::liquid::Context *context = get_liquid_context(c->arg(1));
  QString result = get_liquid_template(c)->render(context);
  return c->engine()->newString(result);
}

/* liquid */

// Template parse(const String & str);
script::Value liquid_parse(script::FunctionCall *c)
{
  QString str = c->arg(0).toString();

  dex::liquid::Template tmplt = dex::liquid::parse(str, c->engine());

  return c->engine()->construct(LiquidTemplate, [&tmplt](script::Value & val) -> void {
    val.setPtr(new dex::liquid::Template{ tmplt });
  });
}

} // namespace callbacks

} // namespace script

namespace dex
{

namespace liquid
{

void context_expose_deduce(script::TemplateArgumentDeduction & tad, const script::FunctionTemplate & ft, const std::vector<script::TemplateArgument> & targs, const std::vector<script::Type> & params)
{
  if (targs.size() == 1)
    return;
  else if (targs.size() > 1 || params.size() != 2)
    return tad.fail();

  tad.record_deduction(0, script::TemplateArgument{ params.at(1).baseType() });
}

void context_expose_subsitute(script::FunctionBuilder & fb, script::FunctionTemplate ft, const std::vector<script::TemplateArgument> & targs)
{
  fb.params(script::Type::cref(script::Type::String), script::Type::cref(targs.front().type));
}

std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> context_expose_instantiate(script::FunctionTemplate ft, script::Function f)
{
  return { script::callbacks::liquid_context_expose, nullptr };
}

static script::Type register_context_class(script::Namespace ns)
{
  script::Class context = ns.newClass("Context").get();

  // Context();
  context.newConstructor(script::callbacks::liquid_context_ctor).create();
  // ~Context();
  context.newDestructor(script::callbacks::liquid_context_dtor).create();

  // int loadFilters(const String & namespace);
  context.newMethod("loadFilters", script::callbacks::liquid_context_loadfilters)
    .returns(script::Type::Int)
    .params(script::Type::cref(script::Type::String))
    .create();

  // int loadStringConverters(const String & namespace);
  context.newMethod("loadStringConverters", script::callbacks::liquid_context_loadstringconverters)
    .returns(script::Type::Int)
    .params(script::Type::cref(script::Type::String))
    .create();

  // template<typename T> void expose(const String & name, const T & val);
  {
    std::vector<script::TemplateParameter> tparams{
      script::TemplateParameter{script::TemplateParameter::TypeParameter{}, "T"}
    };

    script::Symbol{ context }.newFunctionTemplate("expose")
      .setParams(std::move(tparams))
      .setScope(script::Scope{ ns })
      .deduce(context_expose_deduce).substitute(context_expose_subsitute).instantiate(context_expose_instantiate)
      .create();
  }

  return context.id();
}

static script::Type register_template_class(script::Namespace ns)
{
  script::Class tmplt = ns.newClass("Template").get();

  // Template();
  tmplt.newConstructor(script::callbacks::liquid_template_ctor).create();
  // Template(const Template & other);
  tmplt.newConstructor(script::callbacks::liquid_template_copyctor)
    .params(script::Type::cref(tmplt.id()))
    .create();
  // ~Template();
  tmplt.newDestructor(script::callbacks::liquid_template_dtor).create();

  // String render(Context & context);
  tmplt.newMethod("render", script::callbacks::liquid_template_render)
    .returns(script::Type::String)
    .params(script::Type::ref(ns.findClass("Context").id()))
    .create();

  return tmplt.id();
}

void expose(script::Engine *e)
{
  script::Namespace ns = e->rootNamespace().getNamespace("liquid");

  script::Type con = register_context_class(ns);
  script::LiquidTemplate = register_template_class(ns);

  ns.newFunction("parse", script::callbacks::liquid_parse)
    .returns(script::LiquidTemplate)
    .params(script::Type::cref(script::Type::String))
    .create();
}

} // namespace liquid

} // namespace dex
