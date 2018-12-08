// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_H
#define DEX_H

#include <QApplication>

#include "dex/state.h"

#include <script/classtemplate.h>
#include <script/engine.h>

#include <QDir>
#include <QSharedPointer>

namespace dex
{
class Environment;
class Parser;
class Output;
} // namespace dex

class QSettings;

class Application : public QApplication
{
  Q_OBJECT
public:
  Application(int & argc, char **argv);
  ~Application() = default;

  inline script::Engine * scriptEngine() { return &mEngine; }

  int run();

  void setup();

  void process(const QString & dirPath);

  void output(const QString & outputname, const QString & dir);

  QDir inputDirectory() const;
  QDir outputDirectory() const;
  QDir profilesDirectory() const;
  QString outputFormat() const;

  QString activeProfile() const;

  QDir activeProfileDir() const;

private:
  void parserCommandLineArgs();

  void register_span_types();
  void load_nodes();
  void load_state();
  void process(dex::Parser & parser, const QDir & dir);
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

  QSharedPointer<dex::Environment> mRootEnvironment;
  QList<QSharedPointer<dex::Output>> mOutputs;
  QSettings *mSettings;
};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application *>(QCoreApplication::instance()))

#endif // DEX_H
