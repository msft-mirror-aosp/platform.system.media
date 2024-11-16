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

//#define LOG_NDEBUG 0
#define LOG_TAG "audio_stringutils_tests"

#include <audio_utils/StringUtils.h>
#include <gtest/gtest.h>

TEST(audio_utils_string, parseVector) {
    {
        std::vector<int32_t> values;
        EXPECT_EQ(true, android::audio_utils::stringutils::parseVector(
                "0{4,300,0,-112343,350}9", &values));
        EXPECT_EQ(values, std::vector<int32_t>({0, 4, 300, 0, -112343, 350, 9}));
    }
    {
        std::vector<int32_t> values;
        EXPECT_EQ(true, android::audio_utils::stringutils::parseVector("53", &values));
        EXPECT_EQ(values, std::vector<int32_t>({53}));
    }
    {
        std::vector<int32_t> values;
        EXPECT_EQ(false, android::audio_utils::stringutils::parseVector("5{3,6*3}3", &values));
        EXPECT_EQ(values, std::vector<int32_t>({}));
    }
    {
        std::vector<int32_t> values = {1}; // should still be this when parsing fails
        std::vector<int32_t> expected = {1};
        EXPECT_EQ(false, android::audio_utils::stringutils::parseVector("51342abcd,1232", &values));
        EXPECT_EQ(values, std::vector<int32_t>({1}));
    }
    {
        std::vector<int32_t> values = {2}; // should still be this when parsing fails
        EXPECT_EQ(false, android::audio_utils::stringutils::parseVector(
                "12345678901234,12345678901234", &values));
        EXPECT_EQ(values, std::vector<int32_t>({2}));
    }
}


TEST(audio_utils_string, device_parsing) {
    auto devaddr = android::audio_utils::stringutils::getDeviceAddressPairs("(DEVICE, )");
    ASSERT_EQ((size_t)1, devaddr.size());
    ASSERT_EQ("DEVICE", devaddr[0].first);
    ASSERT_EQ("", devaddr[0].second);

    devaddr = android::audio_utils::stringutils::getDeviceAddressPairs(
            "(DEVICE1, A)|(D, ADDRB)");
    ASSERT_EQ((size_t)2, devaddr.size());
    ASSERT_EQ("DEVICE1", devaddr[0].first);
    ASSERT_EQ("A", devaddr[0].second);
    ASSERT_EQ("D", devaddr[1].first);
    ASSERT_EQ("ADDRB", devaddr[1].second);

    devaddr = android::audio_utils::stringutils::getDeviceAddressPairs(
            "(A,B)|(C,D)");
    ASSERT_EQ((size_t)2, devaddr.size());
    ASSERT_EQ("A", devaddr[0].first);
    ASSERT_EQ("B", devaddr[0].second);
    ASSERT_EQ("C", devaddr[1].first);
    ASSERT_EQ("D", devaddr[1].second);

    devaddr = android::audio_utils::stringutils::getDeviceAddressPairs(
            "  ( A1 , B )  | ( C , D2 )  ");
    ASSERT_EQ((size_t)2, devaddr.size());
    ASSERT_EQ("A1", devaddr[0].first);
    ASSERT_EQ("B", devaddr[0].second);
    ASSERT_EQ("C", devaddr[1].first);
    ASSERT_EQ("D2", devaddr[1].second);

    devaddr = android::audio_utils::stringutils::getDeviceAddressPairs(
            " Z  ");
    ASSERT_EQ((size_t)1, devaddr.size());
    ASSERT_EQ("Z", devaddr[0].first);

    devaddr = android::audio_utils::stringutils::getDeviceAddressPairs(
            "  A | B|C  ");
    ASSERT_EQ((size_t)3, devaddr.size());
    ASSERT_EQ("A", devaddr[0].first);
    ASSERT_EQ("", devaddr[0].second);
    ASSERT_EQ("B", devaddr[1].first);
    ASSERT_EQ("", devaddr[1].second);
    ASSERT_EQ("C", devaddr[2].first);
    ASSERT_EQ("", devaddr[2].second);

    devaddr = android::audio_utils::stringutils::getDeviceAddressPairs(
            "  A | (B1, 10) |C  ");
    ASSERT_EQ((size_t)3, devaddr.size());
    ASSERT_EQ("A", devaddr[0].first);
    ASSERT_EQ("", devaddr[0].second);
    ASSERT_EQ("B1", devaddr[1].first);
    ASSERT_EQ("10", devaddr[1].second);
    ASSERT_EQ("C", devaddr[2].first);
    ASSERT_EQ("", devaddr[2].second);
}

