// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/core/list.h"

#include "dex/core/value.h"

#include <script/classtemplate.h>
#include <script/constructorbuilder.h>
#include <script/templatebuilder.h>
#include <script/classtemplateinstancebuilder.h>
#include <script/destructorbuilder.h>
#include <script/engine.h>
#include <script/functionbuilder.h>
#include <script/namelookup.h>
#include <script/operatorbuilder.h>
#include <script/overloadresolution.h>
#include <script/private/engine_p.h>
#include <script/private/value_p.h>
#include <script/templatebuilder.h>
#include <script/value.h>

#include <QList>

namespace dex
{

static script::Value make_list(const QList<Value> & list, const script::Type & list_type, script::Engine *e)
{
  return e->construct(list_type, [list](script::Value & v) -> void {
    new (v.memory()) QList<Value>{list};
  });
}

QList<dex::Value> & list_cast(const script::Value & val)
{
  return *reinterpret_cast<QList<dex::Value>*>(val.memory());
}

namespace callbacks
{

// QList<T>();
static script::Value default_ctor(script::FunctionCall *c)
{
  c->thisObject().init<QList<dex::Value>>();
  return c->thisObject();
}

// QList<T>(const QList<T> &);
static script::Value copy_ctor(script::FunctionCall *c)
{
  QList<dex::Value>& other = list_cast(c->arg(1));
  c->thisObject().init<QList<dex::Value>>(other);
  return c->thisObject();
}

// ~QList<T>();
static script::Value dtor(script::FunctionCall *c)
{
  c->thisObject().destroy<QList<dex::Value>>();
  return script::Value::Void;
}

// void append(const T &value)
// void push_back(const T &value)
static script::Value append_value(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  auto typeinfo = ValueTypeInfo::get(c->callee());
  self.append(Value{ typeinfo, c->arg(1) });
  return script::Value::Void;
}

// void append(const QList<T> &value)
static script::Value append_list(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  QList<Value> & other = list_cast(c->arg(1));
  self.append(other);
  return script::Value::Void;
}

// const T & at(int i) const
// const T & operator[](int i) const
static script::Value at(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  return self.at(c->arg(1).toInt()).value;
}

// T & back()
// const T & back() const
static script::Value back(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  return self.back().value;
}

// iterator begin()
/// TODO !!!
// const_iterator begin() const
/// TODO !!!
// const_iterator cbegin() const
/// TODO !!!
// const_iterator cend() const
/// TODO !!!

// void clear()
static script::Value clear(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  self.clear();
  return script::Value::Void;
}

// const_iterator constBegin() const
/// TODO !!!
// const_iterator constEnd() const
/// TODO !!!

// bool contains(const T &value) const
static script::Value contains(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(1) };
  const bool result = self.contains(value);
  return c->engine()->newBool(result);
}

// int count(const T &value) const
static script::Value count_element(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(1) };
  const int result = self.count(value);
  return c->engine()->newInt(result);
}

// int count() const
// int length() const
// int size() const
static script::Value count(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  return c->engine()->newInt(self.count());
}

// const_reverse_iterator crbegin() const

// const_reverse_iterator crend() const

// bool empty() const
// bool isEmpty() const
static script::Value empty(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  return c->engine()->newBool(self.isEmpty());
}

// iterator end()
// const_iterator end() const

// bool endsWith(const T &value) const
static script::Value ends_with(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(1) };
  const bool result = self.endsWith(value);
  return c->engine()->newBool(result);
}

// iterator erase(iterator pos)
/// l.add_fun<QList<T>::iterator, QList<T>::iterator, &QList<T>::erase>("erase");
// iterator erase(iterator begin, iterator end)
/// l.add_fun<QList<T>::iterator, QList<T>::iterator, QList<T>::iterator, &QList<T>::erase>("erase");

// T & first()
// const T & first() const
// const T & constFirst() const
// T & front()
// const T & front() const
static script::Value first(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  return self.first().value;
}

// int indexOf(const T &value, int from = 0) const
static script::Value index_of(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(1) };
  const int from = c->arg(2).toInt();
  const int result = self.indexOf(value, from);
  return c->engine()->newInt(result);
}

