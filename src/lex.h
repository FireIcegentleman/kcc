//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_LEX_H_
#define KCC_SRC_LEX_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dict.h"
#include "encoding.h"
#include "location.h"
#include "token.h"

namespace kcc {

class Scanner {
 public:
  explicit Scanner(std::string preprocessed_code);
  // for parser
  Scanner(std::string code, const Location& loc);

  std::vector<Token> Tokenize();

  std::string HandleIdentifier();
  std::pair<std::int32_t, Encoding> HandleCharacter();
  std::pair<std::string, Encoding> HandleStringLiteral(bool handle_escape);

 private:
  bool HasNext();
  std::int32_t Peek();
  std::int32_t Next(bool push = true);
  void PutBack();
  bool Test(std::int32_t c);
  bool Try(std::int32_t c);
  bool IsUCN(std::int32_t ch);

  Token MakeToken(Tag tag);
  void MarkLocation();

  Token Scan();

  void SkipSpace();
  void SkipLineDirectives();

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
  std::string::size_type index_{};

  Location loc_;

  Token token_;
  std::string buffer_;

  constexpr static std::size_t kTokenReserve{1024};

  inline static KeywordsDictionary Keywords;
};

}  // namespace kcc

#endif  // KCC_SRC_LEX_H_
