//
// Created by kaiser on 2019/11/2.
//

#include "location.h"

namespace kcc {

SourceLocation SourceLocation::NextColumn() const {
  auto copy{*this};
  ++copy.column;
  return copy;
}

}  // namespace kcc
