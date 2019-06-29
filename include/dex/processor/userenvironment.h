// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_USER_ENVIRONMENT_H
#define DEX_USER_ENVIRONMENT_H

#include "dex/processor/environment.h"

#include <script/function.h>

namespace dex
{

class UserEnvironment : public Environment
{
public:
  UserEnvironment(const script::Function & ef, const script::Function & lf);

  QString name() const override;

  void enter(const Options& opts) override;
  void leave() override;

private:
  script::Function mEnterFunction;
  script::Function mLeaveFunction;
};

} // namespace dex

#endif // DEX_USER_ENVIRONMENT_H
