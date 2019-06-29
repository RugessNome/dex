// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/core/options.h"

#include <script/class.h>
#include <script/classbuilder.h>
#include <script/constructorbuilder.h>
#include <script/destructorbuilder.h>
#include <script/engine.h>
#include <script/functionbuilder.h>
#include <script/operatorbuilder.h>
#include <script/interpreter/executioncontext.h>

namespace dex
{

namespace callbacks
{

/*!
 * class Options
 */

/*!
 * \fn Options()
 */
static script::Value ctor(script::FunctionCall *c)
{
  c->thisObject().init<Options>();
  return c->thisObject();
}

/*!
 * \fn Options(const Options &)
 */
static script::Value copy_ctor(script::FunctionCall* c)
{
  c->thisObject().init<Options>(script::get<Options>(c->arg(1)));
  return c->thisObject();
}

/*!
 * \fn ~Options()
 */
static script::Value dtor(script::FunctionCall *c)
{
  c->thisObject().destroy<Options>();
  return script::Value::Void;
}

/*!
 * \fn Options::Iterator begin() const
 */
static script::Value begin(script::FunctionCall* c)
{
  Options& self = script::get<Options>(c->arg(0));
  return c->engine()->construct<Options::Iterator>(self.data().cbegin());
}

/*!
 * \fn Options::Iterator end() const
 */
static script::Value end(script::FunctionCall* c)
{
  Options& self = script::get<Options>(c->arg(0));
  return c->engine()->construct<Options::Iterator>(self.data().cend());
}

/*!
 * class Iterator
 */

/*!
 * \fn Iterator()
 */
static script::Value iterator_ctor(script::FunctionCall* c)
{
  c->thisObject().init<Options::Iterator>();
  return c->thisObject();
}

/*!
 * \fn Iterator(const Iterator&)
 */
static script::Value iterator_copy_ctor(script::FunctionCall* c)
{
  c->thisObject().init<Options::Iterator>(script::get<Options::Iterator>(c->arg(1)));
  return c->thisObject();
}

/*!
 * \fn ~Iterator()
 */
static script::Value iterator_dtor(script::FunctionCall* c)
{
  c->thisObject().destroy<Options::Iterator>();
  return script::Value::Void;
}

/*!
 * \fn String key() const;
 */
static script::Value iterator_key(script::FunctionCall* c)
{
  Options::Iterator& self = script::get<Options::Iterator>(c->arg(0));
  return c->engine()->newString(self->first);
}

/*!
 * \fn json::Json value() const;
 */
static script::Value iterator_value(script::FunctionCall* c)
{
  Options::Iterator& self = script::get<Options::Iterator>(c->arg(0));
  return c->engine()->construct<json::Json>(self->second);
}

/*!
 * \fn Iterator& operator=(const Iterator&)
 */
static script::Value iterator_assign(script::FunctionCall* c)
{
  Options::Iterator& self = script::get<Options::Iterator>(c->arg(0));
  self = script::get<Options::Iterator>(c->arg(1));
  return c->arg(0);
}

/*!
 * \fn bool operator==(const Iterator&) const
 */
static script::Value iterator_eq(script::FunctionCall* c)
{
  Options::Iterator& self = script::get<Options::Iterator>(c->arg(0));
  Options::Iterator& other = script::get<Options::Iterator>(c->arg(1));
  return c->engine()->newBool(self == other);
}

/*!
 * \fn bool operator!=(const Iterator&) const
 */
static script::Value iterator_neq(script::FunctionCall* c)
{
  Options::Iterator& self = script::get<Options::Iterator>(c->arg(0));
  Options::Iterator& other = script::get<Options::Iterator>(c->arg(1));
  return c->engine()->newBool(self != other);
}

/*!
 * \fn Iterator operator++(int)
 */
static script::Value iterator_postincr(script::FunctionCall* c)
{
  Options::Iterator& self = script::get<Options::Iterator>(c->arg(0));
  return c->engine()->construct<Options::Iterator>(self++);
}

/*!
 * \fn Iterator operator--(int)
 */
static script::Value iterator_postdecr(script::FunctionCall* c)
{
  Options::Iterator& self = script::get<Options::Iterator>(c->arg(0));
  return c->engine()->construct<Options::Iterator>(self--);
}

} // namespace callbacks

static void register_iterator_type(script::Class& opts)
{
  using namespace script;

  Class self = opts.newNestedClass("Iterator").setId(Type::DexOptionsIterator).setFinal(true).get();

  self.newConstructor(callbacks::iterator_ctor).create();

  self.newConstructor(callbacks::iterator_copy_ctor)
    .params(make_type<const Options::Iterator&>()).create();

  self.newDestructor(callbacks::iterator_dtor).create();

  self.newMethod("key", callbacks::iterator_key)
    .returns(Type::String)
    .setConst().create();

  self.newMethod("value", callbacks::iterator_value)
    .returns(make_type<json::Json>())
    .setConst().create();

  self.newOperator(script::AssignmentOperator, callbacks::iterator_assign)
    .params(make_type<const Options::Iterator&>())
    .returns(make_type<Options::Iterator&>()).create();

  self.newOperator(script::EqualOperator, callbacks::iterator_eq)
    .returns(Type::Boolean)
    .setConst()
    .params(make_type<const Options::Iterator&>()).create();

  self.newOperator(script::InequalOperator, callbacks::iterator_neq)
    .returns(Type::Boolean)
    .setConst()
    .params(make_type<const Options::Iterator&>()).create();

  self.newOperator(script::PostIncrementOperator, callbacks::iterator_postincr)
    .returns(make_type<Options::Iterator>()).create();

  self.newOperator(script::PostDecrementOperator, callbacks::iterator_postdecr)
    .returns(make_type<Options::Iterator>()).create();
}

void Options::expose(script::Namespace& ns)
{
  using namespace script;

  Class self = ns.newClass("Options").setId(Type::DexOptions).setBase(Type::JsonObject).setFinal(true).get();

  self.newConstructor(callbacks::ctor).create();
  self.newConstructor(callbacks::copy_ctor).params(make_type<const Options&>()).create();
  self.newDestructor(callbacks::dtor).create();

  register_iterator_type(self);

  self.newMethod("begin", callbacks::begin)
    .returns(make_type<Options::Iterator>())
    .setConst().create();

  self.newMethod("end", callbacks::end)
    .returns(make_type<Options::Iterator>())
    .setConst().create();

}

} // namespace dex
