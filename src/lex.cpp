//
// Created by kaiser on 2019/10/30.
//

#include "lex.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>

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
    Error("ISO C forbids an empty translation unit");
  }

  location_.line_content = source_.data();
}

std::vector<Token> Scanner::Tokenize() {
  std::vector<Token> token_sequence;

  while (true) {
    auto token{Scan()};
    token_sequence.push_back(token);

    if (token.TagIs(Tag::kEof)) {
      break;
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
      return MakeToken(Tag::kLeftBrace);
    case '}':
      return MakeToken(Tag::kRightBrace);
    case '.':
      if (std::isdigit(Peek())) {
        return SkipNumber();
      }

      if (Try('.')) {
        if (Try('.')) {
          return MakeToken(Tag::kEllipsis);
        } else {
          PutBack();
          return MakeToken(Tag::kPeriod);
        }
      } else {
        return MakeToken(Tag::kPeriod);
      }
    case '+':
      if (Try('+')) {
        return MakeToken(Tag::kPlusPlus);
      } else if (Try('=')) {
        return MakeToken(Tag::kPlusEqual);
      } else {
        return MakeToken(Tag::kPlus);
      }
    case '-':
      if (Try('>')) {
        return MakeToken(Tag::kArrow);
      } else if (Try('-')) {
        return MakeToken(Tag::kMinusMinus);
      } else if (Try('=')) {
        return MakeToken(Tag::kMinusEqual);
      } else {
        return MakeToken(Tag::kMinus);
      }
    case '&':
      if (Try('&')) {
        return MakeToken(Tag::kAmpAmp);
      } else if (Try('=')) {
        return MakeToken(Tag::kAmpEqual);
      } else {
        return MakeToken(Tag::kAmp);
      }
    case '*':
      return MakeToken(Try('=') ? Tag::kStarEqual : Tag::kStar);
    case '~':
      return MakeToken(Tag::kTilde);
    case '!':
      return MakeToken(Try('=') ? Tag::kExclaimEqual : Tag::kExclaim);
    case '/':
      return MakeToken(Try('=') ? Tag::kSlashEqual : Tag::kSlash);
    case '%':
      if (Try('=')) {
        return MakeToken(Tag::kPercentEqual);
      } else {
        return MakeToken(Tag::kPercent);
      }
    case '<':
      if (Try('<')) {
        return MakeToken(Try('=') ? Tag::kLessLessEqual : Tag::kLessLess);
      } else if (Try('=')) {
        return MakeToken(Tag::kLessEqual);
      } else {
        return MakeToken(Tag::kLess);
      }
    case '>':
      if (Try('>')) {
        return MakeToken(Try('=') ? Tag::kGreaterGreaterEqual
                                  : Tag::kGreaterGreater);
      } else {
        return MakeToken(Try('=') ? Tag::kGreaterEqual : Tag::kGreater);
      }
    case '=':
      return MakeToken(Try('=') ? Tag::kEqualEqual : Tag::kEqual);
    case '^':
      return MakeToken(Try('=') ? Tag::kCaretEqual : Tag::kCaret);
    case '|':
      if (Try('|')) {
        return MakeToken(Tag::kPipePipe);
      } else if (Try('=')) {
        return MakeToken(Tag::kPipeEqual);
      } else {
        return MakeToken(Tag::kPipe);
      }
    case '?':
      return MakeToken(Tag::kQuestion);
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
      buffer_.clear();
      return MakeToken(Tag::kEof);
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
  // # 后的数字指示的是下一行的行号
  location_.row = std::stoi(SkipNumber().GetStr()) - 1;
  // eat space
  Next(false);

  // eat "
  Next();
  auto file_name{SkipStringLiteral().GetStr()};
  location_.file_name = file_name.substr(1, std::size(file_name) - 2);

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
    Error(location_, "missing terminating ' character");
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
    Error(location_, "missing terminating \" character");
  }

  return MakeToken(Tag::kStringLiteral);
}

Token Scanner::SkipIdentifier() {
  char ch;

  do {
    ch = Next();
  } while (std::isalnum(ch) || ch == '_');
  PutBack();

  return MakeToken(keywords_.Find(buffer_));
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
      Error(location_, "unknown escape sequence '\\{}'", ch);
    }
  }
}

std::int32_t Scanner::HandleHexEscape() {
  std::int32_t value{};

  if (auto peek{Peek()}; !std::isxdigit(peek)) {
    Error(location_, "\\x used with no following hex digits: '{}'", peek);
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
    Error(location_, "expect oct digit, but got {}", ch);
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

std::string Scanner::ScanStringLiteral(bool handle_escape) {
  std::string s;
  // eat "
  Next();

  while (!Test('"')) {
    auto ch{Next()};
    if (handle_escape && ch == '\\') {
      ch = HandleEscape();
    }
    s.push_back(ch);
  }

  return s;
}

}  // namespace kcc