// void insert(int i, const T &value)
static script::Value insert(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int i = c->arg(1).toInt();
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(2) };
  self.insert(i, value);
  return script::Value::Void;
}

// iterator insert(iterator before, const T &value)
/// l.add_fun<QList<T>::iterator, QList<T>::iterator, const T &, &QList<T>::insert>("insert");

// T & last()
// const T & last() const
// const T & constLast() const
static script::Value last(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  return self.last().value;
}

// int lastIndexOf(const T &value, int from = -1) const
static script::Value last_index_of(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(1) };
  const int from = c->arg(2).toInt();
  const int result = self.lastIndexOf(value, from);
  return c->engine()->newInt(result);
}

// QList<T> mid(int pos, int length = -1) const
static script::Value mid(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int pos = c->arg(1).toInt();
  const int length = c->arg(2).toInt();
  return make_list(self.mid(pos, length), c->callee().returnType(), c->engine());
}

// void move(int from, int to)
static script::Value move(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int from = c->arg(1).toInt();
  const int to = c->arg(2).toInt();
  self.move(from, to);
  return script::Value::Void;
}

// void pop_back()
static script::Value pop_back(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  self.pop_back();
  return script::Value::Void;
}

// void pop_front()
static script::Value pop_front(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  self.pop_front();
  return script::Value::Void;
}

// void prepend(const T &value)
// void push_front(const T &value)
static script::Value prepend(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  auto typeinfo = ValueTypeInfo::get(c->callee());
  self.prepend(Value{ typeinfo, c->arg(1) });
  return script::Value::Void;
}

// reverse_iterator rbegin()
// const_reverse_iterator rbegin() const

// int removeAll(const T &value)
static script::Value remove_all(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(1) };
  const int result = self.removeAll(value);
  return c->engine()->newInt(result);
}

// void removeAt(int i)
static script::Value remove_at(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int i = c->arg(1).toInt();
  self.removeAt(i);
  return script::Value::Void;
}

// void removeFirst()
static script::Value remove_first(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  self.removeFirst();
  return script::Value::Void;
}

// void removeLast()
static script::Value remove_last(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  self.removeLast();
  return script::Value::Void;
}

// bool removeOne(const T &value)
static script::Value remove_one(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(1) };
  const bool result = self.removeOne(value);
  return c->engine()->newBool(result);
}

// reverse_iterator rend()
// const_reverse_iterator rend() const

// void replace(int i, const T &value)
static script::Value replace(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int i = c->arg(1).toInt();
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(2) };
  self.replace(i, value);
  return script::Value::Void;
}

// void reserve(int alloc)
static script::Value reserve(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int i = c->arg(1).toInt();
  self.reserve(i);
  return script::Value::Void;
}

// bool startsWith(const T &value) const
static script::Value starts_with(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(1) };
  const bool result = self.startsWith(value);
  return c->engine()->newBool(result);
}

// void swap(QList<T> &other)
static script::Value swap_list(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  QList<Value> & other = list_cast(c->arg(1));
  self.swap(other);
  return script::Value::Void;
}

// void swap(int i, int j)
static script::Value swap_element(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int i = c->arg(1).toInt();
  const int j = c->arg(2).toInt();
  self.swap(i, j);
  return script::Value::Void;
}

// T takeAt(int i)
static script::Value take_at(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int i = c->arg(1).toInt();
  auto ret = self.takeAt(i);
  return ret.release();
}

// T takeFirst()
static script::Value take_first(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  auto ret = self.takeFirst();
  return ret.release();
}

// T takeLast()
static script::Value take_last(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  auto ret = self.takeLast();
  return ret.release();
}

// QSet<T> toSet() const
// std::list<T> toStdList() const
// QVector<T> toVector() const

// T value(int i) const
static script::Value value(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int i = c->arg(1).toInt();
  Value ret = self.value(i);
  if (ret.isValid())
    return ret.release();
  auto typeinfo = ValueTypeInfo::get(c->callee());
  return c->engine()->construct(typeinfo->element_type, {});
}

// T value(int i, const T &defaultValue) const
static script::Value value_with_default(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int i = c->arg(1).toInt();
  Value ret = self.value(i, Value{});
  if (ret.isValid())
    return ret.release();
  return c->engine()->copy(c->arg(2));
}


