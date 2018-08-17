// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/dex.h"

#include "dex/list.h"
#include "dex/ref.h"

#include "dex/endoflinenode.h"
#include "dex/groupnode.h"
#include "dex/spacenode.h"
#include "dex/wordnode.h"

#include "dex/bracketsarguments.h"
#include "dex/file.h"
#include "dex/variant.h"

#include "dex/rootenvironment.h"
#include "dex/builtincommand.h"

#include "dex/parser.h"

#include "dex/output.h"

#include <script/class.h>
#include <script/enum.h>
#include <script/functionbuilder.h>
#include <script/interpreter/executioncontext.h>
#include <script/script.h>

#include <QDebug>

#include <iostream>

namespace callbacks
{

script::Value print_int(script::FunctionCall *c)
{
  int n = c->arg(0).toInt();
  qDebug() << n;
  return script::Value::Void;
}

script::Value print_bool(script::FunctionCall *c)
{
  qDebug() << c->arg(0).toBool();
  return script::Value::Void;
}

script::Value print_double(script::FunctionCall *c)
{
  qDebug() << c->arg(0).toDouble();
  return script::Value::Void;
}

script::Value print_string(script::FunctionCall *c)
{
  qDebug() << c->arg(0).toString().toUtf8().data();
  return script::Value::Void;
}

} // namespace callbacks

Application::Application(int & argc, char **argv)
  : QApplication(argc, argv)
{
  mEngine.setup();

  mEngine.reserveTypeRange(script::Type::FirstEnumType, script::Type::LastEnumType);
  mEngine.reserveTypeRange(script::Type::FirstClassType, script::Type::LastClassType);

  dex::register_ref_template(mEngine.rootNamespace());
  dex::register_list_template(mEngine.rootNamespace());

  mEngine.rootNamespace().Function("print", callbacks::print_int)
    .params(script::Type::Int).create();

  mEngine.rootNamespace().Function("print", callbacks::print_bool)
    .params(script::Type::Boolean).create();

  mEngine.rootNamespace().Function("print", callbacks::print_double)
    .params(script::Type::Double).create();

  mEngine.rootNamespace().Function("print", callbacks::print_string)
    .params(script::Type::cref(script::Type::String)).create();

  dex::Variant::register_type(mEngine.rootNamespace());
  dex::File::register_type(mEngine.rootNamespace());

  mInputDirectory = QDir::current();
}

static script::Script get_script(const std::vector<script::Script> & list, const std::string & path)
{
  for (const auto & s : list)
  {
    if (s.path() == path)
      return s;
  }

  return { };
}

int Application::run()
{
  try {
    parserCommandLineArgs();
    setup();
    process(mInputDirectory.absolutePath());
    output(mOutputFormat, mOutputDirectory.absolutePath());
  }
  catch (...)
  {
    return 1;
  }

  return 0;
}

void Application::setup()
{
  if (!mProfileDir.exists())
  {
    qDebug() << "Profile dir does not exist";
    throw std::runtime_error{ "Profile dir does not exists" };
  }

  scriptEngine()->setScriptExtension(".dex");
  scriptEngine()->setSearchDirectory(mProfileDir.absolutePath().toUtf8().data());

  register_span_types();
  dex::BracketsArguments::register_type(scriptEngine()->rootNamespace());
  load_nodes();
  load_state();
  
  mState = dex::State::create(scriptEngine());
  scriptEngine()->rootNamespace().addValue("state", mState);
  scriptEngine()->manage(mState);

  QDir commands = QDir{ mProfileDir.absoluteFilePath("commands") };

  QList<script::Script> scripts;
  for (const auto & f : commands.entryInfoList())
  {
    if (f.suffix() != "dex")
      continue;

    script::Script s = get_script(scriptEngine()->scripts(), f.absoluteFilePath().toUtf8().data());
    if (s.isNull())
    {
      s = scriptEngine()->newScript(script::SourceFile{ f.absoluteFilePath().toUtf8().data() });
      if (!s.compile())
      {
        qDebug() << "Failed to compile " << f.filePath();
        for (const auto &m : s.messages())
          qDebug() << m.to_string().data();
        throw std::runtime_error{ "Failed to compile a script" };
      }
    }

    scripts.push_back(s);
  }

  auto root_env = QSharedPointer<dex::RootEnvironment>::create();
  root_env->commands.append(QSharedPointer<dex::BeginCommand>::create());
  root_env->commands.append(QSharedPointer<dex::EndCommand>::create());

  for (const auto & s : scripts)
    root_env->fill(s);

  mRootEnvironment = root_env;

  load_outputs();
}

