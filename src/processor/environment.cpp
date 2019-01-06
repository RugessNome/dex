// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/processor/environment.h"

#include "dex/processor/command.h"

namespace dex
{

QSharedPointer<Command> Environment::getCommand(const QString & name) const
{
  for (const auto & c : commands)
  {
    if (c->name() == name)
      return c;
  }

  return nullptr;
}

QSharedPointer<Environment> Environment::getEnvironment(const QString & name) const
{
  for (const auto & e : environments)
  {
    if (e->name() == name)
      return e;
  }

  return nullptr;
}

} // namespace dex
