//
// Created by kaiser on 2019/11/9.
//

#include "util.h"

#include <unistd.h>
#include <wait.h>

#include <cstddef>
#include <cstring>

#include "error.h"

namespace kcc {

bool CommandSuccess(std::int32_t status) {
  return status != -1 && WIFEXITED(status) && !WEXITSTATUS(status);
}

std::string GetPath() {
  constexpr std::size_t kBufSize{256};
  char buf[kBufSize];
  // 将符号链接的内容读入buf,不超过kBufSize字节.
  // 内容不以空字符终止,返回读取的字符数,返回-1表示错误
  // /proc/self/exe代表当前程序
  auto count{readlink("/proc/self/exe", buf, kBufSize)};
  if (count == -1) {
    Error("readlink error");
  }

  auto end{std::strrchr(buf, '/')};
  return std::string(buf, end - buf);
}

}  // namespace kcc
