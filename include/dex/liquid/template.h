// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_LIQUID_TEMPLATE_H
#define DEX_LIQUID_TEMPLATE_H

#include <QList>
#include <QSharedPointer>

namespace script
{
class Engine;
class Namespace;
} // namespace script

namespace dex
{

namespace liquid
{

class Context;
class TemplateNode;

class Template
{
public:
  Template();
  Template(const Template &) = default;
  ~Template();

  Template(const QList<std::shared_ptr<TemplateNode>> & nodes);

  const QList<std::shared_ptr<TemplateNode>> & nodes() const { return mNodes; }

  QString render(Context *context) const;

private:
  QList<std::shared_ptr<TemplateNode>> mNodes;
};

Template parse(const QString & str, script::Engine *engine);

} // namespace liquid

} // namespace dex

#endif // DEX_LIQUID_TEMPLATE_H
