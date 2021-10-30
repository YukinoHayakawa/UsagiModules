#pragma once

#include "AppDatabase.hpp"
#include "AppDirectory.hpp"
#include "DatabaseTraits.hpp"

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
    typename AdditionalComponents = C<>,
    typename EdbConfig = entity::EntityDatabaseConfiguration<>
>
class AppHost
    : AppDirectory
    , public DatabaseTraits<
        typename Systems::EnabledComponents,
        AdditionalComponents,
        EdbConfig
    >
{
public:
    using DatabaseTraits = DatabaseTraits<
        typename Systems::EnabledComponents,
        AdditionalComponents,
        EdbConfig
    >;

protected:
    AppDatabase<typename DatabaseTraits::WorldDatabaseT> mDatabaseWorld;
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
