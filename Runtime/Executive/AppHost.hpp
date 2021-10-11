#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Memory/PagedStorage.hpp>

#include "AppDatabase.hpp"
#include "AppDirectory.hpp"

namespace usagi
{
/**
 * \brief AppHost is where you put pieces together: the services, task graph
 * (not implemented yet), and the location for storing the data. The services
 * and systems are provided with named storage for automatic data persistence.
 *
 * \tparam EdbConfig Note: StorageT is overridden by PagedStorageFileBacked.
 */
template <
    typename Services,
    typename Systems,
    typename EdbConfig = entity::EntityDatabaseConfiguration<>
>
class AppHost : AppDirectory
{
protected:
    struct DatabaseConfig : EdbConfig
    {
        template <typename T>
        using StorageT = PagedStorageFileBacked<T>;
    };

    using EnabledComponents = typename Systems::EnabledComponents;

    template <Component... C>
    using DatabaseBaseT = EntityDatabase<DatabaseConfig, C...>;

    using WorldDatabaseT =
        typename EnabledComponents::template apply<DatabaseBaseT>;

    AppDatabase<WorldDatabaseT> mDatabaseWorld;
    Services mServices;
    Systems mSystems;

public:
    AppHost() = default;

    explicit AppHost(std::filesystem::path base_folder)
    {
        init(std::move(base_folder));
    }

    void init(std::filesystem::path base_folder)
    {
        AppDirectory::init(std::move(base_folder));
        mDatabaseWorld.init_storage(mBaseFolder / SUBFOLDER_DB_WORLD);
    }

    void update(auto &&observer)
    {
        mSystems.update(mServices, mDatabaseWorld, observer);
        mDatabaseWorld.reclaim_pages();
    }

    void update()
    {
        update([](auto &&, auto &&) { });
    }

    auto & services()
    {
        return mServices;
    }

    auto & database_world()
    {
        return mDatabaseWorld;
    }
};
}
