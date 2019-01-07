// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_LIQUID_FILTER_H
#define DEX_LIQUID_FILTER_H

#include "dex/core/value.h"

namespace dex
{

namespace liquid
{

class Context;

class Filter
{
public:
  Filter() = default;
  virtual ~Filter() = default;

  virtual QString name() const = 0;
  virtual dex::Value invoke(const QList<dex::Value> & args, Context *context) = 0;

  static std::shared_ptr<Filter> create(const script::Function & func);
};

} // namespace liquid

} // namespace dex

#endif // DEX_LIQUID_FILTER_H
