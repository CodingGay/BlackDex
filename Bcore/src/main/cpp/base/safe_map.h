/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ART_LIBARTBASE_BASE_SAFE_MAP_H_
#define ART_LIBARTBASE_BASE_SAFE_MAP_H_

#include <map>
#include <memory>
#include <type_traits>

#include <android-base/logging.h>

namespace art_lkchan {

// Equivalent to std::map, but without operator[] and its bug-prone semantics (in particular,
// the implicit insertion of a default-constructed value on failed lookups).
template <typename K, typename V, typename Comparator = std::less<K>,
          typename Allocator = std::allocator<std::pair<const K, V>>>
class SafeMap {
 private:
  typedef SafeMap<K, V, Comparator, Allocator> Self;

 public:
  typedef typename ::std::map<K, V, Comparator, Allocator>::key_compare key_compare;
  typedef typename ::std::map<K, V, Comparator, Allocator>::value_compare value_compare;
  typedef typename ::std::map<K, V, Comparator, Allocator>::allocator_type allocator_type;
  typedef typename ::std::map<K, V, Comparator, Allocator>::iterator iterator;
  typedef typename ::std::map<K, V, Comparator, Allocator>::const_iterator const_iterator;
  typedef typename ::std::map<K, V, Comparator, Allocator>::size_type size_type;
  typedef typename ::std::map<K, V, Comparator, Allocator>::key_type key_type;
  typedef typename ::std::map<K, V, Comparator, Allocator>::value_type value_type;

  SafeMap() = default;
  SafeMap(const SafeMap&) = default;
  SafeMap(SafeMap&&) = default;
  explicit SafeMap(const key_compare& cmp, const allocator_type& allocator = allocator_type())
    : map_(cmp, allocator) {
  }

  Self& operator=(const Self& rhs) {
    map_ = rhs.map_;
    return *this;
  }

  allocator_type get_allocator() const { return map_.get_allocator(); }
  key_compare key_comp() const { return map_.key_comp(); }
  value_compare value_comp() const { return map_.value_comp(); }

  iterator begin() { return map_.begin(); }
  const_iterator begin() const { return map_.begin(); }
  iterator end() { return map_.end(); }
  const_iterator end() const { return map_.end(); }

  bool empty() const { return map_.empty(); }
  size_type size() const { return map_.size(); }

  void swap(Self& other) { map_.swap(other.map_); }
  void clear() { map_.clear(); }
  iterator erase(iterator it) { return map_.erase(it); }
  size_type erase(const K& k) { return map_.erase(k); }

  iterator find(const K& k) { return map_.find(k); }
  const_iterator find(const K& k) const { return map_.find(k); }

  iterator lower_bound(const K& k) { return map_.lower_bound(k); }
  const_iterator lower_bound(const K& k) const { return map_.lower_bound(k); }

  iterator upper_bound(const K& k) { return map_.upper_bound(k); }
  const_iterator upper_bound(const K& k) const { return map_.upper_bound(k); }

  size_type count(const K& k) const { return map_.count(k); }

  // Note that unlike std::map's operator[], this doesn't return a reference to the value.
  V Get(const K& k) const {
    const_iterator it = map_.find(k);
    DCHECK(it != map_.end());
    return it->second;
  }

  // Used to insert a new mapping.
  iterator Put(const K& k, const V& v) {
    std::pair<iterator, bool> result = map_.emplace(k, v);
    DCHECK(result.second);  // Check we didn't accidentally overwrite an existing value.
    return result.first;
  }
  iterator Put(const K& k, V&& v) {
    std::pair<iterator, bool> result = map_.emplace(k, std::move(v));
    DCHECK(result.second);  // Check we didn't accidentally overwrite an existing value.
    return result.first;
  }

  // Used to insert a new mapping at a known position for better performance.
  iterator PutBefore(const_iterator pos, const K& k, const V& v) {
    // Check that we're using the correct position and the key is not in the map.
    DCHECK(pos == map_.end() || map_.key_comp()(k, pos->first));
    DCHECK(pos == map_.begin() || map_.key_comp()((--const_iterator(pos))->first, k));
    return map_.emplace_hint(pos, k, v);
  }
  iterator PutBefore(const_iterator pos, const K& k, V&& v) {
    // Check that we're using the correct position and the key is not in the map.
    DCHECK(pos == map_.end() || map_.key_comp()(k, pos->first));
    DCHECK(pos == map_.begin() || map_.key_comp()((--const_iterator(pos))->first, k));
    return map_.emplace_hint(pos, k, std::move(v));
  }

  // Used to insert a new mapping or overwrite an existing mapping. Note that if the value type
  // of this container is a pointer, any overwritten pointer will be lost and if this container
  // was the owner, you have a leak. Returns iterator pointing to the new or overwritten entry.
  iterator Overwrite(const K& k, const V& v) {
    std::pair<iterator, bool> result = map_.insert(std::make_pair(k, v));
    if (!result.second) {
      // Already there - update the value for the existing key
      result.first->second = v;
    }
    return result.first;
  }

  template <typename CreateFn>
  V GetOrCreate(const K& k, CreateFn create) {
    static_assert(std::is_same<V, typename std::result_of<CreateFn()>::type>::value,
                  "Argument `create` should return a value of type V.");
    auto lb = lower_bound(k);
    if (lb != end() && !key_comp()(k, lb->first)) {
      return lb->second;
    }
    auto it = PutBefore(lb, k, create());
    return it->second;
  }

  iterator FindOrAdd(const K& k, const V& v) {
    iterator it = find(k);
    return it == end() ? Put(k, v) : it;
  }

  iterator FindOrAdd(const K& k) {
    iterator it = find(k);
    return it == end() ? Put(k, V()) : it;
  }

  bool Equals(const Self& rhs) const {
    return map_ == rhs.map_;
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return map_.emplace(std::forward<Args>(args)...);
  }

 private:
  ::std::map<K, V, Comparator, Allocator> map_;
};

template <typename K, typename V, typename Comparator, typename Allocator>
bool operator==(const SafeMap<K, V, Comparator, Allocator>& lhs,
                const SafeMap<K, V, Comparator, Allocator>& rhs) {
  return lhs.Equals(rhs);
}

template <typename K, typename V, typename Comparator, typename Allocator>
bool operator!=(const SafeMap<K, V, Comparator, Allocator>& lhs,
                const SafeMap<K, V, Comparator, Allocator>& rhs) {
  return !(lhs == rhs);
}

}  // namespace art_lkchan

#endif  // ART_LIBARTBASE_BASE_SAFE_MAP_H_
