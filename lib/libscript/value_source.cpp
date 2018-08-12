

namespace script
{

ValueImpl::Data::Data()
{
  ptr = 0;
}

ValueImpl::Data::~Data()
{
  ptr = 0;
}

ValueImpl::BuiltIn::BuiltIn()
  : ref(nullptr)
{

}

ValueImpl::ValueImpl(const ValueImpl & other)
 : ref(other.ref), type(other.type), engine(other.engine)
{
  assert(type.isNull() || type == Type::Void);
}

ValueImpl::~ValueImpl()
{
  clear();
}

void ValueImpl::set_ref(ValueImpl *val)
{
  check_builtin_storage();

  if (val == this)
    throw std::runtime_error{ "Invalid ref" };
  
  if (val)
    val->ref += 1;

  if (data.builtin.ref != nullptr)
  {
    data.builtin.ref->ref -= 1;
    if (data.builtin.ref->ref == 0)
      this->engine->destroy(Value{ data.builtin.ref });
  }

  data.builtin.ref = val;
}


bool ValueImpl::is_object() const
{
  if(!type.testFlag(Type::BuiltInStorageFlag))
	  return false;
  return !data.builtin.object.isNull();
}

const Object & ValueImpl::get_object() const
{
  if (data.builtin.object.isNull())
    throw std::runtime_error{ "Null object" };
  return data.builtin.object;
}

void ValueImpl::init_object()
{
  check_builtin_storage();

  if (!data.builtin.object.isNull())
    return;

  auto impl = std::make_shared<ObjectImpl>(this->engine->getClass(this->type));
  data.builtin.object = Object{ impl };
}


bool ValueImpl::is_array() const
{
  if(!type.testFlag(Type::BuiltInStorageFlag))
	  return false;
  return !data.builtin.array.isNull();
}

const Array & ValueImpl::get_array() const
{
  return data.builtin.array;
}

void ValueImpl::set_array(const Array & aval)
{
  check_builtin_storage();
  
  data.builtin.array = aval;
}


bool ValueImpl::is_function() const
{
  if(!type.testFlag(Type::BuiltInStorageFlag))
	  return false;
  return !data.builtin.function.isNull();
}

const Function & ValueImpl::get_function() const
{
  return data.builtin.function;
}

void ValueImpl::set_function(const Function & fval)
{
  check_builtin_storage();
  
  data.builtin.function = fval;
}

bool ValueImpl::is_lambda() const
{
  if(!type.testFlag(Type::BuiltInStorageFlag))
	  return false;
  return !data.builtin.lambda.isNull();
}

const Lambda & ValueImpl::get_lambda() const
{
  return data.builtin.lambda;
}

void ValueImpl::set_lambda(const Lambda & lval)
{
  check_builtin_storage();
  
  data.builtin.lambda = lval;
}

const Enumerator & ValueImpl::get_enumerator() const
{
  return data.builtin.enumerator;
}

void ValueImpl::set_enumerator(const Enumerator & evval)
{
  check_builtin_storage();
  
  data.builtin.enumerator = evval;
}

bool ValueImpl::is_initializer_list() const
{
  if (!type.testFlag(Type::BuiltInStorageFlag))
    return false;

  return data.builtin.initializer_list.begin() != nullptr;
}

InitializerList ValueImpl::get_initializer_list() const
{
  return data.builtin.initializer_list;
}

void ValueImpl::set_initializer_list(const InitializerList & il)
{
  if (!type.testFlag(Type::BuiltInStorageFlag))
  {
    new (&data.builtin) BuiltIn;
    type = type.withFlag(Type::BuiltInStorageFlag);
  }

  data.builtin.initializer_list = il;
}

void ValueImpl::remove_uninitialized_flag()
{
  type = type.withoutFlag(Type::UninitializedFlag);
}

void ValueImpl::check_builtin_storage()
{
  if (type.testFlag(Type::BuiltInStorageFlag))
    return;

  new (&data.builtin) BuiltIn;
  type = type.withFlag(Type::BuiltInStorageFlag);
}

void* Value::memory() const
{
  return &(d->data.memory);
}

void* Value::getPtr() const
{
  return d->data.ptr;
}

void Value::setPtr(void* val)
{
  d->data.ptr = val;
}

} // namespace script