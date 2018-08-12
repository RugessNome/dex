// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_NODE_H
#define DEX_NODE_H

#include <script/function.h>
#include <script/value.h>

#include <QSharedPointer>

namespace dex
{

class WordNode;
class GlueNode;
class GroupNode;

class Node
{
public:
  Node() = default;
  Node(const Node &) = default;
  ~Node() = default;

  Node(const script::Value & val);

  inline bool isNull() const { return mValue.isNull(); }
  
  bool isGlueNode() const;
  bool isGroupNode() const;
  bool isWordNode() const;

  struct TypeInfo {
    script::Type type;
  };

  WordNode asWordNode() const;
  GlueNode asGlueNode() const;
  GroupNode asGroupNode() const;

  static TypeInfo static_type_info;
  static TypeInfo & type_info() { return static_type_info; }

  static void register_node_type(const script::Class & node);

  Node & operator=(const Node &) = default;

protected:
  script::Value mValue;
};


class NodeRef
{
public:
  NodeRef() {}
  NodeRef(const NodeRef & other);
  NodeRef(NodeRef && other);
  ~NodeRef();

  NodeRef(const script::Value & val);
  NodeRef(const Node & n);

  inline bool isNull() const { return mValue.isNull(); }

  Node getNode() const;

  inline bool isWord() const { return getNode().isWordNode(); }
  bool toBool() const;
  int toInt() const;
  QString toString() const;

  inline bool isGlue() const { return getNode().isGlueNode(); }

  inline bool isGroup() const { return getNode().isGroupNode(); }
  int size() const;
  inline bool isEmpty() const { return size() == 0; }
  void push_back(const NodeRef & n);
  NodeRef at(int n);
  NodeRef back();

  static NodeRef createWordNode(script::Engine *e, const QString & str);
  static NodeRef createGlueNode(script::Engine *e);
  static NodeRef createGroupNode(script::Engine *e);

  inline const script::Value & impl() const { return mValue; }

  NodeRef & operator=(const NodeRef & other);
  NodeRef & operator=(NodeRef && other);

  struct TypeInfo {
    script::Type type;
  };

  static TypeInfo static_type_info;
  static TypeInfo & type_info() { return static_type_info; }

private:
  script::Value mValue;
};

} // namespace dex

#endif // DEX_NODE_H
