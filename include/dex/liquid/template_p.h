// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_LIQUID_TEMPLATE_P_H
#define DEX_LIQUID_TEMPLATE_P_H

#include "dex/value.h"

#include <QList>
#include <QString>

namespace dex
{

namespace liquid
{

class TemplateNode
{
public:
  TemplateNode() = default;
  virtual ~TemplateNode() = default;

  template<typename T>
  bool is() const { return dynamic_cast<const T*>(this) != nullptr; }

  template<typename T>
  const T & as() const { return *dynamic_cast<const T*>(this); }

  template<typename T>
  T & as() { return *dynamic_cast<T*>(this); }
};

namespace tnodes
{

class Text : public TemplateNode
{
public:
  Text(const QString & str);
  ~Text() = default;

public:
  QString text;
};

/* Builtin objects */

class Object : public TemplateNode
{
public:
  Object() = default;
  ~Object() = default;
};

class Value : public Object
{
public:
  Value(const dex::Value & val);
  ~Value() = default;

public:
  dex::Value value;
};

class Variable : public Object
{
public:
  Variable(const QString & n);
  ~Variable() = default;

public:
  QString name;
};


class ArrayAccess : public Object
{
public:
  ArrayAccess(const std::shared_ptr<Object> & obj, const std::shared_ptr<Object> & ind);
  ~ArrayAccess() = default;

public:
  std::shared_ptr<Object> object;
  std::shared_ptr<Object> index;
};

class MemberAccess : public Object
{
public:
  MemberAccess(const std::shared_ptr<Object> & obj, const std::string & name);
  ~MemberAccess() = default;

public:
  std::shared_ptr<Object> object;
  std::string name;
};

class BinOp : public Object
{
public:
  enum Operation {
    Less, 
    Leq,
    Greater,
    Geq,
    Equal,
    Inequal,
    And,
    Or
  };

  BinOp(Operation op, const std::shared_ptr<Object> & left, const std::shared_ptr<Object> & right);
  ~BinOp() = default;

public:
  Operation operation;
  std::shared_ptr<Object> lhs;
  std::shared_ptr<Object> rhs;
};

class Pipe : public Object
{
public:
  Pipe(const std::shared_ptr<Object> & object, const QString & filtername, const QList<dex::Value> & args = {});
  ~Pipe() = default;

public:
  std::shared_ptr<Object> object;
  QString filterName;
  QList<dex::Value> arguments;
};

/* Builtin tags */

class Tag : public TemplateNode
{

};

class Assign : public Tag
{
public:
  Assign(const QString & varname, const std::shared_ptr<Object> & expr);
  ~Assign() = default;

public:
  QString variable;
  std::shared_ptr<Object> value;
};

class For : public Tag
{
public:
  For(const QString & varname, const std::shared_ptr<Object> & expr);
  ~For() = default;

public:
  QString variable;
  std::shared_ptr<Object> object;
  QList<std::shared_ptr<TemplateNode>> body;
};

class Break : public Tag
{
public:
  Break();
  ~Break() = default;
};

class Continue : public Tag
{
public:
  Continue();
  ~Continue() = default;
};

class If : public Tag
{
public:
  struct Block
  {
    std::shared_ptr<Object> condition;
    QList<std::shared_ptr<TemplateNode>> body;
  };

  If(std::shared_ptr<Object> cond);
  ~If() = default;

public:
  QList<Block> blocks;
};

} // tnodes

} // namespace liquid

} // namespace dex

#endif // DEX_LIQUID_TEMPLATE_P_H
