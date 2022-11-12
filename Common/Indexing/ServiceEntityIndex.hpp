#pragma once

#include <any>
#include <map>
#include <typeindex>
#include <Usagi/Runtime/Service/SimpleService.hpp>

namespace usagi
{
struct ServiceEntityIndex
{
    // todo: when to clear unused indices?
    std::map<std::type_index, std::any> indices;

};

using ServiceEntityIndexing = SimpleService<ServiceEntityIndex>;
}
