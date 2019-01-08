// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_API_H
#define DEX_API_H

namespace script
{
class Engine;
} // namespace script

namespace dex
{

namespace api
{

void expose(script::Engine *e);

} // namespace api

} // namespace dex

#endif // DEX_API_H