void Application::parserCommandLineArgs()
{
  QStringList args = QCoreApplication::arguments();

  for (int i(0); i < args.size(); ++i)
  {
    if (args.at(i) == "-p")
      mProfileDir = QDir{ args.at(i + 1) };

    if (args.at(i) == "-i")
      mInputDirectory = QDir{ args.at(i + 1) };

    if (args.at(i) == "-o")
      mOutputDirectory = QDir{ args.at(i + 1) };

    if (args.at(i) == "-g")
      mOutputFormat = args.at(i + 1);
  }
}

void Application::register_span_types()
{
  dex::CommandSpan::register_span_types(scriptEngine()->rootNamespace());
}

static void register_node_type(Application &app, const script::Class &node)
{
  dex::Node::register_node_type(node);
}

static void register_word_node_type(Application &app, const script::Class &wordnode)
{
  auto& ti = dex::WordNode::type_info();
  ti.type = wordnode.id();

  for (const auto & f : wordnode.memberFunctions())
  {
    if (f.name() == "value" && f.returnType().baseType() == script::Type::String && f.prototype().size() == 1)
    {
      ti.get_value = f;
      return;
    }
  }

  throw std::runtime_error{ "Could not find WordNode::value() member function" };
}

static void register_space_node_type(Application &app, const script::Class &spacenode)
{
  auto& ti = dex::SpaceNode::type_info();
  ti.type = spacenode.id();

  for (const auto & f : spacenode.memberFunctions())
  {
    if (f.name() == "value" && f.returnType().baseType() == script::Type::String && f.prototype().size() == 1)
    {
      ti.get_value = f;
      return;
    }
  }

  throw std::runtime_error{ "Could not find Space::value() member function" };
}

static void register_eol_node_type(Application &app, const script::Class &eolnode)
{
  auto& ti = dex::EndOfLineNode::type_info();
  ti.type = eolnode.id();
}


static void register_groupnode_type(Application & app, const script::Class &groupnode)
{
  if (!groupnode.inherits(app.scriptEngine()->getClass(dex::Node::type_info().type)))
    throw std::runtime_error{ "GroupNode class must be derived from Node" };

  auto& ti = dex::GroupNode::type_info();
  ti.type = groupnode.id();

  for (const auto & f : groupnode.memberFunctions())
  {
    if (f.name() == "size" && f.returnType() == script::Type::Int && f.prototype().size() == 1)
      ti.size = f;
    else if (f.name() == "at" && f.returnType().baseType() == dex::NodeRef::type_info().type
      && f.prototype().size() == 2 && f.prototype().at(1).baseType() == script::Type::Int)
      ti.at = f;
    else if (f.name() == "push_back" && f.parameter(1).baseType() == dex::NodeRef::type_info().type)
      ti.push_back = f;
  }

  if (ti.size.isNull() || ti.at.isNull() || ti.push_back.isNull())
    throw std::runtime_error{ "One or more required member functions of GroupNode could not be found" };
}


void Application::load_nodes()
{
  using namespace script;

  Script s = mEngine.newScript(SourceFile{ mProfileDir.absoluteFilePath("node.dex").toUtf8().data() });
  if (!s.compile())
  {
    for (const auto &m : s.messages())
      qDebug() << m.to_string().data();

    return;
  }

  typedef void(*ClassActionCallback)(Application&, const script::Class&);
  QMap<std::string, ClassActionCallback> actions;
  actions["Node"] = register_node_type;
  actions["EndOfLine"] = register_eol_node_type;
  actions["Space"] = register_space_node_type;
  actions["GroupNode"] = register_groupnode_type;
  actions["WordNode"] = register_word_node_type;

  for (const auto & c : s.classes())
  {
    if (actions.contains(c.name()))
    {
      auto action = actions.value(c.name(), nullptr);
      actions.remove(c.name());
      action(*this, c);
    }
  }

  if (!actions.isEmpty())
  {
    qDebug() << "The following required types could not be found :";
    for (const auto k : actions.keys())
      qDebug() << k.data();
    throw std::runtime_error{ "Some required types could not be found" };
  }
}


