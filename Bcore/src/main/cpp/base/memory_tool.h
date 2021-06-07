/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_LIBARTBASE_BASE_MEMORY_TOOL_H_
#define ART_LIBARTBASE_BASE_MEMORY_TOOL_H_

#include <stddef.h>

#if !defined(__has_feature)
#define __has_feature(x) 0
#endif

#if __has_feature(address_sanitizer)

#include <sanitizer/asan_interface.h>
#define ADDRESS_SANITIZER

#ifdef ART_ENABLE_ADDRESS_SANITIZER
#define MEMORY_TOOL_MAKE_NOACCESS(p, s) __asan_poison_memory_region(p, s)
#define MEMORY_TOOL_MAKE_UNDEFINED(p, s) __asan_unpoison_memory_region(p, s)
#define MEMORY_TOOL_MAKE_DEFINED(p, s) __asan_unpoison_memory_region(p, s)
constexpr bool kMemoryToolIsAvailable = true;
#else
#define MEMORY_TOOL_MAKE_NOACCESS(p, s) do { (void)(p); (void)(s); } while (0)
#define MEMORY_TOOL_MAKE_UNDEFINED(p, s) do { (void)(p); (void)(s); } while (0)
#define MEMORY_TOOL_MAKE_DEFINED(p, s) do { (void)(p); (void)(s); } while (0)
constexpr bool kMemoryToolIsAvailable = false;
#endif

extern "C" void __asan_handle_no_return();

#define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#define MEMORY_TOOL_HANDLE_NO_RETURN __asan_handle_no_return()
#define RUNNING_ON_MEMORY_TOOL 1U
constexpr bool kMemoryToolIsValgrind = false;
constexpr bool kMemoryToolDetectsLeaks = true;
constexpr bool kMemoryToolAddsRedzones = true;
constexpr size_t kMemoryToolStackGuardSizeScale = 2;

#else

/*
 * chensenhua
#include <memcheck/memcheck.h>
#include <valgrind.h>
#define MEMORY_TOOL_MAKE_NOACCESS(p, s) VALGRIND_MAKE_MEM_NOACCESS(p, s)
#define MEMORY_TOOL_MAKE_UNDEFINED(p, s) VALGRIND_MAKE_MEM_UNDEFINED(p, s)
#define MEMORY_TOOL_MAKE_DEFINED(p, s) VALGRIND_MAKE_MEM_DEFINED(p, s)
#define ATTRIBUTE_NO_SANITIZE_ADDRESS
#define MEMORY_TOOL_HANDLE_NO_RETURN do { } while (0)
#define RUNNING_ON_MEMORY_TOOL RUNNING_ON_VALGRIND
constexpr bool kMemoryToolIsAvailable = true;
constexpr bool kMemoryToolIsValgrind = true;
constexpr bool kMemoryToolDetectsLeaks = true;
constexpr bool kMemoryToolAddsRedzones = true;
constexpr size_t kMemoryToolStackGuardSizeScale = 1;
*/

#endif

#endif  // ART_LIBARTBASE_BASE_MEMORY_TOOL_H_
