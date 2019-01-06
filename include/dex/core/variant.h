// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_VARIANT_H
#define DEX_VARIANT_H

#include <script/value.h>

#include <QVariant>

namespace script
{
class Namespace;
} // namespace script

namespace dex
{

class Variant
{
public:

  struct TypeInfo {
    script::Type type;
  };

  static TypeInfo static_type_info;
  inline static TypeInfo & type_info() { return static_type_info; }

  static void register_type(script::Namespace ns);

  static script::Value create(script::Engine *e, const QVariant & value);
  static QVariant & get(script::Value val);
};

} // namespace dex

#endif // DEX_VARIANT_H
