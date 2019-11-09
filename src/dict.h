//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_DICT_H_
#define KCC_SRC_DICT_H_

#include <string>
#include <string_view>
#include <unordered_map>

#include "token.h"

namespace kcc {

class KeywordsDictionary {
 public:
  KeywordsDictionary();
  Tag Find(const std::string& name) const;

 private:
  std::unordered_map<std::string_view, Tag> keywords_;
};

}  // namespace kcc

#endif  // KCC_SRC_DICT_H_
