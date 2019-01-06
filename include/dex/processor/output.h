// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_OUTPUT_H
#define DEX_OUTPUT_H

#include <QString>
#include <QSharedPointer>

namespace script
{
class Function;
} // namespace script

namespace dex
{

class Output
{
public:
  virtual ~Output() = default;

  virtual QString name() const = 0;

  static QSharedPointer<Output> build(const script::Function & func);

  virtual void invoke(const QString & outdir) = 0;
};

} // namespace dex


#endif // DEX_OUTPUT_H
