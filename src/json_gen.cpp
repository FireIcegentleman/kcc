//
// Created by kaiser on 2019/11/1.
//

#include "json_gen.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <fstream>

namespace kcc {

void JsonGen::GenJson(const std::shared_ptr<TranslationUnit>& root,
                      const std::string& file_name) {
  Visit(*root);

  static std::string_view before(
      "<!DOCTYPE html>\n"
      "<meta charset=\"UTF-8\">\n"
      "<style>\n"
      "    .node circle {\n"
      "        fill: #fff;\n"
      "        stroke: steelblue;\n"
      "        stroke-width: 5px;\n"
      "    }\n"
      "\n"
      "    .node text {\n"
      "        font: 12px sans-serif;\n"
      "    }\n"
      "\n"
      "    .link {\n"
      "        fill: none;\n"
      "        stroke: #ccc;\n"
      "        stroke-width: 5px;\n"
      "    }\n"
      "</style>\n"
      "\n"
      "<body>\n"
      "\n"
      "    <!-- load the d3.js library -->\n"
      "    <script src=\"https://d3js.org/d3.v4.min.js\"></script>\n"
      "    <script>\n"
      "    var treeData =");

  static std::string_view after(
      ";// Set the dimensions and margins of the diagram\n"
      "    var margin = {top: 20, right: 90, bottom: 30, left: 90},\n"
      "        width = 8000 - margin.left - margin.right,\n"
      "        height = 1500 - margin.top - margin.bottom;\n"
      "\n"
      "    // append the svg object to the body of the page\n"
      "    // appends a 'group' element to 'svg'\n"
      "    // moves the 'group' element to the top left margin\n"
      "    var svg = d3.select(\"body\").append(\"svg\")\n"
      "        .attr(\"width\", width + margin.right + margin.left)\n"
      "        .attr(\"height\", height + margin.top + margin.bottom)\n"
      "        .append(\"g\")\n"
      "        .attr(\"transform\", \"translate(\"\n"
      "            + margin.left + \",\" + margin.top + \")\");\n"
      "\n"
      "    var i = 0,\n"
      "        duration = 750,\n"
      "        root;\n"
      "\n"
      "    // declares a tree layout and assigns the size\n"
      "    var treemap = d3.tree().size([height, width]);\n"
      "\n"
      "    // Assigns parent, children, height, depth\n"
      "    root = d3.hierarchy(treeData, function (d) {\n"
      "        return d.children;\n"
      "    });\n"
      "    root.x0 = height / 2;\n"
      "    root.y0 = 0;\n"
      "\n"
      "    // Collapse after the second level\n"
      "    root.children.forEach(collapse);\n"
      "\n"
      "    update(root);\n"
      "\n"
      "    // Collapse the node and all it's children\n"
      "    function collapse(d) {\n"
      "        if (d.children) {\n"
      "            d._children = d.children\n"
      "            d._children.forEach(collapse)\n"
      "            d.children = null\n"
      "        }\n"
      "    }\n"
      "\n"
      "    function update(source) {\n"
      "\n"
      "        // Assigns the x and y position for the nodes\n"
      "        var treeData = treemap(root);\n"
      "\n"
      "        // Compute the new tree layout.\n"
      "        var nodes = treeData.descendants(),\n"
      "            links = treeData.descendants().slice(1);\n"
      "\n"
      "        // Normalize for fixed-depth.\n"
      "        nodes.forEach(function (d) {\n"
      "            d.y = d.depth * 180\n"
      "        });\n"
      "\n"
      "        // ****************** Nodes section "
      "***************************\n"
      "\n"
      "        // Update the nodes...\n"
      "        var node = svg.selectAll('g.node')\n"
      "            .data(nodes, function (d) {\n"
      "                return d.id || (d.id = ++i);\n"
      "            });\n"
      "\n"
      "        // Enter any new modes at the parent's previous position.\n"
      "        var nodeEnter = node.enter().append('g')\n"
      "            .attr('class', 'node')\n"
      "            .attr(\"transform\", function (d) {\n"
      "                return \"translate(\" + source.y0 + \",\" + source.x0 + "
      "\")\";\n"
      "            })\n"
      "            .on('click', click);\n"
      "\n"
      "        // Add Circle for the nodes\n"
      "        nodeEnter.append('circle')\n"
      "            .attr('class', 'node')\n"
      "            .attr('r', 1e-6)\n"
      "            .style(\"fill\", function (d) {\n"
      "                return d._children ? \"lightsteelblue\" : \"#fff\";\n"
      "            });\n"
      "\n"
      "        // Add labels for the nodes\n"
      "        nodeEnter.append('text')\n"
      "            .attr(\"dy\", \".35em\")\n"
      "            .attr(\"x\", function (d) {\n"
      "                return d.children || d._children ? -13 : 13;\n"
      "            })\n"
      "            .attr(\"text-anchor\", function (d) {\n"
      "                return d.children || d._children ? \"end\" : "
      "\"start\";\n"
      "            })\n"
      "            .text(function (d) {\n"
      "                return d.data.name;\n"
      "            });\n"
      "\n"
      "        // UPDATE\n"
      "        var nodeUpdate = nodeEnter.merge(node);\n"
      "\n"
      "        // Transition to the proper position for the node\n"
      "        nodeUpdate.transition()\n"
      "            .duration(duration)\n"
      "            .attr(\"transform\", function (d) {\n"
      "                return \"translate(\" + d.y + \",\" + d.x + \")\";\n"
      "            });\n"
      "\n"
      "        // Update the node attributes and style\n"
      "        nodeUpdate.select('circle.node')\n"
      "            .attr('r', 10)\n"
      "            .style(\"fill\", function (d) {\n"
      "                return d._children ? \"lightsteelblue\" : \"#fff\";\n"
      "            })\n"
      "            .attr('cursor', 'pointer');\n"
      "\n"
      "\n"
      "        // Remove any exiting nodes\n"
      "        var nodeExit = node.exit().transition()\n"
      "            .duration(duration)\n"
      "            .attr(\"transform\", function (d) {\n"
      "                return \"translate(\" + source.y + \",\" + source.x + "
      "\")\";\n"
      "            })\n"
      "            .remove();\n"
      "\n"
      "        // On exit reduce the node circles size to 0\n"
      "        nodeExit.select('circle')\n"
      "            .attr('r', 1e-6);\n"
      "\n"
      "        // On exit reduce the opacity of text labels\n"
      "        nodeExit.select('text')\n"
      "            .style('fill-opacity', 1e-6);\n"
      "\n"
      "        // ****************** links section "
      "***************************\n"
      "\n"
      "        // Update the links...\n"
      "        var link = svg.selectAll('path.link')\n"
      "            .data(links, function (d) {\n"
      "                return d.id;\n"
      "            });\n"
      "\n"
      "        // Enter any new links at the parent's previous position.\n"
      "        var linkEnter = link.enter().insert('path', \"g\")\n"
      "            .attr(\"class\", \"link\")\n"
      "            .attr('d', function (d) {\n"
      "                var o = {x: source.x0, y: source.y0}\n"
      "                return diagonal(o, o)\n"
      "            });\n"
      "\n"
      "        // UPDATE\n"
      "        var linkUpdate = linkEnter.merge(link);\n"
      "\n"
      "        // Transition back to the parent element position\n"
      "        linkUpdate.transition()\n"
      "            .duration(duration)\n"
      "            .attr('d', function (d) {\n"
      "                return diagonal(d, d.parent)\n"
      "            });\n"
      "\n"
      "        // Remove any exiting links\n"
      "        var linkExit = link.exit().transition()\n"
      "            .duration(duration)\n"
      "            .attr('d', function (d) {\n"
      "                var o = {x: source.x, y: source.y}\n"
      "                return diagonal(o, o)\n"
      "            })\n"
      "            .remove();\n"
      "\n"
      "        // Store the old positions for transition.\n"
      "        nodes.forEach(function (d) {\n"
      "            d.x0 = d.x;\n"
      "            d.y0 = d.y;\n"
      "        });\n"
      "\n"
      "        // Creates a curved (diagonal) path from parent to the child "
      "nodes\n"
      "        function diagonal(s, d) {\n"
      "\n"
      "            path = `M ${s.y} ${s.x}\n"
      "            C ${(s.y + d.y) / 2} ${s.x},\n"
      "              ${(s.y + d.y) / 2} ${d.x},\n"
      "              ${d.y} ${d.x}`\n"
      "\n"
      "            return path\n"
      "        }\n"
      "\n"
      "        // Toggle children on click.\n"
      "        function click(d) {\n"
      "            if (d.children) {\n"
      "                d._children = d.children;\n"
      "                d.children = null;\n"
      "            } else {\n"
      "                d.children = d._children;\n"
      "                d._children = null;\n"
      "            }\n"
      "            update(d);\n"
      "        }\n"
      "    }\n"
      "\n"
      "    </script>\n"
      "</body>");

  QJsonDocument document{result_};

  std::ofstream ofs{file_name};
  ofs << before << document.toJson().toStdString() << after;
}

void JsonGen::Visit(const UnaryOpExpr& node) {
  QJsonObject root;
  root["name"] =
      AstNodeTypes::ToString(node.Kind())
          .append(' ')
          .append(QString::fromStdString(TokenTag::ToString(node.op_)));

  QJsonArray children;
  node.expr_->Accept(*this);
  children.append(result_);
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const BinaryOpExpr& node) {
  QJsonObject root;
  root["name"] =
      AstNodeTypes::ToString(node.Kind())
          .append(' ')
          .append(QString::fromStdString(TokenTag::ToString(node.op_)));

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
  root["name"] = AstNodeTypes::ToString(node.Kind());

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

void JsonGen::Visit(const TypeCastExpr& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;
  node.expr_->Accept(*this);
  children.append(result_);

  QJsonObject type;
  type["name"] = QString::fromStdString("cast to: " + node.to_->ToString());
  children.append(type);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const Constant& node) {
  auto str{AstNodeTypes::ToString(node.Kind()).append(' ')};
  if (node.GetQualType()->IsIntegerTy()) {
    str.append(QString::fromStdString(std::to_string(node.integer_val_)));
  } else if (node.GetQualType()->IsFloatPointTy()) {
    str.append(QString::fromStdString(std::to_string(node.float_point_val_)));
  } else {
    str.append(QString::fromStdString(node.str_val_));
  }

  QJsonObject root;
  root["name"] = str;

  result_ = root;
}

void JsonGen::Visit(const Enumerator& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind())
                     .append(' ')
                     .append(QString::fromStdString(std::to_string(node.val_)));

  result_ = root;
}

void JsonGen::Visit(const CompoundStmt& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;

  for (const auto& item : node.stmts_) {
    item->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;
  result_ = root;
}

void JsonGen::Visit(const IfStmt& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());
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

void JsonGen::Visit(const ReturnStmt& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;
  if (node.expr_) {
    node.expr_->Accept(*this);
    children.append(result_);
  }
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const LabelStmt& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;
  QJsonObject name;
  name["name"] = QString::fromStdString("label: " + node.ident_->GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const FuncCallExpr& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

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

void JsonGen::Visit(const Identifier& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;

  QJsonObject type;
  type["name"] =
      QString::fromStdString("type: " + node.GetQualType()->ToString());
  children.append(type);

  QJsonObject name;
  name["name"] = "name: " + QString::fromStdString(node.GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const Object& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;

  QJsonObject type;
  type["name"] =
      QString::fromStdString("type: " + node.GetQualType()->ToString());
  children.append(type);

  QJsonObject name;
  name["name"] = "name: " + QString::fromStdString(node.GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const TranslationUnit& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;

  for (const auto& item : node.ext_decls_) {
    item->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;
  result_ = root;
}

void JsonGen::Visit(const BreakStmt& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());
  result_ = root;
}

void JsonGen::Visit(const ContinueStmt& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());
  result_ = root;
}

void JsonGen::Visit(const GotoStmt& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;
  QJsonObject name;
  name["name"] = QString::fromStdString("label: " + node.ident_->GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

// FIXME
void JsonGen::Visit(const Declaration& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;

  QJsonObject type;
  type["name"] = QString::fromStdString(
      "type: " + node.GetIdent()->GetQualType()->ToString());
  children.append(type);

  QJsonObject name;
  name["name"] = "name: " + QString::fromStdString(node.GetIdent()->GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const FuncDef& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;

  node.ident_->Accept(*this);
  children.append(result_);

  node.body_->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ExprStmt& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());
  QJsonArray children;

  node.expr_->Accept(*this);
  children.append(result_);
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const WhileStmt& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());
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
  root["name"] = AstNodeTypes::ToString(node.Kind());
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
  root["name"] = AstNodeTypes::ToString(node.Kind());
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

void JsonGen::Visit(const CaseStmt& node) {
  QJsonObject root;

  if (!node.has_range_) {
    root["name"] =
        AstNodeTypes::ToString(node.Kind()) +
        QString::fromStdString(":" + std::to_string(node.case_value_));
  } else {
    root["name"] = AstNodeTypes::ToString(node.Kind()) +
                   QString::fromStdString(
                       ":" + std::to_string(node.case_value_range_.first) +
                       " to " + std::to_string(node.case_value_range_.second));
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
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;
  if (node.block_) {
    node.block_->Accept(*this);
    children.append(result_);
  }
  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const SwitchStmt& node) {
  QJsonObject root;
  root["name"] = AstNodeTypes::ToString(node.Kind());

  QJsonArray children;
  node.cond_->Accept(*this);
  children.append(result_);

  node.block_->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

}  // namespace kcc
