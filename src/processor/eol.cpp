// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <script/engine.h>
#include <script/namespace.h>

#include <script/interpreter/executioncontext.h>

#include <script/class.h>
#include <script/classbuilder.h>
#include <script/constructorbuilder.h>
#include <script/destructorbuilder.h>
#include <script/functionbuilder.h>
#include <script/operatorbuilder.h>

#include <QDebug>

namespace script
{

namespace eol_callbacks
{

Value ctor(FunctionCall* c)
{
  c->thisObject().init();
  return c->arg(0);
}

Value copy_ctor(FunctionCall* c)
{
  c->thisObject().init();
  return c->arg(0);
}

Value dtor(FunctionCall* c)
{
  c->thisObject().destroy();
  return script::Value::Void;
}

} // namespace eol_callbacks

} // namespace script

namespace dex
{

void register_eol_type(script::Namespace& ns)
{
  using namespace script;

  Class eol = ns.newClass("EOL").setId(Type::DexEOL).setFinal(true).get();
  
  eol.newConstructor(script::eol_callbacks::ctor).create();
  eol.newConstructor(script::eol_callbacks::copy_ctor).params(Type::cref(Type::DexEOL)).create();
  eol.newDestructor(script::eol_callbacks::dtor).create();
}

} // namespace dex
