// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/useroutput.h"

#include <script/engine.h>
#include <script/value.h>

namespace dex
{

QSharedPointer<Output> Output::build(const script::Function & func)
{
  if (func.returnType() != script::Type::Void)
    return nullptr;
  else if (func.prototype().parameterCount() != 1)
    return nullptr;
  else if (func.parameter(0).baseType() != script::Type::String)
    return nullptr;

  return QSharedPointer<UserOutput>::create(func);
}

UserOutput::UserOutput(const script::Function & func)
  : mFunction(func)
{

}

QString UserOutput::name() const
{
  return QString::fromUtf8(mFunction.name().data());
}

void UserOutput::invoke(const QString & outdir)
{
  script::Engine *e = mFunction.engine();
  script::Value arg = e->newString(outdir);
 
  e->call(mFunction, { arg });

  e->destroy(arg);
}

} // namespace dex
