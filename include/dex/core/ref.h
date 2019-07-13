// Copyright (C) 2018 Vincent Chambrin
// This file is part of the Dex project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_REF_H
#define DEX_REF_H

#include <script/classtemplatenativebackend.h>
#include <script/namespace.h>
#include <script/value.h>

namespace dex
{

struct ValuePtr
{
  script::ValueImpl *value;

public:
  ValuePtr();
  ValuePtr(const script::Value& val);
  ValuePtr(const ValuePtr& other);
  ~ValuePtr();

  void reset();

  ValuePtr& operator=(const ValuePtr& other);
  ValuePtr& operator=(script::ValueImpl* ptr);

  bool operator==(nullptr_t) const;
  bool operator!=(nullptr_t) const;
};

bool operator==(const ValuePtr& lhs, const ValuePtr& rhs);
inline bool operator!=(const ValuePtr& lhs, const ValuePtr& rhs) { return !(lhs == rhs); }

void register_ref_template(script::Namespace ns);


class RefTemplate : public script::ClassTemplateNativeBackend
{
  script::Class instantiate(script::ClassTemplateInstanceBuilder& builder) override;
};

} // namespace dex

#endif // DEX_REF_H
