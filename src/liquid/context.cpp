// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/liquid/context.h"

#include "dex/liquid/filters.h"

#include <script/overloadresolution.h>

namespace dex
{

namespace liquid
{

Context::Context(script::Engine *e)
  : mEngine(e)
{
  liquid::StringFilter::registerStringFilters(this);
}

void Context::addFilter(Filter *f)
{
  mFilters[f->name()] = std::shared_ptr<Filter>(f);
}

QString Context::stringify(const dex::Value & value)
{
  auto resol = script::OverloadResolution::New(engine());

  std::vector<script::Value> args{ value.impl() };

  if (!resol.process(mStringifyFunctions, args))
    return QString{};

  auto ret = engine()->call(resol.selectedOverload(), args);
  QString str = ret.toString();
  engine()->destroy(ret);
  return str;
}

} // namespace liquid

} // namespace dex
