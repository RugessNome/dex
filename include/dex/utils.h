// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_UTILS_H
#define DEX_UTILS_H

#include <script/engine.h>
#include <script/function.h>
#include <script/value.h>

namespace dex
{

inline int readInt(const script::Value & val)
{
  int result = val.toInt();
  script::Engine *e = val.engine();
  e->destroy(val);
  return result;
}


} // namespace dex

#endif // DEX_UTILS_H
