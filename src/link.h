//
// Created by kaiser on 2019/11/12.
//

#pragma once

#include <string>
#include <vector>

#include "util.h"

namespace kcc {

bool Link(const std::vector<std::string> &obj_file, OptLevel opt_level,
          const std::string &output_file, bool shared = false,
          const std::vector<std::string> &r_path = {},
          const std::vector<std::string> &so_file = {},
          const std::vector<std::string> &a_file = {},
          const std::vector<std::string> &libs = {});

}  // namespace kcc
