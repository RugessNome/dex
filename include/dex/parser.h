// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_PARSER_H
#define DEX_PARSER_H

#include "dex/environment.h"

#include <script/function.h>

#include <QSet>
#include <QStack>

class QStringRef;

namespace dex
{

class NodeRef;

class Lexer
{
public:
  Lexer();

  void start(const QString & doc);

  static const QChar AntiSlash;
  static const QString Space;

  QStringRef read();
  inline QChar nextChar() const { return peekChar(); }
  bool atBlockEnd() const;

  bool seekBlock();

  inline int currentLine() const { return mLine; }
  inline int lastProductiveLine() const { return mLastProducedTokenLine; }

protected:
  void readChar(int count);
  QChar readChar();
  QChar peekChar() const;
  bool atBlockBegin() const;
  bool isTerminator(const QChar & c) const;
  bool isDiscardable(const QChar & c) const;
  bool readDiscardable();
  bool readIgnoredSequence();
  void consumeDiscardables();
  QStringRef substring(int count) const;
  QStringRef substring(int pos, int count) const;

private:
  int mLine;
  int mColumn;
  int mPos;
  QString mDocument;
  QPair<QString, QString> mBlockDelimiter;
  bool mLineBeginning;
  QStringList mIgnoredSequences;
  QSet<QChar> mPunctuators;
  int mLastProducedTokenLine;
};

class Parser
{
public:
  Parser(const QSharedPointer<Environment> & root);

  void setup(const script::Script & parser);

  void process(const QStringList & files);

  QSharedPointer<Environment> getEnvironment(const QString & name) const;
  void enter(const QSharedPointer<Environment> & env);
  void leave();

protected:
  void start(const QString & text);
  NodeRef read();
  NodeRef readArgument();
  NodeRef createNode(const QStringRef & str);
  BracketsArguments readBracketsArguments();
  QSharedPointer<Command> findCommand(const QString & name) const;
  NodeRef readCommand(const QString & command);

  void processFile(const QString & path);
  void beginFile(const QString & path);
  void endFile();

  void beginBlock();
  void endBlock();

  void dispatch(const NodeRef & node);

  script::Engine* engine() const;

private:
  Lexer mLexer;
  QStack<QSharedPointer<Environment>> mEnvironments;
  script::Function mBeginFile;
  script::Function mEndFile;
  script::Function mBeginBlock;
  script::Function mEndBlock;
  script::Function mDispatch;
};

} // namespace dex

#endif // DEX_PARSER_H
