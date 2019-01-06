// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/liquid/template.h"
#include "dex/liquid/template_p.h"

#include "dex/liquid/context.h"
#include "dex/liquid/liquidparser.h"

#include <script/class.h>
#include <script/engine.h>

namespace dex
{

namespace liquid
{

namespace tnodes
{

Text::Text(const QString & str)
  : text(str)
{

}

Value::Value(const dex::Value & val)
  : value(val)
{

}

Variable::Variable(const QString & n)
  : name(n)
{

}

ArrayAccess::ArrayAccess(const std::shared_ptr<Object> & obj, const std::shared_ptr<Object> & ind)
  : object(obj)
  , index(ind)
{

}

MemberAccess::MemberAccess(const std::shared_ptr<Object> & obj, const std::string & name)
  : object(obj)
  , name(name)
{

}

BinOp::BinOp(Operation op, const std::shared_ptr<Object> & left, const std::shared_ptr<Object> & right)
  : operation(op)
  , lhs(left)
  , rhs(right)
{

}

Pipe::Pipe(const std::shared_ptr<Object> & object, const QString & filtername, const QList<dex::Value> & args)
  : object(object)
  , filterName(filtername)
  , arguments(args)
{

}


Assign::Assign(const QString & varname, const std::shared_ptr<Object> & expr)
  : variable(varname)
  , value(expr)
{

}

For::For(const QString & varname, const std::shared_ptr<Object> & expr)
  : variable(varname)
  , object(expr)
{

}

Break::Break() { }
Continue::Continue() { }

If::If(std::shared_ptr<Object> cond)
{
  Block b;
  b.condition = cond;
  blocks.append(b);
}

} // namespace tnodes

class Renderer
{
public:
  Context *context;
  QString result_;

  QString process(const QList<std::shared_ptr<TemplateNode>> & nodes);

  dex::Value eval(const std::shared_ptr<tnodes::Object> & obj);
  void exec(const std::shared_ptr<tnodes::Tag> & tag);
  void exec(const QList<std::shared_ptr<TemplateNode>> & nodes);
  QString stringify(const dex::Value & val);

  bool eval_condition(const dex::Value & val);

protected:
  void process(const std::shared_ptr<TemplateNode> & n);

  /* Tags */
  void exec_assign(const tnodes::Assign & assign);
  void exec_for(const tnodes::For & tag);
  void exec_if(const tnodes::If & tag);
  void exec_break(const tnodes::Break & tag);
  void exec_continue(const tnodes::Continue & tag);

  /* Objects */
  dex::Value eval_value(const tnodes::Value & val);
  dex::Value eval_variable(const tnodes::Variable & var);
  dex::Value eval_memberaccess(const tnodes::MemberAccess & ma);
  dex::Value eval_arrayaccess(const tnodes::ArrayAccess & aa);
  dex::Value eval_binop(const tnodes::BinOp & binop);
  dex::Value eval_pipe(const tnodes::Pipe & pipe);

  /* Binary operations */
  dex::Value eval_binop_eq(const dex::Value & lhs, const dex::Value & rhs);
  dex::Value eval_binop_neq(const dex::Value & lhs, const dex::Value & rhs);
  dex::Value eval_binop_less(const dex::Value & lhs, const dex::Value & rhs);
  dex::Value eval_binop_leq(const dex::Value & lhs, const dex::Value & rhs);
  dex::Value eval_binop_greater(const dex::Value & lhs, const dex::Value & rhs);
  dex::Value eval_binop_geq(const dex::Value & lhs, const dex::Value & rhs);

