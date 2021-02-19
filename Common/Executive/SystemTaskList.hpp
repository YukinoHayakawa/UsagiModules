#pragma once

#include <tuple>

#include <Usagi/Entity/System.hpp>
#include <Usagi/Entity/detail/ComponentAccessSystemAttribute.hpp>

namespace usagi
{
template <System... Sys>
struct SystemTaskList
{
    std::tuple<Sys...> systems;

    template <System S>
    void update_system(auto &&rt, auto &&db, auto &&observer)
    {
        auto access = db.template create_access<
            ComponentAccessSystemAttribute<S>
        >();
        auto &sys = std::get<S>(systems);
        if constexpr(std::is_same_v<void, decltype(sys.update(rt, access))>)
        {
            sys.update(rt, access);
            observer(sys, nullptr);
        }
        else
        {
            auto ret = sys.update(rt, access);
            observer(sys, ret);
        }
    }

    void update(auto &&rt, auto &&db, auto &&observer)
    {
        (..., update_system<Sys>(rt, db, observer));
    }

    using EnabledComponents = SystemComponentUsage<Sys...>;
};
}
