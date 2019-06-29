// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_COMMAND_SPAN_H
#define DEX_COMMAND_SPAN_H

#include <script/types.h>

namespace script
{
class Engine;
class Namespace;
class Value;
} // namespace script

namespace dex
{

class CommandSpan
{
public:
  enum Value {
    NotApplicable = 0,
    Word,
    Line,
    Paragraph,
  };

  Value value = NotApplicable;

  static script::Type getBaseType();
  static script::Type getType(Value val);

  struct TypeInfo {
    script::Type span_type;
    script::Type word_type;
    script::Type line_type;
    script::Type paragraph_type;
  };

  static TypeInfo static_type_info;
  inline static TypeInfo & type_info() { return static_type_info; }

  static void register_span_types(script::Namespace ns);

  static script::Value expose(Value val, script::Engine *e);
};

namespace span
{

class Word : public CommandSpan
{
public:
  Word() { value = CommandSpan::Word; }
};

class Line : public CommandSpan
{
public:
  Line() { value = CommandSpan::Line; }
};

class Paragraph : public CommandSpan
{
public:
  Paragraph() { value = CommandSpan::Paragraph; }
};

} // namespace span

} // namespace dex

namespace script
{
template<> struct make_type_helper<dex::span::Word> { static Type get() { return Type::DexSpanWord; } };
template<> struct make_type_helper<dex::span::Line> { static Type get() { return Type::DexSpanLine; } };
template<> struct make_type_helper<dex::span::Paragraph> { static Type get() { return Type::DexSpanParagraph; } };
} // namespace script

#endif // DEX_COMMAND_SPAN_H
