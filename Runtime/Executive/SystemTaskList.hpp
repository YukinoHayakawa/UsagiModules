﻿#pragma once

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

    template <System S, std::size_t I>
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
        auto &sys = std::get<I>(systems);
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

    template <std::size_t... I>
    void update_with_index(
        auto &&rt,
        auto &&db,
        auto &&observer,
        std::index_sequence<I...>)
    {
        (..., update_system<Sys, I>(
            std::forward<decltype(rt)>(rt),
            std::forward<decltype(db)>(db),
            std::forward<decltype(observer)>(observer)
        ));
    }

    template <std::size_t... I>
    void update(
        auto &&rt,
        auto &&db,
        auto &&observer) // todo remove
    {
        update_with_index(
               std::forward<decltype(rt)>(rt),
            std::forward<decltype(db)>(db),
            std::forward<decltype(observer)>(observer),
            std::index_sequence_for<Sys...>()
        );
    }

    using EnabledComponents = SystemComponentUsage<Sys...>;
};
}
