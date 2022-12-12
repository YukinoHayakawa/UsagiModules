#include "RbInputTaskGraph.hpp"

#include <nlohmann/json.hpp>

#include <Usagi/Modules/Resources/ResJson/JsonSchemaHelper.hpp>
#include <Usagi/Modules/Resources/ResJson/RbCascadingJsonConfig.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>

#include "RbStandardTaskGraph.hpp"

namespace usagi
{
ResourceState RbInputTaskGraph::construct(
    ResourceConstructDelegate<ProductT> &delegate,
    const AssetPath &path)
{
    using p = nlohmann::json::json_pointer;

    const auto asset = delegate.resource<RbCascadingJsonConfig>(path).await();
    const auto &config = *asset;

    // currently only stg is supported
    enum TaskGraphLoader
    {
        STG
    } loader;

    FIELD("/input_loader", loader, TaskGraphLoader)
    MATCH("stg", STG)
    END_FIELD()

    constexpr auto path_input_name = "/input_name";
    const auto &input_name = config[p(path_input_name)];
    CHECK_TYPE(input_name, path_input_name, string)

    // load input graph using the specified loader. todo: support more types?
    delegate.emplace(
        delegate.resource<RbStandardTaskGraph>(
            input_name.get<std::string>(),
            0
        ).await()
    );

    return ResourceState::READY;
}
}
