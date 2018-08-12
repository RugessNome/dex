// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/value.h"

#include <script/class.h>
#include <script/engine.h>
#include <script/function.h>

namespace dex
{

std::shared_ptr<ValueTypeInfo> ValueTypeInfo::get(const script::Function & f)
{
  return std::static_pointer_cast<ValueTypeInfo>(f.memberOf().data());
}

Value::Value()
  : container(nullptr)
{

}

Value::Value(const Value & other)
  : container(other.container)
{
  if (isValid())
    value = engine()->copy(other.value);
}

Value::Value(Value && other)
  : container(other.container)
  , value(other.value)
{
  other.container = nullptr;
  other.value = script::Value{};
}

Value::Value(const std::shared_ptr<ValueTypeInfo> & c, const script::Value & val)
  : container(c)
{
  value = engine()->copy(val);
}

Value::~Value()
{
  if (isValid())
    engine()->destroy(value);
  value = script::Value{};
}

script::Engine * Value::engine() const
{
  return container->assignment.engine();
}

script::Value Value::release()
{
  container = nullptr;
  script::Value ret = value;
  value = script::Value{};
  return ret;
}

Value & Value::operator=(const Value & other)
{
  if (value == other.value)
    return *(this);

  if (isValid())
    engine()->destroy(value);

  container = other.container;
  if (isValid())
    value = engine()->copy(other.value);

  return *(this);
}

bool Value::operator==(const Value & other) const
{
  if (other.isNull() && isNull())
    return true;

  if (other.isNull() != isNull())
    return false;

  auto ret = engine()->invoke(container->eq, { value, other.value });
  bool result = ret.toBool();
  engine()->destroy(ret);
  return result;
}

TemporaryValueWrap::TemporaryValueWrap(const std::shared_ptr<ValueTypeInfo> & c, const script::Value & val)
  : Value()
{
  container = c;
  value = val;
}

TemporaryValueWrap::~TemporaryValueWrap()
{
  container = nullptr;
  value = script::Value{};
}

} // namespace dex