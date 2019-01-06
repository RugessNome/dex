// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Yasl project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/liquid/context.h"
#include "dex/liquid/template.h"

#include "dex/core/ref.h"
#include "dex/core/list.h"

#include <script/engine.h>

#include <QDebug>

void liquid_test_1()
{
  using namespace dex;

  script::Engine engine;
  engine.setup();

  register_ref_template(engine.rootNamespace());
  register_list_template(engine.rootNamespace());

  liquid::Context context{&engine};
  context.variables()["name"] = dex::Value{ engine.newString("Bob"), script::ParameterPolicy::Take };

  liquid::Template model = liquid::parse("Hello {{ name }}!", &engine);

  QString result = model.render(&context);
  qDebug() << result;
  Q_ASSERT(result == "Hello Bob!");
}

void run_liquid_tests()
{
  liquid_test_1();
}
