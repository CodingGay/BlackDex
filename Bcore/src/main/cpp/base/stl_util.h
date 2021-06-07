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

#ifndef ART_LIBARTBASE_BASE_STL_UTIL_H_
#define ART_LIBARTBASE_BASE_STL_UTIL_H_

#include <algorithm>
#include <set>
#include <sstream>

#include <android-base/logging.h>

namespace art_lkchan {

// STLDeleteContainerPointers()
//  For a range within a container of pointers, calls delete
//  (non-array version) on these pointers.
// NOTE: for these three functions, we could just implement a DeleteObject
// functor and then call for_each() on the range and functor, but this
// requires us to pull in all of algorithm.h, which seems expensive.
// For hash_[multi]set, it is important that this deletes behind the iterator
// because the hash_set may call the hash function on the iterator when it is
// advanced, which could result in the hash function trying to deference a
// stale pointer.
template <class ForwardIterator>
void STLDeleteContainerPointers(ForwardIterator begin,
                                ForwardIterator end) {
  while (begin != end) {
    ForwardIterator temp = begin;
    ++begin;
    delete *temp;
  }
}

// STLDeleteElements() deletes all the elements in an STL container and clears
// the container.  This function is suitable for use with a vector, set,
// hash_set, or any other STL container which defines sensible begin(), end(),
// and clear() methods.
//
// If container is null, this function is a no-op.
//
// As an alternative to calling STLDeleteElements() directly, consider
// using a container of std::unique_ptr, which ensures that your container's
// elements are deleted when the container goes out of scope.
template <class T>
void STLDeleteElements(T *container) {
  if (container != nullptr) {
    STLDeleteContainerPointers(container->begin(), container->end());
    container->clear();
  }
}

// Given an STL container consisting of (key, value) pairs, STLDeleteValues
// deletes all the "value" components and clears the container.  Does nothing
// in the case it's given a null pointer.
template <class T>
void STLDeleteValues(T *v) {
  if (v != nullptr) {
    for (typename T::iterator i = v->begin(); i != v->end(); ++i) {
      delete i->second;
    }
    v->clear();
  }
}

// Deleter using free() for use with std::unique_ptr<>. See also UniqueCPtr<> below.
struct FreeDelete {
  // NOTE: Deleting a const object is valid but free() takes a non-const pointer.
  void operator()(const void* ptr) const {
    free(const_cast<void*>(ptr));
  }
};

// Alias for std::unique_ptr<> that uses the C function free() to delete objects.
template <typename T>
using UniqueCPtr = std::unique_ptr<T, FreeDelete>;

// Find index of the first element with the specified value known to be in the container.
template <typename Container, typename T>
size_t IndexOfElement(const Container& container, const T& value) {
  auto it = std::find(container.begin(), container.end(), value);
  DCHECK(it != container.end());  // Must exist.
  return std::distance(container.begin(), it);
}

// Remove the first element with the specified value known to be in the container.
template <typename Container, typename T>
void RemoveElement(Container& container, const T& value) {
  auto it = std::find(container.begin(), container.end(), value);
  DCHECK(it != container.end());  // Must exist.
  container.erase(it);
}

// Replace the first element with the specified old_value known to be in the container.
template <typename Container, typename T>
void ReplaceElement(Container& container, const T& old_value, const T& new_value) {
  auto it = std::find(container.begin(), container.end(), old_value);
  DCHECK(it != container.end());  // Must exist.
  *it = new_value;
}

// Search for an element with the specified value and return true if it was found, false otherwise.
template <typename Container, typename T>
bool ContainsElement(const Container& container, const T& value, size_t start_pos = 0u) {
  DCHECK_LE(start_pos, container.size());
  auto start = container.begin();
  std::advance(start, start_pos);
  auto it = std::find(start, container.end(), value);
  return it != container.end();
}

// 32-bit FNV-1a hash function suitable for std::unordered_map.
// It can be used with any container which works with range-based for loop.
// See http://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
template <typename Vector>
struct FNVHash {
  size_t operator()(const Vector& vector) const {
    uint32_t hash = 2166136261u;
    for (const auto& value : vector) {
      hash = (hash ^ value) * 16777619u;
    }
    return hash;
  }
};

// Merge `other` entries into `to_update`.
template <typename T>
static inline void MergeSets(std::set<T>& to_update, const std::set<T>& other) {
  to_update.insert(other.begin(), other.end());
}

// Returns a copy of the passed vector that doesn't memory-own its entries.
template <typename T>
static inline std::vector<T*> MakeNonOwningPointerVector(const std::vector<std::unique_ptr<T>>& src) {
  std::vector<T*> result;
  result.reserve(src.size());
  for (const std::unique_ptr<T>& t : src) {
    result.push_back(t.get());
  }
  return result;
}

}  // namespace art_lkchan

#endif  // ART_LIBARTBASE_BASE_STL_UTIL_H_
