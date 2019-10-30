//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_TOKEN_H_
#define KCC_SRC_TOKEN_H_

#include <cstdint>
#include <string>
#include <unordered_set>

#include <QMetaEnum>
#include <QObject>

namespace kcc {
struct SourceLocation {
  std::string file_name;
  const char *line_content;

  std::string::size_type line_begin{};
  std::int32_t row{1};
  std::int32_t column{1};
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

    kIdentifier,

    kIntegerConstant,
    kFloatingConstant,
    kCharacterConstant,

    kStringLiteral,

    kLeftSquare,   // [
    kRightSquare,  // ]
    kLeftParen,    // (
    kRightParen,   // )
    kLeftCurly,    // {
    kRightCurly,   // }
    kDot,          // .
    kArrow,        // ->

    kInc,       // ++
    kDec,       // --
    kAnd,       // &
    kMul,       // *
    kAdd,       // +
    kSub,       // -
    kNot,       // ~
    kLogicNot,  // !

    kDiv,           // /
    kMod,           // %
    kShl,           // <<
    kShr,           // >>
    kLess,          // <
    kGreater,       // >
    kLessEqual,     // <=
    kGreaterEqual,  // >=
    kEqual,         // ==
    kNotEqual,      // !=
    kXor,           // ^
    kOr,            // |
    kLogicAnd,      // &&
    kLogicOr,       // ||

    kQuestionMark,  // ?
    kColon,         // :
    kSemicolon,     // ;
    kEllipsis,      // ...

    kAssign,     // =
    kMulAssign,  // *=
    kDivAssign,  // /=
    kModAssign,  // %=
    kAddAssign,  // +=
    kSubAssign,  // -=
    kShlAssign,  // <<=
    kShrAssign,  // >>=
    kAndAssign,  // &=
    kXorAssign,  // ^=
    kOrAssign,   // |=

    kComma,  // ,

    kNone,
    kNewLine,
    kEnd,
    kInvalid,
  };

  Q_ENUM(Values)
};

using Tag = TokenTag::Values;

class Token {
 public:
  bool TagIs(Tag tag) const;
  void SetTag(Tag tag);

  std::string GetStr() const;
  void SetStr(const std::string &str);

  SourceLocation GetSourceLocation() const;
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
