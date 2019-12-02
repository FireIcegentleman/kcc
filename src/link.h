//
// Created by kaiser on 2019/11/12.
//

#pragma once

#include <string>
#include <vector>

#include "llvm_common.h"

namespace kcc {

bool Link(const std::vector<std::string> &obj_file, OptLevel opt_level,
          const std::string &output_file);

}  // namespace kcc
