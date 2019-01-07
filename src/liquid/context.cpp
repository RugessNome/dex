// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/liquid/context.h"

#include "dex/liquid/filters.h"

#include <script/overloadresolution.h>
#include <script/scope.h>
#include <script/script.h>

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

void Context::addFilter(std::shared_ptr<Filter> f)
{
  mFilters[f->name()] = f;
}

int Context::loadFilters(const QString & ns)
{
  script::Scope scp{ engine()->rootNamespace() };

  for(const auto & s : engine()->scripts())
    scp.merge(script::Scope{ s.rootNamespace() } );

  QStringList blocks = ns.split("::");
  while (!blocks.isEmpty())
    scp = scp.child(blocks.takeFirst().toStdString());

  int added = 0;

  for (const auto & f : scp.functions())
  {
    auto filter = Filter::create(f);
    if (filter)
    {
      addFilter(filter);
      added++;
    }
  }

  return added;
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

int Context::loadStringConverters(const QString & ns)
{
  script::Scope scp{ engine()->rootNamespace() };

  for (const auto & s : engine()->scripts())
    scp.merge(script::Scope{ s.rootNamespace() });

  QStringList blocks = ns.split("::");
  while (!blocks.isEmpty())
    scp = scp.child(blocks.takeFirst().toStdString());

  int added = 0;

  for (const auto & f : scp.functions())
  {
    if (f.name() == "stringify")
    {
      mStringifyFunctions.push_back(f);
      added++;
    }
  }

  return added;
}

} // namespace liquid

} // namespace dex
