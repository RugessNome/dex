// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_LIST_H
#define DEX_LIST_H

#include <script/classtemplatenativebackend.h>
#include <script/namespace.h>

#include <QList>

namespace dex
{

class Value;

void register_list_template(script::Namespace ns);
QList<dex::Value> & list_cast(const script::Value & val);


class ListTemplate : public script::ClassTemplateNativeBackend
{
  script::Class instantiate(script::ClassTemplateInstanceBuilder& builder) override;
};

} // namespace dex

#endif // DEX_LIST_H
