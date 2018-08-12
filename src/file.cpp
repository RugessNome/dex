// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/file.h"

#include <script/class.h>
#include <script/classbuilder.h>
#include <script/engine.h>
#include <script/enum.h>
#include <script/enumbuilder.h>
#include <script/functionbuilder.h>
#include <script/namespace.h>
#include <script/interpreter/executioncontext.h>
#include <script/private/value_p.h>

namespace dex
{

File::TypeInfo File::static_type_info = File::TypeInfo{};

namespace callbacks
{

static script::Value ctor(script::FunctionCall *c)
{
  new (c->thisObject().memory()) QFile{};
  return c->thisObject();
}

static script::Value dtor(script::FunctionCall *c)
{
  (static_cast<QFile*>(c->thisObject().memory()))->~QFile();
  return script::Value::Void;
}

static script::Value ctor_string(script::FunctionCall *c)
{
  new (c->thisObject().memory()) QFile{ c->arg(0).toString() };
  return c->thisObject();
}

static script::Value close(script::FunctionCall *c)
{
  QFile & self = File::get(c->thisObject());
  self.close();
  return script::Value::Void;
}

static script::Value filename(script::FunctionCall *c)
{
  QFile & self = File::get(c->thisObject());
  return c->engine()->newString(self.fileName());
}

static script::Value set_filename(script::FunctionCall *c)
{
  QFile & self = File::get(c->thisObject());
  self.setFileName(c->arg(1).toString());
  return script::Value::Void;
}

static script::Value open(script::FunctionCall *c)
{
  QFile & self = File::get(c->thisObject());
  const bool result = self.open(File::getOpenMode(c->arg(1)));
  return c->engine()->newBool(result);
}

static script::Value read(script::FunctionCall *c)
{
  QFile & self = File::get(c->thisObject());
  QString text = QString::fromUtf8(self.read(c->arg(1).toInt()));
  return c->engine()->newString(text);
}

static script::Value read_line(script::FunctionCall *c)
{
  QFile & self = File::get(c->thisObject());
  QString text = QString::fromUtf8(self.readLine());
  return c->engine()->newString(text);
}

static script::Value read_all(script::FunctionCall *c)
{
  QFile & self = File::get(c->thisObject());
  QString text = QString::fromUtf8(self.readAll());
  return c->engine()->newString(text);
}

static script::Value size(script::FunctionCall *c)
{
  QFile & self = File::get(c->thisObject());
  return c->engine()->newInt(self.size());
}

static script::Value write(script::FunctionCall *c)
{
  QFile & self = File::get(c->thisObject());
  self.write(c->arg(1).toString().toUtf8());
  return script::Value::Void;
}

} // namespace callbacks


static void register_open_mode_flag(script::Class & file)
{
  using namespace script;

  Enum open_mode_flag = file.Enum("OpenModeFlag").get();
  open_mode_flag.addValue("ReadOnly", QIODevice::ReadOnly);
  open_mode_flag.addValue("WriteOnly", QIODevice::WriteOnly);
  open_mode_flag.addValue("ReadWrite", QIODevice::ReadWrite);
  open_mode_flag.addValue("Append", QIODevice::Append);
  open_mode_flag.addValue("Truncate", QIODevice::Truncate);
}

void File::register_type(script::Namespace ns)
{
  using namespace script;

  Class file = ns.Class("File").setFinal(true).get();
  type_info().type = file.id();
  register_open_mode_flag(file);

  file.Constructor(callbacks::ctor).create();
  file.newDestructor(callbacks::dtor);

  file.Constructor(callbacks::ctor_string).params(Type::String).create();

  file.Method("close", callbacks::close)
    .create();

  file.Method("fileName", callbacks::filename)
    .setConst()
    .returns(Type::String)
    .create();

  file.Method("setFileName", callbacks::set_filename)
    .params(Type::cref(Type::String))
    .create();

  file.Method("open", callbacks::open)
    .returns(Type::Boolean)
    .params(Type::Int)
    .create();

  file.Method("read", callbacks::read)
    .returns(Type::String)
    .params(Type::Int)
    .create();

  file.Method("readLine", callbacks::read_line)
    .returns(Type::String)
    .create();

  file.Method("readAll", callbacks::read_all)
    .returns(Type::String)
    .create();

  file.Method("size", callbacks::size)
    .setConst()
    .returns(Type::Int)
    .create();

  file.Method("write", callbacks::write)
    .params(Type::cref(Type::String))
    .create();
}

QFile & File::get(const script::Value & val)
{
  return *static_cast<QFile*>(val.memory());
}

QFile::OpenMode File::getOpenMode(const script::Value & val)
{
  return static_cast<QFile::OpenModeFlag>(val.toInt());
}

} // namespace dex
