/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <audio_utils/elementwise_op.h>
#include <gtest/gtest.h>
#include <log/log.h>

#include <string>

using android::audio_utils::elementwise_clamp;
using android::audio_utils::kMaxStructMember;
using android::audio_utils::op_tuple_elements;

enum class ClampTestEnum { E1, E2, E3 };

struct ClampTestSSS {
  double a;
  bool b;
};

struct ClampTestSS {
  ClampTestSSS sss;
  int c;
  std::vector<float> d;
  ClampTestEnum e;
};

struct ClampTestS {
  ClampTestSS ss;
  int f;
  bool g;
  std::string h;
};

std::ostream& operator<<(std::ostream& os, const ClampTestEnum& e) {
  switch (e) {
    case ClampTestEnum::E1: {
      os << "E1";
      break;
    }
    case ClampTestEnum::E2: {
      os << "E2";
      break;
    }
    case ClampTestEnum::E3: {
      os << "E3";
      break;
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const ClampTestSSS& sss) {
  os << "a: " << sss.a << ", b: " << sss.b;
  return os;
}

std::ostream& operator<<(std::ostream& os, const ClampTestSS& ss) {
  os << ss.sss << ", c: " << ss.c << ", d: [";
  for (const auto& itor : ss.d) {
    os << itor << " ";
  }
  os << "], e: " << ss.e;
  return os;
}

std::ostream& operator<<(std::ostream& os, const ClampTestS& s) {
  os << s.ss << ", f: " << s.f << ", g: " << s.g << ", h" << s.h;
  return os;
}

constexpr bool operator==(const ClampTestSSS& lhs, const ClampTestSSS& rhs) {
  return lhs.a == rhs.a && lhs.b == rhs.b;
}

constexpr bool operator==(const ClampTestSS& lhs, const ClampTestSS& rhs) {
  return lhs.sss == rhs.sss && lhs.c == rhs.c && lhs.d == rhs.d &&
         lhs.e == rhs.e;
}

constexpr bool operator==(const ClampTestS& lhs, const ClampTestS& rhs) {
  return lhs.ss == rhs.ss && lhs.f == rhs.f && lhs.g == rhs.g && lhs.h == rhs.h;
}

const ClampTestSSS sss1{.a = 1, .b = false};
const ClampTestSSS sss2{.a = sss1.a + 1, .b = true};
const ClampTestSSS sss3{.a = sss2.a + 1, .b = true};
const ClampTestSSS sss_mixed{.a = sss1.a - 1, .b = true};
const ClampTestSSS sss_clamped_1_3{.a = sss1.a, .b = true};
const ClampTestSSS sss_clamped_2_3{.a = sss2.a, .b = true};

const ClampTestSS ss1{.sss = sss1, .c = 1, .d = {1.f}, .e = ClampTestEnum::E1};
const ClampTestSS ss2{
    .sss = sss2, .c = ss1.c + 1, .d = {ss1.d[0] + 1}, .e = ClampTestEnum::E2};
const ClampTestSS ss3{
    .sss = sss3, .c = ss2.c + 1, .d = {ss2.d[0] + 1}, .e = ClampTestEnum::E3};
const ClampTestSS ss_mixed{.sss = sss_mixed,
                           .c = ss1.c - 1,
                           .d = {ss3.d[0] + 1},
                           .e = ClampTestEnum::E3};
const ClampTestSS ss_clamped_1_3{.sss = sss_clamped_1_3,
                                 .c = ss1.c,
                                 .d = {ss3.d[0]},
                                 .e = ClampTestEnum::E3};
const ClampTestSS ss_clamped_2_3{.sss = sss_clamped_2_3,
                                 .c = ss2.c,
                                 .d = {ss3.d[0]},
                                 .e = ClampTestEnum::E3};

const ClampTestS s1{.ss = ss1, .f = 1, .g = false, .h = "s1"};
const ClampTestS s2{.ss = ss2, .f = s1.f + 1, .g = false, .h = "s2"};
const ClampTestS s3{.ss = ss3, .f = s2.f + 1, .g = true, .h = "s3"};
const ClampTestS s_mixed{
    .ss = ss_mixed, .f = s1.f - 1, .g = true, .h = "mixed"};
const ClampTestS s_clamped_1_3{
    .ss = ss_clamped_1_3, .f = s1.f, .g = true, .h = "s1"};
const ClampTestS s_clamped_2_3{
    .ss = ss_clamped_2_3, .f = s2.f, .g = true, .h = "s2"};

// clamp a structure with range of min == max
TEST(ClampOpTest, elementwise_clamp) {
  std::optional<ClampTestS> clamped;

  clamped = elementwise_clamp(s2, s1, s3);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s2);

  clamped = elementwise_clamp(s1, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s2);

  clamped = elementwise_clamp(s3, s1, s2);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s2);
}

