// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_OUTPUT_H
#define DEX_OUTPUT_H

#include <script/function.h>
#include <script/value.h>

#include <json-toolkit/json.h>

#include <QString>

#include <map>

namespace dex
{

class Output
{
public:
  Output(const script::Value& impl);
  ~Output();

  QString name() const;

  QString stringify(const json::Json& data);

  void write(const QString& outdir);

  static Output* staticCurrentOutput;
  static Output* current();

  static void expose(script::Namespace& ns);

protected:
  QString stringify(const script::Value& val);
  QString stringify(const json::Array& val, const script::Function& to_string);
  QString stringify(const json::Object& val, const script::Function& to_string);

private:
  script::Value m_self;
  script::Function m_write;
  std::map<int, script::Function> m_tostring_functions;
};

} // namespace dex


#endif // DEX_OUTPUT_H
