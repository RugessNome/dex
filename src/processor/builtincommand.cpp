// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/processor/builtincommand.h"

#include "dex/processor/groupnode.h"
#include "dex/processor/node.h"
#include "dex/processor/documentprocessor.h"

#include <QDebug>

namespace dex
{

BuiltinCommand::BuiltinCommand(const QString & name, int param_count, CommandSpan::Value span, bool accepts_brackets)
  : mName(name)
  , mParamCount(param_count)
  , mSpan(span)
  , mAcceptsBracketArguments(accepts_brackets)
{

}

QString BuiltinCommand::name() const
{
  return mName;
}

int BuiltinCommand::parameterCount() const
{
  return mParamCount;
}

CommandSpan::Value BuiltinCommand::span() const
{
  return mSpan;
}

bool BuiltinCommand::acceptsBracketArguments() const
{
  return mAcceptsBracketArguments;
}


BeginCommand::BeginCommand()
  : BuiltinCommand("begin", 1, CommandSpan::Word)
{

}

QString BeginCommand::getEnvironmentName(const QList<NodeRef> & args)
{
  NodeRef arg = args.front();

  QString env_name;
  if (arg.isWord())
    env_name = arg.toString();
  else if (arg.isGroup())
    env_name = arg.at(0).toString();
  else
    throw std::runtime_error{ "Invalid argument to begin" };

  return env_name;
}

QSharedPointer<Environment> BeginCommand::get_environment(DocumentProcessor *processor, const QList<NodeRef> & args)
{
  QString env_name = getEnvironmentName(args);

  auto env = processor->getEnvironment(env_name);
  if (env == nullptr)
  {
    qDebug() << "No such environment " << env_name;
    throw std::runtime_error{ "No such environment" };
  }

  return env;
}

NodeRef BeginCommand::invoke(DocumentProcessor *processor, const BracketsArguments & brackets, const QList<NodeRef> & arguments)
{
  auto env = get_environment(processor, arguments);

  env->enter(brackets);
  processor->enter(env);

  return NodeRef{};
}


EndCommand::EndCommand()
  : BuiltinCommand("end", 1, CommandSpan::Word, false)
{

}

NodeRef EndCommand::invoke(DocumentProcessor *processor, const BracketsArguments & brackets, const QList<NodeRef> & arguments)
{
  auto env = BeginCommand::get_environment(processor, arguments);

  env->leave();
  processor->leave();

  return NodeRef{};
}


InputCommand::InputCommand()
  : BuiltinCommand("input", 1, CommandSpan::Word, false)
{

}

NodeRef InputCommand::invoke(DocumentProcessor *processor, const BracketsArguments & brackets, const QList<NodeRef> & arguments)
{
  QString file;
  if (arguments.first().isWord())
  {
    file = arguments.first().toString();
  }
  else if (arguments.first().isGroup())
  {
    file = arguments.first().getNode().asGroupNode().toString();
  }
  else
  {
    qDebug() << "Invalid file for input command";
    throw std::runtime_error{ "Input command : invalid file" };
  }

  processor->input(file);

  return NodeRef{};
}

} // namespace dex