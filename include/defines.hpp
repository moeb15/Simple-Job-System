#pragma once

#include <stdint.h>
#include <cfloat>
#include <cmath>

#define ArraySize(arr) (sizeof(arr)/sizeof(arr[0]))

#ifndef JS_ASSERT
#include <assert.h>
#define JS_ASSERT(x) assert(x)
#endif

#ifndef DISABLE_COPY
#define DISABLE_COPY(className) className(const className&) = delete; className& operator=(const className&) = delete;
#endif

#ifndef DISABLE_MOVE
#define DISABLE_MOVE(className) className(className&& obj) = delete; className& operator=(className&&) = delete;
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define JS_PLATFORM_WINDOWS
#endif

#if _DEBUG
#ifndef DEBUG_OP
#define DEBUG_OP(x) x
#endif
#else
#ifndef DEBUG_OP
#define DEBUG_OP(x)
#endif
#endif

typedef uint8_t                 u8;
typedef uint16_t                u16;
typedef uint32_t                u32;
typedef uint64_t                u64;

typedef int8_t                  i8;
typedef int16_t                 i16;
typedef int32_t                 i32;
typedef int64_t                 i64;

typedef float                   f32;
typedef double                  f64;

typedef const char* cstring;

static const u64                U64_MAX = UINT64_MAX;
static const i64                I64_MAX = INT64_MAX;
static const u32                U32_MAX = UINT32_MAX;
static const i32                I32_MAX = INT32_MAX;
static const u16                U16_MAX = UINT16_MAX;
static const i16                I16_MAX = INT16_MAX;
static const u8                  U8_MAX = UINT8_MAX;
static const i8                  I8_MAX = INT8_MAX;