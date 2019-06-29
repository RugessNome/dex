// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/core/value.h"

#include "dex/core/list.h"
#include "dex/core/ref.h"

#include <script/class.h>
#include <script/classtemplate.h>
#include <script/engine.h>
#include <script/function.h>
#include <script/namelookup.h>
#include <script/overloadresolution.h>

namespace dex
{

std::shared_ptr<ValueTypeInfo> ValueTypeInfo::get(const script::Function & f)
{
  return std::static_pointer_cast<ValueTypeInfo>(f.memberOf().data());
}

std::shared_ptr<ValueTypeInfo> ValueTypeInfo::get(const script::Type & t, script::Engine *e)
{
  auto it = cache().find(t.baseType().data());
  if (it != cache().end())
    return it->second;

  auto result = std::make_shared<ValueTypeInfo>();
  result->element_type = t;

  const script::Scope scp = t.isObjectType() ? script::Scope{ e->getClass(t) } : script::Scope{ e->rootNamespace() };

  script::NameLookup lookup = script::NameLookup::resolve(script::AssignmentOperator, scp);
  script::OverloadResolution resol = script::OverloadResolution::New(e);
  if (!resol.process(lookup.functions(), { script::Type::ref(t), script::Type::cref(t) }))
    throw std::runtime_error{ "T must be assignable" };
  result->assignment = resol.selectedOverload();

  lookup = script::NameLookup::resolve(script::EqualOperator, scp);
  resol = script::OverloadResolution::New(e);
  if (!resol.process(lookup.functions(), { script::Type::cref(t), script::Type::cref(t) }))
    throw std::runtime_error{ "T must have operator==" };
  result->eq = resol.selectedOverload();

  cache()[t.baseType().data()] = result;

  return result;
}

std::map<int, std::shared_ptr<ValueTypeInfo>> & ValueTypeInfo::cache()
{
  static std::map<int, std::shared_ptr<ValueTypeInfo>> ret = {};
  return ret;
}

Value::Value()
  : typeinfo(nullptr)
{

}

Value::Value(const Value & other)
  : typeinfo(other.typeinfo)
{
  if (isValid())
    value = engine()->copy(other.value);
}

Value::Value(Value && other)
  : typeinfo(other.typeinfo)
  , value(other.value)
{
  other.typeinfo = nullptr;
  other.value = script::Value{};
}

Value::Value(const std::shared_ptr<ValueTypeInfo> & c, const script::Value & val)
  : typeinfo(c)
{
  value = engine()->copy(val);
}

Value::Value(const script::Value & val, script::ParameterPolicy policy)
{
  typeinfo = ValueTypeInfo::get(val.type(), val.engine());
  if (policy != script::ParameterPolicy::Take)
    value = engine()->copy(val);
  else
    value = val;
}

Value::Value(script::Value && val)
{
  typeinfo = ValueTypeInfo::get(val.type(), val.engine());
  value = val;
}

Value::~Value()
{
  if (isValid())
    engine()->destroy(value);
  value = script::Value{};
}

bool Value::isRef() const
{
  if (isNull() || !impl().type().isObjectType())
    return false;

  script::Class cla = engine()->getClass(impl().type());
  return cla.isTemplateInstance() && cla.instanceOf() == engine()->getTemplate(script::Engine::RefTemplate);
}

script::Value Value::getRef() const
{
  return script::Value{ script::get<dex::ValuePtr>(impl()).value };
}

bool Value::isList() const
{
  if (isNull() || !impl().type().isObjectType())
    return false;

  script::Class cla = engine()->getClass(impl().type());
  return cla.isTemplateInstance() && cla.instanceOf() == engine()->getTemplate(script::Engine::ListTemplate);
}

QList<dex::Value> Value::getList() const
{
  return dex::list_cast(impl());
}

script::Engine * Value::engine() const
{
  return typeinfo->assignment.engine();
}

script::Value Value::release()
{
  typeinfo = nullptr;
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

  typeinfo = other.typeinfo;
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

  auto ret = engine()->invoke(typeinfo->eq, { value, other.value });
  bool result = ret.toBool();
  engine()->destroy(ret);
  return result;
}

TemporaryValueWrap::TemporaryValueWrap(const std::shared_ptr<ValueTypeInfo> & c, const script::Value & val)
  : Value()
{
  typeinfo = c;
  value = val;
}

TemporaryValueWrap::~TemporaryValueWrap()
{
  typeinfo = nullptr;
  value = script::Value{};
}

} // namespace dex