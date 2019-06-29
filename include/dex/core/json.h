// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_JSON_H
#define DEX_JSON_H

#include <json-toolkit/json.h>

#include <script/types.h>

namespace script
{
class Namespace;
} // namespace script

namespace dex
{

struct JsonProxy
{
  json::Json data;

  JsonProxy(const json::Json& d);

  virtual json::Json& get() = 0;
};

struct JsonArrayProxy : JsonProxy
{
  int index;

  JsonArrayProxy(const json::Json& d, int i);
  JsonArrayProxy(const JsonArrayProxy&) = default;

  json::Json& get() override;
};

struct JsonObjectProxy : JsonProxy
{
  QString key;

  JsonObjectProxy(const json::Json& d, const QString& k);
  JsonObjectProxy(const JsonObjectProxy&) = default;

  json::Json& get() override;
};

void registerJsonTypes(script::Namespace& ns);

} // namespace dex

namespace script
{
template<> struct make_type_helper<json::Json> { static Type get() { return Type::Json; } };
template<> struct make_type_helper<json::Array> { static Type get() { return Type::JsonArray; } };
template<> struct make_type_helper<json::Object> { static Type get() { return Type::JsonObject; } };
template<> struct make_type_helper<dex::JsonArrayProxy> { static Type get() { return Type::JsonArrayProxy; } };
template<> struct make_type_helper<dex::JsonObjectProxy> { static Type get() { return Type::JsonObjectProxy; } };
} // namespace script

#endif // DEX_JSON_H
