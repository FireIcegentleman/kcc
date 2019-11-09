//
// Created by kaiser on 2019/11/2.
//

#ifndef KCC_SRC_LOCATION_H_
#define KCC_SRC_LOCATION_H_

#include <cstddef>
#include <cstdint>
#include <string>

namespace kcc {

class Location {
 public:
  std::string ToLocStr() const;
  std::string GetLineContent() const;

  std::string file_name;
  const char *content;

  std::size_t line_begin{};
  std::int32_t row{1};
  std::int32_t column{1};
};

}  // namespace kcc

#endif  // KCC_SRC_LOCATION_H_
