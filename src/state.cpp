// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/state.h"

#include <script/engine.h>

namespace dex
{

State::TypeInfo State::static_type_info = State::TypeInfo{};


State::State(const script::Value & val)
  : mValue(val)
{

}

State State::create(script::Engine *e)
{
  return e->construct(type_info().type, {});
}

} // namespace dex
