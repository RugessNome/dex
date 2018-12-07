
namespace script
{

const Engine::list_template_t Engine::ListTemplate = Engine::list_template_t{};

ClassTemplate Engine::getTemplate(list_template_t) const
{
  return d->list_template_;
}


const Engine::ref_template_t Engine::RefTemplate = Engine::ref_template_t{};

ClassTemplate Engine::getTemplate(ref_template_t) const
{
  return d->ref_template_;
}


} // namespace script