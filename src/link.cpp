//
// Created by kaiser on 2019/11/12.
//

#include "link.h"

#include <lld/Common/Driver.h>

namespace kcc {
bool Link(const std::vector<std::string> &obj_file,
          const std::string &output_file) {
  static std::vector<const char *> args{
      //"-pie",
      //"--eh-frame-hdr",
      "-m elf_x86_64",
      "-dynamic-linker",
      "/lib64/ld-linux-x86-64.so.2",
      "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/9.2.0/../../../../lib64/"
      "Scrt1.o",
      "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/9.2.0/../../../../lib64/"
      "crti.o",
      "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/9.2.0/crtbeginS.o",
      "-L/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/9.2.0",
      "-L/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/9.2.0/../../../../lib64",
      "-L/usr/bin/../lib64 -L/lib/../lib64 -L/usr/lib/../lib64",
      "-L/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/9.2.0/../../..",
      "-L/usr/bin/../lib",
      "-L/lib",
      "-L/usr/lib",
      "-lgcc",
      "--as-needed",
      "-lgcc_s",
      "--no-as-needed",
      "-lc",
      "-lgcc",
      "--as-needed",
      "-lgcc_s",
      "--no-as-needed",
      "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/9.2.0/crtendS.o",
      "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/9.2.0/../../../../lib64/"
      "crtn.o"};

  std::string str{"-o " + output_file};
  args.push_back(str.c_str());

  for (const auto &item : obj_file) {
    args.push_back(item.c_str());
  }

  return lld::elf::link(args, false);
}

}  // namespace kcc
