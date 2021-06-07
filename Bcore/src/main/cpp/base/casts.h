/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef ART_LIBARTBASE_BASE_CASTS_H_
#define ART_LIBARTBASE_BASE_CASTS_H_

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <limits>
#include <type_traits>

#include <android-base/logging.h>

#include "stl_util_identity.h"

namespace art_lkchan {

// Use implicit_cast as a safe version of static_cast or const_cast
// for upcasting in the type hierarchy (i.e. casting a pointer to Foo
// to a pointer to SuperclassOfFoo or casting a pointer to Foo to
// a const pointer to Foo).
// When you use implicit_cast, the compiler checks that the cast is safe.
// Such explicit implicit_casts are necessary in surprisingly many
// situations where C++ demands an exact type match instead of an
// argument type convertible to a target type.
//
// The From type can be inferred, so the preferred syntax for using
// implicit_cast is the same as for static_cast etc.:
//
//   implicit_cast<ToType>(expr)
//
// implicit_cast would have been part of the C++ standard library,
// but the proposal was submitted too late.  It will probably make
// its way into the language in the future.
template<typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}

// When you upcast (that is, cast a pointer from type Foo to type
// SuperclassOfFoo), it's fine to use implicit_cast<>, since upcasts
// always succeed.  When you downcast (that is, cast a pointer from
// type Foo to type SubclassOfFoo), static_cast<> isn't safe, because
// how do you know the pointer is really of type SubclassOfFoo?  It
// could be a bare Foo, or of type DifferentSubclassOfFoo.  Thus,
// when you downcast, you should use this macro.  In debug mode, we
// use dynamic_cast<> to double-check the downcast is legal (we die
// if it's not).  In normal mode, we do the efficient static_cast<>
// instead.  Thus, it's important to test in debug mode to make sure
// the cast is legal!
//    This is the only place in the code we should use dynamic_cast<>.
// In particular, you SHOULDN'T be using dynamic_cast<> in order to
// do RTTI (eg code like this:
//    if (dynamic_cast<Subclass1>(foo)) HandleASubclass1Object(foo);
//    if (dynamic_cast<Subclass2>(foo)) HandleASubclass2Object(foo);
// You should design the code some other way not to need this.

template<typename To, typename From>     // use like this: down_cast<T*>(foo);
inline To down_cast(From* f) {                   // so we only accept pointers
  static_assert(std::is_base_of<From, typename std::remove_pointer<To>::type>::value,
                "down_cast unsafe as To is not a subtype of From");

  return static_cast<To>(f);
}

template<typename To, typename From>     // use like this: down_cast<T&>(foo);
inline To down_cast(From& f) {           // so we only accept references
  static_assert(std::is_base_of<From, typename std::remove_reference<To>::type>::value,
                "down_cast unsafe as To is not a subtype of From");

  return static_cast<To>(f);
}

template <class Dest, class Source>
inline Dest bit_cast(const Source& source) {
  // Compile time assertion: sizeof(Dest) == sizeof(Source)
  // A compile error here means your Dest and Source have different sizes.
  static_assert(sizeof(Dest) == sizeof(Source), "sizes should be equal");
  Dest dest;
  memcpy(&dest, &source, sizeof(dest));
  return dest;
}

// A version of static_cast that DCHECKs that the value can be precisely represented
// when converting to Dest.
template <typename Dest, typename Source>
constexpr Dest dchecked_integral_cast(Source source) {
  DCHECK(
      // Check that the value is within the lower limit of Dest.
      (static_cast<intmax_t>(std::numeric_limits<Dest>::min()) <=
          static_cast<intmax_t>(std::numeric_limits<Source>::min()) ||
          source >= static_cast<Source>(std::numeric_limits<Dest>::min())) &&
      // Check that the value is within the upper limit of Dest.
      (static_cast<uintmax_t>(std::numeric_limits<Dest>::max()) >=
          static_cast<uintmax_t>(std::numeric_limits<Source>::max()) ||
          source <= static_cast<Source>(std::numeric_limits<Dest>::max())))
      << "dchecked_integral_cast failed for " << source
      << " (would be " << static_cast<Dest>(source) << ")";

  return static_cast<Dest>(source);
}

// A version of dchecked_integral_cast casting between an integral type and an enum type.
// When casting to an enum type, the cast does not check if the value corresponds to an enumerator.
// When casting from an enum type, the target type can be omitted and the enum's underlying type
// shall be used.

template <typename Dest, typename Source>
constexpr
typename std::enable_if<!std::is_enum<Source>::value, Dest>::type
enum_cast(Source value) {
  return static_cast<Dest>(
      dchecked_integral_cast<typename std::underlying_type<Dest>::type>(value));
}

template <typename Dest = void, typename Source>
constexpr
typename std::enable_if<std::is_enum<Source>::value,
                        typename std::conditional<std::is_same<Dest, void>::value,
                                                  std::underlying_type<Source>,
                                                  Identity<Dest>>::type>::type::type
enum_cast(Source value) {
  using return_type = typename std::conditional<std::is_same<Dest, void>::value,
                                                std::underlying_type<Source>,
                                                Identity<Dest>>::type::type;
  return dchecked_integral_cast<return_type>(
      static_cast<typename std::underlying_type<Source>::type>(value));
}

// A version of reinterpret_cast<>() between pointers and int64_t/uint64_t
// that goes through uintptr_t to avoid treating the pointer as "signed."

template <typename Dest, typename Source>
inline Dest reinterpret_cast64(Source source) {
  // This is the overload for casting from int64_t/uint64_t to a pointer.
  static_assert(std::is_same<Source, int64_t>::value || std::is_same<Source, uint64_t>::value,
                "Source must be int64_t or uint64_t.");
  static_assert(std::is_pointer<Dest>::value, "Dest must be a pointer.");
  // Check that we don't lose any non-0 bits here.
  DCHECK_EQ(static_cast<Source>(static_cast<uintptr_t>(source)), source);
  return reinterpret_cast<Dest>(static_cast<uintptr_t>(source));
}

template <typename Dest, typename Source>
inline Dest reinterpret_cast64(Source* ptr) {
  // This is the overload for casting from a pointer to int64_t/uint64_t.
  static_assert(std::is_same<Dest, int64_t>::value || std::is_same<Dest, uint64_t>::value,
                "Dest must be int64_t or uint64_t.");
  static_assert(sizeof(uintptr_t) <= sizeof(Dest), "Expecting at most 64-bit pointers.");
  return static_cast<Dest>(reinterpret_cast<uintptr_t>(ptr));
}

}  // namespace art_lkchan

#endif  // ART_LIBARTBASE_BASE_CASTS_H_
