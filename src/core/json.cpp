// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/core/json.h"

#include "dex/core/null.h"

#include <script/castbuilder.h>
#include <script/class.h>
#include <script/classbuilder.h>
#include <script/constructorbuilder.h>
#include <script/destructorbuilder.h>
#include <script/engine.h>
#include <script/enum.h>
#include <script/enumbuilder.h>
#include <script/functionbuilder.h>
#include <script/namespace.h>
#include <script/operatorbuilder.h>
#include <script/interpreter/executioncontext.h>
#include <script/private/value_p.h>

namespace script
{

namespace json_callbacks
{

/*!
 * \namespace json
 */

/*!
 * \class Json
 * \brief Provides a Json object
 */

/*!
 * \fn Json()
 * \brief Constructs an empty object.
 */
static script::Value ctor(script::FunctionCall* c)
{
  c->thisObject().init<json::Json>();
  return c->thisObject();
}

/*!
 * \fn Json(const Json& other)
 * \brief Copy constructor
 * Note that this does not construct a true copy of the object, 
 * Json object are implicitly shared.
 */
static Value copy_ctor(FunctionCall* c)
{
  json::Json& other = get<json::Json>(c->arg(1));
  c->thisObject().init<json::Json>(other);
  return c->arg(0);
}

/*!
 * \fn ~Json()
 */
static script::Value dtor(script::FunctionCall* c)
{
  c->thisObject().destroy<json::Json>();
  return script::Value::Void;
}

/*!
 * \fn Json(NullType)
 * \brief Constructs a null value.
 */
static script::Value null_ctor(script::FunctionCall* c)
{
  c->thisObject().init<json::Json>(nullptr);
  return c->thisObject();
}

/*!
 * \fn Json(const String& str)
 * \brief Constructs a string literal value.
 */
static script::Value ctor_string(script::FunctionCall* c)
{
  c->thisObject().init<json::Json>(c->arg(1).toString());
  return c->thisObject();
}

/*!
 * \fn bool isNull() const
 * \brief Returns whether this Json is null.
 */
static script::Value is_null(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->newBool(self.isNull());
}

/*!
 * \fn bool isBool() const
 * \brief Returns whether this Json is a boolean.
 * \sa toBool()
 */
static script::Value is_bool(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->newBool(self.isBoolean());
}

/*!
 * \fn bool isInt() const
 * \brief Returns whether this Json is an integer.
 * \sa toInt()
 */
static script::Value is_int(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->newBool(self.isInteger());
}

/*!
 * \fn bool isString() const
 * \brief Returns whether this Json is a string.
 * \sa toString()
 */
static script::Value is_string(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->newBool(self.isString());
}

/*!
 * \fn bool isArray() const
 * \brief Returns whether this Json is an array.
 * \sa toArray()
 */
static script::Value is_array(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->newBool(self.isArray());
}

/*!
 * \fn bool isObject() const
 * \brief Returns whether this Json is an object.
 * \sa toArray()
 */
static script::Value is_object(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->newBool(self.isObject());
}

/*!
 * \fn bool toBool() const
 * \brief Returns the boolean value that this Json holds
 */
static script::Value to_bool(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->newBool(self.toBool());
}

/*!
 * \fn int toInt() const
 * \brief Returns the integer value that this Json holds
 */
static script::Value to_int(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->newInt(self.toInt());
}

/*!
 * \fn String toString() const
 * \brief Returns the string that this Json holds
 */
static script::Value to_string(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->newString(self.toString());
}

/*!
 * \fn Array toArray() const
 * \brief Returns this Json as an Array
 */
static script::Value to_array(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->construct<json::Array>(self.toArray());
}

/*!
 * \fn Object toObject() const
 * \brief Returns this Json as an Object
 */
static script::Value to_object(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->construct<json::Object>(self.toObject());
}

/*!
 * \fn int length() const
 * \brief Returns the length of the array.
 */
static script::Value length(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  return c->engine()->newInt(self.length());
}

/*!
 * \fn Json at(int index) const
 * \brief Access the array element by index
 */
static script::Value at(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  int pos = script::get<int>(c->arg(1));
  return c->engine()->construct<json::Json>(self.at(pos));
}

/*!
 * \fn void push(const Json& element)
 * \brief Appends an element to an array
 */
static script::Value push(script::FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  const json::Json& element = script::get<json::Json>(c->arg(1));
  self.push(element);
  return script::Value::Void;
}

/*!
 * \fn JsonArrayProxy operator[](int index);
 */
static Value op_at_array(FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  int index = script::get<int>(c->arg(1));
  dex::JsonArrayProxy proxy{ self, index };
  return c->engine()->construct<dex::JsonArrayProxy>(proxy);
}

/*!
 * \fn JsonObjectProxy operator[](const String& key);
 */
static Value op_at_object(FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  const QString& key = script::get<QString>(c->arg(1));
  dex::JsonObjectProxy proxy{ self, key };
  return c->engine()->construct<dex::JsonObjectProxy>(proxy);
}

/*!
 * \fn const Json operator[](const String& key) const;
 */
static Value op_at_object_const(FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  const QString& key = script::get<QString>(c->arg(1));
  return c->engine()->construct<json::Json>(self[key]);
}

/*!
 * \fn Json& operator=(const Json& other)
 */
static Value op_assign(FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  const json::Json& other = script::get<json::Json>(c->arg(1));
  self = other;
  return c->arg(0);
}

/*!
 * \fn Json& operator=(NullType)
 */
static Value op_assign_null(FunctionCall* c)
{
  json::Json& self = script::get<json::Json>(c->arg(0));
  self = nullptr;
  return c->arg(0);
}

/* Array */

static script::Value array_ctor(script::FunctionCall* c)
{
  c->thisObject().init<json::Array>();
  return c->thisObject();
}

static Value array_copy_ctor(FunctionCall* c)
{
  json::Array& other = get<json::Array>(c->arg(1));
  c->thisObject().init<json::Array>(other);
  return c->arg(0);
}

static script::Value array_dtor(script::FunctionCall* c)
{
  c->thisObject().destroy<json::Array>();
  return script::Value::Void;
}

/* Object */

static Value object_ctor(FunctionCall* c)
{
  c->thisObject().init<json::Object>();
  return c->arg(0);
}

static Value object_copy_ctor(FunctionCall* c)
{
  json::Object& other = get<json::Object>(c->arg(1));
  c->thisObject().init<json::Object>(other);
  return c->arg(0);
}

static Value object_dtor(FunctionCall* c)
{
  c->thisObject().destroy<json::Object>();
  return Value::Void;
}

/* Proxy */

static script::Value proxy_is_null(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->newBool(self.isNull());
}

static script::Value proxy_is_bool(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->newBool(self.isBoolean());
}

static script::Value proxy_is_int(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->newBool(self.isInteger());
}

static script::Value proxy_is_string(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->newBool(self.isString());
}

static script::Value proxy_is_array(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->newBool(self.isArray());
}

static script::Value proxy_is_object(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->newBool(self.isObject());
}

static script::Value proxy_to_bool(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->newBool(self.toBool());
}

static script::Value proxy_to_int(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->newInt(self.toInt());
}

static script::Value proxy_to_string(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->newString(self.toString());
}

static script::Value proxy_to_array(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->construct<json::Array>(self.toArray());
}

static script::Value proxy_to_object(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->construct<json::Object>(self.toObject());
}

static script::Value proxy_length(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  return c->engine()->newInt(self.length());
}

static script::Value proxy_at(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  int pos = script::get<int>(c->arg(1));
  return c->engine()->construct<json::Json>(self.at(pos));
}

static script::Value proxy_push(script::FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  const json::Json& element = script::get<json::Json>(c->arg(1));
  self.push(element);
  return script::Value::Void;
}

static script::Value proxy_op_at_array(FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  int index = script::get<int>(c->arg(1));
  dex::JsonArrayProxy proxy{ self, index };
  return c->engine()->construct<dex::JsonArrayProxy>(proxy);
}

static script::Value proxy_op_at_object(FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  const QString& key = script::get<QString>(c->arg(1));
  dex::JsonObjectProxy proxy{ self, key };
  return c->engine()->construct<dex::JsonObjectProxy>(proxy);
}

static script::Value proxy_op_at_object_const(FunctionCall* c)
{
  json::Json& self = script::get<dex::JsonProxy>(c->arg(0)).get();
  const QString& key = script::get<QString>(c->arg(1));
  return c->engine()->construct<json::Json>(self[key]);
}

/* Array Proxy */

static Value arrayproxy_copy_ctor(FunctionCall* c)
{
  dex::JsonArrayProxy& other = get<dex::JsonArrayProxy>(c->arg(1));
  c->thisObject().init<dex::JsonArrayProxy>(other);
  return c->arg(0);
}

static Value arrayproxy_dtor(FunctionCall* c)
{
  c->thisObject().destroy<dex::JsonArrayProxy>();
  return Value::Void;
}

static Value arrayproxy_op_assign(FunctionCall* c)
{
  dex::JsonArrayProxy& self = get<dex::JsonArrayProxy>(c->arg(0));
  const json::Json& other = script::get<json::Json>(c->arg(1));
  self.data[self.index] = other;
  return c->arg(0);
}

static Value arrayproxy_to_json(FunctionCall* c)
{
  dex::JsonArrayProxy& self = get<dex::JsonArrayProxy>(c->arg(0));
  return c->engine()->construct<json::Json>(self.data);
}

/* Object Proxy */

static Value objectproxy_copy_ctor(FunctionCall* c)
{
  dex::JsonObjectProxy& other = get<dex::JsonObjectProxy>(c->arg(1));
  c->thisObject().init<dex::JsonObjectProxy>(other);
  return c->arg(0);
}

static Value objectproxy_dtor(FunctionCall* c)
{
  c->thisObject().destroy<dex::JsonObjectProxy>();
  return Value::Void;
}

static Value objectproxy_op_assign(FunctionCall* c)
{
  dex::JsonObjectProxy& self = get<dex::JsonObjectProxy>(c->arg(0));
  const json::Json& other = script::get<json::Json>(c->arg(1));
  self.data[self.key] = other;
  return c->arg(0);
}

static Value objectproxy_to_json(FunctionCall* c)
{
  dex::JsonObjectProxy& self = get<dex::JsonObjectProxy>(c->arg(0));
  return c->engine()->construct<json::Json>(self.data);
}

} // namespace json_callbacks

} // namespace script

