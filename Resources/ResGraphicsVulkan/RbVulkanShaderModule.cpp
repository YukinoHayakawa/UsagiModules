#include "RbVulkanShaderModule.hpp"

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Resources/ResGraphicsCommon/RbSpirvBytecodes.hpp>

namespace usagi
{
ResourceState RbVulkanShaderModule::construct(
    ResourceConstructDelegate<RbVulkanShaderModule> &delegate)
{
    // Get shader source code
    const auto res = delegate.resource<RbSpirvBytecodes>(
        arg<AssetPath>(),
        arg<GpuShaderStage>()
    ).await();
    const auto &bytecodes = res->bytecodes();

    // Create the object
    vk::ShaderModuleCreateInfo info;

    info.pCode = bytecodes.data();
    info.codeSize = bytecodes.size() * sizeof(std::uint32_t);

    delegate.allocate(info, bytecodes);

    return ResourceState::READY;
}
}
