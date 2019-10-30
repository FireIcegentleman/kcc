//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_DICT_H_
#define KCC_SRC_DICT_H_

#include <map>
#include <string>

#include "token.h"

namespace kcc {

class KeywordsDictionary {
 public:
  KeywordsDictionary();
  Tag Find(const std::string& name) const;

 private:
  std::map<std::string, Tag> keywords_;
};

}  // namespace kcc

#endif  // KCC_SRC_DICT_H_
