#pragma once

#include "util/Logger.h"

#define CHECK_NULLPTR_RET_VOID(ptr, name)               \
do                                                      \
{                                                       \
    if ((ptr) == nullptr)                               \
    {                                                   \
        LOG_FATAL("{} is nullptr.", (name));            \
        return;                                         \
    }                                                   \
} while(0)


#define CHECK_NULLPTR_RET_BOOL(ptr, name)               \
do                                                      \
{                                                       \
    if ((ptr) == nullptr)                               \
    {                                                   \
        LOG_FATAL("{} is nullptr.", (name));            \
        return false;                                   \
    }                                                   \
} while(0)

#define REQUIRE_NOT_NULLPTR(ptr, name)                  \
do                                                      \
{                                                       \
    if ((ptr) == nullptr)                               \
    {                                                   \
        LOG_FATAL("{} is unexpectedly null", (name));   \
        std::abort();                                   \
    }                                                   \
} while(0)                                              \


