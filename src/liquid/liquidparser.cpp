// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/liquid/liquidparser.h"

#include <script/engine.h>

#include <QMap>

#include <QDebug>

namespace dex
{

namespace liquid
{

bool Token::operator==(const char *str) const
{
  return toStringRef() == str;
}

Tokenizer::Tokenizer()
{
  mPunctuators << '!' << '<' << '>' << '=';
}

QList<Token> Tokenizer::tokenize(const QString & str)
{
  QList<Token> result;
  
  mInput = str;
  mPosition = 0;

  readSpaces();

  while (!atEnd())
    result.append(read());

  return result;
}

Token Tokenizer::read()
{
  if (atEnd())
  {
    qDebug() << "Lexer::read() : unexpected end of input";
    throw std::runtime_error{ "Unexpected end of input" };
  }

  mStartPos = position();

  if (peekChar() == '|')
    return readChar(), produce(Token::Pipe);
  else   if (peekChar() == ':')
    return readChar(), produce(Token::Colon);
  else if (peekChar() == '.')
    return readChar(), produce(Token::Dot);
  else if (peekChar() == ',')
    return readChar(), produce(Token::Comma);
  else if (peekChar() == '[')
    return readChar(), produce(Token::LeftBracket);
  else if (peekChar() == ']')
    return readChar(), produce(Token::RightBracket);
  else if (peekChar().isDigit())
    return readIntegerLiteral();
  else if (peekChar() == '\'' || peekChar() == '"')
    return readStringLiteral();
  else if (peekChar().isLetter() || peekChar() == '_')
    return readIdentifier();
  else if (isPunctuator(peekChar()))
    return readOperator();

  throw std::runtime_error{ "Tokenizer::read() : error" };
}

void Tokenizer::readChar(int count)
{
  while (count >= 0)
  {
    readChar();
    --count;
  }
}

QChar Tokenizer::readChar()
{
  return mInput[mPosition++];
}

QChar Tokenizer::peekChar() const
{
  return mInput[mPosition];
}

void Tokenizer::seek(int pos)
{
  mPosition = std::min(pos, input().length());
}

bool Tokenizer::isPunctuator(const QChar & c) const
{
  return mPunctuators.contains(c);
}

bool Tokenizer::readSpaces()
{
  const int offset = position();

  while (!atEnd() && peekChar().isSpace() && peekChar() != '\n')
    readChar();

  return position() != offset;
}

Token Tokenizer::produce(Token::Kind k)
{
  const int pos = position();
  readSpaces();
  return Token{ k, mInput, mStartPos, pos - mStartPos };
}

Token Tokenizer::readIdentifier()
{
  auto is_valid = [](const QChar & c) -> bool {
    return c.isLetterOrNumber() || c == '_';
  };

  while (!atEnd() && is_valid(peekChar()))
    readChar();

  Token ret = produce(Token::Identifier);

  if (ret.toStringRef() == "or" || ret.toStringRef() == "and")
    ret.kind = Token::Operator;
  else if (ret.toStringRef() == "true" || ret.toStringRef() == "false")
    ret.kind = Token::BooleanLiteral;

  return ret;
}

Token Tokenizer::readIntegerLiteral()
{
  while (!atEnd() && peekChar().isDigit())
    readChar();

  return produce(Token::IntegerLiteral);
}

Token Tokenizer::readStringLiteral()
{
  const QChar quote = readChar();
  int index = input().indexOf(quote, position());
  if (index == -1)
    throw std::runtime_error{ "Malformed string literal" };
  seek(index);
  readChar();
  return produce(Token::StringLiteral);
}

Token Tokenizer::readOperator()
{
  if (peekChar() == '<' || peekChar() == '>' || peekChar() == '==')
  {
    QChar first_char = readChar();
    if (atEnd())
      return produce(Token::Operator);
    else if (peekChar() == '=')
      return readChar(), produce(Token::Operator);
    else if (first_char == '<' && peekChar() == '>')
      return readChar(), produce(Token::Operator);
  }
  else if (peekChar() == '!')
  {
    readChar();
    if (atEnd())
      return produce(Token::Operator);
    else if (peekChar() == '=')
      return readChar(), produce(Token::Operator);
  }
  else
  {
    readChar();
  }

  return produce(Token::Operator);
}

bool Tokenizer::atEnd() const
{
  return mInput.size() == mPosition;
}


Parser::Parser(script::Engine *e)
  : mEngine(e)
{

}

QList<std::shared_ptr<liquid::TemplateNode>> Parser::parse(const QString & document)
{
  mDocument = document;
  mPosition = 0;
  mNodes.clear();
  mStack.clear();

  while (!atEnd())
    readNode();

  return mNodes;
}

void Parser::readNode()
{
  int pos = document().indexOf('{', position());
  if (pos == -1 || pos == document().length() - 1)
  {
    auto ret = std::make_shared<tnodes::Text>(document().mid(position()));
    mPosition = document().length();
    dispatchNode(ret);
    return;
  }

  if (pos == position())
  {
    if (document().at(pos + 1) == '{')
    {
      pos = pos + 2;
      int endpos = document().indexOf("}}", pos);
      if(endpos == -1)
        throw std::runtime_error{ "liquid::Parser::dispatchNode() error" };

      auto tokens = tokenizer().tokenize(document().mid(pos, endpos - pos));
      auto obj = processObject(tokens);
      dispatchNode(obj);

      mPosition = endpos + 2;
    }
    else if (document().at(pos + 1) == '%')
    {
      pos = pos + 2;
      int endpos = document().indexOf("%}", pos);
      if (endpos == -1)
        throw std::runtime_error{ "liquid::Parser::dispatchNode() error" };

      auto tokens = tokenizer().tokenize(document().mid(pos, endpos - pos));
      processTag(tokens);

      mPosition = endpos + 2;
    }
    else
    {
      auto ret = std::make_shared<tnodes::Text>(document().mid(position(), pos + 1 - position()));
      mPosition = pos + 1;
      dispatchNode(ret);
    }
  }
  else
  {
    auto ret = std::make_shared<tnodes::Text>(document().mid(position(), pos - position()));
    mPosition = pos;
    dispatchNode(ret);
    return;
  }
}

void Parser::dispatchNode(std::shared_ptr<liquid::TemplateNode> n)
{
  if (mStack.isEmpty())
  {
    mNodes.append(n);
  }
  else
  {
    auto top = mStack.top();
    if (top->is<tnodes::For>())
      top->as<tnodes::For>().body.append(n);
    else if (top->is<tnodes::If>())
      top->as<tnodes::If>().blocks.back().body.append(n);
    else
      throw std::runtime_error{ "liquid::Parser::dispatchNode() error" };
  }
}

void Parser::processTag(QList<Token> & tokens)
{
  Token tok = tokens.takeFirst();

  if (tok == "assign")
    process_tag_if(tokens);
  else if (tok == "if")
    process_tag_if(tokens);
  else if (tok == "elsif")
    process_tag_elsif(tokens);
  else if (tok == "else")
    process_tag_else(tokens);
  else if (tok == "endif")
    process_tag_else(tokens);
  else if (tok == "for")
    process_tag_for(tokens);
  else if (tok == "break")
    process_tag_break(tokens);
  else if (tok == "continue")
    process_tag_continue(tokens);
  else if (tok == "endfor")
    process_tag_endfor(tokens);
}

std::shared_ptr<liquid::tnodes::Object> Parser::processObject(QList<Token> & tokens)
{
  if (tokens.size() == 1 && tokens.first().kind == Token::Identifier)
    return std::make_shared<tnodes::Variable>(tokens.first().toString());

  auto obj = process_object_read_operand(tokens);

  if (tokens.empty())
    return obj;

  QList<std::shared_ptr<liquid::tnodes::Object>> operands;
  operands.append(obj);
  QList<QString> operators;

  while (!tokens.empty() && tokens.first().kind != Token::Pipe)
  {
    operators.append(process_object_read_operator(tokens));
    operands.append(process_object_read_operand(tokens));
  }

  obj = process_object_build_expr(operands, operators);

  /* Apply filters */
  while (!tokens.isEmpty() && tokens.first().kind == Token::Pipe)
  {
    obj = process_object_apply_filter(obj, tokens);
  }

  return obj;
}

void Parser::process_tag_assign(QList<Token> & tokens)
{
  Token name = tokens.takeFirst();
  Token eq = tokens.takeFirst();

  auto expr = processObject(tokens);

  auto node = std::make_shared<tnodes::Assign>(name.toString(), expr);
  dispatchNode(node);
}

void Parser::process_tag_if(QList<Token> & tokens)
{
  auto cond = processObject(tokens);
  auto tag = std::make_shared<tnodes::If>(cond);
  mStack.push(tag);
}

void Parser::process_tag_elsif(QList<Token> & tokens)
{
  if (mStack.isEmpty() || !mStack.top()->is<tnodes::If>())
    throw std::runtime_error{ "Bad elsif" };

  tnodes::If::Block block;
  block.condition = processObject(tokens);

  mStack.top()->as<tnodes::If>().blocks.append(block);
}

void Parser::process_tag_else(QList<Token> & tokens)
{
  if (mStack.isEmpty() || !mStack.top()->is<tnodes::If>())
    throw std::runtime_error{ "Bad else" };

  script::Value cond = engine()->newBool(true);
  dex::Value val{ std::move(cond) };

  tnodes::If::Block block;
  block.condition = std::make_shared<tnodes::Value>(std::move(val));

  mStack.top()->as<tnodes::If>().blocks.append(block);
}

void Parser::process_tag_endif(QList<Token> & tokens)
{
  if (mStack.isEmpty() || !mStack.top()->is<tnodes::If>())
    throw std::runtime_error{ "Bad endif" };

  auto node = mStack.pop();
  Q_ASSERT(node->is<tnodes::If>());
  dispatchNode(node);
}

void Parser::process_tag_for(QList<Token> & tokens)
{
  QString name = tokens.takeFirst().toString();

  QString in = tokens.takeFirst().toString();
  if (in != "in")
    throw std::runtime_error{ "Bad for" };

  auto container = processObject(tokens);

  auto tag = std::make_shared<tnodes::For>(name, container);
  mStack.push(tag);
}

void Parser::process_tag_break(QList<Token> & tokens)
{
  dispatchNode(std::make_shared<tnodes::Break>());
}

void Parser::process_tag_continue(QList<Token> & tokens)
{
  dispatchNode(std::make_shared<tnodes::Continue>());
}

void Parser::process_tag_endfor(QList<Token> & tokens)
{
  if (mStack.isEmpty() || !mStack.top()->is<tnodes::For>())
    throw std::runtime_error{ "Bad endfor" };

  auto node = mStack.pop();
  Q_ASSERT(node->is<tnodes::For>());
  dispatchNode(node);
}

std::shared_ptr<liquid::tnodes::Object> Parser::process_object_read_operand(QList<Token> & tokens)
{
  std::shared_ptr<liquid::tnodes::Object> obj;

  Token tok = tokens.takeFirst();
  if (tok.kind == Token::Identifier)
    obj = std::make_shared<tnodes::Variable>(tok.toString());
  else if (tok.kind == Token::BooleanLiteral)
    obj = process_object_bool_literal(tok);
  else if (tok.kind == Token::IntegerLiteral)
    obj = process_object_int_literal(tok);
  else if (tok.kind == Token::StringLiteral)
    obj = process_object_string_literal(tok);
  else
    throw std::runtime_error{ "Bad object" };

  while (!tokens.isEmpty())
  {
    if (tokens.first().kind == Token::Dot)
    {
      tokens.takeFirst();

      if (tokens.empty() || tokens.first().kind != Token::Identifier)
        throw std::runtime_error{ "Bad object" };

      tok = tokens.takeFirst();
      obj = std::make_shared<tnodes::MemberAccess>(obj, tok.toString().toStdString());
    }
    else if (tokens.first().kind == Token::LeftBracket)
    {
      tokens.takeFirst();
      QList<Token> subtokens;
      while (!tokens.empty() && tokens.first().kind != Token::RightBracket)
      {
        subtokens.append(tokens.takeFirst());
      }

      if(tokens.isEmpty())
        throw std::runtime_error{ "Bad array" };

      tokens.takeFirst();

      auto index = processObject(subtokens);
      obj = std::make_shared<tnodes::ArrayAccess>(obj, index);
    }
    else
    {
      break;
    }
  }

  return obj;
}

QString Parser::process_object_read_operator(QList<Token> & tokens)
{
  Token tok = tokens.takeFirst();
  if (tok.kind != Token::Operator)
    throw std::runtime_error{ "Bad expression" };
  return tok.toString();
}

std::shared_ptr<liquid::tnodes::Object> Parser::process_object_build_expr(QList<std::shared_ptr<liquid::tnodes::Object>> operands, QList<QString> operators)
{
  struct OpInfo { tnodes::BinOp::Operation name; int precedence; };

  static QMap<QString, OpInfo> map{
    { "or", { tnodes::BinOp::Or, 4 } },
    { "and", { tnodes::BinOp::And, 3 } },
    { "!=", { tnodes::BinOp::Inequal, 2 } },
    { "<>", { tnodes::BinOp::Inequal, 2 } },
    { "==", { tnodes::BinOp::Equal, 2 } },
    { "<", { tnodes::BinOp::Less, 1 } },
    { "<=", { tnodes::BinOp::Leq, 1 } },
    { ">", { tnodes::BinOp::Greater, 1 } },
    { ">=", { tnodes::BinOp::Geq,1 } },
  };

  if (operators.size() == 0)
    return operands.first();

  int op_index = operators.size() - 1;
  OpInfo op_info = map.find(operators.last()).value();

  for (int i(operators.size() - 2); i >= 0; --i)
  {
    auto it = map.find(operators.at(i));
    if (it.value().precedence > op_info.precedence)
      op_index = i, op_info = it.value();
  }

  auto lhs = process_object_build_expr(operands.mid(0, op_index + 1), operators.mid(0, op_index));
  auto rhs = process_object_build_expr(operands.mid(op_index + 1), operators.mid(op_index + 1));
  return std::make_shared<tnodes::BinOp>(op_info.name, lhs, rhs);
}

std::shared_ptr<liquid::tnodes::Object> Parser::process_object_bool_literal(const Token & tok)
{
  script::Value val = engine()->newBool(tok == "true");
  dex::Value obj{ std::move(val) };
  return std::make_shared<tnodes::Value>(std::move(obj));
}

std::shared_ptr<liquid::tnodes::Object> Parser::process_object_int_literal(const Token & tok)
{
  script::Value val = engine()->newInt(tok.toString().toInt());
  dex::Value obj{ std::move(val) };
  return std::make_shared<tnodes::Value>(std::move(obj));
}

std::shared_ptr<liquid::tnodes::Object> Parser::process_object_string_literal(const Token & tok)
{
  script::Value val = engine()->newString(tok.toString().mid(1, tok.length_ - 2));
  dex::Value obj{ std::move(val) };
  return std::make_shared<tnodes::Value>(std::move(obj));
}

std::shared_ptr<liquid::tnodes::Object> Parser::process_object_apply_filter(std::shared_ptr<liquid::tnodes::Object> obj, QList<Token> & tokens)
{
  Token tok = tokens.takeFirst();
  tok = tokens.takeFirst();

  QString name = tok.toString();
  
  auto ret = std::make_shared<tnodes::Pipe>(obj, name);

  if (tokens.isEmpty() || tokens.first().kind == Token::Pipe)
    return ret;

  if (tokens.first().kind != Token::Colon)
    throw std::runtime_error{ "Bad filter" };

  tokens.takeFirst();

  while (!tokens.isEmpty() && tokens.first().kind != Token::Pipe)
  {
    ret->arguments.append(process_object_read_literal(tokens));

    if (tokens.isEmpty() || tokens.first().kind == Token::Comma)
      throw std::runtime_error{ "Bad filter" };

    tokens.takeFirst();
  }

  return ret;
}

dex::Value Parser::process_object_read_literal(QList<Token> & tokens)
{
  Token tok = tokens.takeFirst();

  if (tok.kind == Token::BooleanLiteral)
  {
    script::Value val = engine()->newBool(tok == "true");
    dex::Value obj{ std::move(val) };
    return obj;
  }
  else if (tok.kind == Token::IntegerLiteral)
  {
    script::Value val = engine()->newInt(tok.toString().toInt());
    dex::Value obj{ std::move(val) };
    return obj;
  }
  else if (tok.kind == Token::StringLiteral)
  {
    script::Value val = engine()->newString(tok.toString().mid(1, tok.length_ - 2));
    dex::Value obj{ std::move(val) };
    return obj;
  }
  else
  {
    throw std::runtime_error{ "Bad literal" };
  }
}

} // namespace liquid

} // namespace dex