// clamp a structure with range of min == max
TEST(ClampOpTest, clamp_same_min_max) {
  std::optional<ClampTestS> clamped;

  clamped = elementwise_clamp(s1, s1, s1);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s1);

  clamped = elementwise_clamp(s2, s1, s1);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s1);

  clamped = elementwise_clamp(s3, s1, s1);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s1);

  clamped = elementwise_clamp(s1, s2, s2);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s2);

  clamped = elementwise_clamp(s2, s2, s2);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s2);

  clamped = elementwise_clamp(s3, s2, s2);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s2);

  clamped = elementwise_clamp(s1, s3, s3);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s3);

  clamped = elementwise_clamp(s2, s3, s3);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s3);

  clamped = elementwise_clamp(s3, s3, s3);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s3);
}

// clamp a structure with invalid range (min > max)
TEST(ClampOpTest, clamp_invalid_range) {
  EXPECT_EQ(std::nullopt, elementwise_clamp(s1, s2, s1));
  EXPECT_EQ(std::nullopt, elementwise_clamp(s2, s3, s2));
  EXPECT_EQ(std::nullopt, elementwise_clamp(s3, s3, s1));
}

// all members in p3 clamped to s2 but p3.ss.sss.a
TEST(ClampOpTest, clamp_to_max_a) {
  ClampTestS p3 = s3;
  std::optional<ClampTestS> clamped;

  p3.ss.sss.a = s1.ss.sss.a;
  clamped = elementwise_clamp(p3, s1, s2);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p3.ss.sss.a is not clamped
  EXPECT_EQ(clamped->ss.sss.a, s1.ss.sss.a);
  // ensure all other members correctly clamped to max
  clamped->ss.sss.a = s2.ss.sss.a;
  EXPECT_EQ(*clamped, s2);
}

// all members in p3 clamped to s2 but p3.ss.sss.b
TEST(ClampOpTest, clamp_to_max_b) {
  ClampTestS p3 = s3;
  std::optional<ClampTestS> clamped;

  p3.ss.sss.b = s1.ss.sss.b;
  clamped = elementwise_clamp(p3, s1, s2);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p3.ss.sss.b is not clamped
  EXPECT_EQ(clamped->ss.sss.b, s1.ss.sss.b);
  // ensure all other members correctly clamped to max
  clamped->ss.sss.b = s2.ss.sss.b;
  EXPECT_EQ(*clamped, s2);
}

// all members in p3 clamped to s2 but p3.ss.c
TEST(ClampOpTest, clamp_to_max_c) {
  ClampTestS p3 = s3;
  std::optional<ClampTestS> clamped;

  p3.ss.c = s1.ss.c;
  clamped = elementwise_clamp(p3, s1, s2);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(p3.ss.c, s1.ss.c);
  // ensure p3.ss.c is not clamped
  EXPECT_EQ(clamped->ss.c, s1.ss.c);
  // ensure all other members correctly clamped to max
  clamped->ss.c = s2.ss.c;
  EXPECT_EQ(*clamped, s2);
}

// all members in p3 clamped to s2 but p3.ss.d
TEST(ClampOpTest, clamp_to_max_d) {
  ClampTestS p3 = s3;
  std::optional<ClampTestS> clamped;

  p3.ss.d = s1.ss.d;
  clamped = elementwise_clamp(p3, s1, s2);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p3.ss.d is not clamped
  EXPECT_EQ(clamped->ss.d, s1.ss.d);
  // ensure all other members correctly clamped to max
  clamped->ss.d = s2.ss.d;
  EXPECT_EQ(*clamped, s2);
}

