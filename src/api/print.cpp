// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/api/print.h"

#include <script/class.h>
#include <script/engine.h>
#include <script/enum.h>
#include <script/enumbuilder.h>
#include <script/functionbuilder.h>
#include <script/namespace.h>
#include <script/interpreter/executioncontext.h>

#include <QDebug>

#include <iostream>

namespace script
{

namespace callbacks
{

script::Value print_int(script::FunctionCall *c)
{
  int n = c->arg(0).toInt();
  qDebug() << n;
  return script::Value::Void;
}

script::Value print_bool(script::FunctionCall *c)
{
  qDebug() << c->arg(0).toBool();
  return script::Value::Void;
}

script::Value print_double(script::FunctionCall *c)
{
  qDebug() << c->arg(0).toDouble();
  return script::Value::Void;
}

script::Value print_string(script::FunctionCall *c)
{
  qDebug() << c->arg(0).toString().toUtf8().data();
  return script::Value::Void;
}

} // namespace callbacks

} // namespace callbacks


namespace dex
{

namespace api
{

void registerPrintFunctions(script::Namespace ns)
{
  ns.newFunction("print", script::callbacks::print_int)
    .params(script::Type::Int).create();

  ns.newFunction("print", script::callbacks::print_bool)
    .params(script::Type::Boolean).create();

  ns.newFunction("print", script::callbacks::print_double)
    .params(script::Type::Double).create();

  ns.newFunction("print", script::callbacks::print_string)
    .params(script::Type::cref(script::Type::String)).create();
}

} // namespace api

} // namespace dex