namespace dex
{

JsonProxy::JsonProxy(const json::Json& d)
  : data(d)
{

}

JsonArrayProxy::JsonArrayProxy(const json::Json& d, int i)
  : JsonProxy(d),
    index(i)
{

}

json::Json& JsonArrayProxy::get()
{
  return data[index];
}

JsonObjectProxy::JsonObjectProxy(const json::Json& d, const QString& k)
  : JsonProxy(d),
    key(k)
{

}

json::Json& JsonObjectProxy::get()
{
  return data[key];
}

void register_json_type(script::Namespace& ns)
{
  using namespace script;

  Class json = ns.newClass("Json").setId(Type::Json).setFinal(true).get();

  Class arrayproxy = ns.newClass("JsonArrayProxy").setId(Type::JsonArrayProxy).setFinal(true).get();
  Class objectproxy = ns.newClass("JsonObjectProxy").setId(Type::JsonObjectProxy).setFinal(true).get();

  json.newConstructor(script::json_callbacks::ctor).create();
  json.newConstructor(script::json_callbacks::copy_ctor).params(make_type<const json::Json&>()).create();
  json.newDestructor(script::json_callbacks::dtor).create();

  json.newConstructor(script::json_callbacks::null_ctor).params(make_type<const dex::Null&>()).create();
  json.newConstructor(script::json_callbacks::ctor_string).params(Type::String).create();

  json.newMethod("isNull", script::json_callbacks::is_null)
    .returns(Type::Boolean)
    .setConst()
    .create();

  json.newMethod("isBool", script::json_callbacks::is_bool)
    .returns(Type::Boolean)
    .setConst()
    .create();

  json.newMethod("isInt", script::json_callbacks::is_int)
    .returns(Type::Boolean)
    .setConst()
    .create();

  json.newMethod("isString", script::json_callbacks::is_string)
    .returns(Type::Boolean)
    .setConst()
    .create();

  json.newMethod("isArray", script::json_callbacks::is_array)
    .returns(Type::Boolean)
    .setConst()
    .create();

  json.newMethod("isObject", script::json_callbacks::is_object)
    .returns(Type::Boolean)
    .setConst()
    .create();

  json.newMethod("toBool", script::json_callbacks::to_bool)
    .returns(Type::Boolean)
    .setConst()
    .create();

  json.newMethod("toInt", script::json_callbacks::to_int)
    .returns(Type::Int)
    .setConst()
    .create();

  json.newMethod("toString", script::json_callbacks::to_string)
    .returns(Type::String)
    .setConst()
    .create();

  json.newMethod("toArray", script::json_callbacks::to_array)
    .returns(script::make_type<json::Array>())
    .setConst()
    .create();

  json.newMethod("toObject", script::json_callbacks::to_object)
    .returns(script::make_type<json::Object>())
    .setConst()
    .create();

  /* Array methods */

  json.newMethod("length", script::json_callbacks::length)
    .returns(Type::Int)
    .setConst()
    .create();

  json.newMethod("at", script::json_callbacks::at)
    .returns(script::make_type<json::Json>())
    .params(Type::Int)
    .setConst()
    .create();

  json.newMethod("push", script::json_callbacks::push)
    .params(script::make_type<const json::Json&>())
    .create();

  /* Operators */

  json.newOperator(AssignmentOperator, script::json_callbacks::op_assign)
    .returns(script::make_type<json::Json&>())
    .params(script::make_type<const json::Json&>())
    .create();

  json.newOperator(AssignmentOperator, script::json_callbacks::op_assign_null)
    .returns(script::make_type<json::Json&>())
    .params(script::make_type<const dex::Null&>())
    .create();

  json.newOperator(SubscriptOperator, script::json_callbacks::op_at_array)
    .returns(script::make_type<dex::JsonArrayProxy>())
    .params(script::Type::Int)
    .create();

  json.newOperator(SubscriptOperator, script::json_callbacks::op_at_object)
    .returns(script::make_type<dex::JsonObjectProxy>())
    .params(script::Type::cref(script::Type::String))
    .create();

  json.newOperator(SubscriptOperator, script::json_callbacks::op_at_object_const)
    .returns(script::make_type<const json::Json>())
    .params(script::Type::cref(script::Type::String))
    .setConst()
    .create();

  /* Array Proxy */

  arrayproxy.newConstructor(script::json_callbacks::arrayproxy_copy_ctor).params(script::make_type<const dex::JsonArrayProxy&>()).create();
  arrayproxy.newDestructor(script::json_callbacks::arrayproxy_dtor).create();

  arrayproxy.newOperator(AssignmentOperator, script::json_callbacks::arrayproxy_op_assign)
    .returns(script::make_type<dex::JsonArrayProxy&>())
    .params(script::make_type<const json::Json&>())
    .create();

  arrayproxy.newConversion(script::make_type<json::Json>(), script::json_callbacks::arrayproxy_to_json)
    .setConst()
    .create();

  /* Object Proxy */

  objectproxy.newConstructor(script::json_callbacks::objectproxy_copy_ctor).params(script::make_type<const dex::JsonObjectProxy&>()).create();
  objectproxy.newDestructor(script::json_callbacks::objectproxy_dtor).create();

  objectproxy.newOperator(AssignmentOperator, script::json_callbacks::objectproxy_op_assign)
    .returns(script::make_type<dex::JsonObjectProxy&>())
    .params(script::make_type<const json::Json&>())
    .create();

  objectproxy.newConversion(script::make_type<json::Json>(), script::json_callbacks::objectproxy_to_json)
    .setConst()
    .create();

  objectproxy.newMethod("at", script::json_callbacks::proxy_at)
    .returns(script::make_type<json::Json>())
    .params(Type::Int)
    .setConst()
    .create();
}

void register_array_type(script::Namespace& ns)
{
  using namespace script;

  Class self = ns.newClass("Array").setId(Type::JsonArray).setBase(Type::Json).setFinal(true).get();

  self.newConstructor(script::json_callbacks::array_ctor).create();
  self.newConstructor(script::json_callbacks::array_copy_ctor).params(script::make_type<const json::Array&>()).create();
  self.newDestructor(script::json_callbacks::array_dtor).create();

}

void registerJsonTypes(script::Namespace& ns)
{
  register_json_type(ns);
  register_array_type(ns);

  /* Object */

  script::Class object = ns.newClass("Object").setId(script::Type::JsonObject).setBase(script::Type::Json).setFinal(true).get();

  object.newConstructor(script::json_callbacks::object_ctor).create();
  object.newConstructor(script::json_callbacks::object_copy_ctor).params(script::make_type<const json::Object&>()).create();
  object.newDestructor(script::json_callbacks::object_dtor).create();
}

} // namespace dex