// all members in p3 clamped to s2 but p3.ss.e
TEST(ClampOpTest, clamp_to_max_e) {
  ClampTestS p3 = s3;
  std::optional<ClampTestS> clamped;

  p3.ss.e = s1.ss.e;
  clamped = elementwise_clamp(p3, s1, s2);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p3.ss.e is not clamped
  EXPECT_EQ(clamped->ss.e, s1.ss.e);
  // ensure all other members correctly clamped to max
  clamped->ss.e = s2.ss.e;
  EXPECT_EQ(*clamped, s2);
}

// all members in p3 clamped to s2 but p3.f
TEST(ClampOpTest, clamp_to_max_f) {
  ClampTestS p3 = s3;
  std::optional<ClampTestS> clamped;

  p3.f = s1.f;
  clamped = elementwise_clamp(p3, s1, s2);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p3.f is not clamped
  EXPECT_EQ(clamped->f, s1.f);
  // ensure all other members correctly clamped to max
  clamped->f = s2.f;
  EXPECT_EQ(*clamped, s2);
}

// all members in p3 clamped to s2 but p3.g
TEST(ClampOpTest, clamp_to_max_g) {
  ClampTestS p3 = s3;
  std::optional<ClampTestS> clamped;

  p3.g = s1.g;
  clamped = elementwise_clamp(p3, s1, s2);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p3.g is not clamped
  EXPECT_EQ(clamped->g, s1.g);
  // ensure all other members correctly clamped to max
  clamped->g = s2.g;
  EXPECT_EQ(*clamped, s2);
}

// all members in p3 clamped to s2 but p3.h
TEST(ClampOpTest, clamp_to_max_h) {
  ClampTestS p3 = s3;
  std::optional<ClampTestS> clamped;

  p3.h = s1.h;
  clamped = elementwise_clamp(p3, s1, s2);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p3.g is not clamped
  EXPECT_EQ(clamped->h, s1.h);
  // ensure all other members correctly clamped to max
  clamped->h = s2.h;
  EXPECT_EQ(*clamped, s2);
}

