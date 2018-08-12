// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_WORD_NODE_H
#define DEX_WORD_NODE_H

#include "dex/node.h"

namespace dex
{

class WordNode : public Node
{
public:
  WordNode() = default;
  WordNode(const WordNode & ) = default;
  ~WordNode() = default;

  WordNode(const script::Value & val);

  QString value() const;

  struct TypeInfo {
    script::Type type;
    script::Function get_value;
  };

  static TypeInfo static_type_info;
  static TypeInfo & type_info() { return static_type_info; }
};

} // namespace dex

#endif // DEX_WORD_NODE_H
