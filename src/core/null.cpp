// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/core/null.h"

#include <script/class.h>
#include <script/classbuilder.h>
#include <script/constructorbuilder.h>
#include <script/destructorbuilder.h>
#include <script/engine.h>
#include <script/interpreter/executioncontext.h>
#include <script/functionbuilder.h>
#include <script/namespace.h>
#include <script/operatorbuilder.h>

namespace script
{

namespace null_callbacks
{

static script::Value default_ctor(script::FunctionCall* c)
{
  c->thisObject().init<dex::Null>();
  return c->thisObject();
}

static script::Value dtor(script::FunctionCall* c)
{
  c->thisObject().destroy<dex::Null>();
  return script::Value::Void;
}

} // namespace null_callbacks

} // namespace script

namespace dex
{

void Null::expose(script::Namespace& ns)
{
  using namespace script;

  Class null = ns.newClass("Null").setId(Type::NullType).setFinal().get();

  null.newConstructor(null_callbacks::default_ctor).create();
  null.newConstructor(null_callbacks::default_ctor).params(Type::cref(null.id())).create();
  null.newDestructor(null_callbacks::dtor).create();

  script::Value val = ns.engine()->construct<dex::Null>();
  ns.engine()->manage(val);
  ns.addValue("null", val);
}

} // namespace dex
