// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/rootenvironment.h"

#include "dex/bracketsarguments.h"
#include "dex/command.h"

#include <script/class.h>
#include <script/function.h>
#include <script/namespace.h>
#include <script/script.h>

namespace dex
{

RootEnvironment::RootEnvironment()
{

}

QString RootEnvironment::name() const
{
  return QString{};
}

void RootEnvironment::fill(const script::Script & s)
{
  script::Namespace ns = s.rootNamespace();
  for (const auto & f : ns.functions())
  {
    auto command = dex::Command::build(f);
    if (command != nullptr)
      this->commands.append(command);
  }

  for (const auto & c : ns.classes())
  {
    auto command = dex::Command::build(c);
    if (command != nullptr)
      this->commands.append(command);
  }

  for (const auto & nns : ns.namespaces())
  {
    auto env = dex::Environment::build(nns);
    if (env != nullptr)
      this->environments.append(env);
  }
}

void RootEnvironment::enter(const BracketsArguments & brackets) 
{
  (void)brackets;
}

void RootEnvironment::leave()
{

}

} // namespace dex
