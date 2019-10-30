//
// Created by kaiser on 2019/10/30.
//

#include "lex.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>

#include "error.h"

namespace kcc {

bool IsOctDigit(char ch) { return ch >= '0' && ch <= '7'; }

std::int32_t CharToDigit(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  } else if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  } else {
    assert(false);
    return 0;
  }
}

Scanner::Scanner(const std::string& preprocessed_code)
    : source_{preprocessed_code} {
  if (std::empty(preprocessed_code) ||
      std::all_of(std::begin(preprocessed_code), std::end(preprocessed_code),
                  [](char ch) { return ch == '\n'; })) {
    ErrorReportAndExit("ISO C forbids an empty translation unit");
  }

  location_.line_content = source_.data();
}

std::vector<Token> Scanner::Tokenize() {
  std::vector<Token> token_sequence;

  while (true) {
    auto token{Scan()};

    if (token.TagIs(Tag::kEnd)) {
      break;
    } else {
      token_sequence.push_back(token);
    }
  }

  return token_sequence;
}

Token Scanner::Scan() {
  SkipSpace();

  MarkLocation();

  switch (auto ch{Next()}; ch) {
    case '[':
      return MakeToken(Tag::kLeftSquare);
    case ']':
      return MakeToken(Tag::kRightSquare);
    case '(':
      return MakeToken(Tag::kLeftParen);
    case ')':
      return MakeToken(Tag::kRightParen);
    case '{':
      return MakeToken(Tag::kLeftCurly);
    case '}':
      return MakeToken(Tag::kRightCurly);
    case '.':
      if (std::isdigit(Peek())) {
        return SkipNumber();
      }

      if (Try('.')) {
        if (Try('.')) {
          return MakeToken(Tag::kEllipsis);
        } else {
          PutBack();
          return MakeToken(Tag::kDot);
        }
      } else {
        return MakeToken(Tag::kDot);
      }
    case '+':
      if (Try('+')) {
        return MakeToken(Tag::kInc);
      } else if (Try('=')) {
        return MakeToken(Tag::kAddAssign);
      } else {
        return MakeToken(Tag::kAdd);
      }
    case '-':
      if (Try('>')) {
        return MakeToken(Tag::kArrow);
      } else if (Try('-')) {
        return MakeToken(Tag::kDec);
      } else if (Try('=')) {
        return MakeToken(Tag::kSubAssign);
      } else {
        return MakeToken(Tag::kSub);
      }
    case '&':
      if (Try('&')) {
        return MakeToken(Tag::kLogicAnd);
      } else if (Try('=')) {
        return MakeToken(Tag::kAndAssign);
      } else {
        return MakeToken(Tag::kAnd);
      }
    case '*':
      return MakeToken(Try('=') ? Tag::kMulAssign : Tag::kMul);
    case '~':
      return MakeToken(Tag::kNot);
    case '!':
      return MakeToken(Try('=') ? Tag::kNotEqual : Tag::kLogicNot);
    case '/':
      return MakeToken(Try('=') ? Tag::kDivAssign : Tag::kDiv);
    case '%':
      if (Try('=')) {
        return MakeToken(Tag::kModAssign);
      } else {
        return MakeToken(Tag::kMod);
      }
    case '<':
      if (Try('<')) {
        return MakeToken(Try('=') ? Tag::kShlAssign : Tag::kShl);
      } else if (Try('=')) {
        return MakeToken(Tag::kLessEqual);
      } else {
        return MakeToken(Tag::kLess);
      }
    case '>':
      if (Try('>')) {
        return MakeToken(Try('=') ? Tag::kShrAssign : Tag::kShr);
      } else {
        return MakeToken(Try('=') ? Tag::kGreaterEqual : Tag::kGreater);
      }
    case '=':
      return MakeToken(Try('=') ? Tag::kEqual : Tag::kAssign);
    case '^':
      return MakeToken(Try('=') ? Tag::kXorAssign : Tag::kXor);
    case '|':
      if (Try('|')) {
        return MakeToken(Tag::kLogicOr);
      } else if (Try('=')) {
        return MakeToken(Tag::kOrAssign);
      } else {
        return MakeToken(Tag::kOr);
      }
    case '?':
      return MakeToken(Tag::kQuestionMark);
    case ':':
      return MakeToken(Try(Tag::kGreater) ? Tag::kRightSquare : Tag::kColon);
    case ';':
      return MakeToken(Tag::kSemicolon);
    case ',':
      return MakeToken(Tag::kComma);
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return SkipNumber();
    case '\'':
      return SkipCharacter();
    case '"':
      return SkipStringLiteral();
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '_':
      return SkipIdentifier();
    case '#':
      SkipComment();
      return Scan();
    case '\0':
      return MakeToken(Tag::kEnd);
    default: {
      return MakeToken(Tag::kInvalid);
    }
  }
}

Token Scanner::MakeToken(Tag tag) {
  curr_token_.SetTag(tag);
  curr_token_.SetStr(buffer_);
  buffer_.clear();
  return curr_token_;
}

bool Scanner::HasNext() { return curr_index_ + 1 < std::size(source_); }

