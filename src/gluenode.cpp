// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/gluenode.h"

#include <script/engine.h>

namespace dex
{

GlueNode::TypeInfo GlueNode::static_type_info = GlueNode::TypeInfo{};

GlueNode::GlueNode(const script::Value & val)
  : Node(val)
{

}

} // namespace dex
