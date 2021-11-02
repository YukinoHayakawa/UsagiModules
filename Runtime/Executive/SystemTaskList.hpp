#pragma once

#include <tuple>

#include <Usagi/Entity/System.hpp>
#include <Usagi/Entity/detail/ComponentAccessAllowAll.hpp>
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
        // If the System declares WriteAccess = AllComponents, pass
        // ComponentAccessAllowAll instead of reading SystemAttribute.
        // This allows scripting system to invoke the script without
        // having to expose the definition of itself to the scripts,
        // which complicates the JIT compilation.
        using AccessT = std::conditional_t<
            SystemDeclaresWriteAllAccess<S>,
            ComponentAccessAllowAll,
            ComponentAccessSystemAttribute<S>
        >;
        auto access = db.template create_access<AccessT>();
        auto &sys = std::get<S>(systems);
        if constexpr(std::is_same_v<void, decltype(sys.update(rt, access))>)
        {
            sys.update(rt, access);
            observer(sys, nullptr);
        }
        else
        {
            auto ret = sys.update(rt, access);
            observer(sys, std::move(ret));
        }
    }

    void update(auto &&rt, auto &&db, auto &&observer)
    {
        (..., update_system<Sys>(
            std::forward<decltype(rt)>(rt),
            std::forward<decltype(db)>(db),
            std::forward<decltype(observer)>(observer)
        ));
    }

    using EnabledComponents = SystemComponentUsage<Sys...>;
};
}