char Scanner::Peek() {
  // 注意 C++11 起, 若 curr_index_ == size() 则返回空字符
  return source_[curr_index_];
}

char Scanner::Next(bool push) {
  auto ch{Peek()};
  ++curr_index_;

  if (push) {
    buffer_.push_back(ch);
  }

  if (ch == '\n') {
    ++location_.row;
    pre_column_ = location_.column;
    location_.column = 1;
    location_.line_begin = curr_index_;
  } else {
    ++location_.column;
  }

  return ch;
}

void Scanner::PutBack() {
  auto ch{source_[--curr_index_]};
  buffer_.pop_back();

  if (ch == '\n') {
    --location_.row;
    location_.column = pre_column_;
  } else {
    --location_.column;
  }
}

bool Scanner::Test(char c) { return Peek() == c; }

bool Scanner::Try(char c) {
  if (Peek() == c) {
    Next();
    return true;
  } else {
    return false;
  }
}

void Scanner::MarkLocation() { curr_token_.SetSourceLocation(location_); }

void Scanner::SkipSpace() {
  while (std::isspace(Peek())) {
    Next(false);
  }
}

void Scanner::SkipComment() {
  // clear #
  buffer_.clear();
  // eat space
  Next(false);

  // eat first number
  Next();
  location_.row = std::stoi(SkipNumber().GetStr());
  // eat space
  Next(false);

  // eat "
  Next();
  location_.file_name = SkipStringLiteral().GetStr();

  while (HasNext() && Next(false) != '\n') {
  }

  buffer_.clear();
}

Token Scanner::SkipNumber() {
  bool saw_hex_prefix{false};
  auto tag{Tag::kIntegerConstant};

  auto ch{buffer_.front()};
  while (ch == '.' || std::isalnum(ch)) {
    if (ch == 'e' || ch == 'E' || ch == 'p' || ch == 'P') {
      if (!Try('-')) {
        Try('+');
      }

      if ((ch == 'p' || ch == 'P') && saw_hex_prefix) {
        tag = Tag::kFloatingConstant;
      }
    } else if (ch == '.') {
      tag = Tag::kFloatingConstant;
    } else if (ch == 'x' || ch == 'X') {
      // 这里如果前面是其他数字或 . 也会执行下面的语句, 在后面处理
      // 过程中, 如 2x541, 将 x541 视为后缀, 也是不合法的
      saw_hex_prefix = true;
    }

    ch = Next();
  }
  PutBack();

  return MakeToken(tag);
}

Token Scanner::SkipCharacter() {
  // 此时 buffer 中已经有了字符 '
  auto ch{Next()};
  // ch=='\0' 时已经没有剩余字符了
  while (ch != '\'' && ch != '\n' && ch != '\0') {
    // 这里不处理转义序列
    if (ch == '\\') {
      Next();
    }
    ch = Next();
  }

  if (ch != '\'') {
    ErrorReportAndExit(location_, "missing terminating ' character");
  }

  return MakeToken(Tag::kCharacterConstant);
}

Token Scanner::SkipStringLiteral() {
  auto ch{Next()};
  while (ch != '\"' && ch != '\n' && ch != '\0') {
    if (ch == '\\') {
      Next();
    }
    ch = Next();
  }

  if (ch != '\"') {
    ErrorReportAndExit(location_, "missing terminating \" character");
  }

  return MakeToken(Tag::kStringLiteral);
}

Token Scanner::SkipIdentifier() {
  char ch;

  do {
    ch = Next();
  } while (std::isalnum(ch) || ch == '_');
  PutBack();

  return MakeToken(Tag::kIdentifier);
}

std::int32_t Scanner::HandleEscape() {
  auto ch{Next()};
  switch (ch) {
    case '\\':
    case '\'':
    case '\"':
    case '\?':
      return ch;
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    case 'X':
    case 'x':
      return HandleHexEscape();
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      return HandleOctEscape(ch);
    default: {
      ErrorReportAndExit(location_, "unknown escape sequence '\\{}'", ch);
    }
  }
}

std::int32_t Scanner::HandleHexEscape() {
  std::int32_t value{};

  if (auto peek{Peek()}; !std::isxdigit(peek)) {
    ErrorReportAndExit(location_, "\\x used with no following hex digits: '{}'",
                       peek);
  }

  for (std::int32_t i{0}; i < 2; ++i) {
    if (char next{Next()}; std::isxdigit(next)) {
      value = (static_cast<std::uint32_t>(value) << 4U) + CharToDigit(next);
    } else {
      PutBack();
      break;
    }
  }

  return value;
}

std::int32_t Scanner::HandleOctEscape(char ch) {
  std::int32_t value{CharToDigit(ch)};

  if (!IsOctDigit(ch)) {
    ErrorReportAndExit(location_, "expect oct digit, but got {}", ch);
  }

  for (std::int32_t i{0}; i < 3; ++i) {
    if (char next{Next()}; IsOctDigit(next)) {
      value = (static_cast<std::uint32_t>(value) << 3U) + CharToDigit(next);
    } else {
      PutBack();
      break;
    }
  }

  return value;
}

}  // namespace kcc
