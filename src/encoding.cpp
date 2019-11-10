//
// Created by kaiser on 2019/11/8.
//

#include "encoding.h"

#include <boost/locale/encoding.hpp>

namespace kcc {

void AppendUCN(std::string& s, std::int32_t val) {
  std::u32string str;
  str.push_back(static_cast<char32_t>(val));
  s += boost::locale::conv::utf_to_utf<char>(str);
}

}  // namespace kcc
