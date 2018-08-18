// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/bracketsarguments.h"

#include "dex/variant.h"

#include <script/classbuilder.h>
#include <script/engine.h>
#include <script/functionbuilder.h>
#include <script/interpreter/executioncontext.h>
#include <script/namespace.h>
#include <script/value.h>
#include <script/private/value_p.h>

namespace dex
{

BracketsArguments::TypeInfo BracketsArguments::static_type_info = BracketsArguments::TypeInfo{};

BracketsArguments & bracket_arguments_cast(const script::Value & val)
{
  return *static_cast<BracketsArguments*>(val.memory());
}

static BracketsArguments::Iterator & iterator_cast(const script::Value & val)
{
  return *static_cast<BracketsArguments::Iterator*>(val.memory());
}

static script::Value make_iterator(script::Engine *e, const BracketsArguments::Iterator & it)
{
  auto ret = e->uninitialized(BracketsArguments::type_info().iterator_type);
  new (ret.memory()) BracketsArguments::Iterator{ it };
  ret.impl()->remove_uninitialized_flag();
  return ret;
}

namespace callbacks
{

static script::Value ctor(script::FunctionCall *c)
{
  new (c->thisObject().memory()) BracketsArguments{};
  return c->thisObject();
}

static script::Value copy_ctor(script::FunctionCall *c)
{
  new (c->thisObject().memory()) BracketsArguments{ bracket_arguments_cast(c->arg(0)) };
  return c->thisObject();
}

static script::Value dtor(script::FunctionCall *c)
{
  (static_cast<BracketsArguments*>(c->thisObject().memory()))->~BracketsArguments();
  return script::Value::Void;
}

static script::Value contains(script::FunctionCall *c)
{
  BracketsArguments & self = bracket_arguments_cast(c->thisObject());
  const QString & key = c->arg(1).toString();
  return c->engine()->newBool(self.contains(key));
}

static script::Value count(script::FunctionCall *c)
{
  BracketsArguments & self = bracket_arguments_cast(c->thisObject());
  return c->engine()->newInt(self.size());
}

static script::Value at(script::FunctionCall *c)
{
  BracketsArguments & self = bracket_arguments_cast(c->thisObject());
  return make_iterator(c->engine(), self.begin() + c->arg(1).toInt());
}

static script::Value begin(script::FunctionCall *c)
{
  BracketsArguments & self = bracket_arguments_cast(c->thisObject());
  return make_iterator(c->engine(), self.begin());
}

static script::Value end(script::FunctionCall *c)
{
  BracketsArguments & self = bracket_arguments_cast(c->thisObject());
  return make_iterator(c->engine(), self.end());
}

static script::Value value(script::FunctionCall *c)
{
  BracketsArguments & self = bracket_arguments_cast(c->thisObject());
  return Variant::create(c->engine(), self.get(c->arg(1).toString(), QVariant{}));
}


namespace iterator
{

static script::Value ctor(script::FunctionCall *c)
{
  new (c->thisObject().memory()) BracketsArguments::Iterator{};
  return c->thisObject();
}

static script::Value copy_ctor(script::FunctionCall *c)
{
  BracketsArguments::Iterator & other = iterator_cast(c->arg(0));
  new (c->thisObject().memory()) BracketsArguments::Iterator{other};
  return c->thisObject();
}

static script::Value dtor(script::FunctionCall *c)
{
  using It = BracketsArguments::Iterator;
  (static_cast<It*>(c->thisObject().memory()))->~It();
  return script::Value::Void;
}

static script::Value key(script::FunctionCall *c)
{
  BracketsArguments::Iterator & self = iterator_cast(c->thisObject());
  return c->engine()->newString(self->first);
}

static script::Value value(script::FunctionCall *c)
{
  BracketsArguments::Iterator & self = iterator_cast(c->thisObject());
  return Variant::create(c->engine(), self->second);
}

static script::Value eq(script::FunctionCall *c)
{
  BracketsArguments::Iterator & self = iterator_cast(c->thisObject());
  BracketsArguments::Iterator & other = iterator_cast(c->arg(1));

  return c->engine()->newBool(self == other);
}

static script::Value neq(script::FunctionCall *c)
{
  BracketsArguments::Iterator & self = iterator_cast(c->thisObject());
  BracketsArguments::Iterator & other = iterator_cast(c->arg(1));

  return c->engine()->newBool(self != other);
}

static script::Value pre_incr(script::FunctionCall *c)
{
  BracketsArguments::Iterator & self = iterator_cast(c->thisObject());
  ++self;
  return c->thisObject();
}

static script::Value post_incr(script::FunctionCall *c)
{
  BracketsArguments::Iterator & self = iterator_cast(c->thisObject());
  script::Value ret = make_iterator(c->engine(), self);
  self++;
  return ret;
}

static script::Value pre_decr(script::FunctionCall *c)
{
  BracketsArguments::Iterator & self = iterator_cast(c->thisObject());
  --self;
  return c->thisObject();
}

static script::Value post_decr(script::FunctionCall *c)
{
  BracketsArguments::Iterator & self = iterator_cast(c->thisObject());
  script::Value ret = make_iterator(c->engine(), self);
  self--;
  return ret;
}

} // namspace iterator

} // namespace callbacks

void BracketsArguments::add(const QVariant & value)
{
  mPairs.push_back(QPair<QString, QVariant>(QString(), value));
}

void BracketsArguments::add(const QString & key, const QVariant & value)
{
  mPairs.push_back(QPair<QString, QVariant>(key, value));
  mMap[key] = value;
}

bool BracketsArguments::isEmpty() const
{
  return mPairs.isEmpty();
}

QVariant BracketsArguments::get(const QString & key, const QVariant & defaultValue) const
{
  return mMap.value(key, defaultValue);
}

QVariant BracketsArguments::get(int i) const
{
  return mPairs.at(i).second;
}

script::Value BracketsArguments::expose(script::Engine *e) const
{
  auto ret = e->uninitialized(BracketsArguments::type_info().type);
  new (ret.memory()) BracketsArguments{ *this };
  ret.impl()->remove_uninitialized_flag();
  return ret;
}

static void register_iterator_type(script::Class brackets)
{
  using namespace script;

  Class it = brackets.NestedClass("Iterator").setFinal().get();
  BracketsArguments::type_info().iterator_type = it.id();

  it.Constructor(callbacks::iterator::ctor).create();
  it.Constructor(callbacks::iterator::copy_ctor).params(Type::cref(it.id())).create();
  it.newDestructor(callbacks::iterator::dtor);

  it.Method("key", callbacks::iterator::key)
    .setConst()
    .returns(Type::String)
    .create();

  it.Method("value", callbacks::iterator::value)
    .setConst()
    .returns(Variant::type_info().type)
    .create();

  it.Operation(EqualOperator, callbacks::iterator::eq)
    .setConst()
    .params(Type::cref(it.id()))
    .returns(Type::Boolean)
    .create();

  it.Operation(InequalOperator, callbacks::iterator::neq)
    .setConst()
    .params(Type::cref(it.id()))
    .returns(Type::Boolean)
    .create();

  it.Operation(PreIncrementOperator, callbacks::iterator::pre_incr)
    .returns(Type::ref(it.id()))
    .create();

  it.Operation(PostIncrementOperator, callbacks::iterator::post_incr)
    .returns(it.id())
    .create();

  it.Operation(PreDecrementOperator, callbacks::iterator::pre_decr)
    .returns(Type::ref(it.id()))
    .create();

  it.Operation(PostDecrementOperator, callbacks::iterator::post_decr)
    .returns(it.id())
    .create();
}

void BracketsArguments::register_type(script::Namespace ns)
{
  using namespace script;

  Class c = ns.Class("BracketArguments").get();
  type_info().type = c.id();
  register_iterator_type(c);

  c.Constructor(callbacks::ctor).create();
  c.Constructor(callbacks::copy_ctor).params(Type::cref(c.id())).create();
  c.newDestructor(callbacks::dtor);

  c.Method("contains", callbacks::contains)
    .setConst()
    .returns(Type::Boolean)
    .params(Type::cref(Type::String))
    .create();

  c.Method("count", callbacks::count)
    .setConst()
    .returns(Type::Int)
    .create();

  c.Method("at", callbacks::at)
    .setConst()
    .returns(type_info().iterator_type)
    .create();

  c.Method("begin", callbacks::begin)
    .setConst()
    .returns(type_info().iterator_type)
    .create();

  c.Method("end", callbacks::end)
    .setConst()
    .returns(type_info().iterator_type)
    .create();

  c.Method("value", callbacks::value)
    .setConst()
    .returns(Variant::type_info().type)
    .params(Type::cref(Type::String))
    .create();

  /// TODO !!!

}

} // namespace dex
