// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_GLUE_NODE_H
#define DEX_GLUE_NODE_H

#include "dex/node.h"

namespace dex
{

class GlueNode : public Node
{
public:
  GlueNode() = default;
  GlueNode(const GlueNode & ) = default;
  ~GlueNode() = default;

  explicit GlueNode(const script::Value & val);

  struct TypeInfo {
    script::Type type;
  };

  static TypeInfo static_type_info;
  static TypeInfo & type_info() { return static_type_info; }
};

} // namespace dex

#endif // DEX_GLUE_NODE_H
