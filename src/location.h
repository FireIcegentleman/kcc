//
// Created by kaiser on 2019/11/2.
//

#ifndef KCC_SRC_LOCATION_H_
#define KCC_SRC_LOCATION_H_

#include <cstdint>
#include <string>

namespace kcc {

struct SourceLocation {
  std::string file_name;
  const char *line_content;

  std::string::size_type line_begin{};
  std::int32_t row{1};
  std::int32_t column{1};

  SourceLocation NextColumn() const;
};

}  // namespace kcc

#endif  // KCC_SRC_LOCATION_H_
