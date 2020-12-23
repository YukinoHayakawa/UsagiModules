#pragma once

#include <sstream>

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Memory/PagedStorage.hpp>

namespace usagi
{
class ResumableAppBase
{
protected:
    std::filesystem::path mBaseFolder;

    static constexpr auto EXT_META = ".yaml";
    static constexpr auto EXT_STORAGE = ".dat";
    static constexpr auto FILENAME_ENTITY_DATABASE = "database";
    static constexpr auto PATH_COMPONENTS = "components";

    std::filesystem::path path_edb() const
    {
        auto path = mBaseFolder / FILENAME_ENTITY_DATABASE;
        path.replace_extension(EXT_STORAGE);
        return path;
    }

    void init(std::filesystem::path base_folder)
    {
        mBaseFolder = std::move(base_folder);

        create_directories(mBaseFolder);
        create_directories(mBaseFolder / PATH_COMPONENTS);
    }

    ResumableAppBase() = default;

    explicit ResumableAppBase(std::filesystem::path base_folder)
    {
        init(std::move(base_folder));
    }
};

template <Component... EnabledComponents>
class ResumableApp
    : ResumableAppBase
    , EntityDatabaseFileBacked<EnabledComponents...>
{
public:
    using DatabaseT = EntityDatabaseFileBacked<EnabledComponents...>;

protected:
    using StorageT = typename DatabaseT::EntityPageStorageT;

    // entity database
    constexpr static std::uint16_t MAGIC_CHECK = 0xDB01;

    void init_entity_page_storage()
    {
        const bool exists = DatabaseT::entity_pages().init(path_edb());
        StorageT::template push_header<MAGIC_CHECK>(DatabaseT::mMeta, exists);
    }

    template <Component C>
    void init_component_storage()
    {
        if constexpr (!TagComponent<C>)
        {
            std::stringstream name;
            name << std::setfill('0') << std::setw(16) << typeid(C).hash_code();
            // todo: file name should follow certain format
            auto path = mBaseFolder / PATH_COMPONENTS / name.str();
            path.replace_extension(EXT_STORAGE);
            DatabaseT::template component_storage<C>().init(std::move(path));
        }
    }

public:
    ResumableApp() = default;

    explicit ResumableApp(std::filesystem::path base_folder)
    {
        init(std::move(base_folder));
    }

    ~ResumableApp()
    {
        StorageT::template pop_header<MAGIC_CHECK>(DatabaseT::mMeta, true);
    }

    void init(std::filesystem::path base_folder)
    {
        ResumableAppBase::init(std::move(base_folder));
        init_entity_page_storage();
        (..., init_component_storage<EnabledComponents>());
    }

    DatabaseT & database() { return *this; }
};
}
