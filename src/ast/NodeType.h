#pragma once

namespace c_hat {
namespace ast {

// 基础节点类型
enum class NodeType {
  // 表达式
  Expression,
  BinaryExpr,
  UnaryExpr,
  Literal,
  Identifier,
  CallExpr,
  MemberExpr,
  SubscriptExpr,
  ThisExpr,
  SelfExpr,
  SuperExpr,
  NewExpr,
  DeleteExpr,
  FoldExpr,
  ExpansionExpr,
  ArrayInitExpr,
  StructInitExpr,
  TupleExpr,
  LambdaExpr,

  // 声明
  Declaration,
  VariableDecl,
  TupleDestructuringDecl,
  FunctionDecl,
  ClassDecl,
  StructDecl,
  EnumDecl,
  NamespaceDecl,
  ModuleDecl,
  ImportDecl,
  ExportDecl,
  ExtensionDecl,
  GetterDecl,
  SetterDecl,
  TypeAliasDecl,
  ExternDecl,

  // 语句
  Statement,
  ExprStmt,
  CompoundStmt,
  IfStmt,
  MatchStmt,
  ForStmt,
  WhileStmt,
  DoWhileStmt,
  BreakStmt,
  ContinueStmt,
  ReturnStmt,
  GotoStmt,
  LabelStmt,
  ThrowStmt,
  TryStmt,
  CatchStmt,
  DeferStmt,
  YieldStmt,
  ComptimeStmt,

  // 类型
  Type,
  PrimitiveType,
  NamedType,
  ArrayType,
  RectangularArrayType,
  SliceType,
  RectangularSliceType,
  PointerType,
  ReferenceType,
  FunctionType,
  ClassType,
  StructType,
  EnumType,
  GenericType,
  ReadonlyType,
  TupleType,

  // 其他
  Program,
  Parameter,
  Argument,
  PropertyDecl,
  ConstructorDecl,
  DestructorDecl,
  EnumMember,
  MatchArm,
  Pattern,
  CaptureList,
  Capture,
  TemplateParameter,
  TemplateArgument,
  WhereClause,
  RequiresClause,
  RequiresExpression,
  Requirement
};

} // namespace ast
} // namespace c_hat
