#pragma once

#include "volk.h"
#include "vma/vk_mem_alloc.h"
#include "VkBootstrap.h"
#include "vulkan/vk_enum_string_helper.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // force to vulkans 0,1 range for depth
#include "glm/glm.hpp"

#include "Exception.h"

#include <memory>
#include <optional>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <functional>
#include <limits>

#define _USE_MATH_DEFINES
#include <math.h>

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct UniformStruct {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 inv_view;
    alignas(16) glm::mat4 virtual_view;
    alignas(16) glm::mat4 proj;
    alignas(8) glm::vec2 near_far_plane;
    alignas(8) glm::vec2 sun_direction;
    alignas(16) glm::vec4 sun_color;
};

// std430
struct PushConstants {
    alignas(8) VkDeviceAddress vertex_buffer;
    alignas(16) glm::mat4 model;
};

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

struct SharingInfo {
    VkSharingMode mode;
    std::vector<uint32_t> queue_families;
};

inline constexpr SharingInfo sharing_exlusive() {
    return { VK_SHARING_MODE_EXCLUSIVE, {} };
};

glm::vec2 dir_to_spherical(const glm::vec3& dir);

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            throw EngineException(std::string(string_VkResult(err)), __FILE__, __LINE__); \
        }                                                               \
    } while (0)

#define VK_CHECK_T(x, text)                                          \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            throw EngineException(std::string(string_VkResult(err)) + ";\n" + text, __FILE__, __LINE__); \
        }                                                               \
    } while (0)

#define VK_CHECK_E(x, Exception)                             \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            throw Exception(std::string(string_VkResult(err)), __FILE__, __LINE__); \
        }                                                               \
    } while (0)

#define VK_CHECK_ET(x, Exception, text)                                \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            throw Exception(std::string(string_VkResult(err)) + ";\n" + text, __FILE__, __LINE__); \
        }                                                               \
    } while (0)

#define VK_DESTROY(var, dest, ...)      \
    do {                                \
        if (var) {                      \
            dest(__VA_ARGS__,nullptr);  \
            var = nullptr;                 \
        }                               \
    } while (0)

#define VK_DESTROY_FROM(var, dest, ...)      \
    do {                                \
        if (var) {                      \
            dest(__VA_ARGS__);  \
            var = nullptr;                 \
        }                               \
    } while (0)