//
// Created by kaiser on 2019/11/12.
//

#ifndef KCC_SRC_LINK_H_
#define KCC_SRC_LINK_H_

#include <string>
#include <vector>

#include "util.h"

namespace kcc {

bool Link(const std::vector<std::string> &obj_file, OptLevel opt_level,
          const std::string &output_file);

}  // namespace kcc

#endif  // KCC_SRC_LINK_H_
