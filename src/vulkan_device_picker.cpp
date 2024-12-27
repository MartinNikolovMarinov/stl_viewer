#include <vulkan_device_picker.h>

u32 getDeviceSutabilityScore(const GPUDevice& gpu) {
    const auto& props = gpu.props;
    const auto& features = gpu.features;
    u32 score = 0;

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        // Discrete GPUs are preffered.
        score += 100;
    }

    return score;
}

const GPUDevice* pickDevice(core::Memory<const GPUDevice> gpus) {
    i32 prefferedIdx = -1;
    u32 maxScore = 0;

    for (addr_size i = 0; i < gpus.len(); i++) {
        u32 currScore = getDeviceSutabilityScore(gpus[i]);
        if (currScore > maxScore) {
            prefferedIdx = i32(i);
            maxScore = currScore;
        }
    }

    if (prefferedIdx < 0) {
        return nullptr;
    }

    return &gpus[addr_size(prefferedIdx)];
}
