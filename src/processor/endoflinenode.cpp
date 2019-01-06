// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/processor/endoflinenode.h"

#include <script/engine.h>

namespace dex
{

EndOfLineNode::TypeInfo EndOfLineNode::static_type_info = EndOfLineNode::TypeInfo{};

EndOfLineNode::EndOfLineNode(const script::Value & val)
  : Node(val)
{

}

} // namespace dex
