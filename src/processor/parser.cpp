// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/processor/parser.h"

#include "dex/processor/bracketsarguments.h"
#include "dex/processor/command.h"
#include "dex/processor/environment.h"
#include "dex/processor/node.h"

#include <script/engine.h>
#include <script/namespace.h>
#include <script/script.h>

#include <QDebug>
#include <QFile>

namespace dex
{

const QChar Lexer::AntiSlash = '\\';

Lexer::Lexer()
{
  mBlockDelimiter = QPair<QString, QString>("/*!", "*/");
  mPunctuators << '{' << '[' << '}' << ']' << '=' << ',' << '.' << ';' << '\'' << '"';
  mIgnoredSequences.append("*");
}

void Lexer::start(const QString & doc)
{
  mDocuments.clear();

  Document d;
  d.content = doc;
  d.pos.line = 0;
  d.pos.column = 0;
  d.pos.offset = 0;
  mDocuments.push(d);
  mLastProducedTokenLine = 0;
}

void Lexer::input(const QString & content)
{
  assert(mDocuments.size() > 0);

  Document d;
  d.content = content;
  d.pos.line = 0;
  d.pos.column = 0;
  d.pos.offset = 0;
  mDocuments.push(d);
  mLastProducedTokenLine = 0;
}

Token Lexer::read()
{
  if (atInputEnd())
  {
    if (mDocuments.size() > 1)
    {
      mDocuments.pop();
    }
    else
    {
      qDebug() << "Lexer::read() : unexpected end of input";
      throw std::runtime_error{ "Unexpected end of input" };
    }
  }

  const Position startpos = currentPos();

  if(readSpaces())
    return Token{ Token::Space, substringFrom(startpos.offset) };

  if (peekChar() == AntiSlash)
  {
    readChar();
    return produce(Token::EscapeCharacter, startpos.offset);
  }
  else if (peekChar() == '\n')
  {
    readChar();
    return Token{ Token::EndOfLine, substringFrom(startpos.offset) };
  }
  else if (peekChar() == '{')
  {
    readChar();
    return produce(Token::BeginGroup, startpos.offset);;
  }
  else if (peekChar() == '}')
  {
    readChar();
    return produce(Token::EndGroup, startpos.offset);;
  }
  else if (mPunctuators.contains(peekChar()))
  {
    readChar();
    return produce(Token::Other, startpos.offset);
  }

  /// TODO: remove the use of isDiscardable and isTerminator here by 
  // using Token::Kind and stopping when char is not Token::Word
  while (!atInputEnd() && !atBlockEnd() && !isDiscardable(peekChar()) && !isTerminator(peekChar()))
  {
    readChar();
  }

  return produce(Token::Word, startpos.offset);
}

bool Lexer::atBlockEnd() const
{
  return mDocuments.size() == 1
    && substring(currentPos().offset, mBlockDelimiter.second.length()) == mBlockDelimiter.second;;
}

bool Lexer::seekBlock()
{
  assert(mDocuments.size() == 1);

  Position & pos = currentDocument().pos;

  while (pos.offset < currentDocument().length() - mBlockDelimiter.first.length())
  {
    if (!atBlockBegin())
    {
      QChar c = currentDocument().content[pos.offset];
      pos.offset += 1;
      if (c == '\n')
      {
        pos.line += 1;
        pos.column = 0;
      }
      else
      {
        pos.column += 1;
      }
    }
    else
      break;
  }

  if (!atBlockBegin())
    return false;
  readChar(mBlockDelimiter.first.length());
  return true;
}

void Lexer::readChar(int count)
{
  while (count >= 0)
  {
    readChar();
    --count;
  }
}

QChar Lexer::readChar()
{
  Position & pos = currentDocument().pos;

  QChar result = currentDocument().content[pos.offset];
  pos.offset += 1;
  if (result == '\n')
  {
    pos.line += 1;
    pos.column = 0;
    beginLine();
  }
  else
  {
    pos.column += 1;
  }
  return result;
}

QChar Lexer::peekChar() const
{
  const Position & pos = currentDocument().pos;
  return currentDocument().content[pos.offset];
}

bool Lexer::atBlockBegin() const
{
  const QString & content = currentDocument().content;
  const Position & pos = currentDocument().pos;

  return mDocuments.size() == 1 && QStringRef{ &content, pos.offset, mBlockDelimiter.first.length() } == mBlockDelimiter.first;
}

bool Lexer::isTerminator(const QChar & c) const
{
  return mPunctuators.contains(peekChar());
}

bool Lexer::isDiscardable(const QChar & c) const
{
  return c.isSpace();
}

bool Lexer::readIgnoredSequence()
{
  for (const auto & seq : mIgnoredSequences)
  {
    if (substring(seq.length()) == seq)
    {
      readChar(seq.length());
      return true;
    }
  }

  return false;
}

bool Lexer::readSpaces()
{
  const int offset = currentDocument().pos.offset;

  while (!atBlockEnd() && peekChar().isSpace() && peekChar() != '\n')
    readChar();

  return currentDocument().pos.offset != offset;
}

void Lexer::beginLine()
{
  const Position pos = currentDocument().pos;

  while (!atBlockEnd() && !atInputEnd() && peekChar().isSpace())
    readChar();

  if (atBlockEnd() || atInputEnd())
    return;

  if (!readIgnoredSequence())
    currentDocument().pos = pos;
}

QStringRef Lexer::substring(int count) const
{
  const QString & content = currentDocument().content;
  const Position & pos = currentDocument().pos;

  return QStringRef{ &content, pos.offset, std::min(count, content.length() - pos.offset) };
}

QStringRef Lexer::substring(int pos, int count) const
{
  const QString & content = currentDocument().content;

  return QStringRef{ &content, pos, std::min(count, content.length() - pos) };
}

QStringRef Lexer::substringFrom(int offset) const
{
  const QString & content = currentDocument().content;
  const Position & pos = currentDocument().pos;

  return QStringRef{ &content, offset, std::min(pos.offset - offset, content.length() - offset) };
}

Token Lexer::produce(Token::Kind k, int offset)
{
  mLastProducedTokenLine = currentDocument().pos.line;
  return Token{ k, substringFrom(offset) };
}

bool Lexer::atInputEnd() const
{
  return currentDocument().length() == currentDocument().pos.offset;
}

Lexer::Document & Lexer::currentDocument()
{
  return mDocuments.back();
}

const Lexer::Document & Lexer::currentDocument() const
{
  return mDocuments.back();
}

Lexer::Position Lexer::currentPos() const
{
  return currentDocument().pos;
}

Parser::Parser(dex::State & state, const QSharedPointer<Environment> & root)
  : mState(state)
{
  mEnvironments.push(root);
}

void Parser::process(const QDir & directory)
{
  for (const auto & f : directory.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
  {
    if (f.isDir())
    {
      process(QDir{ f.absoluteFilePath() });
    }
    else
    {
      mCurrentDir = directory;
      processFile(f.absoluteFilePath());
    }
  }
}

QSharedPointer<Environment> Parser::getEnvironment(const QString & name) const
{
  for (int i(mEnvironments.size() - 1); i >= 0; --i)
  {
    auto env = mEnvironments.at(i);
    auto result = env->getEnvironment(name);
    if (result != nullptr)
      return result;
  }

  return nullptr;
}

void Parser::enter(const QSharedPointer<Environment> & env)
{
  mEnvironments.push_back(env);
}

void Parser::leave()
{
  mEnvironments.pop_back();
}

void Parser::input(const QString & filename)
{
  if (!mCurrentDir.exists(filename))
  {
    qDebug() << "Could not find input file " << filename;
    return;
  }

  QString path = mCurrentDir.filePath(filename);
  QFile f{ path };
  if (!f.open(QIODevice::ReadOnly))
  {
    qDebug() << "Could not open input file " << filename;
    return;
  }

  QString content = QString::fromUtf8(f.readAll());
  f.close();

  mLexer.input(content);
}

void Parser::start(const QString & text)
{
  mLexer.start(text);
}

NodeRef Parser::read()
{
  auto token = mLexer.read();

  if (token.kind == Token::EscapeCharacter)
    return readCommand(mLexer.read());

  return createNode(token);
}

/// TODO: maybe refactor with Parser::read()
NodeRef Parser::readArgument()
{
  auto token = mLexer.read();
  if (token.kind == Token::Space) /// TODO: should we consider EOL too ?
    token = mLexer.read();

  if (token.kind == Token::EscapeCharacter)
    return readCommand(mLexer.read());

  return createNode(token);
}

NodeRef Parser::createNode(const Token & tok)
{
  if (tok.kind == Token::BeginGroup)
  {
    auto result = NodeRef::createGroupNode(engine());
    while (mLexer.nextChar() != '}')
    {
      auto element = read();
      if (!element.isNull())
        result.push_back(element);
    }
   
    mLexer.read(); /// TODO: assert its a Token::EndGroup

    return result;
  }
  else if (tok.kind == Token::Space)
  {
    return NodeRef::createSpaceNode(engine(), tok.text.toString());
  }
  else if (tok.kind == Token::EndOfLine)
  {
    return NodeRef::createEndOfLine(engine());
  }
  else
  {
    return NodeRef::createWordNode(engine(), tok.text.toString());
  }
}

QSharedPointer<Command> Parser::findCommand(const QString & name) const
{
  for (int i(mEnvironments.size() - 1); i >= 0; --i)
  {
    auto env = mEnvironments.at(i);
    auto com = env->getCommand(name);
    if (com != nullptr)
      return com;
  }

  return nullptr;
}

BracketsArguments Parser::readBracketsArguments()
{
  BracketsArguments result;

  if (mLexer.nextChar() != '[')
    return result;

  mLexer.read();

  QPair<QString, QString> argument;
  bool equal_sign_read = false;
  Token tok = mLexer.read();
  for(;;)
  {
    if (tok.text == "," || tok.text == "]")
    {
      if (!argument.first.isEmpty())
      {
        if (equal_sign_read)
          result.add(argument.first, argument.second);
        else
          result.add(argument.first);
      }

      argument.first.clear();
      argument.second.clear();
      equal_sign_read = false;

      if (tok.text == "]")
        break;
    }
    else if (tok.text == "=")
    {
      equal_sign_read = true;
    }
    else
    {
      if (equal_sign_read)
        argument.second = tok.text.toString();
      else
        argument.first = tok.text.toString();
    }

    tok = mLexer.read();
  }

  return result;
}

NodeRef Parser::readCommand(const Token & token)
{
  auto command = findCommand(token.text.toString());
  if (command == nullptr)
  {
    qDebug() << "No such command " << token.text.toString();
    throw std::runtime_error{ "No such command" };
  }

  BracketsArguments brackets = readBracketsArguments();
  const int argc = command->parameterCount();
  QList<NodeRef> nodes;
  if (argc != 1)
  {
    for (int i(0); i < argc; ++i)
    {
      NodeRef arg = readArgument();
      nodes.append(arg);
    }
  }
  else
  {
    if (command->span() == CommandSpan::Word || command->span() == CommandSpan::NotApplicable)
    {
      NodeRef arg = readArgument();
      nodes.append(arg);
    }
    else if (command->span() == CommandSpan::Line)
    {
      const int line = mLexer.currentLine();
      NodeRef arg = readArgument();
      if (mLexer.currentLine() != line) 
      {
        nodes.append(arg);
      }
      else
      {
        NodeRef group = NodeRef::createGroupNode(engine());
        group.push_back(arg);
        while (mLexer.currentLine() == line && !mLexer.atBlockEnd())
        {
          arg = read();
          group.push_back(arg);
        }

        if (!group.isEmpty() && (group.back().isSpace() || group.back().isEndOfLine()))
        {
          /// TODO : remove last element from group
        }

        nodes.push_back(group);
      }
    }
    else if (command->span() == CommandSpan::Paragraph)
    {
      NodeRef group = NodeRef::createGroupNode(engine());
      while (!mLexer.atBlockEnd() && mLexer.currentLine() - mLexer.lastProductiveLine() <= 1)
      {
        NodeRef node = read();
        group.push_back(node);
      }
      nodes.push_back(group);

      /// TODO : remove space of eol at end and beginning of group.
    }
  }

  if (!brackets.isEmpty() && !command->acceptsBracketArguments())
  {
    qDebug() << command->name() << "does not support bracket arguments";
  }

  return command->invoke(this, brackets, nodes);
}

void Parser::processFile(const QString & path)
{
  {
    QFile f{ path };
    if (!f.open(QIODevice::ReadOnly))
      return;
    start(QString::fromUtf8(f.readAll()));
    f.close();
  }

  mState.beginFile(path);

  while (mLexer.seekBlock())
  {
    mState.beginBlock();

    while (!mLexer.atBlockEnd())
    {
      NodeRef node = read();
      if (!node.isNull())
        mState.dispatch(node);
    }

    mState.endBlock();
  }

  mState.endFile();
}

script::Engine* Parser::engine() const
{
  return mState.engine();
}

InputStream::InputStream(const QString & doc)
{
  Document document;
  document.content = doc;
  document.pos = 0;
  mDocuments.push(document);
}

void InputStream::inject(const QString & content)
{
  Document d;
  d.content = content;
  d.pos = 0;
  mDocuments.push(d);
}

QChar InputStream::peekChar() const
{
  return currentDocument().content[currentPos()];
}

QChar InputStream::readChar()
{
  int & pos = currentDocument().pos;

  QChar result = currentDocument().content[pos++];

  if (pos == currentDocument().length() && stackSize() > 1)
    mDocuments.pop();

  return result;
}

QStringRef InputStream::peek(int n) const
{
  auto & doc = currentDocument();
  return doc.content.midRef(doc.pos, n);
}

QStringRef InputStream::peekLine() const
{
  int n = std::max(currentDocument().content.indexOf('\n', currentPos()), currentDocument().length());
  return peek(n - currentPos());
}

bool InputStream::read(const QString & text)
{
  if (peek(text.size()) != text)
    return false;

  discard(text.size());

  return true;
}

void InputStream::discard(int n)
{
  while (n > 0)
    readChar(), --n;
}

InputStream::Document & InputStream::currentDocument()
{
  return mDocuments.top();
}

const InputStream::Document & InputStream::currentDocument() const
{
  return mDocuments.top();
}

int InputStream::currentPos() const
{
  return currentDocument().pos;
}

bool InputStream::atEnd() const
{
  return mDocuments.size() == 1 && currentDocument().pos == currentDocument().content.length();
}

InputStream & InputStream::operator=(const QString & str)
{
  mDocuments.clear();

  Document document;
  document.content = str;
  document.pos = 0;
  mDocuments.push(document);

  return *this;
}


StreamTokenizer::StreamTokenizer(InputStream & is)
  : mStream(&is)
  , EscapeCharacter(QChar('\\'))
{
  mPunctuators << '{' << '[' << '}' << ']' << '=' << ',' << '.' << ';' << '\'' << '"';
}

StreamTokenizer::Token StreamTokenizer::read()
{
  mBuffer.clear();
  
  if (readSpaces())
    return produce(Token::Space);

  if (stream().peekChar() == EscapeCharacter)
    return readChar(), produce(Token::EscapeCharacter);
  else if (stream().peekChar() == '\n')
    return readChar(), produce(Token::EndOfLine);
  else if (stream().peekChar() == '{')
    return readChar(), produce(Token::BeginGroup);
  else if (stream().peekChar() == '}')
    return readChar(), produce(Token::EndGroup);
  else if (mPunctuators.contains(stream().peekChar()))
    return readChar(), produce(Token::Other);

  while (!stream().atEnd() && !isPunctuatorOrSpace(stream().peekChar()))
    readChar();

  return produce(Token::Word);
}

void StreamTokenizer::readChar()
{
  mBuffer.append(stream().readChar());
}

bool StreamTokenizer::isPunctuatorOrSpace(const QChar & c) const
{
  return c.isSpace() || mPunctuators.contains(c);
}

bool StreamTokenizer::readSpaces()
{
  while (stream().peekChar().isSpace() && stream().peekChar() != '\n')
    readChar();

  return mBuffer.size() > 0;
}

StreamTokenizer::Token StreamTokenizer::produce(Token::Kind k)
{
  return Token{ k, mBuffer };
}


DocumentProcessor::DocumentProcessor(dex::State & state, const QSharedPointer<Environment> & root)
  : mInputStream(QString())
  , mTokenizer(mInputStream)
  , mState(state)
{
  mEnvironments.push(root);

  mBlockDelimiter = QPair<QString, QString>("/*!", "*/");
  mIgnoredSequences << "* " << "*" << " * " << " *";
}

void DocumentProcessor::process(const QDir & directory)
{
  for (const auto & f : directory.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
  {
    if (f.isDir())
    {
      process(QDir{ f.absoluteFilePath() });
    }
    else
    {
      mCurrentDir = directory;
      processFile(f.absoluteFilePath());
    }
  }
}

QSharedPointer<Environment> DocumentProcessor::getEnvironment(const QString & name) const
{
  for (int i(mEnvironments.size() - 1); i >= 0; --i)
  {
    auto env = mEnvironments.at(i);
    auto result = env->getEnvironment(name);
    if (result != nullptr)
      return result;
  }

  return nullptr;
}

void DocumentProcessor::enter(const QSharedPointer<Environment> & env)
{
  mEnvironments.push(env);
}

void DocumentProcessor::leave()
{
  mEnvironments.pop();
}


void DocumentProcessor::input(const QString & filename)
{
  if (!mCurrentDir.exists(filename))
  {
    qDebug() << "Could not find input file " << filename;
    return;
  }

  QString path = mCurrentDir.filePath(filename);
  QFile f{ path };
  if (!f.open(QIODevice::ReadOnly))
  {
    qDebug() << "Could not open input file " << filename;
    return;
  }

  QString content = QString::fromUtf8(f.readAll());
  f.close();

  mInputStream.inject(content);
}

NodeRef DocumentProcessor::read()
{
  auto token = mTokenizer.read();

  qDebug() << token.text;

  if (token.kind == Token::EscapeCharacter)
    return readCommand(mTokenizer.read());

  return createNode(token);
}

NodeRef DocumentProcessor::readArgument()
{
  auto token = mTokenizer.read();
  if (token.kind == Token::Space) /// TODO: should we consider EOL too ?
    token = mTokenizer.read();

  if (token.kind == Token::EscapeCharacter)
    return readCommand(mTokenizer.read());

  return createNode(token);
}

NodeRef DocumentProcessor::readLineArgument()
{
  NodeRef arg = readArgument();

  NodeRef group = NodeRef::createGroupNode(engine());
  while (!arg.isEndOfLine())
  {
    group.push_back(arg);
    if (atBlockEnd())
      break;
    arg = read();
  }

  if (!group.isEmpty() && (group.back().isSpace() || group.back().isEndOfLine()))
  {
    /// TODO : remove last element from group
  }

  if (group.size() == 1)
    return group.at(0);

  return group;
}

NodeRef DocumentProcessor::readParagraphArgument()
{
  NodeRef group = NodeRef::createGroupNode(engine());

  bool eol = false;

  while (!atBlockEnd())
  {
    NodeRef node = read();
    if (node.isEndOfLine() && eol)
      break;
    eol = node.isEndOfLine();
    group.push_back(node);
  }

  /// TODO : remove space of eol at end and beginning of group.

  return group;
}

NodeRef DocumentProcessor::createNode(const StreamTokenizer::Token & tok)
{
  if (tok.kind == Token::BeginGroup)
    return readGroup(tok);
  else if (tok.kind == Token::Space)
    return NodeRef::createSpaceNode(engine(), tok.text);
  else if (tok.kind == Token::EndOfLine)
    return beginLine(), NodeRef::createEndOfLine(engine());
  else
    return NodeRef::createWordNode(engine(), tok.text);
}

NodeRef DocumentProcessor::readGroup(const StreamTokenizer::Token & tok)
{
  Q_ASSERT(tok.kind == StreamTokenizer::Token::BeginGroup);

  auto result = NodeRef::createGroupNode(engine());
  while (mInputStream.nextChar() != '}')
  {
    auto element = read();
    if (!element.isNull())
      result.push_back(element);
  }

  mTokenizer.read(); /// TODO: assert its a Token::EndGroup

  return result;
}

BracketsArguments DocumentProcessor::readBracketsArguments()
{
  BracketsArguments result;

  if (mInputStream.nextChar() != '[')
    return result;

  mInputStream.readChar();

  QPair<QString, QString> argument;
  bool equal_sign_read = false;
  StreamTokenizer::Token tok = mTokenizer.read();
  for (;;)
  {
    if (tok.text == "," || tok.text == "]")
    {
      if (!argument.first.isEmpty())
      {
        if (equal_sign_read)
          result.add(argument.first, argument.second);
        else
          result.add(argument.first);
      }

      argument.first.clear();
      argument.second.clear();
      equal_sign_read = false;

      if (tok.text == "]")
        break;
    }
    else if (tok.text == "=")
    {
      equal_sign_read = true;
    }
    else
    {
      if (equal_sign_read)
        argument.second = tok.text;
      else
        argument.first = tok.text;
    }

    tok = mTokenizer.read();
  }

  return result;
}

QSharedPointer<Command> DocumentProcessor::findCommand(const QString & name) const
{
  for (int i(mEnvironments.size() - 1); i >= 0; --i)
  {
    auto env = mEnvironments.at(i);
    auto com = env->getCommand(name);
    if (com != nullptr)
      return com;
  }

  return nullptr;
}

NodeRef DocumentProcessor::readCommand(const StreamTokenizer::Token & token)
{
  auto command = findCommand(token.text);
  if (command == nullptr)
  {
    qDebug() << "No such command " << token.text;
    throw std::runtime_error{ "No such command" };
  }

  BracketsArguments brackets = readBracketsArguments();
  const int argc = command->parameterCount();
  QList<NodeRef> arguments;
  if (argc != 1)
  {
    for (int i(0); i < argc; ++i)
    {
      NodeRef arg = readArgument();
      arguments.append(arg);
    }
  }
  else
  {
    if (command->span() == CommandSpan::Word || command->span() == CommandSpan::NotApplicable)
    {
      NodeRef arg = readArgument();
      arguments.append(arg);
    }
    else if (command->span() == CommandSpan::Line)
    {
      NodeRef arg = readLineArgument();
      arguments.append(arg);
    }
    else if (command->span() == CommandSpan::Paragraph)
    {
      NodeRef arg = readParagraphArgument();
      arguments.append(arg);
    }
  }

  if (!brackets.isEmpty() && !command->acceptsBracketArguments())
  {
    qDebug() << command->name() << "does not support bracket arguments";
  }

  return command->invoke(this, brackets, arguments);
}


void DocumentProcessor::processFile(const QString & path)
{
  {
    QFile f{ path };
    if (!f.open(QIODevice::ReadOnly))
      return;
    mInputStream = QString::fromUtf8(f.readAll());
    f.close();
  }

  mState.beginFile(path);

  while (seekBlock())
  {
    mState.beginBlock();

    while (!atBlockEnd())
    {
      NodeRef node = read();
      if (!node.isNull())
        mState.dispatch(node);
    }

    mState.endBlock();
  }

  mState.endFile();
}

bool DocumentProcessor::seekBlock()
{
  while (!mInputStream.atEnd() && !mInputStream.read(mBlockDelimiter.first))
    mInputStream.readChar();

  return !mInputStream.atEnd();
}

bool DocumentProcessor::atBlockEnd() const
{
  return mInputStream.peek(mBlockDelimiter.second.length()) == mBlockDelimiter.second;
}

void DocumentProcessor::beginLine()
{
  if (mInputStream.stackSize() > 1)
    return;

  auto line = mInputStream.peekLine();
  
  int blockend = line.indexOf(mBlockDelimiter.second);

  for (int i(0); i < blockend; ++i)
  {
    if (!line.at(i).isSpace())
      blockend = -1;
  }


  if (blockend == -1)
  {
    for (auto ignore : mIgnoredSequences)
    {
      if (mInputStream.read(ignore))
        return;
    }
  }
  else
  {
    mInputStream.discard(blockend);
  }
}

script::Engine* DocumentProcessor::engine() const
{
  return mState.engine();
}

} // namespace dex
