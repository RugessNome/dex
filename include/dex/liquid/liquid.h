// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_LIQUID_H
#define DEX_LIQUID_H

#include "dex/liquid/context.h"
#include "dex/liquid/template.h"

namespace dex
{

namespace liquid
{

void expose(script::Engine *e);

} // namespace liquid

} // namespace dex

#endif // DEX_LIQUID_H
