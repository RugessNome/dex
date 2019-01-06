// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_LIST_H
#define DEX_LIST_H

#include <script/namespace.h>

#include <QList>

namespace dex
{

class Value;

void register_list_template(script::Namespace ns);
QList<dex::Value> & list_cast(const script::Value & val);

} // namespace dex

#endif // DEX_LIST_H
