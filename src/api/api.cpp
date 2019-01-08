// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/api/api.h"

#include "dex/api/file.h"
#include "dex/api/print.h"

#include <script/engine.h>
#include <script/namespace.h>

namespace dex
{

namespace api
{

void expose(script::Engine *e)
{
  registerPrintFunctions(e->rootNamespace());
  File::register_type(e->rootNamespace());
}

} // namespace api

} // namespace dex
