// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/builtincommand.h"

#include "dex/node.h"
#include "dex/parser.h"

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

QSharedPointer<Environment> BeginCommand::get_environment(Parser *parser, const QList<NodeRef> & args)
{
  NodeRef arg = args.front();

  QString env_name;
  if (arg.isWord())
    env_name = arg.toString();
  else if (arg.isGroup())
    env_name = arg.at(0).toString();
  else
    throw std::runtime_error{ "Invalid argument to begin" };

  auto env = parser->getEnvironment(env_name);
  if (env == nullptr)
  {
    qDebug() << "No such environment " << env_name;
    throw std::runtime_error{ "No such environment" };
  }

  return env;
}

NodeRef BeginCommand::invoke(Parser *parser, const BracketsArguments & brackets, const QList<NodeRef> & arguments)
{
  auto env = get_environment(parser, arguments);

  env->enter(brackets);
  parser->enter(env);
  
  return NodeRef{};
}


EndCommand::EndCommand()
  : BuiltinCommand("end", 1, CommandSpan::Word, false)
{

}

NodeRef EndCommand::invoke(Parser *parser, const BracketsArguments & brackets, const QList<NodeRef> & arguments)
{
  auto env = BeginCommand::get_environment(parser, arguments);

  env->leave();
  parser->leave();

  return NodeRef{};
}

} // namespace dex