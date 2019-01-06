// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_EOL_NODE_H
#define DEX_EOL_NODE_H

#include "dex/processor/node.h"

namespace dex
{

class EndOfLineNode : public Node
{
public:
  EndOfLineNode() = default;
  EndOfLineNode(const EndOfLineNode &) = default;
  ~EndOfLineNode() = default;

  EndOfLineNode(const script::Value & val);

  struct TypeInfo {
    script::Type type;
  };

  static TypeInfo static_type_info;
  static TypeInfo & type_info() { return static_type_info; }
};

} // namespace dex

#endif // DEX_EOL_NODE_H
