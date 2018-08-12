// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_GROUP_NODE_H
#define DEX_GROUP_NODE_H

#include "dex/node.h"

namespace dex
{

class GroupNode : public Node
{
public:
  GroupNode() = default;
  GroupNode(const GroupNode &) = default;
  ~GroupNode() = default;

  GroupNode(const script::Value & val);

  int size() const;
  void push_back(const NodeRef & n);
  NodeRef at(int n) const;

  struct TypeInfo {
    script::Type type;
    script::Function size;
    script::Function at;
    script::Function push_back;
  };

  static TypeInfo static_type_info;
  static TypeInfo & type_info() { return static_type_info; }
};

} // dex

#endif // DEX_GROUP_NODE_H
