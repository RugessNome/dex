// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_API_LIQUID_H
#define DEX_API_LIQUID_H

#include <liquid/liquid.h>
#include <liquid/renderer.h>

#include <script/types.h>
#include <script/function.h>
#include <script/value.h>

namespace script
{
class Namespace;
} // namespace script

namespace dex
{

class LiquidRenderer : public liquid::Renderer
{
public: 
  LiquidRenderer() = default;

  void init(const script::Value& s);

protected:
  QString stringify(const json::Json& val) override;
  json::Json applyFilter(const QString& name, const json::Json& object, const std::vector<json::Json>& args) override;

private:
  script::Value m_self;
  std::map<QString, script::Function> m_filters;
};

namespace api
{
void registerLiquidApi(script::Engine* e);
} // namespace api
} // namespace dex

namespace script
{
template<> struct make_type_helper<dex::LiquidRenderer> { static Type get() { return Type::DexLiquidRenderer; } };
template<> struct make_type_helper<liquid::Template> { static Type get() { return Type::LiquidTemplate; } };
} // namespace script

#endif // DEX_API_LIQUID_H
