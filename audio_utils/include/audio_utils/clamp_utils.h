/*
 * Copyright 2024 The Android Open Source Project
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

#pragma once

#include "template_utils.h"

#ifdef __cplusplus

#include <algorithm>
#include <optional>
#include <type_traits>
#include <vector>

namespace android::audio_utils {

/**
 * Utility functions for clamping values of different types within a specified
 * range of [min, max]. Supported types include primitive types, structs, and
 * vectors.
 *
 * - For **primitive types**, `std::clamp` is used directly, std::string
 *   comparison and clamp is performed lexicographically.
 * - For **structures**, each member is clamped individually and reassembled
 *   after clamping.
 * - For **vectors**, the `min` and `max` ranges (if defined) may have either
 *   one element or match the size of the target vector. If `min`/`max` have
 *   only one element, each target vector element is clamped within that range.
 *   If `min`/`max` match the target's size, each target element is clamped
 *   within the corresponding `min`/`max` elements.
 *
 * The maximum number of members supported in a structure is `kMaxStructMember
 * as defined in the template_utils.h header.
 */

/**
 * @brief Clamp function for aggregate types (structs).
 */
template <typename T>
  requires std::is_class_v<T> && std::is_aggregate_v<T>
std::optional<T> clampInRange(const T& target, const T& min, const T& max);

/**
 * @brief Clamp function for primitive types, including integers, floating-point
 * types, enums, and `std::string`.
 */
template <typename T>
  requires PrimitiveType<T>
std::optional<T> clampInRange(const T& target, const T& min, const T& max) {
  if (min > max) {
    return std::nullopt;
  }
  return std::clamp(target, min, max);
}

/**
 * @brief Clamp function for vectors.
 *
 * Clamping each vector element within a specified range. The `min` and `max`
 * vectors may have either one element or the same number of elements as the
 * `target` vector.
 *
 * - If `min` or `max` contain only one element, each element in `target` is
 *   clamped by this single value.
 * - If `min` or `max` match `target` in size, each element in `target` is
 *   clamped by the corresponding elements in `min` and `max`.
 * - If size of `min` or `max` vector is neither 1 nor same size as `target`,
 *   the range will be considered as invalid, and `std::nullopt` will be
 *   returned.
 *
 * Some examples:
 * std::vector<int> target({3, 0, 5, 2});
 * std::vector<int> min({1});
 * std::vector<int> max({3});
 * clampInRange(target, min, max) result will be std::vector({3, 1, 3, 2})
 *
 * std::vector<int> target({3, 0, 5, 2});
 * std::vector<int> min({1, 2, 3, 4});
 * std::vector<int> max({3, 4, 5, 6});
 * clampInRange(target, min, max) result will be std::vector({3, 2, 5, 4})
 *
 * std::vector<int> target({3, 0, 5, 2});
 * std::vector<int> min({});
 * std::vector<int> max({3, 4});
 * clampInRange(target, min, max) result will be std::nullopt
 */
template <typename T>
  requires is_specialization_v<T, std::vector>
std::optional<T> clampInRange(const T& target, const T& min, const T& max) {
  using ElemType = typename T::value_type;

  const size_t min_size = min.size(), max_size = max.size(),
               target_size = target.size();
  if (min_size == 0 || max_size == 0 || target_size == 0) {
    return std::nullopt;
  }

  T result;
  result.reserve(target_size);

  if (min_size == 1 && max_size == 1) {
    const ElemType clamp_min = min[0], clamp_max = max[0];
    for (size_t i = 0; i < target_size; ++i) {
      auto clamped_elem = clampInRange(target[i], clamp_min, clamp_max);
      if (clamped_elem) {
        result.emplace_back(*clamped_elem);
      } else {
        return std::nullopt;
      }
    }
  } else if (min_size == target_size && max_size == target_size) {
    for (size_t i = 0; i < target_size; ++i) {
      auto clamped_elem = clampInRange(target[i], min[i], max[i]);
      if (clamped_elem) {
        result.emplace_back(*clamped_elem);
      } else {
        return std::nullopt;
      }
    }
  } else if (min_size == 1 && max_size == target_size) {
    const ElemType clamp_min = min[0];
    for (size_t i = 0; i < target_size; ++i) {
      auto clamped_elem = clampInRange(target[i], clamp_min, max[i]);
      if (clamped_elem) {
        result.emplace_back(*clamped_elem);
      } else {
        return std::nullopt;
      }
    }
  } else if (min_size == target_size && max_size == 1) {
    const ElemType clamp_max = max[0];
    for (size_t i = 0; i < target_size; ++i) {
      auto clamped_elem = clampInRange(target[i], min[i], clamp_max);
      if (clamped_elem) {
        result.emplace_back(*clamped_elem);
      } else {
        return std::nullopt;
      }
    }
  } else {
    // incompatible size
    return std::nullopt;
  }

  return result;
}

/**
 * @brief Clamp function for class and aggregate type (structs).
 *
 * Uses `opAggregate` with clampOp to perform clamping on each member of the
 * aggregate type `T`, the max number of supported members in `T` is
 * `kMaxStructMember`.
 */
template <typename T>
  requires std::is_class_v<T> && std::is_aggregate_v<T>
std::optional<T> clampInRange(const T& target, const T& min, const T& max) {
  const auto clampOp = [](const auto& a, const auto& b, const auto& c) {
    return clampInRange(a, b, c);
  };

  return opAggregate(clampOp, target, min, max);
}

}  // namespace android::audio_utils

#endif  // __cplusplus