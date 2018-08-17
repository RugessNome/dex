// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/node.h"

#include "dex/endoflinenode.h"
#include "dex/groupnode.h"
#include "dex/spacenode.h"
#include "dex/wordnode.h"
#include "dex/utils.h"

#include <script/classtemplate.h>
#include <script/engine.h>
#include <script/private/value_p.h>
#include <script/templateargument.h>

namespace dex
{

Node::TypeInfo Node::static_type_info = Node::TypeInfo{};
NodeRef::TypeInfo NodeRef::static_type_info = NodeRef::TypeInfo{};


Node::Node(const script::Value & val)
  : mValue(val)
{

}

bool Node::isEndOfLine() const
{
  if (mValue.isNull())
    return false;
  return mValue.type().baseType() == EndOfLineNode::type_info().type;
}

bool Node::isSpaceNode() const
{
  if (mValue.isNull())
    return false;
  return mValue.type().baseType() == SpaceNode::type_info().type;
}

bool Node::isGroupNode() const
{
  if (mValue.isNull())
    return false;
  return mValue.type().baseType() == GroupNode::type_info().type;
}

bool Node::isWordNode() const
{
  if (mValue.isNull())
    return false;
  return mValue.type().baseType() == WordNode::type_info().type;
}

WordNode Node::asWordNode() const
{
  if (!isWordNode())
    return WordNode{};
  return WordNode{ mValue };
}

SpaceNode Node::asSpaceNode() const
{
  if (!isSpaceNode())
    return SpaceNode{};
  return SpaceNode{ mValue };
}

GroupNode Node::asGroupNode() const
{
  if (!isGroupNode())
    return GroupNode{};
  return GroupNode{ mValue };
}

void Node::register_node_type(const script::Class & node)
{
  auto & ti = type_info();

  ti.type = node.id();

  auto ref_template = node.engine()->getTemplate(script::Engine::RefTemplate);
  auto ref_node_class = ref_template.getInstance({ script::TemplateArgument{ script::Type{ node.id() } } });
  dex::NodeRef::type_info().type = ref_node_class.id();
}


NodeRef::NodeRef(const NodeRef & other) 
{
  if (!other.impl().isNull())
    mValue = other.impl().engine()->copy(other.impl());
}

NodeRef::NodeRef(NodeRef && other)
  : mValue(other.mValue)
{
  other.mValue = script::Value{};
}

NodeRef::NodeRef(const script::Value & val)
  : mValue(val)
{
  if (!mValue.isNull() && mValue == script::Value::Void)
    mValue = script::Value{};
}

NodeRef::NodeRef(const Node & n)
{

}

NodeRef::~NodeRef()
{
  if (!this->mValue.isNull())
    this->mValue.engine()->destroy(this->mValue);
  this->mValue = script::Value{};
}

Node NodeRef::getNode() const
{
  return Node{ script::Value{ mValue.impl()->data.builtin.ref } };
}

bool NodeRef::toBool() const
{
  QString value = getNode().asWordNode().value();
  return QString::compare(value, "true", Qt::CaseInsensitive) == 0;
}

int NodeRef::toInt() const
{
  QString value = getNode().asWordNode().value();
  return value.toInt();
}

QString NodeRef::toString() const
{
  WordNode n = getNode().asWordNode();
  return n.value();
}


int NodeRef::size() const
{
  GroupNode group = getNode().asGroupNode();
  return group.size();
}

void NodeRef::push_back(const NodeRef & n)
{
  GroupNode group = getNode().asGroupNode();
  group.push_back(n);
}

NodeRef NodeRef::at(int n)
{
  GroupNode group = getNode().asGroupNode();
  return group.at(n);
}

NodeRef NodeRef::back()
{
  GroupNode group = getNode().asGroupNode();
  return group.at(group.size() - 1);
}

NodeRef NodeRef::createWordNode(script::Engine *e, const QString & str)
{
  script::Value arg = e->newString(str);
  script::Value val = e->construct(WordNode::type_info().type, {arg});
  e->destroy(arg);
  script::Value result = e->construct(type_info().type, {});
  result.impl()->set_ref(val.impl());
  return result;
}

NodeRef NodeRef::createEndOfLine(script::Engine *e)
{
  script::Value val = e->construct(EndOfLineNode::type_info().type, {});
  script::Value result = e->construct(type_info().type, {});
  result.impl()->set_ref(val.impl());
  return result;
}

NodeRef NodeRef::createSpaceNode(script::Engine *e, const QString & str)
{
  script::Value arg = e->newString(str);
  script::Value val = e->construct(SpaceNode::type_info().type, { arg });
  e->destroy(arg);
  script::Value result = e->construct(type_info().type, {});
  result.impl()->set_ref(val.impl());
  return result;
}

NodeRef NodeRef::createGroupNode(script::Engine *e)
{
  script::Value val = e->construct(GroupNode::type_info().type, {});
  script::Value result = e->construct(type_info().type, {});
  result.impl()->set_ref(val.impl());
  return result;
}

NodeRef & NodeRef::operator=(const NodeRef & other)
{
  if (this == &other)
    return (*this);

  if (!this->mValue.isNull())
    this->mValue.engine()->destroy(this->mValue);

  if (other.impl().isNull())
    this->mValue = script::Value{};
  else
    this->mValue = other.impl().engine()->copy(other.impl());

  return (*this);
}

NodeRef & NodeRef::operator=(NodeRef && other)
{
  if (this == &other)
    return (*this);

  if (!this->mValue.isNull())
    this->mValue.engine()->destroy(this->mValue);

  this->mValue = other.mValue;
  other.mValue = script::Value{};

  return (*this);
}

} // namespace dex
