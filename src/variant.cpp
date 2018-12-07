// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/variant.h"

#include <script/class.h>
#include <script/classbuilder.h>
#include <script/constructorbuilder.h>
#include <script/destructorbuilder.h>
#include <script/engine.h>
#include <script/functionbuilder.h>
#include <script/namespace.h>
#include <script/operatorbuilder.h>
#include <script/interpreter/executioncontext.h>

namespace dex
{

Variant::TypeInfo Variant::static_type_info = Variant::TypeInfo{};

namespace callbacks
{

static script::Value ctor(script::FunctionCall *c)
{
  c->thisObject().setVariant(QVariant{});
  return c->thisObject();
}

static script::Value copy_ctor(script::FunctionCall *c)
{
  c->thisObject().setVariant(QVariant{ Variant::get(c->arg(1)) });
  return c->thisObject();
}

static script::Value dtor(script::FunctionCall *c)
{
  c->thisObject().clear(script::passkey{});
  return script::Value::Void;
}

static script::Value ctor_bool(script::FunctionCall *c)
{
  c->thisObject().setVariant(QVariant{c->arg(1).toBool()});
  return c->thisObject();
}

static script::Value ctor_int(script::FunctionCall *c)
{
  c->thisObject().setVariant(QVariant{ c->arg(1).toInt() });
  return c->thisObject();
}

static script::Value ctor_string(script::FunctionCall *c)
{
  c->thisObject().setVariant(QVariant{ c->arg(1).toString() });
  return c->thisObject();
}

static script::Value is_bool(script::FunctionCall *c)
{
  QVariant & self = Variant::get(c->thisObject());
  return c->engine()->newBool(self.type() == QVariant::Bool);
}

static script::Value is_int(script::FunctionCall *c)
{
  QVariant & self = Variant::get(c->thisObject());
  return c->engine()->newBool(self.type() == QVariant::Int);
}

static script::Value is_string(script::FunctionCall *c)
{
  QVariant & self = Variant::get(c->thisObject());
  return c->engine()->newBool(self.type() == QVariant::String);
}

static script::Value to_bool(script::FunctionCall *c)
{
  QVariant & self = Variant::get(c->thisObject());
  return c->engine()->newBool(self.toBool());
}

static script::Value to_int(script::FunctionCall *c)
{
  QVariant & self = Variant::get(c->thisObject());
  return c->engine()->newInt(self.toInt());
}

static script::Value to_string(script::FunctionCall *c)
{
  QVariant & self = Variant::get(c->thisObject());
  return c->engine()->newString(self.toString());
}

static script::Value assign(script::FunctionCall *c)
{
  QVariant & self = Variant::get(c->thisObject());
  QVariant & other = Variant::get(c->arg(1));
  self = other;
  return c->thisObject();
}

} // namespace callbacks


void Variant::register_type(script::Namespace ns)
{
  using namespace script;

  Class variant = ns.newClass("Variant").setFinal(true).get();
  type_info().type = variant.id();

  variant.newConstructor(callbacks::ctor).create();
  variant.newConstructor(callbacks::copy_ctor).params(Type::cref(variant.id())).create();
  variant.newDestructor(callbacks::dtor).create();

  variant.newConstructor(callbacks::ctor_bool).params(Type::Boolean).create();
  variant.newConstructor(callbacks::ctor_int).params(Type::Int).create();
  variant.newConstructor(callbacks::ctor_string).params(Type::String).create();

  variant.newMethod("isBool", callbacks::is_bool)
    .setConst()
    .returns(Type::Boolean)
    .create();

  variant.newMethod("isInt", callbacks::is_int)
    .setConst()
    .returns(Type::Boolean)
    .create();

  variant.newMethod("isString", callbacks::is_string)
    .setConst()
    .returns(Type::Boolean)
    .create();

  variant.newMethod("toBool", callbacks::to_bool)
    .setConst()
    .returns(Type::Boolean)
    .create();

  variant.newMethod("toInt", callbacks::to_int)
    .setConst()
    .returns(Type::Int)
    .create();

  variant.newMethod("toString", callbacks::to_string)
    .setConst()
    .returns(Type::String)
    .create();

  variant.newOperator(AssignmentOperator, callbacks::assign)
    .returns(Type::ref(variant.id()))
    .params(Type::cref(variant.id()))
    .create();
}

script::Value Variant::create(script::Engine *e, const QVariant & value)
{
  return e->construct(type_info().type, [&value](script::Value &ret) -> void {
    ret.setVariant(value);
  });
}

QVariant & Variant::get(script::Value val)
{
  return val.toVariant();
}

} // namespace dex
