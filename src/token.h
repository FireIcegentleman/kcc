//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_TOKEN_H_
#define KCC_SRC_TOKEN_H_

#include <QMetaEnum>
#include <QObject>
#include <cstdint>
#include <string>
#include <unordered_set>

namespace kcc {

struct SourceLocation {
  std::string file_name;
  const char *line_content;

  std::string::size_type line_begin{};
  std::int32_t row{1};
  std::int32_t column{1};

  SourceLocation NextColumn() const {
    auto copy{*this};
    ++copy.column;
    return copy;
  }
};

class TokenTag : public QObject {
  Q_OBJECT
 public:
  enum Values {
    kAuto,
    kBreak,
    kCase,
    kChar,
    kConst,
    kContinue,
    kDefault,
    kDo,
    kDouble,
    kElse,
    kEnum,
    kExtern,
    kFloat,
    kFor,
    kGoto,
    kIf,
    kInline,  // C99
    kInt,
    kLong,
    kRegister,
    kRestrict,  // C99
    kReturn,
    kShort,
    kSigned,
    kSizeof,
    kStatic,
    kStruct,
    kSwitch,
    kTypedef,
    kUnion,
    kUnsigned,
    kVoid,
    kVolatile,
    kWhile,
    kAlignas,       // C11
    kAlignof,       // C11
    kAtomic,        // C11
    kBool,          // C99
    kComplex,       // C99
    kGeneric,       // C11
    kImaginary,     // C99
    kNoreturn,      // C11
    kStaticAssert,  // C11
    kThreadLocal,   // C11

    kAsm,
    kAttribute,

    kIdentifier,

    kIntegerConstant,
    kFloatingConstant,
    kCharacterConstant,

    kStringLiteral,

    kLeftSquare,   // [
    kRightSquare,  // ]
    kLeftParen,    // (
    kRightParen,   // )
    kLeftBrace,    // {
    kRightBrace,   // }
    kPeriod,       // .
    kArrow,        // ->

    kPlusPlus,    // ++
    kMinusMinus,  // --
    kAmp,         // &
    kStar,        // *
    kPlus,        // +
    kMinus,       // -
    kTilde,       // ~
    kExclaim,     // !

    kSlash,           // /
    kPercent,         // %
    kLessLess,        // <<
    kGreaterGreater,  // >>
    kLess,            // <
    kGreater,         // >
    kLessEqual,       // <=
    kGreaterEqual,    // >=
    kEqualEqual,      // ==
    kExclaimEqual,    // !=
    kCaret,           // ^
    kPipe,            // |
    kAmpAmp,          // &&
    kPipePipe,        // ||

    kQuestion,   // ?
    kColon,      // :
    kSemicolon,  // ;
    kEllipsis,   // ...

    kEqual,                // =
    kStarEqual,            // *=
    kSlashEqual,           // /=
    kPercentEqual,         // %=
    kPlusEqual,            // +=
    kMinusEqual,           // -=
    kLessLessEqual,        // <<=
    kGreaterGreaterEqual,  // >>=
    kAmpEqual,             // &=
    kCaretEqual,           // ^=
    kPipeEqual,            // |=

    kComma,  // ,

    kNone,
    kEof,
    kInvalid,
  };

  Q_ENUM(Values)
};

using Tag = TokenTag::Values;

class Token {
 public:
  static Token Get(Tag tag) {
    Token tok;
    tok.SetTag(tag);
    return tok;
  }

  bool TagIs(Tag tag) const;
  void SetTag(Tag tag);
  Tag GetTag() const { return tag_; }

  std::string GetStr() const;
  void SetStr(const std::string &str);

  SourceLocation GetLoc() const;
  void SetSourceLocation(const SourceLocation &location);

  std::string ToString() const;

 private:
  Tag tag_{Tag::kNone};
  std::string str_;
  SourceLocation location_;
};

inline std::string ToString(Tag value) {
  return QMetaEnum::fromType<Tag>().valueToKey(value) + 1;
}

}  // namespace kcc

#endif  // KCC_SRC_TOKEN_H_
