#include "RbVulkanShaderModule.hpp"

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Resources/ResGraphicsCommon/RbSpirvBytecodes.hpp>

namespace usagi
{
ResourceState RbVulkanShaderModule::construct(
    ResourceConstructDelegate<ProductT> &delegate,
    const AssetPath &path,
    GpuShaderStage stage)
{
    connect(*delegate.heap<VulkanDeviceAccess *>());

    // Get shader byte code
    const auto res = delegate.resource<RbSpirvBytecodes>(
        path,
        stage
    ).await();

    // Create the object
    vk::ShaderModuleCreateInfo info;

    info.pCode = res->data();
    info.codeSize = res->size() * sizeof(std::uint32_t);

    delegate.emplace(create(info), *res);

    return ResourceState::READY;
}
}
