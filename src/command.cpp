// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/command.h"

#include <script/class.h>
#include <script/classbuilder.h>
#include <script/constructorbuilder.h>
#include <script/destructorbuilder.h>
#include <script/engine.h>
#include <script/functionbuilder.h>
#include <script/interpreter/executioncontext.h>
#include <script/namespace.h>

namespace dex
{

Command::TypeInfo Command::static_type_info = {};

namespace callbacks
{
static script::Value command_dummy_callback(script::FunctionCall *c)
{
  return c->thisObject();
}
} // namespace callbacks

void Command::registerCommandType(script::Engine *e)
{
  using namespace script;

  Class command = e->rootNamespace().newClass("Command").get();

  command.newConstructor(callbacks::command_dummy_callback).create();
  command.newDestructor(callbacks::command_dummy_callback).create();

  dex::Command::type_info().type = command.id();
}

} // namespace dex