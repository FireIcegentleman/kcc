//
// Created by kaiser on 2019/10/30.
//

// 替用记号(C95)
// {	<%
// }	%>
// [	<:
// ]	:>
// #	%:
// ##	%:%:

#include "lex.h"

#include <cassert>
#include <cctype>

#include "error.h"

namespace kcc {

bool IsOctDigit(std::int32_t ch) { return ch >= '0' && ch <= '7'; }

std::int32_t CharToDigit(std::int32_t ch) {
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

Scanner::Scanner(std::string preprocessed_code)
    : source_{std::move(preprocessed_code)} {
  loc_.content = source_.data();
}

std::vector<Token> Scanner::Tokenize() {
  std::vector<Token> token_sequence;

  while (true) {
    auto token{Scan()};
    token_sequence.push_back(token);

    if (token.TagIs(Tag::kNone)) {
      Error(loc_, "token error");
    } else if (token.IsEof()) {
      break;
    }
  }

  return token_sequence;
}

std::string Scanner::HandleIdentifier() {
  std::string s;

  while (HasNext()) {
    std::int32_t ch{Next(false)};
    if (IsUCN(ch)) {
      AppendUCN(s, HandleEscape());
    } else {
      s.push_back(ch);
    }
  }

  return s;
}

std::pair<std::int32_t, Encoding> Scanner::HandleCharacter() {
  auto encoding{HandleEncoding()};

  std::int32_t val{};
  std::int32_t count{};
  // eat '
  Next(false);

  while (!Test('\'')) {
    std::int32_t ch{Next(false)};
    if (ch == '\\') {
      ch = HandleEscape();
    }

    if (encoding == Encoding::kNone) {
      val = (val << 8) + ch;
    } else {
      val = ch;
    }
    ++count;
  }

  if (count > 1) {
    Warning(loc_, "multi-character character constant");
  }

  return {val, encoding};
}

std::pair<std::string, Encoding> Scanner::HandleStringLiteral() {
  auto encoding{HandleEncoding()};
  std::string s;
  // eat "
  Next(false);

  while (!Test('"')) {
    std::int32_t ch{Next(false)};
    bool is_ucn{IsUCN(ch)};

    if (ch == '\\') {
      ch = HandleEscape();
    }

    if (is_ucn) {
      AppendUCN(s, ch);
    } else {
      s.push_back(ch);
    }
  }

  return {s, encoding};
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
      } else if (Try('>')) {
        return MakeToken(Tag::kRightBrace);
      } else if (Try(':')) {
        if (Try('%')) {
          if (Try(':')) {
            return MakeToken(Tag::kSharpSharp);
          } else {
            PutBack();
            return MakeToken(Tag::kSharp);
          }
        } else {
          return MakeToken(Tag::kSharp);
        }
      } else {
        return MakeToken(Tag::kPercent);
      }
    case '<':
      if (Try('<')) {
        return MakeToken(Try('=') ? Tag::kLessLessEqual : Tag::kLessLess);
      } else if (Try('=')) {
        return MakeToken(Tag::kLessEqual);
      } else if (Try(':')) {
        return MakeToken(Tag::kLeftSquare);
      } else if (Try('%')) {
        return MakeToken(Tag::kLeftBrace);
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
    case '#':
      SkipComment();
      return Scan();
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
    case 'u':
    case 'U':
    case 'L':
      PutBack();
      HandleEncoding();

      if (Try('\'')) {
        return SkipCharacter();
      } else if (Try('"')) {
        return SkipStringLiteral();
      } else {
        Next();
        return SkipIdentifier();
      }
    case '\\':
      if (Test('u') || Test('U')) {
        return SkipIdentifier();
      } else {
        Error(loc_, "Invalid input: {}", ch);
      }
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
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '_':
      // TODO 是扩展吗？
    case '$':
      return SkipIdentifier();
    case '\0':
      buffer_.clear();
      return MakeToken(Tag::kEof);
    default: {
      // 字节 0xFE 和 0xFF 在 UTF-8 编码中从未用到
      if (ch >= 0x80 && ch <= 0xfd) {
        return SkipIdentifier();
      } else {
        Error(loc_, "Invalid input: {}", static_cast<char>(ch));
      }
    }
  }
}

bool Scanner::HasNext() { return curr_index_ < std::size(source_); }

std::int32_t Scanner::Peek() {
  // 注意 C++11 起, 若 curr_index_ == size() 则返回空字符
  auto ret{source_[curr_index_]};
  // 可能是 UTF-8 编码的非 ascii 字符, 此时值为负
  return ret >= 0 ? ret : ret + 256;
}

std::int32_t Scanner::Next(bool push) {
  auto ch{Peek()};
  ++curr_index_;

  if (push) {
    buffer_.push_back(ch);
  }

  if (ch == '\n') {
    ++loc_.row;
    pre_column_ = loc_.column;
    loc_.column = 1;
    loc_.line_begin = curr_index_;
  } else {
    ++loc_.column;
  }

  return ch;
}

void Scanner::PutBack() {
  assert(curr_index_ > 0);
  auto ch{source_[--curr_index_]};

  assert(!std::empty(buffer_));
  buffer_.pop_back();

  if (ch == '\n') {
    --loc_.row;
    loc_.column = pre_column_;
  } else {
    --loc_.column;
  }
}

bool Scanner::Test(std::int32_t c) { return Peek() == c; }

bool Scanner::Try(std::int32_t c) {
  if (Peek() == c) {
    Next();
    return true;
  } else {
    return false;
  }
}

bool Scanner::IsUCN(std::int32_t ch) {
  return ch == '\\' && (Test('u') || Test('U'));
}

Token Scanner::MakeToken(Tag tag) {
  curr_token_.SetTag(tag);
  curr_token_.SetStr(buffer_);
  buffer_.clear();
  return curr_token_;
}

void Scanner::MarkLocation() { curr_token_.SetLoc(loc_); }

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
  loc_.row = std::stoi(SkipNumber().GetStr()) - 1;
  // eat space
  Next(false);

