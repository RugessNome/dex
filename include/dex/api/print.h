// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_API_PRINT_H
#define DEX_API_PRINT_H

namespace script
{
class Namespace;
} // namespace script

namespace dex
{

namespace api
{
void registerPrintFunctions(script::Namespace ns);
} // namespace api

} // namespace dex

#endif // DEX_API_PRINT_H
