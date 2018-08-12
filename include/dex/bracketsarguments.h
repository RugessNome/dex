// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_BRACKETS_ARGUMENTS_H
#define DEX_BRACKETS_ARGUMENTS_H

#include <script/types.h>

#include <QList>
#include <QMap>
#include <QVariant>

namespace script
{
class Engine;
class Namespace;
class Value;
} // namespace script

namespace dex
{

class BracketsArguments
{
public:

  script::Value expose(script::Engine *e) const;

  void add(const QVariant & value);
  void add(const QString & key, const QVariant & value);

  bool isEmpty() const;

  inline bool contains(const QString & key) const { return mMap.contains(key); }
  inline int size() const { return mPairs.size(); }
  QVariant get(const QString & key, const QVariant & defaultValue = QVariant{}) const;
  QVariant get(int i) const;

  struct TypeInfo {
    script::Type type;
    script::Type iterator_type;
  };

  static TypeInfo static_type_info;
  static TypeInfo & type_info() { return static_type_info; }

  static void register_type(script::Namespace ns);

  typedef QList<QPair<QString, QVariant>>::iterator Iterator;
  inline Iterator begin() { return mPairs.begin(); }
  inline Iterator end() { return mPairs.end(); }

private:
  QList<QPair<QString, QVariant>> mPairs;
  QMap<QString, QVariant> mMap;
};

} // namespace dex

#endif // DEX_BRACKETS_ARGUMENTS_H
