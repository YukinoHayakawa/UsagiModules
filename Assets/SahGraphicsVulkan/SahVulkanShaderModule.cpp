#include "SahVulkanShaderModule.hpp"

#include <Usagi/Modules/Assets/SahGraphicsCommon/SahGlslCompiler.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanGpuDevice.hpp>

namespace usagi
{
SahVulkanShaderModule::SahVulkanShaderModule(
    VulkanGpuDevice *device,
    std::string asset_path,
    GpuShaderStage stage)
    : SingleDependencySecondaryAssetHandler(std::move(asset_path))
    , mDevice(device)
    , mStage(stage)
{
    assert(mDevice);
}

std::unique_ptr<SecondaryAsset> SahVulkanShaderModule::construct()
{
	auto &bytecodes = await_secondary<SahGlslCompiler>(asset_path(), mStage);

	vk::ShaderModuleCreateInfo info;
	info.pCode = bytecodes.data();
	info.codeSize = bytecodes.size() * sizeof(std::uint32_t);
	auto shader = mDevice->device().createShaderModuleUnique(
		info, nullptr, mDevice->dispatch());

    auto reflection_compiler = std::make_unique<spirv_cross::Compiler>(
        bytecodes);

    return std::make_unique<SaVulkanShaderModule>(
        std::move(shader),
        std::move(reflection_compiler)
    );
}

void SahVulkanShaderModule::append_features(Hasher &hasher)
{
    SingleDependencySecondaryAssetHandler::append_features(hasher);

    // todo append device trait
    hasher.append(to_string(mStage));
}
}
