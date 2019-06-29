// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/processor/userenvironment.h"

#include "dex/core/options.h"
#include "dex/processor/command.h"

#include <script/engine.h>
#include <script/namespace.h>
#include <script/value.h>

namespace dex
{

QSharedPointer<Environment> Environment::build(const script::Namespace & ns)
{
  script::Function enter_function;
  script::Function leave_function;

  for (const auto & f : ns.functions())
  {
    if (f.name() == "end" && f.returnType() == script::Type::Void && f.prototype().size() == 0)
    {
      leave_function = f;
    }
    else if (f.name() == "begin" && f.returnType() == script::Type::Void && f.prototype().size() == 1
      && f.parameter(0) == script::make_type<Options>())
    {
      enter_function = f;
    }
  }

  if (enter_function.isNull() || leave_function.isNull())
    return nullptr;

  auto result = QSharedPointer<UserEnvironment>::create(enter_function, leave_function);

  for (const auto & f : ns.functions())
  {
    if (f.name() == "begin" || f.name() == "end")
      continue;

    auto command = Command::build(f);
    if (command == nullptr)
      continue;

    result->commands.push_back(command);
  }


  for (const auto & n : ns.namespaces())
  {
    auto env = Environment::build(n);
    if (env == nullptr)
      continue;

    result->environments.push_back(env);
  }

  return result;
}

UserEnvironment::UserEnvironment(const script::Function & ef, const script::Function & lf)
  : mEnterFunction(ef)
  , mLeaveFunction(lf)
{

}

QString UserEnvironment::name() const
{
  return mEnterFunction.enclosingNamespace().name().data();
}

void UserEnvironment::enter(const Options& opts)
{
  script::Engine *e = mEnterFunction.engine();

  script::Value arg = e->construct<Options>(opts);
  e->call(mEnterFunction, { arg });
  e->destroy(arg);
}

void UserEnvironment::leave()
{
  script::Engine *e = mLeaveFunction.engine();
  e->call(mLeaveFunction, {});
}

} // namespace dex
