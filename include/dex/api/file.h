// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_FILE_H
#define DEX_FILE_H

#include <script/value.h>

#include <QFile>

namespace script
{
class Namespace;
} // namespace script

namespace dex
{

class File
{
public:

  struct TypeInfo {
    script::Type type;
  };

  static TypeInfo static_type_info;
  inline static TypeInfo & type_info() { return static_type_info; }

  static void register_type(script::Namespace ns);

  static QFile & get(const script::Value & val);
  static QFile::OpenMode getOpenMode(const script::Value & val);
};

} // namespace dex

#endif // DEX_FILE_H