// all members in p1 clamped to s2 except p1.ss.sss.a
TEST(ClampOpTest, clamp_to_min_a) {
  ClampTestS p1 = s1;
  p1.ss.sss.a = s3.ss.sss.a;
  std::optional<ClampTestS> clamped = elementwise_clamp(p1, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p1.ss.sss.a is not clamped
  EXPECT_EQ(clamped->ss.sss.a, s3.ss.sss.a);
  // ensure all other members correctly clamped to max
  clamped->ss.sss.a = s2.ss.sss.a;
  EXPECT_EQ(*clamped, s2);
}

// all members in p1 clamped to s2 but p1.ss.sss.b
TEST(ClampOpTest, clamp_to_min_b) {
  ClampTestS p1 = s1;
  p1.ss.sss.b = s3.ss.sss.b;
  std::optional<ClampTestS> clamped = elementwise_clamp(p1, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p1.ss.sss.b is not clamped
  EXPECT_EQ(clamped->ss.sss.b, s3.ss.sss.b);
  // ensure all other members correctly clamped to max
  clamped->ss.sss.b = s2.ss.sss.b;
  EXPECT_EQ(*clamped, s2);
}

TEST(ClampOpTest, clamp_to_min_c) {
  ClampTestS p1 = s1;
  p1.ss.c = s3.ss.c;
  std::optional<ClampTestS> clamped = elementwise_clamp(p1, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(p1.ss.c, s3.ss.c);
  // ensure p1.ss.c is not clamped
  EXPECT_EQ(clamped->ss.c, s3.ss.c);
  // ensure all other members correctly clamped to max
  clamped->ss.c = s2.ss.c;
  EXPECT_EQ(*clamped, s2);
}

TEST(ClampOpTest, clamp_to_min_d) {
  ClampTestS p1 = s1;
  p1.ss.d = s3.ss.d;
  std::optional<ClampTestS> clamped = elementwise_clamp(p1, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p1.ss.d is not clamped
  EXPECT_EQ(clamped->ss.d, s3.ss.d);
  // ensure all other members correctly clamped to max
  clamped->ss.d = s2.ss.d;
  EXPECT_EQ(*clamped, s2);
}

TEST(ClampOpTest, clamp_to_min_e) {
  ClampTestS p1 = s1;
  p1.ss.e = s3.ss.e;
  std::optional<ClampTestS> clamped = elementwise_clamp(p1, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p1.ss.e is not clamped
  EXPECT_EQ(clamped->ss.e, s3.ss.e);
  // ensure all other members correctly clamped to max
  clamped->ss.e = s2.ss.e;
  EXPECT_EQ(*clamped, s2);
}

TEST(ClampOpTest, clamp_to_min_f) {
  ClampTestS p1 = s1;
  p1.f = s3.f;
  std::optional<ClampTestS> clamped = elementwise_clamp(p1, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p1.f is not clamped
  EXPECT_EQ(clamped->f, s3.f);
  // ensure all other members correctly clamped to max
  clamped->f = s2.f;
  EXPECT_EQ(*clamped, s2);
}

TEST(ClampOpTest, clamp_to_min_g) {
  ClampTestS p1 = s1;
  p1.g = s3.g;
  std::optional<ClampTestS> clamped = elementwise_clamp(p1, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p1.g is not clamped
  EXPECT_EQ(clamped->g, s3.g);
  // ensure all other members correctly clamped to max
  clamped->g = s2.g;
  EXPECT_EQ(*clamped, s2);
}

TEST(ClampOpTest, clamp_to_min_h) {
  ClampTestS p1 = s1;
  p1.h = s3.h;
  std::optional<ClampTestS> clamped = elementwise_clamp(p1, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  // ensure p1.g is not clamped
  EXPECT_EQ(clamped->h, s3.h);
  // ensure all other members correctly clamped to max
  clamped->h = s2.h;
  EXPECT_EQ(*clamped, s2);
}

// test vector clamp with same size target and min/max
TEST(ClampOpTest, clamp_vector_same_size) {
  std::optional<ClampTestS> clamped;
  ClampTestS target = s2, min = s1, max = s3;

  min.ss.d = {1, 11, 21};
  max.ss.d = {10, 20, 30};
  target.ss.d = {0, 30, 21};
  std::vector<float> expect = {1, 20, 21};
  clamped = elementwise_clamp(target, min, max);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(clamped->ss.d, expect);

  min.ss.d = {10, 11, 1};
  max.ss.d = {10, 20, 30};
  target.ss.d = {20, 20, 20};
  expect = {10, 20, 20};
  clamped = elementwise_clamp(target, min, max);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(clamped->ss.d, expect);

  clamped = elementwise_clamp(target, min, min);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, min);

  clamped = elementwise_clamp(target, max, max);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, max);
}

// test vector clamp with one element min and max
TEST(ClampOpTest, clamp_vector_one_member_min_max) {
  std::optional<ClampTestS> clamped;
  ClampTestS target = s2, min = s1, max = s3;

  min.ss.d = {10};
  max.ss.d = {20};
  target.ss.d = {0, 30, 20};
  std::vector<float> expect = {10, 20, 20};

  clamped = elementwise_clamp(target, min, max);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(clamped->ss.d, expect);
}

TEST(ClampOpTest, clamp_vector_one_min) {
  std::optional<ClampTestS> clamped;
  ClampTestS target = s2, min = s1, max = s3;

  min.ss.d = {0};
  max.ss.d = {20, 10, 30};
  target.ss.d = {-1, 30, 20};
  std::vector<float> expect = {0, 10, 20};

  clamped = elementwise_clamp(target, min, max);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(clamped->ss.d, expect);
}

TEST(ClampOpTest, clamp_vector_one_max) {
  std::optional<ClampTestS> clamped;
  ClampTestS target = s2, min = s1, max = s3;

  min.ss.d = {0, 10, 20};
  max.ss.d = {20};
  target.ss.d = {-1, 30, 20};
  std::vector<float> expect = {0, 20, 20};

  clamped = elementwise_clamp(target, min, max);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(clamped->ss.d, expect);
}

TEST(ClampOpTest, clamp_vector_invalid_range) {
  std::optional<ClampTestS> clamped;
  ClampTestS target = s2, min = s1, max = s3;

  target.ss.d = {-1, 30, 20};
  std::vector<float> expect = {0, 20, 20};

  min.ss.d = {0, 10};
  max.ss.d = {20};
  clamped = elementwise_clamp(target, min, max);
  EXPECT_EQ(clamped, std::nullopt);

  min.ss.d = {0, 10, 20};
  max.ss.d = {};
  clamped = elementwise_clamp(target, min, max);
  EXPECT_EQ(clamped, std::nullopt);

  min.ss.d = {};
  max.ss.d = {0, 10, 20};
  clamped = elementwise_clamp(target, min, max);
  EXPECT_EQ(clamped, std::nullopt);

  min.ss.d = {0, 10, 20};
  max.ss.d = {0, 10, 10};
  clamped = elementwise_clamp(target, min, max);
  EXPECT_EQ(clamped, std::nullopt);

  min.ss.d = {0, 10, 5, 10};
  max.ss.d = {0, 10, 10};
  clamped = elementwise_clamp(target, min, max);
  EXPECT_EQ(clamped, std::nullopt);

  min.ss.d = {};
  max.ss.d = {};
  target.ss.d = {};
  clamped = elementwise_clamp(target, min, max);
  EXPECT_EQ(clamped, std::nullopt);
}

TEST(ClampOpTest, clamp_string) {
  std::optional<ClampTestS> clamped;
  ClampTestS target = s2, min = s1, max = s3;

  min.h = "";
  max.h = "";
  target.h = "";
  clamped = elementwise_clamp(target, min, max);
  EXPECT_EQ(*clamped, target);

  min.h = "apple";
  max.h = "pear";
  target.h = "orange";
  clamped = elementwise_clamp(target, min, max);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(clamped->h, std::clamp(target.h, min.h, max.h));
  EXPECT_EQ(*clamped, target);

  target.h = "aardvark";
  clamped = elementwise_clamp(target, min, max);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(clamped->h, std::clamp(target.h, min.h, max.h));
  target.h = clamped->h;
  EXPECT_EQ(*clamped, target);

  target.h = "zebra";
  clamped = elementwise_clamp(target, min, max);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(clamped->h, std::clamp(target.h, min.h, max.h));
  target.h = clamped->h;
  EXPECT_EQ(*clamped, target);
}

// clamp a mixed structure in range
TEST(ClampOpTest, clamp_mixed) {
  std::optional<ClampTestS> clamped;

  clamped = elementwise_clamp(s_mixed, s1, s3);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s_clamped_1_3);

  clamped = elementwise_clamp(s_mixed, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s_clamped_2_3);
}

// clamp a mixed structure in range
TEST(ClampOpTest, clamp_primitive_type) {
  std::optional<ClampTestS> clamped;

  clamped = elementwise_clamp(s_mixed, s1, s3);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s_clamped_1_3);

  clamped = elementwise_clamp(s_mixed, s2, s3);
  ASSERT_NE(clamped, std::nullopt);
  EXPECT_EQ(*clamped, s_clamped_2_3);
}

// Template function to return an array of size N
template <size_t N>
auto getArrayN() {
  return std::array<int, N>{};
}

// Recursive function to make a tuple of arrays up to size N
template <std::size_t N>
auto makeTupleOfArrays() {
  if constexpr (N == 1) {
    return std::make_tuple(getArrayN<1>());
  } else {
    return std::tuple_cat(makeTupleOfArrays<N - 1>(),
                          std::make_tuple(getArrayN<N>()));
  }
}

// test the clamp utility can handle structures with up to
// `android::audio_utils::kMaxStructMember` members
TEST(ClampOpTest, clamp_different_struct_members) {
  auto clampVerifyOp = [](auto&& arr) {
    auto m1(arr), m2(arr), m3(arr);
    m1.fill(1);
    m2.fill(2);
    m3.fill(3);

    auto clamped = elementwise_clamp(m2, m1, m3);
    ASSERT_NE(clamped, std::nullopt);
    EXPECT_EQ(*clamped, m2);

    clamped = elementwise_clamp(m1, m2, m3);
    ASSERT_NE(clamped, std::nullopt);
    EXPECT_EQ(*clamped, m2);

    clamped = elementwise_clamp(m3, m1, m2);
    ASSERT_NE(clamped, std::nullopt);
    EXPECT_EQ(*clamped, m2);

    // invalid range
    EXPECT_EQ(elementwise_clamp(m3, m2, m1), std::nullopt);
    EXPECT_EQ(elementwise_clamp(m3, m3, m1), std::nullopt);
    EXPECT_EQ(elementwise_clamp(m3, m3, m2), std::nullopt);
  };

  auto arrays = makeTupleOfArrays<kMaxStructMember>();
  for (size_t i = 0; i < kMaxStructMember; i++) {
    op_tuple_elements(arrays, i, clampVerifyOp);
  }
}
