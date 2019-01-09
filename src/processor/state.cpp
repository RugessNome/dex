// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/processor/state.h"

#include "dex/processor/node.h"

#include <script/class.h>
#include <script/engine.h>

#include <QDebug>

namespace dex
{

State::TypeInfo State::static_type_info = State::TypeInfo{};


State::State(const script::Value & val)
  : mValue(val)
{

}

State State::create(script::Engine *e)
{
  State ret{ e->construct(type_info().type, {}) };

  script::Class state_class = e->getClass(type_info().type);

  for (const auto & f : state_class.memberFunctions())
  {
    /// TODO: be a little bit more precise about what is expected...
    if (f.name() == "init" && f.returnType() == script::Type::Void && f.prototype().count() == 1)
      ret.mInit = f;
    else if (f.name() == "beginFile" && f.returnType() == script::Type::Void)
      ret.mBeginFile = f;
    else if (f.name() == "endFile" && f.returnType() == script::Type::Void)
      ret.mEndFile = f;
    else if (f.name() == "beginBlock" && f.returnType() == script::Type::Void)
      ret.mBeginBlock = f;
    else if (f.name() == "endBlock" && f.returnType() == script::Type::Void)
      ret.mEndBlock = f;
    else if (f.name() == "dispatch" && f.returnType() == script::Type::Void)
      ret.mDispatch = f;
  }

  if (ret.mInit.isNull() || ret.mBeginFile.isNull() || ret.mEndFile.isNull() || ret.mBeginBlock.isNull() || ret.mEndBlock.isNull() || ret.mDispatch.isNull())
  {
    qDebug() << "State class does not define some required members";
    throw std::runtime_error{ "State class does not define some required members" };
  }

  return ret;
}

void State::init()
{
  script::Engine *e = engine();
  e->call(mInit, { mValue });
}

void State::beginFile(const QString & path)
{
  script::Engine *e = engine();
  script::Value arg = e->newString(path);
  e->call(mBeginFile, { mValue, arg });
  e->destroy(arg);
}

void State::endFile()
{
  script::Engine *e = engine();
  e->call(mEndFile, {mValue});
}

void State::beginBlock()
{
  script::Engine *e = engine();
  e->call(mBeginBlock, { mValue });
}

void State::endBlock()
{
  script::Engine *e = engine();
  e->call(mEndBlock, { mValue });
}

void State::dispatch(const NodeRef & node)
{
  script::Engine *e = engine();
  e->call(mDispatch, { mValue, node.impl() });
}

void State::destroy()
{
  mValue.engine()->destroy(mValue);
  mValue = script::Value{};
}

} // namespace dex
