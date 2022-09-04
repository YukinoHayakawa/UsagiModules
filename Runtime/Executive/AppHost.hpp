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
    // the database config will be overriden to use a file-backed storage
    // todo don't force it
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
    Services mServices;
    Systems mSystems;
    // Declare the database at the end so it always get saved before destroying
    // services, etc.
    // todo: database should never be left in an inconsistent state - e.g. program crash
    AppDatabase<typename DatabaseTraits::WorldDatabaseT> mDatabaseWorld;

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

    template <typename T>
    auto & service()
    {
        return static_cast<T &>(mServices).get_service();
    }

    auto & database_world()
    {
        return mDatabaseWorld;
    }

    // todo refactor
    auto create_heap(const std::string_view name) const
    {
        const auto heap_folder = mBaseFolder / SUBFOLDER_NAMED_HEAPS;
        auto path = heap_folder / name;
        USAGI_ASSERT_THROW(
            path.lexically_relative(heap_folder) == name,
            std::runtime_error("Escaping data folder.")
        );
        path.replace_extension(".dat");

        VmAllocatorFileBacked allocator;
        allocator.set_backing_file(std::move(path));

        return allocator;
    }
};
}
