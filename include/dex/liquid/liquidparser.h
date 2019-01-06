// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_LIQUID_PARSER_H
#define DEX_LIQUID_PARSER_H

#include "dex/liquid/template_p.h"

#include <QStack>
#include <QSet>
#include <QStringRef>

namespace dex
{

namespace liquid
{

struct Token
{
  enum Kind {
    Identifier,
    Dot,
    LeftBracket,
    RightBracket,
    Operator,
    Pipe,
    Colon,
    Comma,
    BooleanLiteral,
    IntegerLiteral,
    StringLiteral,
    Nil,
  };

  Kind kind;
  QString text_;
  int offset_;
  int length_;

  inline QStringRef toStringRef() const { return text_.midRef(offset_, length_); }
  inline QString toString() const { return text_.mid(offset_, length_); }

  bool operator==(const char *str) const;
};

class Tokenizer
{
public:
  Tokenizer();

  QList<Token> tokenize(const QString & str);

protected:
  Token read();
  inline QChar nextChar() const { return peekChar(); }
  bool atEnd() const;

  inline int position() const { return mPosition; }
  inline const QString & input() const { return mInput; }

protected:
  friend struct TokenProducer;

  void readChar(int count);
  QChar readChar();
  QChar peekChar() const;
  void seek(int pos);
  bool isPunctuator(const QChar & c) const;
  bool readSpaces();
  Token produce(Token::Kind k);

  Token readIdentifier();
  Token readIntegerLiteral();
  Token readStringLiteral();
  Token readOperator();

private:
  int mPosition;
  int mStartPos;
  QString mInput;
  QSet<QChar> mPunctuators;
};


class Parser
{
public:
  Parser(script::Engine *e);

  QList<std::shared_ptr<liquid::TemplateNode>> parse(const QString & document);

protected:
  void readNode();
  void dispatchNode(std::shared_ptr<liquid::TemplateNode> n);
  inline bool atEnd() const { return mPosition == mDocument.length(); }

  void processTag(QList<Token> & tokens);
  std::shared_ptr<liquid::tnodes::Object> processObject(QList<Token> & tokens);

  inline Tokenizer & tokenizer() { return mTokenizer;  }
  inline int position() const { return mPosition; }
  inline const QString & document() const { return mDocument; }
  inline script::Engine* engine() const { return mEngine; }

protected:
  void process_tag_assign(QList<Token> & tokens);
  void process_tag_if(QList<Token> & tokens);
  void process_tag_elsif(QList<Token> & tokens);
  void process_tag_else(QList<Token> & tokens);
  void process_tag_endif(QList<Token> & tokens);
  void process_tag_for(QList<Token> & tokens);
  void process_tag_break(QList<Token> & tokens);
  void process_tag_continue(QList<Token> & tokens);
  void process_tag_endfor(QList<Token> & tokens);

protected:
  std::shared_ptr<liquid::tnodes::Object> process_object_read_operand(QList<Token> & tokens);
  QString process_object_read_operator(QList<Token> & tokens);
  std::shared_ptr<liquid::tnodes::Object> process_object_build_expr(QList<std::shared_ptr<liquid::tnodes::Object>> operands, QList<QString> operators);
  std::shared_ptr<liquid::tnodes::Object> process_object_bool_literal(const Token & tok);
  std::shared_ptr<liquid::tnodes::Object> process_object_int_literal(const Token & tok);
  std::shared_ptr<liquid::tnodes::Object> process_object_string_literal(const Token & tok);
  std::shared_ptr<liquid::tnodes::Object> process_object_apply_filter(std::shared_ptr<liquid::tnodes::Object> obj, QList<Token> & tokens);
  dex::Value process_object_read_literal(QList<Token> & tokens);

private:
  script::Engine *mEngine;
  int mPosition;
  QString mDocument;
  Tokenizer mTokenizer;
  QList<std::shared_ptr<liquid::TemplateNode>> mNodes;
  QStack<std::shared_ptr<liquid::TemplateNode>> mStack;
};

} // namespace liquid

} // namespace dex

#endif // DEX_LIQUID_PARSER_H
