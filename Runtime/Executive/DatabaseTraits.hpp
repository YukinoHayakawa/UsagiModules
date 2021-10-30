#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Memory/PagedStorage.hpp>

namespace usagi
{
template <
    typename SystemComponents,
    typename AdditionalComponents = C<>,
    typename EdbConfig = entity::EntityDatabaseConfiguration<>
>
struct DatabaseTraits
{
    // Overrides the db config to use file-backed storage.
    struct DatabaseConfig : EdbConfig
    {
        template <typename T>
        using StorageT = PagedStorageFileBacked<T>;
    };

    using EnabledComponents = FilterDeduplicatedT<FilterConcatenatedT<
        SystemComponents, AdditionalComponents
    >>;

    template <Component... C>
    using DatabaseBaseT = EntityDatabase<DatabaseConfig, C...>;

    using WorldDatabaseT =
        typename EnabledComponents::template apply<DatabaseBaseT>;
};
}