static void register_state_type(Application &app, const script::Class &state)
{
  dex::State::type_info().type = state.id();

  if (!state.isDefaultConstructible())
    throw std::runtime_error{ "State class must be default constructible" };
}

void Application::load_state()
{
  using namespace script;

  Script s = mEngine.newScript(SourceFile{ mProfileDir.absoluteFilePath("state.dex").toUtf8().data() });
  if (!s.compile())
  {
    qDebug() << "Could not load state file";
    for (const auto &m : s.messages())
      qDebug() << m.to_string().data();

    throw std::runtime_error{ "Could not load state file" };
  }

  typedef void(*ClassActionCallback)(Application&, const script::Class&);
  QMap<std::string, ClassActionCallback> actions;
  actions["State"] = register_state_type;

  for (const auto & c : s.classes())
  {
    if (actions.contains(c.name()))
    {
      auto action = actions.value(c.name(), nullptr);
      actions.remove(c.name());
      action(*this, c);
    }
  }

  if (!actions.isEmpty())
  {
    qDebug() << "The following required types could not be found :";
    for (const auto k : actions.keys())
      qDebug() << k.data();
    throw std::runtime_error{ "Some required types could not be found" };
  }
}

void Application::process(const QString & dirPath)
{
  dex::Parser parser{ mRootEnvironment };

  if (!mProfileDir.exists("parser.dex"))
  {
    qDebug() << "Profile is missing parser.dex";
    throw std::runtime_error{ "Profile is missing parser.dex" };
  }

  const QString filepath = mProfileDir.filePath("parser.dex");

  script::Script parser_script = scriptEngine()->newScript(script::SourceFile{ filepath.toUtf8().data() });
  if (!parser_script.compile())
  {
    for (const auto &m : parser_script.messages())
      qDebug() << m.to_string().data();

    return;
  }

  parser.setup(parser_script);

  QDir dir{ dirPath };
  process(parser, dir);
}

void Application::output(const QString & outputname, const QString & dir)
{
  for (const auto & out : mOutputs)
  {
    if (out->name() != outputname)
      continue;

    out->invoke(dir);
    return;
  }

  qDebug() << "No output found with name " << outputname;
  throw std::runtime_error{ "Output format not supported" };
}

void Application::process(dex::Parser & parser, const QDir & dir)
{
  for (const auto & f : dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
  {
    if (f.isDir())
      process(parser, QDir{ f.absolutePath() });
    else
    {
      parser.process(QStringList{ f.absoluteFilePath() });
    }
  }

}


static void load_outputs_scripts_recur(Application & app, QList<script::Script> & scripts, const QDir & dir)
{
  script::Engine *engine = app.scriptEngine();

  for (const auto & f : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs))
  {
    if (f.isDir())
    {
      load_outputs_scripts_recur(app, scripts, QDir{ f.absoluteFilePath() });
      continue;
    }

    if (f.suffix() != "dex")
      continue;

    script::Script s = get_script(engine->scripts(), f.absoluteFilePath().toUtf8().data());
    if (s.isNull())
    {
      s = engine->newScript(script::SourceFile{ f.absoluteFilePath().toUtf8().data() });
      if (!s.compile())
      {
        qDebug() << "Failed to compile " << f.filePath();
        for (const auto &m : s.messages())
          qDebug() << m.to_string().data();
        continue;
      }
    }

    scripts.push_back(s);
  }
}

static QList<QSharedPointer<dex::Output>> load_outputs(QList<script::Script> & scripts)
{
  QList<QSharedPointer<dex::Output>> result;

  for (const auto & s : scripts)
  {
    auto output_ns = s.rootNamespace().findNamespace("output");
    if (output_ns.isNull())
      continue;

    for (const auto & f : output_ns.functions())
    {
      QSharedPointer<dex::Output> output = dex::Output::build(f);
      if (output != nullptr)
        result.append(output);
    }
  }

  return result;
}

void Application::load_outputs()
{
  QList<script::Script> scripts;
  QDir output = mProfileDir;
  output.cd("output");
  if (!output.exists())
  {
    qDebug() << "No output directory in the profile";
    throw std::runtime_error{ "Missing output directory in profile" };
  }

  load_outputs_scripts_recur(*this, scripts, output);
  mOutputs = ::load_outputs(scripts);
}