// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_NULL_H
#define DEX_NULL_H

#include <script/namespace.h>
#include <script/types.h>

namespace dex
{

struct Null 
{ 
  static void expose(script::Namespace& ns);
};

} // namespace dex

namespace script
{
template<> struct make_type_helper<dex::Null> { static Type get() { return Type::NullType; } };
} // namespace script

#endif // DEX_NULL_H
