#pragma once

#include "vulkan/vulkan.h"
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


#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

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