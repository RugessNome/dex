// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_COMMAND_H
#define DEX_COMMAND_H

#include "dex/processor/commandspan.h"

#include <QString>
#include <QSharedPointer>

namespace script
{
class Class;
class Function;
} // namespace script

namespace dex
{

class BracketsArguments;
class NodeRef;
class Parser;

class Command
{
public:
  virtual ~Command() = default;

  virtual QString name() const = 0;
  virtual int parameterCount() const = 0;
  virtual CommandSpan::Value span() const = 0;
  virtual bool acceptsBracketArguments() const = 0;

  struct TypeInfo {
    script::Type type;
  };

  static TypeInfo static_type_info;
  inline static TypeInfo & type_info() { return static_type_info; }

  static void registerCommandType(script::Engine *e);
  static QSharedPointer<Command> build(const script::Function & func);
  static QSharedPointer<Command> build(const script::Class & cla);

  virtual NodeRef invoke(Parser *parser, const BracketsArguments & brackets, const QList<NodeRef> & arguments) = 0;
};

} // namespace dex


#endif // DEX_COMMAND_H
