// Copyright (C) 2019 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/core/serialization.h"

#include "dex/dex.h"
#include "dex/core/list.h"
#include "dex/core/ref.h"
#include "dex/core/value.h"

#include <script/class.h>
#include <script/datamember.h>
#include <script/engine.h>
#include <script/namelookup.h>
#include <script/namespace.h>
#include <script/object.h>
#include <script/overloadresolution.h>

#include <script/functionbuilder.h>
#include <script/functiontemplate.h>
#include <script/interpreter/executioncontext.h>
#include <script/symbol.h>
#include <script/templateargumentdeduction.h>
#include <script/templatebuilder.h>
#include <script/typesystem.h>

namespace script
{

namespace serialization_callbacks
{

static script::Value encode(script::FunctionCall* c)
{
  return c->engine()->construct<json::Json>(dex::serialization::serialize(c->arg(0)));
}

static script::Value decode(script::FunctionCall* c)
{
  const json::Json& data = get<json::Json>(c->arg(0));
  const script::Type t = c->callee().returnType();
  return dex::serialization::deserialize(data, t);
}

static script::Value candecode(script::FunctionCall* c)
{
  const json::Json& data = get<json::Json>(c->arg(0));
  const script::Type t = c->callee().arguments().front().type;
  bool result = data["__type"].toInt() == t.baseType().data();
  return c->engine()->newBool(result);
}

} // serialization_callbacks

class EncodeTemplate : public script::FunctionTemplateNativeBackend
{
  void deduce(script::TemplateArgumentDeduction& deduc, const std::vector<script::TemplateArgument>& targs, const std::vector<script::Type>& itypes) override
  {
    deduc.record_deduction(0, TemplateArgument(itypes.front()));
  }

  void substitute(script::FunctionBuilder& builder, const std::vector<script::TemplateArgument>& targs) override
  {
    builder.returns(script::Type::Json);
    builder.params(script::Type::cref(targs.front().type));
  }

  std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> instantiate(script::Function& function) override
  {
    return { serialization_callbacks::encode, nullptr };
  }
};

class DecodeTemplate : public script::FunctionTemplateNativeBackend
{
  void deduce(script::TemplateArgumentDeduction& deduc, const std::vector<script::TemplateArgument>& targs, const std::vector<script::Type>& itypes) override
  {
    if (targs.size() != 1)
      return deduc.fail();
  }

  void substitute(script::FunctionBuilder& builder, const std::vector<script::TemplateArgument>& targs) override
  {
    builder.returns(targs.front().type);
    builder.params(script::make_type<const json::Json&>());
  }

  std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> instantiate(script::Function& function) override
  {
    return { serialization_callbacks::decode, nullptr };
  }
};

class CanDecodeTemplate : public script::FunctionTemplateNativeBackend
{
  void deduce(script::TemplateArgumentDeduction& deduc, const std::vector<script::TemplateArgument>& targs, const std::vector<script::Type>& itypes) override
  {
    if (targs.size() != 1)
      return deduc.fail();
  }

  void substitute(script::FunctionBuilder & builder, const std::vector<script::TemplateArgument> & targs) override
  {
    builder.returns(script::Type::Boolean);
    builder.params(script::make_type<const json::Json&>());
  }

  std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> instantiate(script::Function & function) override
  {
    return { serialization_callbacks::candecode, nullptr };
  }
};

} // namespace script

