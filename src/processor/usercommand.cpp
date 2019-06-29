// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/processor/usercommand.h"

#include "dex/core/options.h"

#include "dex/core/serialization.h"

#include <script/class.h>
#include <script/engine.h>
#include <script/operator.h>
#include <script/prototypes.h>

#include <QDebug>

namespace dex
{

QSharedPointer<Command> Command::build(const script::Function & fun)
{
  if (UserCommand::check(fun))
    return QSharedPointer<UserCommand>::create(fun);
  return nullptr;
}

QSharedPointer<Command> Command::build(const script::Class & cla)
{
  return UserCommand::create(cla);
}

UserCommand::UserCommand(const script::Function & fun)
  : mFunction(fun)
{
  mName = fun.name().data();
}

UserCommand::UserCommand(const QString & name, script::Value && object, script::Function func)
  : mName(name)
  , mObject(object)
  , mFunction(func)
{

}

UserCommand::~UserCommand()
{
  if (!mObject.isNull())
  {
    mObject.engine()->destroy(mObject);
    mObject = script::Value{};
  }
}

QString UserCommand::name() const
{
  return mName;
}

int UserCommand::parameterCount() const
{
  int count = mFunction.prototype().size();

  if (acceptsOptions())
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

bool UserCommand::acceptsOptions() const
{
  if (mFunction.prototype().size() == 0)
    return false;

  return mFunction.parameter(0).baseType() == script::make_type<Options>();
}

json::Json UserCommand::invoke(DocumentProcessor*, const Options& opts, const QList<json::Json> & arguments)
{
  if (parameterCount() != arguments.size())
    throw std::runtime_error{ "Invalid argument count" };

  script::Engine *e = mFunction.engine();

  std::vector<script::Value> values;

  int proto_offset = 0;

  if (!mObject.isNull())
  {
    ++proto_offset;
    values.push_back(mObject);
  }

  if (acceptsOptions())
  {
    values.push_back(e->construct<Options>(opts));
    ++proto_offset;
  }

  int span_offset = (span() != CommandSpan::NotApplicable) ? 1 : 0;

  for (size_t i(proto_offset); i < mFunction.prototype().size() - span_offset; ++i)
    values.push_back(dex::serialization::deserialize(arguments.at(i - proto_offset), mFunction.parameter(i)));

  auto command_span = span();
  if (command_span != CommandSpan::NotApplicable)
    values.push_back(CommandSpan::expose(command_span, e));

  script::Value val = e->call(mFunction, values);
  json::Json result = nullptr;

  if (val != script::Value::Void)
  {
    result = dex::serialization::serialize(val);
  }

  for (const auto & v : values)
    e->destroy(v);

  e->destroy(val);

  return result;
}

bool UserCommand::check(const script::Prototype & proto)
{
  if (proto.size() == 0)
    return true;

  int i = 0;
  if (proto.at(0).baseType() == script::make_type<Options>())
    i++;

  while (i < proto.size())
  {
    if (proto.at(i).baseType() != script::Type::Boolean
      && proto.at(i).baseType() != script::Type::Int
      && proto.at(i).baseType() != script::Type::String
      && proto.at(i).baseType() != script::Type::JsonArray
      && proto.at(i).baseType() != script::Type::JsonObject
      && proto.at(i).baseType() != script::Type::Json)
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

bool UserCommand::check(const script::Function & f)
{
  return check(f.prototype());
}

QSharedPointer<UserCommand> UserCommand::create(const script::Class & cla)
{
  script::Engine *e = cla.engine();

  if (!cla.inherits(e->getClass(Command::type_info().type)))
    return nullptr;

  if (!cla.isDefaultConstructible())
  {
    qDebug() << "Class" << cla.name().data() << "derived from Command must be default constructible";
    throw std::runtime_error{ "Invalid command class" };
  }

  script::Function name;

  for (const auto & f : cla.memberFunctions())
  {
    if (f.name() == "name" && f.returnType().baseType() == script::Type::String)
      name = f;
  }

  if (name.isNull())
  {
    qDebug() << "Class" << cla.name().data() << "derived from Command must define a name() function";
    throw std::runtime_error{ "Invalid command class" };
  }

  script::Function call_operator;
  for (const auto & op : cla.operators())
  {
    if (op.operatorId() == script::FunctionCallOperator)
      call_operator = op;
  }

  if (call_operator.isNull())
  {
    qDebug() << "Class" << cla.name().data() << "derived from Command must define a operator() function";
    throw std::runtime_error{ "Invalid command class" };
  }

  const script::Prototype & cop = call_operator.prototype();
  std::vector<script::Type> params{ cop.begin() + 1, cop.end() };
  script::DynamicPrototype proto{ call_operator.returnType(),  std::move(params) };
  if (!check(proto))
  {
    qDebug() << "Class" << cla.name().data() << "derived from Command has invalid operator()";
    throw std::runtime_error{ "Invalid command class" };
  }

  script::Value obj = e->construct(cla.id(), {});
  script::Value command_name_value = e->invoke(name, { obj });
  QString command_name = command_name_value.toString();
  e->destroy(command_name_value);

  return QSharedPointer<UserCommand>::create(command_name, std::move(obj), call_operator);
}

} // namespace dex