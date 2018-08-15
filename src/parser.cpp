// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/parser.h"

#include "dex/bracketsarguments.h"
#include "dex/command.h"
#include "dex/environment.h"
#include "dex/node.h"

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
  mDocument = doc;
  mPos.line = 0;
  mPos.column = 0;
  mPos.offset = 0;
  mLastProducedTokenLine = 0;
}

Token Lexer::read()
{
  const Position startpos = mPos;

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

  while (!atBlockEnd() && !isDiscardable(peekChar()) && !isTerminator(peekChar()))
  {
    readChar();
  }

  return produce(Token::Word, startpos.offset);
}

bool Lexer::atBlockEnd() const
{
  return QStringRef{ &mDocument, mPos.offset, mBlockDelimiter.second.length() } == mBlockDelimiter.second;
}

bool Lexer::seekBlock()
{
  while (mPos.offset < mDocument.length() - mBlockDelimiter.first.length())
  {
    if (!atBlockBegin())
    {
      QChar c = mDocument[mPos.offset];
      mPos.offset += 1;
      if (c == '\n')
      {
        mPos.line += 1;
        mPos.column = 0;
      }
      else
      {
        mPos.column += 1;
      }
    }
    else
      break;
  }

  if (!atBlockBegin())
    return false;
  readChar(mBlockDelimiter.first.length());
  consumeDiscardables(); /// TODO: remove once we handle correctly Space and EndOfLine nodes
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
  QChar result = mDocument[mPos.offset];
  mPos.offset += 1;
  if (result == '\n')
  {
    mPos.line += 1;
    mPos.column = 0;
    beginLine();
  }
  else
  {
    mPos.column += 1;
  }
  return result;
}

QChar Lexer::peekChar() const
{
  return mDocument[mPos.offset];
}

bool Lexer::atBlockBegin() const
{
  return QStringRef{ &mDocument, mPos.offset, mBlockDelimiter.first.length() } == mBlockDelimiter.first;
}

bool Lexer::isTerminator(const QChar & c) const
{
  return mPunctuators.contains(peekChar());
}

bool Lexer::isDiscardable(const QChar & c) const
{
  return c.isSpace();
}

bool Lexer::readDiscardable()
{
  if (!isDiscardable(peekChar()))
    return false;

  readChar();
  return true;
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

void Lexer::consumeDiscardables()
{
  while (!atBlockEnd() && (readDiscardable() || readIgnoredSequence()));
}

bool Lexer::readSpaces()
{
  const int offset = mPos.offset;

  while (!atBlockEnd() && peekChar().isSpace())
    readChar();

  return mPos.offset != offset;
}

void Lexer::beginLine()
{
  const Position pos = mPos;

  while (!atBlockEnd() && peekChar().isSpace())
    readChar();

  if (atBlockEnd())
    return;

  if (!readIgnoredSequence())
    mPos = pos;
}

QStringRef Lexer::substring(int count) const
{
  return QStringRef{ &mDocument, mPos.offset, std::min(count, mDocument.length() - mPos.offset) };
}

QStringRef Lexer::substring(int pos, int count) const
{
  return QStringRef{ &mDocument, pos, std::min(count, mDocument.length() - pos) };
}

QStringRef Lexer::substringFrom(int offset) const
{
  return QStringRef{ &mDocument, offset, std::min(mPos.offset - offset, mDocument.length() - offset) };
}

Token Lexer::produce(Token::Kind k, int offset)
{
  mLastProducedTokenLine = mPos.line;
  return Token{ k, substringFrom(offset) };
}

Parser::Parser(const QSharedPointer<Environment> & root)
{
  mEnvironments.push(root);
}

void Parser::setup(const script::Script & parser)
{
  const script::Namespace parser_ns = parser.rootNamespace().findNamespace("parser");
  if (parser_ns.isNull())
  {
    qDebug() << "Invalid parser file";
    throw std::runtime_error{ "Invalid parser file" };
  }

  for (const auto & f : parser_ns.functions())
  {
    if (f.name() == "beginFile" && f.returnType() == script::Type::Void)
      mBeginFile = f;
    else if (f.name() == "endFile" && f.returnType() == script::Type::Void)
      mEndFile = f;
    else if (f.name() == "beginBlock" && f.returnType() == script::Type::Void)
      mBeginBlock = f;
    else if (f.name() == "endBlock" && f.returnType() == script::Type::Void)
      mEndBlock = f;
    else if (f.name() == "dispatch" && f.returnType() == script::Type::Void)
      mDispatch = f;
  }

  if (mBeginFile.isNull() || mEndFile.isNull() || mBeginBlock.isNull() || mEndBlock.isNull() || mDispatch.isNull())
  {
    qDebug() << "Invalid parser file";
    throw std::runtime_error{ "Invalid parser file" };
  }
}

void Parser::process(const QStringList & files)
{
  for (const auto & f : files)
    processFile(f);
}

QSharedPointer<Environment> Parser::getEnvironment(const QString & name) const
{
  for (int i(mEnvironments.size() - 1); i >= 0; --i)
  {
    auto env = mEnvironments.at(i);
    auto result = env->getEnvironment(name);
    if (result == nullptr)
      continue;
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

NodeRef Parser::readArgument()
{
  auto token = mLexer.read();
  if (token.kind == Token::Space)
    token = mLexer.read();

  return createNode(token);
}

NodeRef Parser::createNode(const Token & tok)
{
  if (tok.kind == Token::BeginGroup)
  {
    auto result = NodeRef::createGroupNode(engine());
    auto element = read();
    /// TODO: use lexer.peekChar() instead to test for '}'
    while (!element.isWord() && element.toString() != "}")
    {
      if (!element.isNull())
        result.push_back(element);
    }
    return result;
  }
  else if (tok.kind == Token::Space || tok.kind == Token::EndOfLine)
  {
    return NodeRef::createGlueNode(engine());
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
  while (tok.text != "]")
  {
    if (tok.text == ",")
    {
      if (equal_sign_read)
      {
        result.add(argument.first, argument.second);
      }
      else
      {
        result.add(argument.first);
      }
      equal_sign_read = false;
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

        if (!group.isEmpty() && group.back().isGlue())
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

      /// TODO : remove glue at end and beginning of group.
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

  beginFile(path);

  while (mLexer.seekBlock())
  {
    beginBlock();

    while (!mLexer.atBlockEnd())
    {
      NodeRef node = read();
      if (!node.isNull())
        dispatch(node);
    }

    endBlock();
  }

  endFile();
}

void Parser::beginFile(const QString & path)
{
  script::Engine *e = mBeginFile.engine();
  script::Value arg = e->newString(path);
  e->call(mBeginFile, {arg});
  e->destroy(arg);
}

void Parser::endFile()
{
  script::Engine *e = mEndFile.engine();
  e->call(mEndFile, {});
}

void Parser::beginBlock()
{
  script::Engine *e = mBeginBlock.engine();
  e->call(mBeginBlock, {});
}

void Parser::endBlock()
{
  script::Engine *e = mEndBlock.engine();
  e->call(mEndBlock, {});
}

void Parser::dispatch(const NodeRef & node)
{
  script::Engine *e = mDispatch.engine();
  e->call(mDispatch, {node.impl()});
}

script::Engine* Parser::engine() const
{
  return mDispatch.engine();
}

} // namespace dex