  // eat "
  Next();
  auto file_name{SkipStringLiteral().GetStr()};
  // 去掉前后的 "
  loc_.file_name = file_name.substr(1, std::size(file_name) - 2);

  while (HasNext() && Next(false) != '\n') {
  }

  buffer_.clear();
}

// pp-number:
//  digit
//  . digit
//  pp-number digit
//  pp-number identifier-nondigit
//  pp-number e sign
//  pp-number E sign
//  pp-number p sign
//  pp-number P sign
//  pp-number .
Token Scanner::SkipNumber() {
  bool saw_hex_prefix{false};
  auto tag{Tag::kIntegerConstant};

  auto ch{buffer_.front()};
  while (ch == '.' || std::isalnum(ch) || ch == '_' || IsUCN(ch)) {
    // 注意有 e 不一定是浮点数
    if (ch == 'e' || ch == 'E' || ch == 'p' || ch == 'P') {
      if (!Try('-')) {
        Try('+');
      }

      if ((ch == 'p' || ch == 'P') && saw_hex_prefix) {
        tag = Tag::kFloatingConstant;
      }
      if ((ch == 'e' || ch == 'E') && !saw_hex_prefix) {
        tag = Tag::kFloatingConstant;
      }
    } else if (IsUCN(ch)) {
      HandleEscape();
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

// identifier:
//  identifier-nondigit
//  identifier identifier-nondigit
//  identifier digit
// identifier-nondigit:
//  nondigit
//  universal-character-name
//  other implementation-defined characters
// nondigit: one of
//  _abcdefghijklm
//  nopqrstuvwxyz
//  ABCDEFGHIJKLM
//  NOPQRSTUVWXYZ
// digit: one of
//  0123456789
Token Scanner::SkipIdentifier() {
  PutBack();
  std::int32_t ch{Next()};

  while (std::isalnum(ch) || ch == '_' || IsUCN(ch) ||
         (0x80 <= ch && ch <= 0xfd) || ch == '$') {
    if (IsUCN(ch)) {
      HandleEscape();
    }
    ch = Next();
  }
  PutBack();

  return MakeToken(Scanner::Keywords.Find(buffer_));
}

// character-constant:
//  ' c-char-sequence '
//  L' c-char-sequence '
//  u' c-char-sequence '
//  U' c-char-sequence '
// c-char:
//  any member of the source character set except
//  the single-quote ', backslash \, or new-line character
//  escape-sequence
Token Scanner::SkipCharacter() {
  auto ch{Next()};
  while (ch != '\'' && ch != '\n' && ch != '\0') {
    if (ch == '\\') {
      Next();
    }
    ch = Next();
  }

  if (ch != '\'') {
    Error(loc_, "missing terminating ' character");
  }

  return MakeToken(Tag::kCharacterConstant);
}

// string-literal:
//  encoding-prefixopt " s-char-sequenceopt "
// encoding-prefix:
//  u8 u U L
// s-char-sequence:
//  s-char
//  s-char-sequence s-char
// s-char:
//  any member of the source character set except
//  the double-quote ", backslash \, or new-line character
//  escape-sequence
Token Scanner::SkipStringLiteral() {
  auto ch{Next()};
  while (ch != '\"' && ch != '\n' && ch != '\0') {
    if (ch == '\\') {
      Next();
    }
    ch = Next();
  }

  if (ch != '\"') {
    Error(loc_, "missing terminating \" character");
  }

  return MakeToken(Tag::kStringLiteral);
}

Encoding Scanner::HandleEncoding() {
  if (Try('u')) {
    if (Try('8')) {
      return Encoding ::kUtf8;
    } else {
      return Encoding ::kChar16;
    }
  } else if (Try('U')) {
    return Encoding ::kChar32;
  } else if (Try('L')) {
    return Encoding ::kWchar;
  } else {
    return Encoding::kNone;
  }
}

// escape-sequence:
//  simple-escape-sequence
//  octal-escape-sequence
//  hexadecimal-escape-sequence
//  universal-character-name
// simple-escape-sequence: one of
/*
 * \' \" \? \\ \a \b \f \n \r \t \v
 */
std::int32_t Scanner::HandleEscape() {
  auto ch{Next()};
  switch (ch) {
    case '\'':
    case '\"':
    case '\?':
    case '\\':
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
      // GNU 扩展
    case 'e':
      return '\033';
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
    case 'u':
      return HandleUCN(4);
    case 'U':
      return HandleUCN(8);
    default: {
      Error(loc_, "unknown escape sequence '\\{}'", ch);
    }
  }
}

// hexadecimal-escape-sequence:
//  \x hexadecimal-digit
//  hexadecimal-escape-sequence hexadecimal-digit
std::int32_t Scanner::HandleHexEscape() {
  std::int32_t value{};
  auto ch{Peek()};

  if (!std::isxdigit(ch)) {
    Error(loc_, "\\x used with no following hex digits: '{}'", ch);
  }

  ch = Next();
  while (std::isxdigit(ch)) {
    value = (static_cast<std::uint32_t>(value) << 4U) + CharToDigit(ch);
    ch = Next();
  }
  PutBack();

  return value;
}

// octal-escape-sequence:
//  \ octal-digit
//  \ octal-digit octal-digit
//  \ octal-digit octal-digit octal-digit
std::int32_t Scanner::HandleOctEscape(std::int32_t ch) {
  std::int32_t value{CharToDigit(ch)};

  if (!IsOctDigit(ch)) {
    Error(loc_, "\\nnn used with no following oct digits: '{}'", ch);
  }

  for (std::int32_t i{0}; i < 3; ++i) {
    if (auto next{Next()}; IsOctDigit(next)) {
      value = (static_cast<std::uint32_t>(value) << 3U) + CharToDigit(next);
    } else {
      PutBack();
      break;
    }
  }

  return value;
}

// universal-character-name:
//  \u hex-quad
//  \U hex-quad hex-quad
// hex-quad:
//  hexadecimal-digit hexadecimal-digit
//  hexadecimal-digit hexadecimal-digit
std::int32_t Scanner::HandleUCN(std::int32_t length) {
  assert(length == 4 || length == 8);

  std::int32_t val{};
  for (std::int32_t i{0}; i < length; ++i) {
    auto ch{Next()};
    if (!std::isxdigit(ch)) {
      Error(loc_, "\\u / \\U used with no following hex digits: '{}'", ch);
    }
    val = (val << 4) + CharToDigit(ch);
  }

  return val;
}

}  // namespace kcc
