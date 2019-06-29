// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_SERIALIZATION_H
#define DEX_SERIALIZATION_H

#include "dex/core/json.h"

#include <script/types.h>
#include <script/value.h>

namespace script
{
class Namespace;
} // namespace script

namespace dex
{

struct DeserializationError
{
  json::Json data;
  script::Type target;

  DeserializationError(const json::Json& d, const script::Type& t)
    : data(d), target(t)
  {

  }
};

namespace serialization
{

json::Json serialize(const script::Value& val);
script::Value deserialize(const json::Json& json, script::Type type);

void expose(script::Namespace& ns);

} // namespace serialization

} // namespace dex

#endif // DEX_SERIALIZATION_H
