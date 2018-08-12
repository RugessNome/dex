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

} // namespace dex

#endif // DEX_COMMAND_SPAN_H
