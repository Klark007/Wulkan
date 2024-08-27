#pragma once

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