namespace dex
{

namespace serialization
{

static int serialize_object(const script::Object& obj, const script::Class& c, json::Json& result)
{
  script::Class base_class = c.parent();

  int attr_offset = base_class.isNull() ? 0 : serialize_object(obj, base_class, result);

  for (size_t i(0); i < c.dataMembers().size(); ++i)
  {
    json::Json dm = serialize(obj.at(attr_offset + i));
    const std::string& dmname = c.dataMembers().at(i).name;

    result[QString::fromStdString(dmname)] = dm;
  }

  return attr_offset + c.dataMembers().size();
}

static json::Array serialize_list(const script::Value& val)
{
  const QList<dex::Value>& list = script::get<QList<dex::Value>>(val);

  json::Array elems;

  for (const auto& e : list)
  {
    elems.push(serialize(e.impl()));
  }

  return elems;
}

static void serialize_ref(const script::Value& val, json::Json& result)
{
  const dex::ValuePtr& ptr = script::get<dex::ValuePtr>(val);

  if (ptr.value == nullptr)
  {
    result = nullptr;
  }
  else
  {
    result = serialize(script::Value(ptr.value));
  }
}

json::Json serialize_object(const script::Value& val)
{

  script::Engine* e = val.engine();
  script::Class cla = e->typeSystem()->getClass(val.type());

  if (cla.isTemplateInstance() && cla.instanceOf() == script::ClassTemplate::get<dex::ListTemplate>(e))
  {
    return serialize_list(val);
  }

  json::Object result;

  result["__type"] = cla.id();

  if (cla.isTemplateInstance() && cla.instanceOf() == script::ClassTemplate::get<dex::RefTemplate>(e))
  {
    serialize_ref(val, result);
  }
  else
  {
    script::Object obj = val.toObject();
    serialize_object(obj, obj.instanceOf(), result);
  }

  return result;
}

json::Json serialize(const script::Value& val)
{
  if (val.type() == script::Type::Void)
    return nullptr;
  else if (val.type() == script::Type::Boolean)
    return val.toBool();
  else if (val.type() == script::Type::Int)
    return val.toInt();
  else if (val.type() == script::Type::Float)
    return val.toFloat();
  else if (val.type() == script::Type::Double)
    return val.toDouble();
  else if (val.type() == script::Type::String)
    return val.toString();
  else if (val.type() == script::make_type<json::Array>())
    return script::get<json::Array>(val);
  else if (val.type() == script::make_type<json::Object>())
    return script::get<json::Object>(val);
  else if (val.type() == script::make_type<json::Json>())
    return script::get<json::Json>(val);

  /// TODO: serialize enum

  return serialize_object(val);
}

static bool perform_assignment(script::Value& lhs, const script::Value& rhs)
{
  script::Engine* e = Dex::scriptEngine();
  auto lookup = script::NameLookup::resolve(script::AssignmentOperator, lhs.type(), rhs.type(), script::Scope(e->rootNamespace()));

  auto resol = script::OverloadResolution::New(e);

  if (!resol.process(lookup, { lhs, rhs }))
  {
    /// TODO: maybe throw ?
    return false;
  }

  resol.selectedOverload().invoke({ lhs, rhs });

  return true;
}

static script::Value deserialize_object(const json::Json& json, script::Type type)
{
  script::Engine* e = Dex::scriptEngine();
  script::Value ret = e->construct(type, {});
  script::Object obj = ret.toObject();
  const script::Class cla = obj.instanceOf();

  for (const auto& member : json.toObject().data())
  {
    if (member.first.startsWith("__"))
      continue;

    int attr_index = cla.attributeIndex(member.first.toStdString());

    if (attr_index != -1)
    {
      script::Value attr = obj.at(attr_index);
      script::Value newval = deserialize(member.second, attr.type());
      perform_assignment(attr, newval);
      e->destroy(newval);
    }
    else
    {
      /// TODO: do something ?
    }
  }

  return ret;
}

static script::Value deserialize_ref(const json::Json& json, script::Class c)
{
  script::Engine* e = Dex::scriptEngine();

  if (json.isNull())
  {
    return e->construct(script::Type(c.id()), {});
  }

  script::Type T = c.arguments().front().type;
  script::Value ret = e->construct(script::Type(c.id()), {});
  dex::ValuePtr& ptr = script::get<dex::ValuePtr>(ret);
  ptr = deserialize(json, T);
  return ret;
}

static script::Type deduceListType(const json::Array& vec);

static script::Type deduceType(const json::Json& data)
{
  script::Engine* e = Dex::scriptEngine();

  if (data.isNull())
  {
    return script::Type::Null;
  }
  else if (data.isBoolean())
  {
    return script::Type::Boolean;
  }
  else if (data.isInteger())
  {
    return script::Type::Int;
  }
  else if (data.isNumber())
  {
    return script::Type::Double;
  }
  else if (data.isString())
  {
    return script::Type::String;
  }
  else if (data.isObject())
  {
    if (data["__type"] != nullptr)
    {
      return script::Type(data["__type"].toInt());
    }

    throw DeserializationError(data, script::Type::Auto);
  }
  else if (data.isArray())
  {
    return deduceListType(data.toArray());
  }

  throw DeserializationError(data, script::Type::Auto);
}

static script::Type commonType(const json::Json& data, const script::Type& T)
{
  script::Engine* e = Dex::scriptEngine();

  const script::Type U = deduceType(data);

  if (T == U)
  {
    return T;
  }

  script::ClassTemplate ref =  script::ClassTemplate::get<dex::RefTemplate>(e);
  script::Type result;

  auto get_common_type = [&result, &ref, e](const script::Type & V, const script::Type & W) -> bool 
  {
    if (W == script::Type::Null)
      return false;

    if (V == script::Type::Null)
    {
      /* Check if W is already a Ref<WW> */

      if (W.isObjectType())
      {
        script::Class Wcla = e->typeSystem()->getClass(W);

        if (Wcla.isTemplateInstance() && Wcla.instanceOf() == ref)
        {
          result = W;
          return true;
        }
      }

      /* Common type is Ref<W> */

      script::Class refW = ref.getInstance({ script::TemplateArgument(W) });
      result = refW.id();
      return true;
    }
    else
    {
      /* Check if W = Ref<V> */

      if (!W.isObjectType())
        return false;

      script::Class Wcla = e->typeSystem()->getClass(W);

      if (!Wcla.isTemplateInstance() || Wcla.instanceOf() != ref)
        return false;

      if (Wcla.arguments().front().type == V)
      {
        result = W;
        return true;
      }

      return false;
    }
  };

  if (get_common_type(T, U) || get_common_type(U, T))
    return result;

  throw DeserializationError(data, T);
}

script::Type deduceListType(const json::Array& vec)
{
  if (vec.length() == 0)
  {
    throw DeserializationError(vec, script::Type::Auto);
  }

  script::Type T = deduceType(vec.at(0));

  for (int i(1); i < vec.length(); ++i)
  {
    T = commonType(vec.at(i), T);
  }
 
  if (T == script::Type::Auto)
  {
    throw DeserializationError(vec, script::Type::Auto);
  }

  return T;
}

static script::Value deserialize_list(const json::Array& vec, script::Class c)
{
  script::Engine* e = Dex::scriptEngine();

  script::Type T = c.arguments().front().type;
  script::Value ret = e->construct(script::Type(c.id()), {});
  QList<dex::Value>& list = script::get<QList<dex::Value>>(ret);

  for (int i(0); i < vec.length(); ++i)
  {
    script::Value elem = deserialize(vec.at(i), T);
    list.push_back(dex::Value(elem, script::ParameterPolicy::Take));
  }

  return ret;
}

script::Value deserialize(const json::Json& json, script::Type type)
{
  script::Engine* e = Dex::scriptEngine();
  
  if (json.isBoolean()
    && (type.baseType() == script::Type::Boolean || type == script::Type::Auto))
  {
    return e->newBool(json.toBool());
  }
  else if (json.isInteger()
    && (type.baseType() == script::Type::Int || type == script::Type::Auto))
  {
    return e->newInt(json.toInt());
  }
  else if (json.isString()
    && (type.baseType() == script::Type::String || type == script::Type::Auto))
  {
    return e->newString(json.toString());

    /// TODO: add conversion from space and eol nodes ?
  }
  else if (type.baseType() == script::make_type<json::Json>())
  {
    return e->construct<json::Json>(json);
  }
  else if (type.baseType() == script::make_type<json::Array>())
  {
    if (json.isArray())
      return e->construct<json::Array>(json.toArray());
  }
  else if (type.baseType() == script::make_type<json::Object>())
  {
    if (json.isObject())
      return e->construct<json::Object>(json.toObject());
  }
  else if ((type == script::Type::Auto || type.isObjectType()) && json.isObject())
  {
    if (type == script::Type::Auto)
    {
      type = script::Type(json["__type"].toInt());
    }

    script::Class cla = e->typeSystem()->getClass(type);

    if (cla.isTemplateInstance() && cla.instanceOf() == script::ClassTemplate::get<dex::RefTemplate>(e))
    {
      return deserialize_ref(json, cla);
    }
    else
    {
      return deserialize_object(json, type);
    }
  }
  else if ((type == script::Type::Auto || type.isObjectType()) && json.isArray())
  {
    if (type == script::Type::Auto)
    {
      type = deduceListType(json.toArray());
    }

    script::Class cla = e->typeSystem()->getClass(type);

    if (cla.isTemplateInstance() && cla.instanceOf() == script::ClassTemplate::get<dex::ListTemplate>(e))
    {
      return deserialize_list(json.toArray(), cla);
    }
  }

  throw DeserializationError(json, type);
}


void expose(script::Namespace& ns)
{
  script::Namespace s = ns.newNamespace("serialization");

  script::Symbol(s).newFunctionTemplate("encode")
    .withBackend<script::EncodeTemplate>()
    .params(script::TemplateParameter(script::TemplateParameter::TypeParameter(), "T"))
    .create();

  script::Symbol(s).newFunctionTemplate("decode")
    .withBackend<script::DecodeTemplate>()
    .params(script::TemplateParameter(script::TemplateParameter::TypeParameter(), "T"))
    .create();

  script::Symbol(s).newFunctionTemplate("canDecode")
    .withBackend<script::CanDecodeTemplate>()
    .params(script::TemplateParameter(script::TemplateParameter::TypeParameter(), "T"))
    .create();
}

} // namespace serialization

} // namespace dex
