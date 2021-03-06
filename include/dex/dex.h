// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_H
#define DEX_H

#include <QApplication>

#include "dex/processor/state.h"

#include <script/classtemplate.h>
#include <script/engine.h>

#include <QDir>
#include <QSharedPointer>

namespace dex
{
class DocumentProcessor;
class Environment;
class Output;
} // namespace dex

class QSettings;

class Application : public QApplication
{
  Q_OBJECT
public:
  Application(int & argc, char **argv);
  ~Application();

  int run();

  void setup();

  void process(const QString & dirPath);

  void output(const QString & dir);

  QDir inputDirectory() const;
  QDir outputDirectory() const;
  QDir profilesDirectory() const;
  QString outputFormat() const;

  QString activeProfile() const;

  QDir activeProfileDir() const;

  dex::DocumentProcessor* documentProcessor();

  static script::Engine* scriptEngine();

private:
  void parserCommandLineArgs();
  void fetchModules();
  void fetchModule(const QString& dirpath);

  void load_state();
  void load_outputs();

private:
  script::Engine mEngine;
  dex::State mState;

  struct CommandLineOptions
  {
    CommandLineOptions();

    QString inputDirectory;
    QString outputDirectory;
    QString outputFormat;
    QString profilesDirectory;
    QString activeProfile;
    bool saveSettings;
  };

  CommandLineOptions mCliOptions;

  std::unique_ptr<dex::DocumentProcessor> mDocumentProcessor;
  //QList<QSharedPointer<dex::Output>> mOutputs;
  std::vector<std::unique_ptr<dex::Output>> mOutputs;
  QSettings *mSettings;
};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application *>(QCoreApplication::instance()))

#define Dex Application

inline Application* DexInstance() { return static_cast<Application*>(QCoreApplication::instance()); }

#endif // DEX_H
