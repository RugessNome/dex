// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_ROOT_ENVIRONMENT_H
#define DEX_ROOT_ENVIRONMENT_H

#include "dex/processor/environment.h"

namespace script
{
class Script;
} // namespace script

namespace dex
{

class RootEnvironment : public Environment
{
public:
  RootEnvironment();

  QString name() const override;

  void fill(const script::Script & s);

  void enter(const Options& opts) override;
  void leave() override;
};

} // namespace dex

#endif // DEX_ROOT_ENVIRONMENT_H
