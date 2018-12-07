// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_STATE_H
#define DEX_STATE_H

#include <script/value.h>

namespace dex
{

class State
{
public:
  State() = default;
  State(const State & ) = default;
  ~State() = default;

  State(const script::Value & val);

  struct TypeInfo {
    script::Type type;
  };

  static TypeInfo static_type_info;
  inline static TypeInfo & type_info() { return static_type_info; }

  static State create(script::Engine *e);

  void destroy();

  State & operator=(const State & ) = default;

  inline const script::Value & get() const { return mValue; }
  inline operator const script::Value &() const { return mValue; }

private:
  script::Value mValue;
};

} // namespace dex

#endif // DEX_STATE_H
