// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/liquid/filters.h"

#include <script/engine.h>

namespace dex
{

namespace liquid
{

class UserFilter : public Filter
{
public:
  script::Function function;
public:
  UserFilter(const script::Function & f) : function(f) { name_ = QString::fromStdString(f.name()); }
  ~UserFilter() = default;

  QString name() const override { return name_; }
  dex::Value invoke(const QList<dex::Value> & args, Context *context) override;

private:
  QString name_;
};

dex::Value UserFilter::invoke(const QList<dex::Value> & args, Context *context)
{
  std::vector<script::Value> arguments;
  arguments.reserve(args.size());

  for (const auto & a : args)
    arguments.push_back(a.impl());

  script::Value ret = function.engine()->call(function, arguments);

  return dex::Value{ std::move(ret) };
}

std::shared_ptr<Filter> Filter::create(const script::Function & func)
{
  if (func.isMemberFunction() || func.isOperator())
    return nullptr;

  return std::make_shared<UserFilter>(func);
}

void StringFilter::registerStringFilters(Context *context)
{

}

} // namespace liquid

} // namespace dex
