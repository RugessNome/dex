// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_ENVIRONMENT_H
#define DEX_ENVIRONMENT_H

#include <QList>
#include <QSharedPointer>

namespace script
{
class Namespace;
} // namespace script

namespace dex
{

class Command;
class Options;

class Environment
{
public:

  virtual QString name() const = 0;

  virtual void enter(const Options& options) = 0;
  virtual void leave() = 0;

  QSharedPointer<Command> getCommand(const QString & name) const;
  QSharedPointer<Environment> getEnvironment(const QString & name) const;

  static QSharedPointer<Environment> build(const script::Namespace & ns);

  QList<QSharedPointer<Environment>> environments;
  QList<QSharedPointer<Command>> commands;
};

} // namespace dex

#endif // DEX_ENVIRONMENT_H
