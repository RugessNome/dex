// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_VALUE_H
#define DEX_VALUE_H

#include <script/function.h>
#include <script/types.h>
#include <script/userdata.h>
#include <script/value.h>

namespace dex
{

struct ValueTypeInfo : public script::UserData
{
  script::Type element_type;
  script::Function assignment;
  script::Function eq;

  static std::shared_ptr<ValueTypeInfo> get(const script::Function & f);
};

// gives value semantic to a script::Value
struct Value
{
  Value();
  Value(const Value & other);
  Value(Value && other);
  Value(const std::shared_ptr<ValueTypeInfo> & c, const script::Value & val);
  ~Value();

  inline bool isValid() const { return typeinfo != nullptr; }
  inline bool isNull() const { return typeinfo == nullptr; }
  script::Engine *engine() const;

  script::Value release();

  Value & operator=(const Value & other);
  bool operator==(const Value & other) const;

  std::shared_ptr<ValueTypeInfo> typeinfo;
  script::Value value;
};

struct TemporaryValueWrap : public Value
{
  TemporaryValueWrap(const std::shared_ptr<ValueTypeInfo> & c, const script::Value & val);
  ~TemporaryValueWrap();
};

} // namespace dex

#endif // DEX_VALUE_H
