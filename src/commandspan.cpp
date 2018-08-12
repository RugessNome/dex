// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/commandspan.h"

#include <script/classbuilder.h>
#include <script/engine.h>
#include <script/functionbuilder.h>
#include <script/namespace.h>
#include <script/value.h>
#include <script/private/value_p.h>
#include <script/interpreter/executioncontext.h>

namespace callbacks
{

static script::Value dummy(script::FunctionCall *c)
{
  return c->thisObject();
}

} // namespace callbacks

namespace dex
{

CommandSpan::TypeInfo CommandSpan::static_type_info = CommandSpan::TypeInfo{};


script::Type CommandSpan::getBaseType()
{
  return type_info().span_type;
}

script::Type CommandSpan::getType(Value val)
{
  switch (val)
  {
  case dex::CommandSpan::Word:
    return type_info().word_type;
  case dex::CommandSpan::Line:
    return type_info().line_type;
  case dex::CommandSpan::Paragraph:
    return type_info().paragraph_type;
  case dex::CommandSpan::NotApplicable:
  default:
    throw std::runtime_error{ "Invalid span value" };
  }
}

void CommandSpan::register_span_types(script::Namespace ns)
{
  using namespace script;

  Class Span = ns.Class("Span").get();
  Class Word = Span.NestedClass("Word").setBase(Span).get();;
  Class Line = Span.NestedClass("Line").setBase(Span).get();;
  Class Paragraph = Span.NestedClass("Paragraph").setBase(Span).get();;

  for (Class c : { Span, Word, Line, Paragraph })
  {
    c.Constructor(callbacks::dummy).create();
    c.Constructor(callbacks::dummy).params(Type::cref(c.id())).create();
    c.newDestructor(callbacks::dummy);
  }

  dex::CommandSpan::type_info().span_type = Span.id();
  dex::CommandSpan::type_info().word_type = Word.id();
  dex::CommandSpan::type_info().line_type = Line.id();
  dex::CommandSpan::type_info().paragraph_type = Paragraph.id();
}

script::Value CommandSpan::expose(Value val, script::Engine *e)
{
  auto ret = e->uninitialized(getType(val));
  ret.impl()->remove_uninitialized_flag();
  return ret;
}

} // namespace dex
