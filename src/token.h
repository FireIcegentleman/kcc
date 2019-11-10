//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_TOKEN_H_
#define KCC_SRC_TOKEN_H_

#include <string>

#include <QMetaEnum>
#include <QObject>

#include "location.h"

namespace kcc {

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

    kPostfixPlusPlus,    // ++
    kPostfixMinusMinus,  // --

    kAmp,      // &
    kStar,     // *
    kPlus,     // +
    kMinus,    // -
    kTilde,    // ~
    kExclaim,  // !

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
    kSharp,
    kSharpSharp,

    kFunc,
    kExtension,  // __extension__
    kTypeof,     // typeof

    kNone,
    kEof,
    kInvalid,
  };

  Q_ENUM(Values)

  static std::string ToString(TokenTag::Values value);
};

using Tag = TokenTag::Values;

class Token {
 public:
  bool TagIs(Tag tag) const;
  void SetTag(Tag tag);
  Tag GetTag() const;

  std::string GetStr() const;
  void SetStr(const std::string &str);

  Location GetLoc() const;
  void SetLoc(const Location &loc);

  std::string ToString() const;

  bool IsEof() const;
  bool IsIdentifier() const;
  bool IsStringLiteral() const;
  bool IsConstant() const;
  bool IsIntegerConstant() const;
  bool IsFloatConstant() const;
  bool IsCharacterConstant() const;

  bool IsTypeSpecQual() const;
  bool IsDecl() const;

 private:
  Tag tag_{Tag::kNone};
  std::string str_;
  Location location_;
};

}  // namespace kcc

#endif  // KCC_SRC_TOKEN_H_
