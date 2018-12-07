// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/usercommand.h"

#include "dex/bracketsarguments.h"
#include "dex/node.h"

#include <script/engine.h>

namespace dex
{

QSharedPointer<Command> Command::build(const script::Function & fun)
{
  if (UserCommand::check(fun))
    return QSharedPointer<UserCommand>::create(fun);
  return nullptr;
}

UserCommand::UserCommand(const script::Function & fun)
  : mFunction(fun)
{
  mName = fun.name().data();
  if (mName.endsWith("_"))
    mName.chop(1);
}

QString UserCommand::name() const
{
  return mName;
}

int UserCommand::parameterCount() const
{
  int count = mFunction.prototype().size();

  if (acceptsBracketArguments())
    count -= 1;
  if (span() != CommandSpan::NotApplicable)
    count -= 1;

  return count;
}

CommandSpan::Value UserCommand::span() const
{
  static const script::Type span_types[] = {
    CommandSpan::getType(CommandSpan::Word),
    CommandSpan::getType(CommandSpan::Line),
    CommandSpan::getType(CommandSpan::Paragraph),
  };

  static const CommandSpan::Value span_values[] = {
    CommandSpan::Word,
    CommandSpan::Line,
    CommandSpan::Paragraph,
  };

  if (mFunction.prototype().size() == 0)
    return CommandSpan::NotApplicable;

  script::Type last_param = mFunction.prototype().at(mFunction.prototype().size() - 1);
  for (size_t i(0); i < 3; ++i)
  {
    if (last_param == span_types[i])
      return span_values[i];
  }

  return CommandSpan::NotApplicable;
}

bool UserCommand::acceptsBracketArguments() const
{
  if (mFunction.prototype().size() == 0)
    return false;

  return mFunction.parameter(0).baseType() == BracketsArguments::type_info().type;
}

NodeRef UserCommand::invoke(Parser*, const BracketsArguments & brackets, const QList<NodeRef> & arguments)
{
  if (parameterCount() != arguments.size())
    throw std::runtime_error{ "Invalid argument count" };

  script::Engine *e = mFunction.engine();

  std::vector<script::Value> values;

  int proto_offset = 0;
  if (acceptsBracketArguments())
  {
    values.push_back(brackets.expose(e));
    proto_offset = 1;
  }

  int span_offset = (span() != CommandSpan::NotApplicable) ? 1 : 0;

  for (size_t i(proto_offset); i < mFunction.prototype().size() - span_offset; ++i)
    values.push_back(convert(arguments.at(i - proto_offset), mFunction.parameter(i)));

  auto command_span = span();
  if (command_span != CommandSpan::NotApplicable)
    values.push_back(CommandSpan::expose(command_span, e));

  script::Value val = e->call(mFunction, values);
  NodeRef result{ std::move(val) };

  for (const auto & v : values)
    e->destroy(v);

  return result;
}

script::Value UserCommand::convert(const NodeRef & node, const script::Type & type)
{
  script::Engine *e = mFunction.engine();

  if (type.baseType() == script::Type::Boolean && node.isWord())
    return e->newBool(node.toBool());
  else if (type.baseType() == script::Type::Int && node.isWord())
    return e->newInt(node.toInt());
  else if (type.baseType() == script::Type::String && node.isWord())
    return e->newString(node.toString());
  else if (type.baseType() == NodeRef::type_info().type)
    return e->copy(node.impl());

  throw std::runtime_error{ "Could not convert argument" };
}

bool UserCommand::check(const script::Function & f)
{
  const auto & proto = f.prototype();
  if (proto.size() == 0)
    return true;

  int i = 0;
  if (proto.at(0).baseType() == BracketsArguments::type_info().type)
    i++;

  while (i < proto.size())
  {
    if (proto.at(i).baseType() != script::Type::Boolean
      && proto.at(i).baseType() != script::Type::Int
      && proto.at(i).baseType() != script::Type::String
      && proto.at(i).baseType() != NodeRef::type_info().type) 
    {
      if (i == proto.size() - 1)
      {
        if (proto.at(i) == CommandSpan::getType(CommandSpan::Word)
          || proto.at(i) == CommandSpan::getType(CommandSpan::Line)
          || proto.at(i) == CommandSpan::getType(CommandSpan::Paragraph))
        {
          return true;
        }
      }

      return false;
    }
     
    ++i;
  }

  return true;
}

} // namespace dex