// bool operator!=(const QList<T> &other) const
static script::Value op_neq(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  QList<Value> & other = list_cast(c->arg(1));
  const bool result = self != other;
  return c->engine()->newBool(result);
}

// QList<T> operator+(const QList<T> &other) const
static script::Value op_add(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  QList<Value> & other = list_cast(c->arg(1));
  return make_list(self + other, c->callee().returnType(), c->engine());
}

// QList<T> & operator+=(const QList<T> &other)
static script::Value op_add_assign(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  QList<Value> & other = list_cast(c->arg(1));
  self += other;
  return c->thisObject();
}

// QList<T> & operator+=(const T &value)
static script::Value op_add_assign_value(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  TemporaryValueWrap value{ ValueTypeInfo::get(c->callee()), c->arg(1) };
  self += value;
  return c->thisObject();
}

// QList<T> & operator<<(const QList<T> &other)
/// TODO !!!
// QList<T> & operator<<(const T &value)
/// TODO !!!
// QList<T> & operator=(const QList<T> &other)
static script::Value op_assign(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  QList<Value> & other = list_cast(c->arg(1));
  self = other;
  return c->thisObject();
}

// QList & operator=(QList<T> &&other)
/// TODO !!!

// bool operator==(const QList<T> &other) const
static script::Value op_eq(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  QList<Value> & other = list_cast(c->arg(1));
  const bool result = self == other;
  return c->engine()->newBool(result);
}

// T & operator[](int i)
static script::Value op_subscript(script::FunctionCall *c)
{
  QList<Value> & self = list_cast(c->thisObject());
  const int i = c->arg(1).toInt();
  return self[i].value;
}

} // namespace callbacks

