// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_PARSER_H
#define DEX_PARSER_H

#include "dex/processor/environment.h"
#include "dex/processor/state.h"

#include <QDir>
#include <QSet>
#include <QStack>

class QStringRef;

namespace dex
{

class NodeRef;

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
  QStringRef text;
};

class Lexer
{
public:
  Lexer();

  void start(const QString & doc);
  
  void input(const QString & content);

  static const QChar AntiSlash;

  Token read();
  inline QChar nextChar() const { return peekChar(); }
  bool atBlockEnd() const;

  bool seekBlock();

  inline int currentLine() const { return currentDocument().pos.line; }
  inline int lastProductiveLine() const { return mLastProducedTokenLine; }

protected:
  void readChar(int count);
  QChar readChar();
  QChar peekChar() const;
  bool atBlockBegin() const;
  bool isTerminator(const QChar & c) const;
  bool isDiscardable(const QChar & c) const;
  bool readIgnoredSequence();
  bool readSpaces();
  void beginLine();
  QStringRef substring(int count) const;
  QStringRef substring(int pos, int count) const;
  QStringRef substringFrom(int offset) const;
  Token produce(Token::Kind k, int offset);
  bool atInputEnd() const;

public:
  struct Position {
    int line;
    int column;
    int offset;
  };

  struct Document
  {
    Position pos;
    QString content;

    inline int length() const { return content.length(); }
  };

  Document & currentDocument();
  const Document & currentDocument() const;
  Position currentPos() const;

private:
  QStack<Document> mDocuments;
  QPair<QString, QString> mBlockDelimiter;
  QStringList mIgnoredSequences;
  QSet<QChar> mPunctuators;
  int mLastProducedTokenLine;
};

class Parser
{
public:
  Parser(dex::State & state, const QSharedPointer<Environment> & root);

  void process(const QDir & directory);

  QSharedPointer<Environment> getEnvironment(const QString & name) const;
  void enter(const QSharedPointer<Environment> & env);
  void leave();

  void input(const QString & filename);

protected:
  void start(const QString & text);
  NodeRef read();
  NodeRef readArgument();
  NodeRef createNode(const Token & tok);
  BracketsArguments readBracketsArguments();
  QSharedPointer<Command> findCommand(const QString & name) const;
  NodeRef readCommand(const Token & command);

  void processFile(const QString & path);

  script::Engine* engine() const;

private:
  Lexer mLexer;
  dex::State mState;
  QDir mCurrentDir;
  QStack<QSharedPointer<Environment>> mEnvironments;
};


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
  DocumentProcessor(dex::State & state, const QSharedPointer<Environment> & root);

  void process(const QDir & directory);

  QSharedPointer<Environment> getEnvironment(const QString & name) const;
  void enter(const QSharedPointer<Environment> & env);
  void leave();

  void input(const QString & filename);

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
  dex::State mState;
  QDir mCurrentDir;
  QStack<QSharedPointer<Environment>> mEnvironments;
};

} // namespace dex

#endif // DEX_PARSER_H
