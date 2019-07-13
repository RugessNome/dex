// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/dex.h"

#include "dex/core/list.h"
#include "dex/core/null.h"
#include "dex/core/options.h"
#include "dex/core/output.h"
#include "dex/core/ref.h"
#include "dex/core/serialization.h"

#include "dex/api/api.h"

#include "dex/processor/documentprocessor.h"
#include "dex/processor/rootenvironment.h"

#include <script/class.h>
#include <script/classbuilder.h>
#include <script/destructorbuilder.h>
#include <script/enum.h>
#include <script/functionbuilder.h>
#include <script/module.h>
#include <script/interpreter/executioncontext.h>
#include <script/script.h>
#include <script/typesystem.h>

#include <QDir>
#include <QDirIterator>
#include <QSettings>

#include <QDebug>

#include <iostream>


namespace script
{

namespace callbacks
{

script::Value dummy(script::FunctionCall *c)
{
  return script::Value::Void;
}

script::Value profile_directory(script::FunctionCall *c)
{
  return c->engine()->newString(qApp->activeProfileDir().absolutePath());
}

script::Value set_block_delimiters(script::FunctionCall *c)
{
  QString left = c->arg(1).toString();
  QString right = c->arg(2).toString();
  qApp->documentProcessor()->setBlockDelimiters(left, right);
  return script::Value::Void;
}

script::Value add_ignored_sequence(script::FunctionCall *c)
{
  QString value = c->arg(1).toString();
  qApp->documentProcessor()->addIgnoredSequence(value);
  return script::Value::Void;
}

} // namespace callbacks

} // namespace script


Application::CommandLineOptions::CommandLineOptions()
  : saveSettings(false)
{

}

Application::Application(int & argc, char **argv)
  : QApplication(argc, argv)
{
  mEngine.setup();

  script::Namespace ns = mEngine.rootNamespace();

  dex::Null::expose(ns);
  dex::register_ref_template(ns);
  dex::register_list_template(ns);

  script::Namespace json_namespace = mEngine.rootNamespace().newNamespace("json");
  dex::registerJsonTypes(json_namespace);
  dex::Options::expose(ns);
  dex::serialization::expose(ns);

  dex::api::expose(&mEngine);

  mEngine.rootNamespace().newFunction("profileDirectory", script::callbacks::profile_directory)
    .returns(script::Type::String)
    .create();

  mSettings = new QSettings("dex.ini", QSettings::IniFormat, this);

  mDocumentProcessor.reset(new dex::DocumentProcessor{});
}

Application::~Application()
{

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
    process(inputDirectory().absolutePath());
    output(outputDirectory().absolutePath());
    mState.destroy();
  }
  catch (std::runtime_error & ex)
  {
    qDebug() << "Fatal error:" << QString(ex.what());
    
    if (!mState.get().isNull())
    {
      mState.destroy();
    }

    return 1;
  }

  return 0;
}

void Application::setup()
{
  QString pdir = activeProfileDir().absolutePath();

  if (!activeProfileDir().exists())
  {
    qDebug() << "Profile dir does not exist";
    throw std::runtime_error{ "Profile dir does not exists" };
  }

  fetchModules();

  script::Class parser = scriptEngine()->rootNamespace().newClass("Parser").get();
  parser.newDestructor(script::callbacks::dummy).create();
  parser.newMethod("setBlockDelimiters", script::callbacks::set_block_delimiters)
    .setConst().params(script::Type::cref(script::Type::String), script::Type::cref(script::Type::String)).create();
  parser.newMethod("addIgnoredSequence", script::callbacks::add_ignored_sequence)
    .setConst().params(script::Type::cref(script::Type::String)).create();
  auto parser_value = scriptEngine()->construct(parser.id(), [](script::Value & val) -> void { });
  scriptEngine()->manage(parser_value);
  scriptEngine()->rootNamespace().addValue("parser_", parser_value);

  dex::DocumentProcessor::registerApi(scriptEngine());

  dex::Output::expose(scriptEngine()->rootNamespace());

  load_state();

  mState = dex::State::create(scriptEngine());
  scriptEngine()->rootNamespace().addValue("state", mState);
  scriptEngine()->manage(mState);

  scriptEngine()->getModule("commands").load();

  QDir commands = QDir{ activeProfileDir().absoluteFilePath("commands") };
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

  for (const auto & s : scripts)
    qSharedPointerCast<dex::RootEnvironment>(mDocumentProcessor->root())->fill(s);

  mState.init();

  load_outputs();

  for (const auto& o : mOutputs)
  {
    if (o->name() == outputFormat())
    {
      dex::Output::staticCurrentOutput = o.get();
    }
  }

  if (dex::Output::staticCurrentOutput == nullptr)
    throw std::runtime_error{ "Could not find valid output" };
}

