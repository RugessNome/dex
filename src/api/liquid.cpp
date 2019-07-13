// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/api/liquid.h"

#include "dex/core/output.h"
#include "dex/core/serialization.h"

#include <script/class.h>
#include <script/classbuilder.h>
#include <script/constructorbuilder.h>
#include <script/destructorbuilder.h>
#include <script/engine.h>
#include <script/enum.h>
#include <script/enumbuilder.h>
#include <script/functionbuilder.h>
#include <script/locals.h>
#include <script/namespace.h>
#include <script/object.h>
#include <script/typesystem.h>

#include <script/interpreter/executioncontext.h>

namespace script
{

namespace liquid_callbacks
{

/*!
 * \namespace liquid
 */

/*!
 * \fn Template parse(const String& input)
 * \brief Parses the liquid template
 */
Value parse(FunctionCall* c)
{
  QString input = c->arg(0).toString();
  liquid::Template tmplt = liquid::parse(input);
  return c->engine()->construct<liquid::Template>(tmplt);
}

/*!
 * \class Template
 * \brief A Liquid template
 */

/*!
 * \fn Template()
 * \brief Constructs an empty template
 */
Value template_ctor(FunctionCall* c)
{
  c->thisObject().init<liquid::Template>();
  return c->arg(0);
}

/*!
 * \fn Template(const Template&)
 * \brief Copy constructor
 */
Value template_copy_ctor(FunctionCall* c)
{
  liquid::Template& other = get<liquid::Template>(c->arg(1));
  c->thisObject().init<liquid::Template>(other);
  return c->arg(0);
}

/*!
 * \fn ~Template()
 */
Value template_dtor(FunctionCall* c)
{
  c->thisObject().destroy<liquid::Template>();
  return Value::Void;
}

/*!
 * \endclass
 */

/*!
 * \class Renderer
 * \brief Liquid template renderer
 *
 * You may derive from this class to define liquid filters.
 * Any member function that you define is used as a potential filter provided 
 * that its return type is serializable to json::Json.
 *
 */

/*!
 * \fn Renderer() 
 */
Value renderer_ctor(FunctionCall* c)
{
  c->thisObject().init();
  c->arg(0).toObject().setUserData<dex::LiquidRenderer>();
  dex::LiquidRenderer& renderer = c->arg(0).toObject().getUserData<dex::LiquidRenderer>();
  renderer.init(c->arg(0));
  return c->arg(0);
}

/*!
 * \fn ~Renderer()
 */
Value renderer_dtor(FunctionCall* c)
{
  c->thisObject().destroy();
  return Value::Void;
}

/*!
 * \fn String render(const Template& tmplt, const json::Object& context)
 */
Value renderer_render(FunctionCall* c)
{
  dex::LiquidRenderer& renderer = c->arg(0).toObject().getUserData<dex::LiquidRenderer>();

  liquid::Template& tmplt = get<liquid::Template>(c->arg(1));
  json::Object& data = get<json::Object>(c->arg(2));

  QString result = renderer.render(tmplt, data);

  return c->engine()->newString(result);
}

} // namespace liquid_callbacks

} // namespace script

namespace dex
{

namespace callbacks
{

} // namespace callbacks


void LiquidRenderer::init(const script::Value& s)
{
  m_self = s;

  script::Class c = s.engine()->typeSystem()->getClass(s.type());

  for (script::Function f : c.memberFunctions())
  {
    m_filters[QString::fromStdString(f.name())] = f;
  }
}

QString LiquidRenderer::stringify(const json::Json& val)
{
  return dex::Output::current()->stringify(val);
}

json::Json LiquidRenderer::applyFilter(const QString& name, const json::Json& object, const std::vector<json::Json>& args)
{
  auto it = m_filters.find(name);

  if (it == m_filters.end())
  {
    /// TODO: define a specific exception
    throw std::runtime_error{ "Unknown filter" };
  }

  script::Function filter = it->second;
  script::Engine* engine = filter.engine();

  script::Locals engine_args;

  if (!filter.isStatic())
  {
    engine_args.push(m_self);
  }
  

  engine_args.push(dex::serialization::deserialize(object, script::Type::Auto));

  for (const auto& a : args)
  {
    engine_args.push(dex::serialization::deserialize(a, script::Type::Auto));
  }

  script::Value result = filter.call(engine_args);
  json::Json json_result = dex::serialization::serialize(result);
  engine->destroy(result);

  return json_result;
}

namespace api
{

void registerLiquidApi(script::Engine* e)
{
  script::Namespace l = e->rootNamespace().newNamespace("liquid");

  /* liquid */

  l.newFunction("parse", script::liquid_callbacks::parse)
    .returns(script::Type::LiquidTemplate)
    .params(script::Type::cref(script::Type::String)).create();

  /* Template */

  script::Class tmplt = l.newClass("Template").setId(script::Type::LiquidTemplate).get();

  tmplt.newConstructor(script::liquid_callbacks::template_ctor).create();
  tmplt.newConstructor(script::liquid_callbacks::template_copy_ctor).params(script::make_type<const liquid::Template&>()).create();
  tmplt.newDestructor(script::liquid_callbacks::template_dtor).create();

  /* Renderer */

  script::Class renderer = l.newClass("Renderer").setId(script::Type::DexLiquidRenderer).get();

  renderer.newConstructor(script::liquid_callbacks::renderer_ctor).create();
  renderer.newDestructor(script::liquid_callbacks::renderer_dtor).create();

  renderer.newMethod("render", script::liquid_callbacks::renderer_render)
    .returns(script::Type::String)
    .params(script::make_type<const liquid::Template&>(), script::make_type<const json::Object&>()).create();
}

} // namespace api

} // namespace dex
