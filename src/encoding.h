//
// Created by kaiser on 2019/11/8.
//

#ifndef KCC_SRC_ENCODING_H_
#define KCC_SRC_ENCODING_H_

#include <cstdint>
#include <string>

namespace kcc {

enum class Encoding { kNone, kUtf8, kChar16, kChar32, kWchar };

void AppendUCN(std::string &s, std::int32_t val);

void ConvertToUtf16(std::string &s);

void ConvertToUtf32(std::string &s);

void ConvertString(std::string &s, Encoding encoding);

}  // namespace kcc

#endif  // KCC_SRC_ENCODING_H_
