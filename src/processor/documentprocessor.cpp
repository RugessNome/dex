// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/processor/documentprocessor.h"

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

  if (token.kind == StreamTokenizer::Token::EscapeCharacter)
    return readCommand(mTokenizer.read());

  return createNode(token);
}

NodeRef DocumentProcessor::readArgument()
{
  auto token = mTokenizer.read();
  if (token.kind == StreamTokenizer::Token::Space) /// TODO: should we consider EOL too ?
    token = mTokenizer.read();

  if (token.kind == StreamTokenizer::Token::EscapeCharacter)
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
  if (tok.kind == StreamTokenizer::Token::BeginGroup)
    return readGroup(tok);
  else if (tok.kind == StreamTokenizer::Token::Space)
    return NodeRef::createSpaceNode(engine(), tok.text);
  else if (tok.kind == StreamTokenizer::Token::EndOfLine)
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