script::Class ListTemplate::instantiate(script::ClassTemplateInstanceBuilder& builder)
{
  using namespace script;

  builder.setFinal();
  const Type element_type = builder.arguments().front().type;

  {
    try
    {
      builder.setData(ValueTypeInfo::get(element_type, builder.getTemplate().engine()));
    }
    catch (std::runtime_error & ex)
    {
      throw TemplateInstantiationError{ std::string("List<T> : ") + ex.what() };
    }
  }

  Class list = builder.get();

  // QList<T>();
  list.newConstructor(callbacks::default_ctor).create();
  // QList<T>(const QList<T> &);
  list.newConstructor(callbacks::copy_ctor)
    .params(Type::cref(list.id())).create();
  // ~QList<T>();
  list.newDestructor(callbacks::dtor).create();

  // void append(const T &value)
  list.newMethod("append", callbacks::append_value)
    .params(Type::cref(element_type)).create();
  // void append(const QList<T> &value)
  list.newMethod("append", callbacks::append_list)
    .params(Type::cref(list.id())).create();
  // const T & at(int i) const
  list.newMethod("at", callbacks::at)
    .setConst()
    .returns(Type::cref(element_type))
    .params(Type::Int).create();
  // T & back()
  list.newMethod("back", callbacks::back)
    .returns(Type::ref(element_type)).create();
  // const T & back() const
  list.newMethod("back", callbacks::back)
    .setConst()
    .returns(Type::cref(element_type)).create();
  // iterator begin()
  /// TODO !!!
  // const_iterator begin() const
  /// TODO !!!
  // const_iterator cbegin() const
  /// TODO !!!
  // const_iterator cend() const
  /// TODO !!!
  // void clear()
  list.newMethod("clear", callbacks::clear).create();
  // const_iterator constBegin() const
  /// TODO !!!
  // const_iterator constEnd() const
  /// TODO !!!
  // const T & constFirst() const
  list.newMethod("constFirst", callbacks::first)
    .returns(Type::cref(element_type)).create();
  // const T & constLast() const
  list.newMethod("constLast", callbacks::last)
    .returns(Type::cref(element_type)).create();
  // bool contains(const T &value) const
  list.newMethod("contains", callbacks::contains)
    .setConst()
    .returns(Type::Boolean)
    .params(Type::cref(element_type)).create();
  // int count(const T &value) const
  list.newMethod("contains", callbacks::count_element)
    .setConst()
    .returns(Type::Int)
    .params(Type::cref(element_type)).create();
  // int count() const
  list.newMethod("count", callbacks::count)
    .setConst()
    .returns(Type::Int).create();
  // const_reverse_iterator crbegin() const
  /// TODO !!!
  // const_reverse_iterator crend() const
  /// TODO !!!
  // bool empty() const
  list.newMethod("empty", callbacks::empty)
    .setConst()
    .returns(Type::Boolean).create();
  // iterator end()
  /// TODO !!!
  // const_iterator end() const
  /// TODO !!!
  // bool endsWith(const T &value) const
  list.newMethod("endsWith", callbacks::ends_with)
    .setConst()
    .returns(Type::Boolean)
    .params(Type::cref(element_type)).create();
  // iterator erase(iterator pos)
  /// TODO !!!
  // iterator erase(iterator begin, iterator end)
  /// TODO !!!
  // T & first()
  list.newMethod("first", callbacks::first)
    .returns(Type::ref(element_type)).create();
  // const T & first() const
  list.newMethod("first", callbacks::first)
    .setConst()
    .returns(Type::cref(element_type)).create();
  // T & front()
  list.newMethod("front", callbacks::first)
    .returns(Type::ref(element_type)).create();
  // const T & front() const
  list.newMethod("front", callbacks::first)
    .setConst()
    .returns(Type::cref(element_type)).create();
  // int indexOf(const T &value, int from = 0) const
  list.newMethod("indexOf", callbacks::index_of)
    .setConst()
    .returns(Type::Int)
    .params(Type::cref(element_type), Type::Int).create();
  // void insert(int i, const T &value)
  list.newMethod("insert", callbacks::insert)
    .params(Type::Int, Type::cref(element_type)).create();
  // iterator insert(iterator before, const T &value)
  /// l.add_fun<QList<T>::iterator, QList<T>::iterator, const T &, &QList<T>::insert>("insert");
  // bool isEmpty() const
  list.newMethod("isEmpty", callbacks::empty)
    .setConst()
    .returns(Type::Boolean).create();
  // T & last()
  list.newMethod("last", callbacks::last)
    .returns(Type::ref(element_type)).create();
  // const T & last() const
  list.newMethod("last", callbacks::last)
    .setConst()
    .returns(Type::cref(element_type)).create();
  // int lastIndexOf(const T &value, int from = -1) const
  list.newMethod("lastIndexOf", callbacks::last_index_of)
    .setConst()
    .returns(Type::Int)
    .params(Type::cref(element_type), Type::Int).create();
  // int length() const
  list.newMethod("length", callbacks::count)
    .setConst()
    .returns(Type::Int).create();
  // QList<T> mid(int pos, int length = -1) const
  list.newMethod("mid", callbacks::mid)
    .setConst()
    .returns(list.id())
    .params(Type::Int, Type::Int).create();
  // void move(int from, int to)
  list.newMethod("move", callbacks::move)
    .params(Type::Int, Type::Int).create();
  // void pop_back()
  list.newMethod("pop_back", callbacks::pop_back).create();
  // void pop_front()
  list.newMethod("pop_front", callbacks::pop_front).create();
  // void prepend(const T &value)
  list.newMethod("prepend", callbacks::prepend)
    .params(Type::cref(element_type)).create();
  // void push_back(const T &value)
  list.newMethod("push_back", callbacks::append_value)
    .params(Type::cref(element_type)).create();
  // void push_front(const T &value)
  list.newMethod("push_front", callbacks::prepend)
    .params(Type::cref(element_type)).create();
  // reverse_iterator rbegin()
  /// TODO !!!
  // const_reverse_iterator rbegin() const
  /// TODO !!!
  // int removeAll(const T &value)
  list.newMethod("removeAll", callbacks::remove_all)
    .returns(Type::Int)
    .params(Type::cref(element_type)).create();
  // void removeAt(int i)
  list.newMethod("removeAt", callbacks::remove_at)
    .params(Type::Int).create();
  // void removeFirst()
  list.newMethod("removeFirst", callbacks::remove_first)
    .params(Type::Int, Type::Int).create();
  // void removeLast()
  list.newMethod("removeLast", callbacks::remove_last)
    .params(Type::Int, Type::Int).create();
  // bool removeOne(const T &value)
  list.newMethod("removeOne", callbacks::remove_one)
    .returns(Type::Boolean)
    .params(Type::cref(element_type)).create();
  // reverse_iterator rend()
  /// TODO !!!
  // const_reverse_iterator rend() const
  /// TODO !!!
  // void replace(int i, const T &value)
  list.newMethod("replace", callbacks::replace)
    .params(Type::Int, Type::cref(element_type)).create();
  // void reserve(int alloc)
  list.newMethod("reserve", callbacks::reserve)
    .params(Type::Int).create();
  // int size() const
  list.newMethod("size", callbacks::count)
    .setConst()
    .returns(Type::Int).create();
  // bool startsWith(const T &value) const
  list.newMethod("startsWith", callbacks::starts_with)
    .setConst()
    .returns(Type::Boolean)
    .params(Type::cref(element_type)).create();
  // void swap(QList<T> &other)
  list.newMethod("swap", callbacks::swap_list)
    .params(Type::ref(list.id())).create();
  // void swap(int i, int j)
  list.newMethod("swap", callbacks::swap_element)
    .params(Type::Int, Type::Int).create();
  // T takeAt(int i)
  list.newMethod("takeAt", callbacks::take_at)
    .returns(element_type)
    .params(Type::Int).create();
  // T takeFirst()
  list.newMethod("takeFirst", callbacks::take_first)
    .returns(element_type).create();
  // T takeLast()
  list.newMethod("takeLast", callbacks::take_last)
    .returns(element_type).create();
  // QSet<T> toSet() const
  /// TODO !!!
  // std::list<T> toStdList() const
  /// TODO !!!
  // QVector<T> toVector() const
  /// TODO !!!
  // T value(int i) const
  list.newMethod("value", callbacks::value)
    .setConst()
    .returns(element_type)
    .params(Type::Int).create();
  // T value(int i, const T &defaultValue) const
  list.newMethod("value", callbacks::value_with_default)
    .setConst()
    .returns(element_type)
    .params(Type::Int, Type::cref(element_type)).create();


  // bool operator!=(const QList<T> &other) const
  list.newOperator(InequalOperator, callbacks::op_neq)
    .setConst()
    .returns(Type::Boolean)
    .params(Type::cref(list.id())).create();
  // QList<T> operator+(const QList<T> &other) const
  list.newOperator(AdditionOperator, callbacks::op_add)
    .setConst()
    .returns(list.id())
    .params(Type::cref(list.id())).create();
  // QList<T> & operator+=(const QList<T> &other)
  list.newOperator(AdditionAssignmentOperator, callbacks::op_add_assign)
    .returns(Type::ref(list.id()))
    .params(Type::cref(list.id())).create();
  // QList<T> & operator+=(const T &value)
  list.newOperator(AdditionAssignmentOperator, callbacks::op_add_assign_value)
    .returns(Type::ref(list.id()))
    .params(Type::cref(element_type)).create();
  // QList<T> & operator<<(const QList<T> &other)
  /// TODO !!!
  // QList<T> & operator<<(const T &value)
  /// TODO !!!
  // QList<T> & operator=(const QList<T> &other)
  list.newOperator(AssignmentOperator, callbacks::op_assign)
    .returns(Type::ref(list.id()))
    .params(Type::cref(list.id())).create();
  // QList & operator=(QList<T> &&other)
  /// TODO !!!
  // bool operator==(const QList<T> &other) const
  list.newOperator(EqualOperator, callbacks::op_eq)
    .setConst()
    .returns(Type::Boolean)
    .params(Type::cref(list.id())).create();
  // T & operator[](int i)
  list.newOperator(SubscriptOperator, callbacks::op_subscript)
    .returns(Type::ref(element_type))
    .params(Type::Int).create();
  // const T & operator[](int i) const
  list.newOperator(SubscriptOperator, callbacks::at)
    .setConst()
    .returns(Type::cref(element_type))
    .params(Type::Int).create();

  return list;
}

void register_list_template(script::Namespace n)
{
  using namespace script;

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
  };

  ClassTemplate list_template = Symbol{ n }.newClassTemplate("List")
    .setParams(std::move(params))
    .setScope(Scope{ n })
    .withBackend<ListTemplate>()
    .get();
}

} // namespace dex