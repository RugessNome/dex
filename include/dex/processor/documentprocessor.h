// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_DOCUMENT_PROCESSOR_H
#define DEX_DOCUMENT_PROCESSOR_H

#include "dex/processor/environment.h"
#include "dex/processor/state.h"

#include <QDir>
#include <QSet>
#include <QStack>

class QStringRef;

namespace dex
{

class NodeRef;

class InputStream
{
public:
  InputStream(const QString & doc);
  InputStream(const InputStream & other) = default;

  void inject(const QString & content);

  QChar peekChar() const;
  inline QChar nextChar() const { return peekChar(); }
  QChar readChar();

  QStringRef peek(int n) const;
  QStringRef peekLine() const;

  bool read(const QString & text);

  void discard(int n);

  struct Document
  {
    int pos;
    QString content;

    inline int length() const { return content.length(); }
  };

  Document & currentDocument();
  const Document & currentDocument() const;
  int currentPos() const;
  bool atEnd() const;

  inline int stackSize() const { return mDocuments.size(); }

  InputStream & operator=(const QString & str);

private:
  QStack<Document> mDocuments;
};

class StreamTokenizer
{
public:
  StreamTokenizer(InputStream & is);

  inline InputStream & stream() const { return *mStream; }

  struct Token
  {
    enum Kind {
      EscapeCharacter = 0,
      BeginGroup,
      EndGroup,
      EndOfLine,
      Space,
      Word,
      Other,
    };

    Kind kind;
    QString text;
  };

  QChar EscapeCharacter;

  Token read();

protected:
  void readChar();
  bool isPunctuatorOrSpace(const QChar & c) const;
  bool readSpaces();
  Token produce(Token::Kind k);

private:
  InputStream *mStream;
  QSet<QChar> mPunctuators;
  QString mBuffer;
};

class DocumentProcessor
{
public:
  DocumentProcessor();

  inline dex::State & state() const { return *mState; }
  void setState(dex::State & state);

  QSharedPointer<Environment> root() const;

  void process(const QDir & directory);

  QSharedPointer<Environment> getEnvironment(const QString & name) const;
  void enter(const QSharedPointer<Environment> & env);
  void leave();

  void input(const QString & filename);

  static void registerApi(script::Engine *e);

  void setBlockDelimiters(const QString & start, const QString & end);
  void addIgnoredSequence(const QString & val);

protected:
  NodeRef read();
  NodeRef readArgument();
  NodeRef readLineArgument();
  NodeRef readParagraphArgument();
  NodeRef createNode(const StreamTokenizer::Token & tok);
  NodeRef readGroup(const StreamTokenizer::Token & tok);
  BracketsArguments readBracketsArguments();
  QSharedPointer<Command> findCommand(const QString & name) const;
  NodeRef readCommand(const StreamTokenizer::Token & command);

  void processFile(const QString & path);

  bool seekBlock();
  bool atBlockEnd() const;
  void beginLine();

  script::Engine* engine() const;

private:
  InputStream mInputStream;
  StreamTokenizer mTokenizer;
  QPair<QString, QString> mBlockDelimiter;
  QStringList mIgnoredSequences;
  dex::State *mState;
  QDir mCurrentDir;
  QStack<QSharedPointer<Environment>> mEnvironments;
};

} // namespace dex

#endif // DEX_DOCUMENT_PROCESSOR_H
