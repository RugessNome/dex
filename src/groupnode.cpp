// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/groupnode.h"

#include "dex/utils.h"

#include <script/engine.h>

namespace dex
{

GroupNode::TypeInfo GroupNode::static_type_info = GroupNode::TypeInfo{};


GroupNode::GroupNode(const script::Value & val)
  : Node(val)
{

}

int GroupNode::size() const
{
  script::Engine *e = type_info().size.engine();
  return readInt(e->call(type_info().size, { mValue }));
}

void GroupNode::push_back(const NodeRef & n)
{
  script::Engine *e = type_info().push_back.engine();
  e->call(type_info().push_back, { mValue, n.impl() });
}

NodeRef GroupNode::at(int n) const
{
  script::Engine *e = type_info().at.engine();

  script::Value index = e->newInt(n);
  script::Value result = e->call(type_info().at, { mValue, index });
  e->destroy(index);
  return NodeRef{ result };
}

} // namespace dex
