//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_LEX_H_
#define KCC_SRC_LEX_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dict.h"
#include "encoding.h"
#include "token.h"

namespace kcc {

class Scanner {
 public:
  explicit Scanner(std::string preprocessed_code);

  std::vector<Token> Tokenize();

  std::string HandleIdentifier();
  std::pair<std::int32_t, Encoding> HandleCharacter();
  std::pair<std::string, Encoding> HandleStringLiteral();

 private:
  Token Scan();

  bool HasNext();
  std::int32_t Peek();
  std::int32_t Next(bool push = true);
  void PutBack();
  bool Test(std::int32_t c);
  bool Try(std::int32_t c);
  bool IsUCN(std::int32_t ch);

  Token MakeToken(Tag tag);
  void MarkLocation();

  void SkipSpace();
  void SkipComment();

  Token SkipNumber();
  Token SkipIdentifier();
  Token SkipCharacter();
  Token SkipStringLiteral();

  Encoding HandleEncoding();
  std::int32_t HandleEscape();
  std::int32_t HandleHexEscape();
  std::int32_t HandleOctEscape(std::int32_t ch);
  std::int32_t HandleUCN(std::int32_t length);

  std::string source_;
  std::string::size_type curr_index_{};

  Location loc_;

  Token curr_token_;
  std::string buffer_;

  inline static KeywordsDictionary Keywords;
};

}  // namespace kcc

#endif  // KCC_SRC_LEX_H_
