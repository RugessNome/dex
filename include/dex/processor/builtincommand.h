// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_BUILTIN_COMMAND_H
#define DEX_BUILTIN_COMMAND_H

#include "dex/processor/command.h"

#include <script/function.h>

namespace dex
{

class BracketsArguments;
class Environment;
class NodeRef;

class BuiltinCommand : public Command
{
public:
  BuiltinCommand(const QString & name, int param_count, CommandSpan::Value span = CommandSpan::NotApplicable, bool accepts_brackets = true);
  ~BuiltinCommand() = default;

  QString name() const override;
  int parameterCount() const override;
  CommandSpan::Value span() const override;
  bool acceptsBracketArguments() const override;

private:
  QString mName;
  int mParamCount;
  CommandSpan::Value mSpan;
  bool mAcceptsBracketArguments;
};

class BeginCommand : public BuiltinCommand
{
public:
  BeginCommand();

  static QSharedPointer<Environment> get_environment(Parser *parser, const QList<NodeRef> & args);

  NodeRef invoke(Parser *parser, const BracketsArguments & brackets, const QList<NodeRef> & arguments) override;
};

class EndCommand : public BuiltinCommand
{
public:
  EndCommand();

  NodeRef invoke(Parser *parser, const BracketsArguments & brackets, const QList<NodeRef> & arguments) override;
};

class InputCommand : public BuiltinCommand
{
public:
  InputCommand();

  NodeRef invoke(Parser *parser, const BracketsArguments & brackets, const QList<NodeRef> & arguments) override;
};

} // namespace dex

#endif // DEX_BUILTIN_COMMAND_H