  /* Value construction */
  dex::Value newBool(bool b);
  dex::Value newInt(int n);
};

QString Renderer::process(const QList<std::shared_ptr<TemplateNode>> & nodes)
{
  result_.clear();

  for (auto n : nodes)
    process(n);

  return result_;
}

void Renderer::process(const std::shared_ptr<TemplateNode> & n)
{
  if (n->is<tnodes::Text>())
    result_ += n->as<tnodes::Text>().text;
  else if (n->is<tnodes::Object>())
    result_ += stringify(eval(std::static_pointer_cast<tnodes::Object>(n)));
  else if (n->is<tnodes::Tag>())
    exec(std::static_pointer_cast<tnodes::Tag>(n));
}

QString Renderer::stringify(const dex::Value & val)
{
  if (val.isNull() || (val.isRef() && val.getRef().isNull()))
    return QString();

  if (val.impl().isString())
    return val.impl().toString();
  else if (val.impl().isInt())
    return QString::number(val.impl().toInt());

  return context->stringify(val);
}

bool Renderer::eval_condition(const dex::Value & val)
{
  if (val.isNull())
    return false;

  if (val.impl().isBool())
    return val.impl().toBool();

  if (val.isRef())
    return !val.getRef().isNull();

  return true;
}

dex::Value Renderer::eval(const std::shared_ptr<tnodes::Object> & obj)
{
  if (obj->is<tnodes::Value>())
    return eval_value(obj->as<tnodes::Value>());
  else if (obj->is<tnodes::Variable>())
    return eval_variable(obj->as<tnodes::Variable>());
  else if (obj->is<tnodes::MemberAccess>())
    return eval_memberaccess(obj->as<tnodes::MemberAccess>());
  else if (obj->is<tnodes::ArrayAccess>())
    return eval_arrayaccess(obj->as<tnodes::ArrayAccess>());
  else if (obj->is<tnodes::BinOp>())
    return eval_binop(obj->as<tnodes::BinOp>());
  else if (obj->is<tnodes::Pipe>())
    return eval_pipe(obj->as<tnodes::Pipe>());
  else
    throw std::runtime_error{ "Not implemented" };
}

dex::Value Renderer::eval_value(const tnodes::Value & val)
{
  return val.value;
}

dex::Value Renderer::eval_variable(const tnodes::Variable & var)
{
  return context->variables().value(var.name, dex::Value{});
}

dex::Value Renderer::eval_memberaccess(const tnodes::MemberAccess & ma)
{
  dex::Value obj = eval(ma.object);

  if (obj.isList())
  {
    if (ma.name == "size")
      return newInt(obj.getList().size());
    else
      return dex::Value{};
  }

  script::Value val = obj.impl();
  if (obj.isRef())
    val = obj.getRef();

  if (val.type().isObjectType())
  {
    script::Class cla = context->engine()->getClass(val.type());
    int index = cla.attributeIndex(ma.name);
    if (index == -1)
      return dex::Value{};
    return dex::Value{ context->engine()->copy(val.getDataMember(index)), script::ParameterPolicy::Take };
  }

  return dex::Value{};
}

dex::Value Renderer::eval_arrayaccess(const tnodes::ArrayAccess & aa)
{
  dex::Value obj = eval(aa.object);
  dex::Value index = eval(aa.index);

  if (index.impl().type() == script::Type::Int)
  {
    if (obj.isList())
    {
      auto list = obj.getList();
      return list.at(index.impl().toInt());
    }
    else
    {
      return dex::Value{};
    }
  }
  else if (index.impl().type() == script::Type::String)
  {
    script::Value val = obj.impl();
    if (obj.isRef())
      val = obj.getRef();

    if (val.type().isObjectType())
    {
      script::Class cla = context->engine()->getClass(val.type());
      int pos = cla.attributeIndex(index.impl().toString().toStdString());
      if (pos == -1)
        return dex::Value{};
      return dex::Value{ context->engine()->copy(val.getDataMember(pos)), script::ParameterPolicy::Take };
    }
    else
    {
      return dex::Value{};
    }
  }
  else
  {
    throw std::runtime_error{ "Bad array access" };
  }
}

dex::Value Renderer::eval_binop(const tnodes::BinOp & binop)
{
  dex::Value lhs = eval(binop.lhs);
  dex::Value rhs = eval(binop.rhs);

  switch (binop.operation)
  {
  case tnodes::BinOp::Or:
    return newBool(eval_condition(lhs) || eval_condition(rhs));
  case tnodes::BinOp::And:
    return newBool(eval_condition(lhs) && eval_condition(rhs));
  case tnodes::BinOp::Equal:
    return eval_binop_eq(lhs, rhs);
  case tnodes::BinOp::Inequal:
    return eval_binop_neq(lhs, rhs);
  case tnodes::BinOp::Less:
    return eval_binop_less(lhs, rhs);
  case tnodes::BinOp::Leq:
    return eval_binop_leq(lhs, rhs);
  case tnodes::BinOp::Greater:
    return eval_binop_greater(lhs, rhs);
  case tnodes::BinOp::Geq:
    return eval_binop_geq(lhs, rhs);
  default:
    break;
  }

  throw std::runtime_error{ "TODO" };
}

dex::Value Renderer::eval_binop_eq(const dex::Value & lhs, const dex::Value & rhs)
{
  if (lhs.isNull() != rhs.isNull())
    return newBool(false);

  if (lhs.isNull())
    return newBool(true);

  if (lhs.typeinfo != rhs.typeinfo)
    return newBool(false);

  return newBool(lhs == rhs);
}

dex::Value Renderer::eval_binop_neq(const dex::Value & lhs, const dex::Value & rhs)
{
  if (lhs.typeinfo != rhs.typeinfo)
    return newBool(true);

  if (lhs.typeinfo == nullptr)
    return newBool(false);

  return newBool(!(lhs == rhs));
}

dex::Value Renderer::eval_binop_less(const dex::Value & lhs, const dex::Value & rhs)
{
  if (lhs.typeinfo != rhs.typeinfo || lhs.typeinfo == nullptr)
    return newBool(false);

  if (lhs.impl().isInt())
    return newBool(lhs.impl().toInt() < rhs.impl().toInt());

  return newBool(false);
}

dex::Value Renderer::eval_binop_leq(const dex::Value & lhs, const dex::Value & rhs)
{
  if (lhs.typeinfo != rhs.typeinfo || lhs.typeinfo == nullptr)
    return newBool(false);

  if (lhs.impl().isInt())
    return newBool(lhs.impl().toInt() <= rhs.impl().toInt());

  return newBool(false);
}

dex::Value Renderer::eval_binop_greater(const dex::Value & lhs, const dex::Value & rhs)
{
  if (lhs.typeinfo != rhs.typeinfo || lhs.typeinfo == nullptr)
    return newBool(false);

  if (lhs.impl().isInt())
    return newBool(lhs.impl().toInt() > rhs.impl().toInt());

  return newBool(false);
}

dex::Value Renderer::eval_binop_geq(const dex::Value & lhs, const dex::Value & rhs)
{
  if (lhs.typeinfo != rhs.typeinfo || lhs.typeinfo == nullptr)
    return newBool(false);

  if (lhs.impl().isInt())
    return newBool(lhs.impl().toInt() >= rhs.impl().toInt());

  return newBool(false);
}

dex::Value Renderer::eval_pipe(const tnodes::Pipe & pipe)
{
  auto filter_it = context->filters().find(pipe.filterName);
  if (filter_it == context->filters().end())
    throw std::runtime_error{ "TODO" };

  auto filter = filter_it.value();

  QList<dex::Value> args = pipe.arguments;
  args.prepend(eval(pipe.object));

  return filter->invoke(args, context);
}

void Renderer::exec(const std::shared_ptr<tnodes::Tag> & tag)
{
  if (tag->is<tnodes::Assign>())
    exec_assign(tag->as<tnodes::Assign>());
  else if (tag->is<tnodes::For>())
    exec_for(tag->as<tnodes::For>());
  else if (tag->is<tnodes::If>())
    exec_if(tag->as<tnodes::If>());
  else if (tag->is<tnodes::Break>())
    exec_break(tag->as<tnodes::Break>());
  else if (tag->is<tnodes::Continue>())
    exec_continue(tag->as<tnodes::Continue>());
}

void Renderer::exec(const QList<std::shared_ptr<TemplateNode>> & nodes)
{
  for (const auto & n : nodes)
  {
    process(n);

    if (context->runtimeFlags() != 0)
      return;
  }
}

void Renderer::exec_assign(const tnodes::Assign & assign)
{
  context->variables().insert(assign.variable, eval(assign.value));
}

void Renderer::exec_for(const tnodes::For & tag)
{
  dex::Value container = eval(tag.object);

  if (container.isList())
  {
    QList<dex::Value> list = container.getList();
    for (int i(0); i < list.size(); ++i)
    {
      context->variables().insert(tag.variable, list.at(i));

      exec(tag.body);

      int rflags = context->runtimeFlags();
      context->runtimeFlags() = 0;

      if (rflags & Context::Break)
        return;
    }
  }
  else
  {
    /// TODO:
  }
}

void Renderer::exec_if(const tnodes::If & tag)
{
  for (int i(0); i < tag.blocks.size(); ++i)
  {
    const auto & b = tag.blocks.at(i);

    if (eval_condition(eval(b.condition)))
    {
      exec(b.body);
      return;
    }
  }
}

void Renderer::exec_break(const tnodes::Break & tag)
{
  context->runtimeFlags() |= Context::Break;
}

void Renderer::exec_continue(const tnodes::Continue & tag)
{
  context->runtimeFlags() |= Context::Continue;
}

dex::Value Renderer::newBool(bool b)
{
  script::Value ret = context->engine()->newBool(b);
  return dex::Value{ std::move(ret) };
}

dex::Value Renderer::newInt(int n)
{
  script::Value ret = context->engine()->newInt(n);
  return dex::Value{ std::move(ret) };
}


Template::Template(const QList<std::shared_ptr<TemplateNode>> & nodes)
  : mNodes(nodes)
{

}

Template::~Template()
{

}

QString Template::render(Context *context) const
{
  Renderer r{ context };
  return r.process(nodes());
}

Template parse(const QString & str, script::Engine *engine)
{
  liquid::Parser lp{ engine };
  return Template{ lp.parse(str) };
}

} // namespace liquid

} // namespace dex
