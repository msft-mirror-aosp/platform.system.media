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

#include <audio_utils/StringUtils.h>
#include <charconv>

namespace android::audio_utils::stringutils {

// Moved from frameworks/av/services/mediametrics

std::string tokenizer(std::string::const_iterator& it,
                      const std::string::const_iterator& end, const char* reserved)
{
    // consume leading white space
    for (; it != end && std::isspace(*it); ++it);
    if (it == end) return {};

    auto start = it;
    // parse until we hit a reserved keyword or space
    if (strchr(reserved, *it)) return {start, ++it};
    for (;;) {
        ++it;
        if (it == end || std::isspace(*it) || strchr(reserved, *it)) return {start, it};
    }
}

std::vector<std::string> split(const std::string& flags, const char* delim)
{
    std::vector<std::string> result;
    for (auto it = flags.begin(); ; ) {
        auto flag = tokenizer(it, flags.end(), delim);
        if (flag.empty() || !std::isalnum(flag[0])) return result;
        result.emplace_back(std::move(flag));

        // look for the delimiter and discard
        auto token = tokenizer(it, flags.end(), delim);
        if (token.size() != 1 || strchr(delim, token[0]) == nullptr) return result;
    }
}

bool parseVector(const std::string& str, std::vector<int32_t>* vector) {
    std::vector<int32_t> values;
    const char* p = str.c_str();
    const char* last = p + str.size();
    while (p != last) {
        if (*p == ',' || *p == '{' || *p == '}') {
            p++;
        }
        int32_t value = -1;
        auto [ptr, error] = std::from_chars(p, last, value);
        if (error == std::errc::invalid_argument || error == std::errc::result_out_of_range) {
            return false;
        }
        p = ptr;
        values.push_back(value);
    }
    *vector = std::move(values);
    return true;
}

std::vector<std::pair<std::string, std::string>>
getDeviceAddressPairs(const std::string& devices)
{
    std::vector<std::pair<std::string, std::string>> result;

    // Currently, the device format is
    //
    // devices = device_addr OR device_addr|devices
    // device_addr = device OR (device, addr)
    //
    // EXAMPLE:
    // device1|(device2, addr2)|...

    static constexpr char delim[] = "()|,";
    for (auto it = devices.begin(); ; ) {
        std::string address;
        std::string device = tokenizer(it, devices.end(), delim);
        if (device.empty()) return result;
        if (device == "(") {  // it is a pair otherwise we consider it a device
            device = tokenizer(it, devices.end(), delim); // get actual device
            auto token = tokenizer(it, devices.end(), delim);
            if (token != ",") return result;  // malformed, must have a comma

            // special handling here for empty addresses
            address = tokenizer(it, devices.end(), delim);
            if (address.empty()) return result;
            if (address == ")") {  // no address, just the ")"
                address.clear();
            } else {
                token = tokenizer(it, devices.end(), delim);
                if (token != ")") return result;
            }
        }
        // misaligned token, device must start alphanumeric.
        if (!std::isalnum(device[0])) return result;

        result.emplace_back(std::move(device), std::move(address));

        auto token = tokenizer(it, devices.end(), delim);
        if (token != "|") return result;  // this includes end of string detection
    }
}

} // namespace android::audio_utils::stringutils
