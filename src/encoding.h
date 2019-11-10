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

}  // namespace kcc

#endif  // KCC_SRC_ENCODING_H_