void Application::parserCommandLineArgs()
{
  QStringList args = QCoreApplication::arguments();

  for (int i(0); i < args.size(); ++i)
  {
    if (args.at(i) == "-p")
      mCliOptions.activeProfile = args.at(i + 1);

    if (args.at(i) == "-i")
      mCliOptions.inputDirectory = args.at(i + 1);

    if (args.at(i) == "-o")
      mCliOptions.outputDirectory = args.at(i + 1);

    if (args.at(i) == "-g")
      mCliOptions.outputFormat = args.at(i + 1);

    if (args.at(i) == "--profiles-dir")
      mCliOptions.profilesDirectory = args.at(i + 1);

    if (args.at(i) == "--save-settings")
      mCliOptions.saveSettings = true;
  }

  if (mCliOptions.saveSettings)
  {
    qDebug() << "Save settings not implemented yet";
  }
}

static void fetch_module(script::Module& m, const QDir& dir)
{
  QDirIterator it{ dir.absolutePath(), QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs };

  while (it.hasNext())
  {
    QString path = it.next();
    QFileInfo info{ path };

    if (info.isDir())
    {
      script::Module submodule = m.newSubModule(info.fileName().toStdString());
      fetch_module(submodule, QDir{ path });
    }
    else if(info.suffix() == "dex")
    {
      m.newSubModule(info.baseName().toStdString(), script::SourceFile(path.toStdString()));
    }
  }
}

void Application::fetchModule(const QString& dirpath)
{
  QDir subdir{ dirpath };
  script::Module m = scriptEngine()->newModule(subdir.dirName().toStdString());
  fetch_module(m, subdir);
}

void Application::fetchModules()
{
  qDebug() << "Fetching all modules in" << activeProfileDir().absolutePath();

  QDirIterator iterator{ activeProfileDir().absolutePath(), QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs };

  while (iterator.hasNext())
  {
    QString dirpath = iterator.next();
    QFileInfo info{ dirpath };

    if(info.isDir())
      fetchModule(dirpath);
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

  Script s = mEngine.newScript(SourceFile{ activeProfileDir().absoluteFilePath("state.dex").toUtf8().data() });
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
  mDocumentProcessor->setState(mState);
  QDir dir{ dirPath };
  mDocumentProcessor->process(dir);
}

void Application::output(const QString & dir)
{
  dex::Output::current()->write(dir);
}

QDir Application::inputDirectory() const
{
  if (!mCliOptions.inputDirectory.isEmpty())
    return QDir{ mCliOptions.inputDirectory };

  if (!mSettings->contains("inputdir"))
    return QDir::current();

  return QDir{ mSettings->value("inputdir").toString() };
}

QDir Application::outputDirectory() const
{
  if (!mCliOptions.outputDirectory.isEmpty())
    return QDir{ mCliOptions.outputDirectory };

  return QDir{ mSettings->value("outputdir").toString() };
}

QDir Application::profilesDirectory() const
{
  if (!mCliOptions.profilesDirectory.isEmpty())
    return QDir{ mCliOptions.profilesDirectory };

  if (mSettings->contains("profilesdir"))
    return QDir{ mSettings->value("profilesdir").toString() };

  return QDir{ QCoreApplication::applicationDirPath() + "/profiles" };
}

QString Application::outputFormat() const
{
  if (!mCliOptions.outputFormat.isEmpty())
    return mCliOptions.outputFormat;

  return mSettings->value("outputformat").toString();
}

QString Application::activeProfile() const
{
  if (!mCliOptions.activeProfile.isEmpty())
    return mCliOptions.activeProfile;

  return mSettings->value("profile").toString();
}

QDir Application::activeProfileDir() const
{
  QDir d = profilesDirectory();
  d.cd(activeProfile());
  return d;
}

dex::DocumentProcessor* Application::documentProcessor()
{
  return mDocumentProcessor.get();
}

script::Engine* Application::scriptEngine()
{
  return &DexInstance()->mEngine;
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

//static QList<QSharedPointer<dex::Output>> load_outputs(QList<script::Script> & scripts)
//{
//  QList<QSharedPointer<dex::Output>> result;
//
//  for (auto s : scripts)
//  {
//    auto output_ns = s.rootNamespace().findNamespace("output");
//    if (output_ns.isNull())
//      continue;
//
//    for (const auto & f : output_ns.functions())
//    {
//      QSharedPointer<dex::Output> output = dex::Output::build(f);
//      if (output != nullptr)
//        result.append(output);
//    }
//    
//    /// TODO: Should we rather run the scripts just before calling the output function ?
//    // And therefore only call run() for the output script that is actually used.
//    s.run();
//  }
//
//  return result;
//}

void Application::load_outputs()
{
  script::Module m = scriptEngine()->getModule("output");

  m.load();

  for (const script::Script& s : scriptEngine()->scripts())
  {
    for (const script::Class& c : s.classes())
    {
      if (c.inherits(scriptEngine()->typeSystem()->getClass(script::Type::DexOutput)))
      {
        script::Value impl = scriptEngine()->construct(c.id(), {});
        mOutputs.push_back(std::unique_ptr<dex::Output>(new dex::Output(impl)));
      }
    }
  }

  //QList<script::Script> scripts;
  //QDir output = activeProfileDir();
  //output.cd("output");
  //if (!output.exists())
  //{
  //  qDebug() << "No output directory in the profile";
  //  throw std::runtime_error{ "Missing output directory in profile" };
  //}

  //load_outputs_scripts_recur(*this, scripts, output);
  //mOutputs = ::load_outputs(scripts);
}
