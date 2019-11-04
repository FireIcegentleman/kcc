//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_LEX_H_
#define KCC_SRC_LEX_H_

#include <cstdint>
#include <string>
#include <vector>

#include "dict.h"
#include "token.h"

namespace kcc {

class Scanner {
 public:
  explicit Scanner(const std::string &preprocessed_code);
  std::vector<Token> Tokenize();
  std::string ScanStringLiteral(bool handle_escape);
  std::int32_t ScanCharacter();

 private:
  Token Scan();
  Token MakeToken(Tag tag);

  bool HasNext();
  char Peek();
  char Next(bool push = true);
  void PutBack();
  bool Test(char c);
  bool Try(char c);
  void MarkLocation();

  void SkipSpace();
  void SkipComment();

  Token SkipNumber();
  Token SkipCharacter();
  Token SkipStringLiteral();
  Token SkipIdentifier();

  std::int32_t HandleEscape();
  std::int32_t HandleHexEscape();
  std::int32_t HandleOctEscape(char ch);

  std::string source_;

  SourceLocation location_;
  std::int32_t pre_column_{1};
  Token curr_token_;

  std::string::size_type curr_index_{};
  std::string buffer_;

  KeywordsDictionary keywords_;
};

}  // namespace kcc

#endif  // KCC_SRC_LEX_H_
