// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_USER_OUTPUT_H
#define DEX_USER_OUTPUT_H

#include "dex/processor/output.h"

#include <script/function.h>

namespace dex
{

class UserOutput : public Output
{
public:
  UserOutput(const script::Function & func);
  ~UserOutput() = default;

  QString name() const override;

  void invoke(const QString & outdir) override;

private:
  script::Function mFunction;
};

} // namespace dex

#endif // DEX_USER_OUTPUT_H
