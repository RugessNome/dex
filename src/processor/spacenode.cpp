// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/processor/spacenode.h"

#include <script/engine.h>

namespace dex
{

SpaceNode::TypeInfo SpaceNode::static_type_info = SpaceNode::TypeInfo{};

SpaceNode::SpaceNode(const script::Value & val)
  : Node(val)
{

}

QString SpaceNode::value() const
{
  script::Engine *e = type_info().get_value.engine();
  script::Value val = e->call(type_info().get_value, { mValue });
  QString result = val.toString();
  if (!type_info().get_value.returnType().isReference())
    e->destroy(val);
  return result;
}

} // namespace dex
