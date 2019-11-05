//
// Created by kaiser on 2019/11/2.
//

#ifndef KCC_SRC_OPT_H_
#define KCC_SRC_OPT_H_

namespace kcc {

enum class OptLevel { kO0, kO1, kO2, kO3 };

void Optimization(OptLevel opt_level);

}  // namespace kcc

#endif  // KCC_SRC_OPT_H_
