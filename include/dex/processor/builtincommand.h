// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_BUILTIN_COMMAND_H
#define DEX_BUILTIN_COMMAND_H

#include "dex/processor/command.h"

#include <script/function.h>

namespace dex
{

class Environment;

class BuiltinCommand : public Command
{
public:
  BuiltinCommand(const QString & name, int param_count, CommandSpan::Value span = CommandSpan::NotApplicable, bool accepts_opts = true);
  ~BuiltinCommand() = default;

  QString name() const override;
  int parameterCount() const override;
  CommandSpan::Value span() const override;
  bool acceptsOptions() const override;

private:
  QString mName;
  int mParamCount;
  CommandSpan::Value mSpan;
  bool mAcceptsOptions;
};

class BeginCommand : public BuiltinCommand
{
public:
  BeginCommand();

  static QString getEnvironmentName(const QList<json::Json> & args);
  static QSharedPointer<Environment> get_environment(DocumentProcessor *processor, const QList<json::Json>& args);

  json::Json invoke(DocumentProcessor *processor, const Options& opts, const QList<json::Json>& arguments) override;
};

class EndCommand : public BuiltinCommand
{
public:
  EndCommand();

  json::Json invoke(DocumentProcessor *processor, const Options& opts, const QList<json::Json>& arguments) override;
};

class InputCommand : public BuiltinCommand
{
public:
  InputCommand();

  json::Json invoke(DocumentProcessor *processor, const Options& opts, const QList<json::Json>& arguments) override;
};

} // namespace dex

#endif // DEX_BUILTIN_COMMAND_H
