// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_OPTIONS_H
#define DEX_OPTIONS_H

#include "dex/core/json.h"

#include <script/namespace.h>
#include <script/types.h>

namespace dex
{

class Options : public json::Object
{
public:
  using json::Object::Object;

  typedef std::map<QString, json::Json>::const_iterator Iterator;

  static void expose(script::Namespace& ns);
};

} // namespace dex

namespace script
{
template<> struct make_type_helper<dex::Options> { static Type get() { return Type::DexOptions; } };
template<> struct make_type_helper<dex::Options::Iterator> { static Type get() { return Type::DexOptionsIterator; } };
} // namespace script

#endif // !DEX_OPTIONS_H
