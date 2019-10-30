//
// Created by kaiser on 2019/10/31.
//

#include "dict.h"

namespace kcc {

KeywordsDictionary::KeywordsDictionary() {
  keywords_.insert({"auto", Tag::kAuto});
  keywords_.insert({"break", Tag::kBreak});
  keywords_.insert({"case", Tag::kCase});
  keywords_.insert({"char", Tag::kChar});
  keywords_.insert({"const", Tag::kConst});
  keywords_.insert({"continue", Tag::kContinue});
  keywords_.insert({"default", Tag::kDefault});
  keywords_.insert({"do", Tag::kDo});
  keywords_.insert({"double", Tag::kDouble});
  keywords_.insert({"else", Tag::kElse});
  keywords_.insert({"enum", Tag::kEnum});
  keywords_.insert({"extern", Tag::kExtern});
  keywords_.insert({"float", Tag::kFloat});
  keywords_.insert({"for", Tag::kFor});
  keywords_.insert({"goto", Tag::kGoto});
  keywords_.insert({"if", Tag::kIf});
  keywords_.insert({"inline", Tag::kInline});
  keywords_.insert({"int", Tag::kInt});
  keywords_.insert({"long", Tag::kLong});
  keywords_.insert({"register", Tag::kRegister});
  keywords_.insert({"restrict", Tag::kRestrict});
  keywords_.insert({"return", Tag::kReturn});
  keywords_.insert({"short", Tag::kShort});
  keywords_.insert({"signed", Tag::kSigned});
  keywords_.insert({"sizeof", Tag::kSizeof});
  keywords_.insert({"static", Tag::kStatic});
  keywords_.insert({"struct", Tag::kStruct});
  keywords_.insert({"switch", Tag::kSwitch});
  keywords_.insert({"typedef", Tag::kTypedef});
  keywords_.insert({"union", Tag::kUnion});
  keywords_.insert({"unsigned", Tag::kUnsigned});
  keywords_.insert({"void", Tag::kVoid});
  keywords_.insert({"volatile", Tag::kVolatile});
  keywords_.insert({"while", Tag::kWhile});
  keywords_.insert({"_Alignas", Tag::kAlignas});
  keywords_.insert({"_Alignof", Tag::kAlignof});
  keywords_.insert({"_Atomic", Tag::kAtomic});
  keywords_.insert({"_Bool", Tag::kBool});
  keywords_.insert({"_Complex", Tag::kComplex});
  keywords_.insert({"_Generic", Tag::kGeneric});
  keywords_.insert({"_Imaginary", Tag::kImaginary});
  keywords_.insert({"_Noreturn", Tag::kNoreturn});
  keywords_.insert({"_Static_assert", Tag::kStaticAssert});
  keywords_.insert({"_Thread_local", Tag::kThreadLocal});
}

Tag KeywordsDictionary::Find(const std::string& name) const {
  if (auto iter{keywords_.find(name)}; iter != std::end(keywords_)) {
    return iter->second;
  } else {
    return Tag::kIdentifier;
  }
}

}  // namespace kcc
