//
// Created by kaiser on 2019/11/1.
//

#include "json_gen.h"

#include <cassert>
#include <fstream>

#include <QJsonArray>
#include <QJsonDocument>

namespace kcc {

void JsonGen::GenJson(const TranslationUnit* root,
                      const std::string& file_name) {
  Visit(*root);

  QJsonDocument document{result_};

  std::ofstream ofs{file_name};
  ofs << JsonGen::kBefore << document.toJson().toStdString() << JsonGen::kAfter
      << std::flush;
}

void JsonGen::Visit(const UnaryOpExpr& node) {
  QJsonObject root;
  root["name"] =
      node.KindStr().append(' ').append(TokenTag::ToQString(node.op_));

  QJsonArray children;
  node.expr_->Accept(*this);
  children.append(result_);
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const TypeCastExpr& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  node.expr_->Accept(*this);
  children.append(result_);

  QJsonObject type;
  type["name"] = QString::fromStdString("cast to: " + node.to_->ToString());
  children.append(type);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const BinaryOpExpr& node) {
  QJsonObject root;
  root["name"] =
      node.KindStr().append(' ').append(TokenTag::ToQString(node.op_));

  QJsonArray children;

  node.lhs_->Accept(*this);
  children.append(result_);

  node.rhs_->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ConditionOpExpr& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  node.cond_->Accept(*this);
  children.append(result_);

  node.lhs_->Accept(*this);
  children.append(result_);

  node.rhs_->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const FuncCallExpr& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  node.callee_->Accept(*this);
  children.append(result_);

  for (const auto& arg : node.args_) {
    arg->Accept(*this);
    children.append(result_);
  }
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ConstantExpr& node) {
  auto str{node.KindStr().append(": ")};

  if (node.GetType()->IsIntegerTy()) {
    str.append(QString::number(node.integer_val_));
  } else if (node.GetType()->IsFloatPointTy()) {
    str.append(QString::number(node.float_point_val_));
  } else {
    assert(false);
  }

  QJsonObject root;
  root["name"] = str;

  result_ = root;
}

void JsonGen::Visit(const StringLiteralExpr& node) {
  QJsonObject root;
  root["name"] =
      node.KindStr().append(": ").append(QString::fromStdString(node.val_));

  result_ = root;
}

void JsonGen::Visit(const IdentifierExpr& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;

  QJsonObject type;
  type["name"] = QString::fromStdString("type: " + node.GetType()->ToString());
  children.append(type);

  QJsonObject name;
  name["name"] = "name: " + QString::fromStdString(node.GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const EnumeratorExpr& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;

  QJsonObject name;
  name["name"] = "name: " + QString::fromStdString(node.GetName());
  children.append(name);

  QJsonObject value;
  value["name"] = QString::number(node.val_);
  children.append(value);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ObjectExpr& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;

  QJsonObject type;
  type["name"] = QString::fromStdString("type: " + node.GetType()->ToString());
  children.append(type);

  QJsonObject name;
  name["name"] = "name: " + QString::fromStdString(node.GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const StmtExpr& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  node.block_->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const LabelStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  QJsonObject name;
  name["name"] = QString::fromStdString("label: " + node.ident_->GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const CaseStmt& node) {
  QJsonObject root;

  if (!node.has_range_) {
    root["name"] =
        node.KindStr().append(' ').append(QString::number(node.case_value_));
  } else {
    root["name"] = node.KindStr()
                       .append(' ')
                       .append(QString::number(node.case_value_range_.first))
                       .append("to ")
                       .append(QString::number(node.case_value_range_.second));
  }

  QJsonArray children;
  if (node.block_) {
    node.block_->Accept(*this);
    children.append(result_);
  }
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const DefaultStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  if (node.block_) {
    node.block_->Accept(*this);
    children.append(result_);
  }
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const CompoundStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;

  for (const auto& item : node.stmts_) {
    item->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;
  result_ = root;
}

void JsonGen::Visit(const ExprStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  if (node.expr_) {
    node.expr_->Accept(*this);
    children.append(result_);
  } else {
    QJsonObject obj;
    obj["name"] = "empty stmt";
    children.append(obj);
  }
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const IfStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;

  node.cond_->Accept(*this);
  children.append(result_);

  node.then_block_->Accept(*this);
  children.append(result_);

  if (node.else_block_) {
    node.else_block_->Accept(*this);
    children.append(result_);
  }
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const SwitchStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  node.cond_->Accept(*this);
  children.append(result_);

  node.block_->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const WhileStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;

  node.cond_->Accept(*this);
  children.append(result_);

  node.block_->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const DoWhileStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;

  node.cond_->Accept(*this);
  children.append(result_);

  node.block_->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ForStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;

  if (node.init_) {
    node.init_->Accept(*this);
    children.append(result_);
  } else if (node.decl_) {
    node.decl_->Accept(*this);
    children.append(result_);
  }

  if (node.cond_) {
    node.cond_->Accept(*this);
    children.append(result_);
  }
  if (node.inc_) {
    node.inc_->Accept(*this);
    children.append(result_);
  }
  if (node.block_) {
    node.block_->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const GotoStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  QJsonObject name;
  name["name"] = QString::fromStdString("label: " + node.ident_->GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ContinueStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();
  result_ = root;
}

void JsonGen::Visit(const BreakStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();
  result_ = root;
}

void JsonGen::Visit(const ReturnStmt& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  if (node.expr_) {
    node.expr_->Accept(*this);
    children.append(result_);
  } else {
    QJsonObject obj;
    obj["name"] = "void";
    children.append(obj);
  }
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const TranslationUnit& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  for (const auto& item : node.ext_decls_) {
    item->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const Declaration& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;
  node.ident_->Accept(*this);
  children.append(result_);

  for (const auto& item : node.inits_) {
    QJsonObject obj;
    obj["name"] = QString::number(item.offset_)
                      .append(' ')
                      .append(QString::fromStdString(item.type_->ToString()));
    item.expr_->Accept(*this);
    QJsonArray arr;
    arr.append(result_);
    obj["children"] = arr;
    children.append(obj);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const FuncDef& node) {
  QJsonObject root;
  root["name"] = node.KindStr();

  QJsonArray children;

  node.ident_->Accept(*this);
  children.append(result_);

  node.body_->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

}  // namespace kcc
