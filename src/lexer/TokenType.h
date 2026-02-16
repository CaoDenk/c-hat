#pragma once

namespace c_hat {
namespace lexer {

// 词法单元类型
enum class TokenType {
  // 关键字
  Func,
  Class,
  Struct,
  Enum,
  Union,
  Interface,
  Module,
  Import,
  Export,
  Extension,
  Get,
  Set,
  Public,
  Private,
  Protected,
  Internal,
  Static,
  Const,
  Virtual,
  Override,
  Final,
  Abstract,
  Inline,
  Using,
  Try,
  Catch,
  Throw,
  Defer,
  Await,
  Yield,
  Comptime,
  Late,
  Var,
  Let,
  Void,
  Bool,
  Byte,
  SByte,
  Short,
  UShort,
  Int,
  UInt,
  Long,
  ULong,
  Float,
  Double,
  Fp16,
  Bf16,
  Char,
  True,
  False,
  Null,
  Self,
  Base,
  New,
  Delete,
  If,
  Else,
  Match,
  Case,
  Default,
  For,
  While,
  Do,
  Break,
  Continue,
  Return,
  Goto,
  As,
  Where,
  Requires,
  Concept,
  Operator,
  Mutable,
  Sizeof,
  Typeof,
  TypeAlias,
  Extern,
  LiteralView,

  // 标识符
  Identifier,

  // 字面量
  IntegerLiteral,
  FloatingLiteral,
  CharacterLiteral,
  StringLiteral,
  BooleanLiteral,

  // 操作符
  Plus,
  Minus,
  Multiply,
  Divide,
  Modulus,
  Power,
  And,
  Or,
  Xor,
  Not,
  BitNot,
  Shl,
  Shr,
  Lt,
  Le,
  Gt,
  Ge,
  Eq,
  Ne,
  LogicAnd,
  LogicOr,
  Assign,
  AddAssign,
  SubAssign,
  MulAssign,
  DivAssign,
  ModAssign,
  AndAssign,
  OrAssign,
  XorAssign,
  ShlAssign,
  ShrAssign,

  // 指针操作符
  AddressOf,   // ^ (前缀)
  Dereference, // ^ (后缀)

  // 其他操作符
  Question, // ?
  Tilde,    // ~
  Arrow,    // ->

  // 其他符号
  LParen,
  RParen, // ()
  LBrace,
  RBrace, // {}
  LBracket,
  RBracket, // []
  Dot,
  Comma,
  Semicolon,
  Colon,
  DoubleColon, // , ; : ::
  Ellipsis,    // ...
  Range,       // ..
  Dollar,      // $
  Bar,         // |
  FatArrow,    // =>
  At,          // @

  // 注释和空白
  LineComment,
  BlockComment,
  Whitespace,

  // 特殊
  EndOfFile
};

} // namespace lexer
} // namespace c_hat
