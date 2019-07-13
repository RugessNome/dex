// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/core/output.h"

#include "dex/core/serialization.h"

#include <script/class.h>
#include <script/engine.h>

#include <script/namespace.h>
#include <script/classbuilder.h>
#include <script/constructorbuilder.h>
#include <script/destructorbuilder.h>
#include <script/functionbuilder.h>
#include <script/locals.h>
#include <script/typesystem.h>
#include <script/interpreter/executioncontext.h>

namespace script
{

namespace output_callbacks
{

/*!
 * \fn String stringify(const json::Json& data)
 * \brief Converts serialized value to a string.
 * This function uses the current Output to convert the input to a String.
 */
Value stringify(FunctionCall* c)
{
  json::Json& data = get<json::Json>(c->arg(0));
  return c->engine()->newString(dex::Output::current()->stringify(data));
}

/*!
 * \class Output
 * \brief Base class for all output formats.
 *
 * You should derive directly from this class to define a new output format.
 *
 * A write() method taking a String as input must be provided and is reponsible 
 * for writing the output to the given directory.
 *
 * Additionally, you can define several overloads of a toString() function that 
 * converts its argument to a String.
 * The global stringify() function uses the currently selected output's toString() 
 * functions to convert its input to a String.
 */

/*!
 * \fn Output()
 * \brief Default constructor
 * Does nothing.
 */
Value ctor(FunctionCall* c)
{
  c->thisObject().init();
  return c->arg(0);
}

/*
 * \fn ~Output()
 * \brief Destructor
 */
Value dtor(FunctionCall* c)
{
  c->thisObject().destroy();
  return script::Value::Void;
}

} // output_callbacks

} // namespace script

namespace dex
{ 

Output* Output::staticCurrentOutput = nullptr;

Output::Output(const script::Value& impl)
  : m_self(impl)
{
  script::Class c = m_self.engine()->typeSystem()->getClass(m_self.type());


  for (const script::Function& f : c.memberFunctions())
  {
    if (f.name() == "write")
    {
      m_write = f;
    }
    else if (f.name() == "toString")
    {
      if (f.returnType() != script::Type::String)
      {
        continue;
      }

      if (f.isStatic() && f.prototype().count() == 1)
      {
        m_tostring_functions[f.parameter(0).baseType().data()] = f;
      }
      else if (!f.isStatic() && f.prototype().count() == 2)
      {
        m_tostring_functions[f.parameter(1).baseType().data()] = f;
      }
    }
  }

  /// TODO: do not crash here, but dismiss Output instance
  assert(!m_write.isNull());
}

Output::~Output()
{
  script::Engine* e = m_self.engine();
  e->destroy(m_self);
}

QString Output::name() const
{
  script::Class c = m_self.engine()->typeSystem()->getClass(m_self.type());
  return QString::fromStdString(c.name()).toLower();
}

QString Output::stringify(const json::Json& data)
{
  if (data == nullptr)
  {
    return "";
  }
  else if (data.isArray())
  {
    auto it = m_tostring_functions.find(script::Type::JsonArray);

    if (it != m_tostring_functions.end())
    {
      return stringify(data.toArray(), it->second);
    }
    else
    {
      QString result;

      for (int i(0); i < data.length(); ++i)
      {
        result += stringify(data.at(i));
      }

      return result;
    }
  }
  else if (data.isObject() && data["__type"] == nullptr)
  {
    auto it = m_tostring_functions.find(script::Type::JsonObject);

    if (it != m_tostring_functions.end())
    {
      return stringify(data.toObject(), it->second);
    }
    else
    {
      throw std::runtime_error{ "No matching toString() function" };
    }
  }

  script::Value val = dex::serialization::deserialize(data, script::Type::Auto);
  QString result = stringify(val);
  m_self.engine()->destroy(val);
  return result;
}

QString Output::stringify(const script::Value& val)
{
  if (val.isString())
  {
    return val.toString();
  }
  else if (val.isInt())
  {
    return QString::number(val.toInt());
  }
  else if (val.isDouble())
  {
    return QString::number(val.toDouble());
  }

  auto it = m_tostring_functions.find(val.type().baseType().data());

  if (it == m_tostring_functions.end())
  {
    throw std::runtime_error{ "No matching toString() function" };
  }

  const script::Function to_string = it->second;

  script::Locals args;

  if (!to_string.isStatic())
  {
    args.push(m_self);
  }

  args.push(val);

  script::Value ret = to_string.call(args);
  QString result = ret.toString();
  to_string.engine()->destroy(ret);
  return result;
}

QString Output::stringify(const json::Array& data, const script::Function& /* to_string */)
{
  script::Value val = dex::serialization::deserialize(data, script::Type::JsonArray);
  QString result = stringify(val);
  m_self.engine()->destroy(val);
  return result;
}

QString Output::stringify(const json::Object& data, const script::Function& /* to_string */)
{
  script::Value val = dex::serialization::deserialize(data, script::Type::JsonObject);
  QString result = stringify(val);
  m_self.engine()->destroy(val);
  return result;
}

void Output::write(const QString& outdir)
{
  script::Engine* e = m_self.engine();

  script::Locals args;

  args.push(m_self);
  args.push(e->newString(outdir));

  m_write.call(args);
}

Output* Output::current()
{
  return staticCurrentOutput;
}

void Output::expose(script::Namespace& ns)
{
  script::Class output = ns.newClass("Output").setId(script::Type::DexOutput).get();

  output.newConstructor(script::output_callbacks::ctor).create();
  output.newDestructor(script::output_callbacks::dtor).create();

  ns.newFunction("stringify", script::output_callbacks::stringify)
    .returns(script::Type::String)
    .params(script::make_type<const json::Json&>())
    .create();
}

} // namespace dex
