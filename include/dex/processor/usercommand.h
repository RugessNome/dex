// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_USER_COMMAND_H
#define DEX_USER_COMMAND_H

#include "dex/processor/command.h"

#include <script/function.h>
#include <script/value.h>

namespace dex
{

class BracketsArguments;
class NodeRef;

class UserCommand : public Command
{
public:
  UserCommand(const script::Function & fun);
  UserCommand(const QString & name, script::Value && object, script::Function func);
  ~UserCommand();

  QString name() const override;
  int parameterCount() const override;
  CommandSpan::Value span() const override;
  bool acceptsBracketArguments() const override;

  NodeRef invoke(DocumentProcessor *, const BracketsArguments & brackets, const QList<NodeRef> & arguments) override;

  static bool check(const script::Prototype & proto);
  static bool check(const script::Function & f);

  static QSharedPointer<UserCommand> create(const script::Class & cla);

private:
  script::Value convert(const NodeRef & node, const script::Type & type);

private:
  QString mName;
  script::Value mObject;
  script::Function mFunction;
};

} // namespace dex

#endif // DEX_USER_COMMAND_H
