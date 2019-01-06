// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_LIQUID_FILTERS_H
#define DEX_LIQUID_FILTERS_H

#include "dex/liquid/filter.h"

namespace dex
{

namespace liquid
{

class Context;

class StringFilter : public Filter
{
public:

  static void registerStringFilters(Context *context);
};

} // namespace liquid

} // namespace dex

#endif // DEX_LIQUID_FILTERS_H
