// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_LIQUID_CONTEXT_H
#define DEX_LIQUID_CONTEXT_H

#include "dex/value.h"
#include "dex/liquid/filter.h"

#include <QMap>

namespace dex
{

namespace liquid
{

class Context
{
public:
  Context(script::Engine *e);

  inline script::Engine* engine() const { return mEngine; }

  inline QMap<QString, dex::Value> & variables() { return mVariables; }
  
  inline QMap<QString, std::shared_ptr<Filter>> & filters() { return mFilters; }
  void addFilter(Filter *f);

  template<typename T>
  void addFilter()
  {
    return addFilter(new T);
  }

  QString stringify(const dex::Value & value);
  inline std::vector<script::Function> & stringifyFunctions() { return mStringifyFunctions; }

  enum Flag {
    NoFlags = 0,
    Break = 1,
    Continue = 2,
  };

  int & runtimeFlags() { return mFlags; }

private:
  script::Engine *mEngine;
  QMap<QString, dex::Value> mVariables;
  QMap<QString, std::shared_ptr<Filter>> mFilters;
  std::vector<script::Function> mStringifyFunctions;
  int mFlags;
};

} // namespace liquid

} // namespace dex

#endif // DEX_LIQUID_CONTEXT_H
