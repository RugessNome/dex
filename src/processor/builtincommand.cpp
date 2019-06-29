// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/processor/builtincommand.h"

#include "dex/processor/documentprocessor.h"

#include <QDebug>

namespace dex
{

BuiltinCommand::BuiltinCommand(const QString & name, int param_count, CommandSpan::Value span, bool accepts_opts)
  : mName(name)
  , mParamCount(param_count)
  , mSpan(span)
  , mAcceptsOptions(accepts_opts)
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

bool BuiltinCommand::acceptsOptions() const
{
  return mAcceptsOptions;
}


BeginCommand::BeginCommand()
  : BuiltinCommand("begin", 1, CommandSpan::Word)
{

}

QString BeginCommand::getEnvironmentName(const QList<json::Json> & args)
{
  json::Json arg = args.front();

  QString env_name;
  if (DocumentProcessor::isWord(arg))
    env_name = arg.toString();
  else if (arg.isArray())
    env_name = arg.at(0).toString();
  else
    throw std::runtime_error{ "Invalid argument to begin" };

  return env_name;
}

QSharedPointer<Environment> BeginCommand::get_environment(DocumentProcessor *processor, const QList<json::Json>& args)
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

json::Json BeginCommand::invoke(DocumentProcessor *processor, const Options& opts, const QList<json::Json> & arguments)
{
  auto env = get_environment(processor, arguments);

  env->enter(opts);
  processor->enter(env);

  return nullptr;
}


EndCommand::EndCommand()
  : BuiltinCommand("end", 1, CommandSpan::Word, false)
{

}

json::Json EndCommand::invoke(DocumentProcessor *processor, const Options& opts, const QList<json::Json> & arguments)
{
  auto env = BeginCommand::get_environment(processor, arguments);

  env->leave();
  processor->leave();

  return nullptr;
}


InputCommand::InputCommand()
  : BuiltinCommand("input", 1, CommandSpan::Word, false)
{

}

json::Json InputCommand::invoke(DocumentProcessor *processor, const Options& opts, const QList<json::Json> & arguments)
{
  QString file;
  if (DocumentProcessor::isWord(arguments.first()))
  {
    file = arguments.first().toString();
  }
  else if (arguments.first().isArray())
  {
    file = processor->stringify(arguments.first().toArray());
  }
  else
  {
    qDebug() << "Invalid file for input command";
    throw std::runtime_error{ "Input command : invalid file" };
  }

  processor->input(file);

  return nullptr;
}

} // namespace dex