//
// Created by kaiser on 2019/11/1.
//

#ifndef KCC_SRC_SCOPE_H_
#define KCC_SRC_SCOPE_H_

#include <string>
#include <unordered_map>

#include "ast.h"
#include "token.h"

namespace kcc {

// C 拥有四种作用域：
// 块作用域
// 文件作用域
// 函数作用域
// 函数原型作用域
enum ScopeType { kBlock, kFile, kFunc, kFuncProto };

// 1) 标号命名空间：所有声明为标号的标识符。
// 2) 标签名：所有声明为 struct 、 union 及枚举类型名称的标识符。
//    注意所有这三种标签共享同一命名空间。
// 3) 成员名：所有声明为至少为一个 struct 或 union 成员的标识符。
//    每个结构体和联合体引入它自己的这种命名空间。
// 4) 所有其他标识符，称之为通常标识符以别于 (1-3)
//    （函数名、对象名、 typedef 名、枚举常量）。

// 在查找点，根据使用方式确定标识符所属的命名空间：
// 1) 作为 goto 语句运算数出现的标识符，会在标号命名空间中查找。
// 2) 跟随关键词 struct 、 union 或 enum 的标识符，会在标签命名空间中查找。
// 3) 跟随成员访问或通过指针的成员访问运算符的标识符，会在类型成员命名空间中查找
//    该类型由成员访问运算符左运算数确定。
// 4) 所有其他标识符，会在通常命名空间中查找。
class Scope {
 public:
  static Scope* Get(Scope* parent, enum ScopeType type);

  void PrintCurrScope() const;

  auto begin() { return std::begin(normal_); }
  auto end() { return std::end(normal_); }

  void InsertTag(const std::string& name, IdentifierExpr* ident);
  void InsertNormal(const std::string& name, IdentifierExpr* ident);

  IdentifierExpr* FindTag(const std::string& name);
  IdentifierExpr* FindNormal(const std::string& name);
  IdentifierExpr* FindTagInCurrScope(const std::string& name);
  IdentifierExpr* FindNormalInCurrScope(const std::string& name);

  IdentifierExpr* FindTag(const Token& tok);
  IdentifierExpr* FindNormal(const Token& tok);
  IdentifierExpr* FindTagInCurrScope(const Token& tok);
  IdentifierExpr* FindNormalInCurrScope(const Token& tok);

  std::unordered_map<std::string, IdentifierExpr*> AllTagInCurrScope() const;
  Scope* GetParent();

  bool IsFileScope() const;

 private:
  Scope(Scope* parent, enum ScopeType type) : parent_{parent}, type_{type} {}

  Scope* parent_;
  enum ScopeType type_;

  // struct / union / enum 的名字
  std::unordered_map<std::string, IdentifierExpr*> tags_;
  // 函数 / 对象 / typedef名 / 枚举常量
  std::unordered_map<std::string, IdentifierExpr*> normal_;
};

}  // namespace kcc

#endif  // KCC_SRC_SCOPE_H_
