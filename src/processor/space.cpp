// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <script/engine.h>
#include <script/namespace.h>

#include <script/interpreter/executioncontext.h>
#include <script/object.h>

#include <script/class.h>
#include <script/classbuilder.h>
#include <script/constructorbuilder.h>
#include <script/destructorbuilder.h>
#include <script/functionbuilder.h>
#include <script/operatorbuilder.h>

#include <QDebug>

namespace script
{

namespace space_callbacks
{

Value ctor(FunctionCall* c)
{
  c->thisObject().init();
  c->thisObject().push(c->engine()->newString(""));
  return c->arg(0);
}

Value copy_ctor(FunctionCall* c)
{
  Object other = c->arg(1).toObject();

  c->thisObject().init();
  c->thisObject().push(c->engine()->newString(other.at(0).toString()));
  return c->arg(0);
}

Value dtor(FunctionCall* c)
{
  c->thisObject().destroy();
  return Value::Void;
}

} // namespace space_callbacks

} // namespace script

namespace dex
{

void register_space_type(script::Namespace& ns)
{
  using namespace script;

  Class space = ns.newClass("Space").setId(Type::DexSpace).setFinal(true)
    .addMember(DataMember(Type::String, "content")).get();

  space.newConstructor(script::space_callbacks::ctor).create();
  space.newConstructor(script::space_callbacks::copy_ctor).params(Type::cref(Type::DexSpace)).create();
  space.newDestructor(script::space_callbacks::dtor).create();
}

} // namespace dex
