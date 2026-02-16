# C^ 语言 EBNF 文法

## 1. 基础规则

```ebnf
program         ::= declaration_list

// 声明列表
declaration_list ::= declaration | declaration_list declaration

// 声明
declaration     ::= attribute_list? variable_declaration
                   | attribute_list? function_declaration
                   | attribute_list? class_declaration
                   | attribute_list? struct_declaration
                   | attribute_list? enum_declaration
                   | attribute_list? import_declaration
                   | attribute_list? concept_declaration
                   | attribute_list? using_declaration
                   | attribute_list? module_declaration
                   | extern_declaration

// 外部声明块
extern_declaration ::= 'extern' string_literal '{' declaration_list '}'

// 属性列表
attribute_list  ::= attribute | attribute_list attribute
attribute       ::= '[' identifier ('(' argument_list? ')')? ']'

// 访问修饰符
access_modifier ::= 'public' | 'private' | 'protected' | 'internal'

// 变量声明
variable_declaration ::= access_modifier? 'const'? 'late'? type_specifier declarator_list ';'

// 声明符列表
declarator_list ::= declarator | declarator_list ',' declarator

// 类型列表 (用于元组)
type_list       ::= type_specifier ',' type_specifier
                   | type_list ',' type_specifier

// 声明符
declarator      ::= identifier
                  | identifier '!' // 绑定不可变修饰符 (let x 语法糖)
                  | identifier '=' initializer
                  | identifier '[' expression ']'
                  | identifier '^'

// 初始化器
initializer     ::= expression
                   | expansion_expression // 扩展操作符
                   | '[' initializer_list ']'      // 数组/切片初始化
                   | '[' initializer_list ',' ']'

// 初始化器列表
initializer_list ::= initializer_element | initializer_list ',' initializer_element

// 初始化元素
initializer_element ::= initializer
                      | identifier '=' initializer  // 命名初始化 (属性/字段赋值)

// 类型说明符
type_specifier  ::= base_type_specifier
                 | func_type_specifier

// 函数指针类型 (不捕获，薄指针)
func_type_specifier ::= 'func' '(' func_type_parameter_types? ')' '->' type_specifier
func_type_parameter_types ::= type_specifier | func_type_parameter_types ',' type_specifier

// 基础类型说明符
base_type_specifier ::= 'void'
                   | 'bool'
                   | 'byte' | 'sbyte'
                   | 'short' | 'ushort'
                   | 'int' | 'uint'
                   | 'long' | 'ulong'
                   | 'float' | 'double'
                   | 'fp16' | 'bf16'
                   | 'char'
                   | 'string'
                   | 'string_view'
                   | 'var'
                   | 'let'
                   | class_name
                   | struct_name
                   | enum_name
                   | namespace_name '::' type_name
                   | base_type_specifier '!'      // 不可变类型 (const view)
                   | base_type_specifier '~'      // 右值引用类型 (rvalue ref)
                   | base_type_specifier '[' comma_list? ']' // 切片/多维数组类型 (int[,] / int[])
                   | base_type_specifier '[' '$' ']' // 栈数组自动推导 (int[$])
                   | base_type_specifier '[' expression ']'
                   | base_type_specifier '^'
                   | base_type_specifier '&'      // 引用类型
                   | base_type_specifier '&' '~'  // 万能引用类型 (仅泛型推导上下文)

// 函数声明
function_declaration ::= function_specifier? 'func' function_name generic_parameter_list? '(' parameter_list? ')' return_type? noexcept_specifier? where_clause? function_body
                   | conversion_operator_declaration

// 转换运算符声明 (C++ 风格：不使用 func)
conversion_operator_declaration ::= function_specifier? 'operator' type_specifier '(' ')' function_body

// noexcept 说明符
noexcept_specifier ::= 'noexcept' ('(' expression ')')?

// 函数名
function_name   ::= identifier
                   | 'operator' op
                   | 'operator' '[' ']'           // 下标操作符
                   | 'operator' '(' ')'           // 调用操作符
                   | 'operator' string_literal identifier? // 自定义字面量 (operator "suffix" id 或 operator "prefix")
                   | 'operator' '!'               // 逻辑非
                   | 'operator' '~'               // 按位取反

// 函数说明符
function_specifier ::= access_modifier | 'static' | 'const' | 'virtual' | 'override' | 'final' | 'abstract' | 'delete' | 'implicit' | 'explicit'

// 参数列表
parameter_list  ::= parameter | parameter_list ',' parameter

// 参数
parameter       ::= attribute_list? type_specifier declarator
                   | attribute_list? type_specifier '...' declarator // 变参参数
                   | attribute_list? 'self' '!'?                     // 显式 self 参数
                   | attribute_list? 'self' '.' identifier           // 构造函数参数绑定 (self.field)
                   | 'void'                                          // 占位符参数 (用于操作符重载)

// 返回类型
return_type     ::= '->' type_specifier

// 函数体
function_body   ::= '{' statement_list '}'

// 语句列表
statement_list  ::= statement | statement_list statement

// 语句
statement       ::= expression_statement
                   | compound_statement
                   | selection_statement
                   | iteration_statement
                   | jump_statement
                   | labeled_statement
                   | try_statement
                   | catch_statement
                   | defer_statement
                   | comptime_statement

// 标号语句
labeled_statement ::= identifier ':' statement

// 编译期语句
comptime_statement ::= 'comptime' compound_statement
                   | 'comptime' 'for' '(' 'var' identifier ':' identifier ')' compound_statement
                   | 'comptime' 'for' '(' 'type' identifier ':' identifier ')' compound_statement

// 表达式语句
expression_statement ::= expression? ';'

// 复合语句
compound_statement ::= '{' statement_list '}'

// 选择语句
selection_statement ::= 'if' '(' expression ')' statement ('else' statement)?
                      | match_expression

// 匹配表达式
match_expression    ::= 'match' '(' expression ')' '{' match_arm_list '}'

// 匹配臂列表
match_arm_list  ::= match_arm | match_arm_list ',' match_arm

// 匹配臂
match_arm       ::= 'case' pattern '=>' expression
                   | 'case' pattern '=>' compound_statement
                   | 'default' '=>' expression
                   | 'default' '=>' compound_statement

// 模式
pattern         ::= literal
                   | identifier
                   | identifier '(' pattern_list? ')' // 解构模式: Case(x, y)
                   | '('_')'
                   | '_'

// 模式列表
pattern_list    ::= pattern | pattern_list ',' pattern

// 迭代语句
iteration_statement ::= 'for' '(' for_init? ';' expression? ';' for_update? ')' statement
                      | 'for' '(' type_specifier identifier ':' expression ')' statement
                      | 'while' '(' expression ')' statement
                      | 'do' statement 'while' '(' expression ')' ';'

// for 初始化
for_init        ::= expression_statement

// for 更新
for_update      ::= expression_statement

// 跳转语句
jump_statement  ::= 'break' ';'
                   | 'continue' ';'
                   | 'return' expression? ';'
                   | 'throw' expression ';'
                   | 'goto' identifier ';'

// try 语句
try_statement   ::= 'try' compound_statement catch_list

// catch 列表
catch_list      ::= catch_clause catch_list?

// catch 子句
catch_clause    ::= 'catch' '(' type_specifier identifier? ')' compound_statement

// defer 语句
defer_statement ::= 'defer' expression_statement
                   | 'defer' compound_statement

// 表达式
expression      ::= assignment_expression

// 赋值表达式
assignment_expression ::= conditional_expression
                        | logical_or_expression assignment_operator assignment_expression

// 折叠表达式
fold_expression ::= '(' expression op '...' ')' // 右折叠
                  | '(' '...' op expression ')' // 左折叠
                  | '(' expression op '...' op expression ')' // 二元折叠

// 扩展操作符
expansion_expression ::= expression '...'

// 运算符
op              ::= '+' | '-' | '*' | '/' | '%' | '&' | '|' | '^' | '<<' | '>>' | '&&' | '||' | '==' | '!=' | '<' | '>' | '<=' | '>='

// 赋值运算符
assignment_operator ::= '=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^=' | '<<=' | '>>='

// 条件表达式
conditional_expression ::= logical_or_expression ('?' expression ':' expression)?

// 逻辑或表达式
logical_or_expression ::= logical_and_expression | logical_or_expression '||' logical_and_expression

// 逻辑与表达式
logical_and_expression ::= inclusive_or_expression | logical_and_expression '&&' inclusive_or_expression

// 包含或表达式
inclusive_or_expression ::= exclusive_or_expression | inclusive_or_expression '|' exclusive_or_expression

// 排他或表达式
exclusive_or_expression ::= and_expression | exclusive_or_expression '^' and_expression

// 与表达式
and_expression  ::= equality_expression | and_expression '&' equality_expression

// 相等表达式
equality_expression ::= relational_expression | equality_expression '==' relational_expression | equality_expression '!=' relational_expression

// 关系表达式
relational_expression ::= shift_expression | relational_expression '<' shift_expression | relational_expression '>' shift_expression | relational_expression '<=' shift_expression | relational_expression '>=' shift_expression

// 移位表达式
shift_expression ::= additive_expression | shift_expression '<<' additive_expression | shift_expression '>>' additive_expression

// 加法表达式
additive_expression ::= multiplicative_expression | additive_expression '+' multiplicative_expression | additive_expression '-' multiplicative_expression

// 乘法表达式
multiplicative_expression ::= unary_expression | multiplicative_expression '*' unary_expression | multiplicative_expression '/' unary_expression | multiplicative_expression '%' unary_expression

// 一元表达式
unary_expression ::= power_expression
                   | '+' unary_expression
                   | '-' unary_expression
                   | '!' unary_expression
                   | '~' unary_expression
                   | '^' unary_expression // 取地址
                   | '&' unary_expression // 取引用
                   | 'delete' unary_expression
                   | 'delete' '[' ']' unary_expression
                   | '@' type_specifier
                   | 'await' unary_expression
                   | '(' type_specifier ')' unary_expression // 显式强转 (含 (T&~)x 用于转发)
                   | unary_expression '^' // 解引用
                   | unary_expression '?' // 错误传播 (Result Pattern)

// 幂运算表达式 (右结合)
power_expression ::= primary_expression
                   | primary_expression '**' power_expression

// 主表达式
primary_expression ::= identifier
                     | literal
                     | '(' expression ')'
                     | fold_expression // 折叠表达式
                     | primary_expression '(' argument_list? ')'
                     | primary_expression '.' identifier
                     | primary_expression '->' identifier
                     | primary_expression '[' expression ']'
                     | 'self'

                     | 'new' type_specifier ('(' argument_list? ')')?

// 参数列表
argument_list   ::= argument | argument_list ',' argument

// 参数
argument        ::= expression
                   | expansion_expression // 扩展操作符

// 字面量
literal         ::= integer_literal
                   | floating_literal
                   | character_literal
                   | string_literal
                   | boolean_literal
                   | 'null'

// 整数转换
integer_literal ::= decimal_literal
                  | hex_literal
                  | binary_literal

// 十进制字面量
decimal_literal ::= digit+ (integer_suffix)?

// 十六进制字面量
hex_literal     ::= '0x' hex_digit+ (integer_suffix)?
                  | '0X' hex_digit+ (integer_suffix)?

// 二进制字面量
binary_literal  ::= '0b' binary_digit+ (integer_suffix)?
                  | '0B' binary_digit+ (integer_suffix)?

// 十六进制数字
hex_digit       ::= digit | 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'A' | 'B' | 'C' | 'D' | 'E' | 'F'

// 二进制数字
binary_digit    ::= '0' | '1'

// 标识符
identifier      ::= letter | identifier letter | identifier digit | identifier '_'

// 字母
letter          ::= 'a' | 'b' | ... | 'z' | 'A' | 'B' | ... | 'Z'

// 数字
digit           ::= '0' | '1' | ... | '9'

// 类声明
class_declaration ::= 'class' identifier generic_parameter_list? inheritance_specifier? where_clause? class_body

// 类体
class_body      ::= '{' class_member_list '}'

// 类成员列表
class_member_list ::= class_member | class_member_list class_member

// 类成员
class_member    ::= variable_declaration
                   | function_declaration
                   | get_declaration          // 分离式 Getter
                   | set_declaration          // 分离式 Setter
                   | constructor_declaration
                   | destructor_declaration

// 分离式 Getter
get_declaration ::= attribute_list? access_modifier? 'get' identifier return_type? (function_body | '=>' expression ';')

// 分离式 Setter
set_declaration ::= attribute_list? access_modifier? 'set' identifier '(' parameter ')' (function_body | '=>' expression ';')

// 继承说明符
inheritance_specifier ::= ':' type_specifier

// 泛型参数列表
generic_parameter_list ::= '<' generic_parameters '>'

// 泛型参数
generic_parameters ::= generic_parameter | generic_parameters ',' generic_parameter
generic_parameter  ::= identifier | identifier ':' type_specifier

// where 子句
where_clause ::= 'where' constraint_list

// 约束列表
constraint_list ::= constraint | constraint_list ',' constraint
constraint      ::= type_specifier ':' type_specifier       // 继承约束: T : Interface
                  | identifier '<' type_specifier '>'       // Concept 约束: Concept<T>

// 属性访问器列表 (已移除，采用分离式设计)
// property_accessor_list ::= ...
// property_accessor ::= ...

// 构造函数声明
constructor_declaration ::= identifier '(' parameter_list? ')' initializer_list_specifier? function_body

// 初始化列表说明符 (构造函数)
initializer_list_specifier ::= ':' constructor_initializer_list

// 构造函数初始化列表
constructor_initializer_list ::= constructor_initializer | constructor_initializer_list ',' constructor_initializer
constructor_initializer ::= identifier '(' argument_list? ')'
                          | 'base' '(' argument_list? ')'  // 基类构造调用

// 析构函数声明
destructor_declaration ::= access_modifier? '~' identifier? '(' ')' (function_body | ';') // 仅支持带括号形式：~ClassName()

// 结构体声明
struct_declaration ::= 'struct' identifier generic_parameter_list? where_clause? struct_body

// 结构体体
struct_body     ::= '{' struct_member_list '}'

// 结构体成员列表
struct_member_list ::= struct_member | struct_member_list struct_member

// 结构体成员
struct_member   ::= variable_declaration
                   | bitfield_declaration
                   | function_declaration

// 位域声明
bitfield_declaration ::= type_specifier identifier ':' constant_expression ';'

// 枚举声明
enum_declaration ::= 'enum' identifier enum_body

// 枚举体
enum_body       ::= '{' enum_member_list '}'

// 枚举成员列表
enum_member_list ::= enum_member | enum_member_list ',' enum_member

// 枚举成员
enum_member     ::= identifier
                   | identifier '=' constant_expression

// 导入声明
import_declaration ::= 'import' module_path ';'

// 模块路径
module_path     ::= identifier | module_path '.' identifier

// 类型别名声明
using_declaration ::= access_modifier? 'using' identifier generic_parameter_list? '=' type_specifier ';'

// 概念声明
concept_declaration ::= 'concept' identifier generic_parameter_list? '=' constraint_expression ';'

// 约束表达式
constraint_expression ::= 'requires' '(' parameter_list? ')' '{' requirement_list '}'

// 需求列表
requirement_list ::= requirement | requirement_list requirement

// 需求
requirement     ::= compound_statement
                   | type_requirement
                   | expression_requirement

// 类型需求
type_requirement ::= type_specifier ';'

// 表达式需求
expression_requirement ::= '{' expression '}' '->' type_specifier ';'
                          | '{' expression '}' ';'


```

## 2. 指针相关规则

```ebnf
// 指针类型声明
declarator      ::= identifier '^' // 指针变量声明

// 取地址/解引用运算符：见上方 unary_expression（已包含 '^' 前缀取地址 与 '^' 后缀解引用）

// 函数指针类型：见上方 func_type_specifier，例如 func(int, int) -> int

// 多级指针
declarator      ::= identifier '^' '^'
                   | identifier '^' '^' '^'
```

## 3. 数组相关规则

```ebnf
// 固定大小数组 (支持多维 int[2, 3])
declarator      ::= identifier '[' expression_list ']'

// 动态切片 (支持多维 int[,])
declarator      ::= identifier '[' comma_list? ']'

// 逗号列表 (用于表示多维切片的秩)
comma_list      ::= ',' | comma_list ','

// 表达式列表
expression_list ::= expression | expression_list ',' expression

// 数组初始化
initializer     ::= '[' initializer_list ']'

// 范围操作符
expression      ::= expression '..' expression
                  | expression '..'
                  | '..' expression
                  | '..'

// 索引表达式 (支持 $ 倒序索引)
primary_expression ::= primary_expression '[' index_argument_list ']'

// 索引参数列表
index_argument_list ::= index_argument | index_argument_list ',' index_argument

// 索引参数
index_argument  ::= expression
                  | range_expression // 范围索引

// 范围表达式
range_expression ::= expression? '..' expression?

## 4. 函数相关规则

```ebnf
// 函数声明
function_declaration ::= function_specifier? 'func' '*'? function_name generic_parameter_list? '(' parameter_list? ')' return_type? where_clause? (function_body | '=>' expression ';')

// 函数体
function_body   ::= '{' statement_list '}'

// Lambda 表达式
primary_expression ::= '[' capture_list? ']' '(' parameter_list? ')' 'mutable'? '=>' expression
                     | '[' capture_list? ']' '(' parameter_list? ')' 'mutable'? '{' statement_list '}'

// 捕获列表
capture_list    ::= capture | capture_list ',' capture

// 捕获
capture         ::= identifier
                   | '&' identifier     // 引用捕获变量
                   | '='                // 值捕获默认 (显式)
                   | '&'                // 引用捕获默认 (显式)
                   | 'self'             // 捕获 self
```

## 5. 类与继承相关规则

```ebnf
// 类声明
class_declaration ::= 'class' identifier generic_parameter_list? inheritance_specifier? where_clause? class_body

/* 接口列表已移除
interface_list  ::= ...
*/

// 成员函数
function_declaration ::= function_specifier? 'func' function_name '(' 'self' '&' ',' parameter_list? ')' return_type? function_body
                       | function_specifier? 'func' function_name '(' 'self' '!' ',' parameter_list? ')' return_type? function_body
                       | function_specifier? 'func' function_name '(' 'self' '~' ',' parameter_list? ')' return_type? function_body

// 覆盖函数
function_declaration ::= function_specifier? 'override' 'func' function_name '(' parameter_list? ')' return_type? function_body

// 虚函数
function_declaration ::= function_specifier? 'virtual' 'func' function_name '(' parameter_list? ')' return_type? function_body

// 最终函数
function_declaration ::= function_specifier? 'final' 'func' function_name '(' parameter_list? ')' return_type? function_body

// 抽象函数
function_declaration ::= function_specifier? 'abstract' 'func' function_name '(' parameter_list? ')' return_type? ';'
```

## 6. 泛型与元编程相关规则

```ebnf
// 泛型类型引用
type_specifier  ::= identifier '<' generic_argument_list '>'

// 泛型参数列表
generic_parameter_list ::= generic_parameter | generic_parameter_list ',' generic_parameter

// 泛型参数
generic_parameter ::= identifier                   // 类型参数 T
                   | 'const' identifier ':' type_specifier // 值参数 N (NTTP)
                   | identifier '...'              // 变参泛型 T...

// 约束列表 (已移除 T: A 语法)
// constraint_list ::= ...

// 泛型实参列表
generic_argument_list ::= type_specifier | generic_argument_list ',' type_specifier
                      | constant_expression
                      | type_specifier '...' // 变参展开

// where 子句 (布尔/命名约束)
where_clause ::= 'where' where_predicate_list
where_predicate_list ::= where_predicate | where_predicate_list ',' where_predicate
where_predicate ::= identifier '<' generic_argument_list '>'           // Concept 检查: Comparable<T>
                 | type_specifier '::' identifier '==' type_specifier // 关联类型约束
                 | expression                                         // 布尔表达式

// requires 子句 (结构性约束)
requires_clause ::= 'requires' requires_expression
requires_expression ::= '(' parameter_list? ')' '{' requirement_list '}'
requirement_list ::= requirement | requirement_list requirement
requirement ::= '{' expression '}' '->' type_specifier ';' // 复合要求
             | 'typename' type_specifier ';'               // 类型要求

// Concept 声明
concept_declaration ::= 'public'? 'concept' identifier '<' generic_parameter_list '>' '=' 'requires' requires_expression ';'

// 泛型函数声明
function_declaration ::= function_specifier? 'func' function_name '<' generic_parameter_list '>' '(' parameter_list? ')' return_type? where_clause? requires_clause? function_body
```

## 7. 内存管理相关规则

```ebnf
// 动态分配
primary_expression ::= 'new' type_specifier ('(' argument_list? ')')?

// 动态数组分配
primary_expression ::= 'new' type_specifier '[' expression ']'

// 智能指针
type_specifier  ::= 'unique_ptr' '<' type_specifier '>'
                   | 'shared_ptr' '<' type_specifier '>'
                   | 'weak_ptr' '<' type_specifier '>'
```

## 8. 错误处理相关规则

```ebnf
// try 语句
try_statement   ::= 'try' compound_statement catch_list

// catch 列表
catch_list      ::= catch_clause | catch_list catch_clause

// catch 子句
catch_clause    ::= 'catch' '(' type_specifier identifier ')' compound_statement

// throw 语句
jump_statement  ::= 'throw' expression ';'
```

## 9. 模块系统相关规则

```ebnf
// 模块声明
module_declaration ::= 'module' identifier ('.' identifier)* ';'

// 导入声明
import_declaration ::= 'public'? 'import' identifier ('.' identifier)* ('as' identifier)? ';'
```

## 10. 延迟初始化相关规则

```ebnf
// 延迟初始化变量
variable_declaration ::= 'lazy' type_specifier declarator_list ';'
```

## 11. 静态反射相关规则

```ebnf
// 反射表达式 (获取元数据)

// 类型操作符 (获取类型)
type_specifier  ::= 'typeof' '(' expression ')'

// 成员反射访问
primary_expression ::= primary_expression '.' '@' '[' expression ']'

// 编译期常量
expression      ::= 'comptime' '(' expression ')'

// 属性
type_specifier  ::= '[' identifier '(' argument_list? ')' ']' type_specifier
```

## 12. 协程相关规则

```ebnf
// 协程函数 (自动识别，无需async关键字)
function_declaration ::= function_specifier? 'func' identifier '(' parameter_list? ')' return_type? function_body

// await 表达式

// yield 语句
jump_statement  ::= 'yield' expression? ';'
```

## 13. 操作符优先级（从高到低）

1. 括号：`()`
2. 成员访问：`.` `->`
3. 数组访问：`[]`
4. 后缀解引用：`^`
5. 前缀取地址/取引用：`^` `&`
6. 一元运算符：`+` `-` `!` `~`
7. 乘法/除法/取模：`*` `/` `%`
8. 加法/减法：`+` `-`
9. 移位运算符：`<<` `>>`
10. 关系运算符：`<` `>` `<=` `>=`
11. 相等运算符：`==` `!=`
12. 按位与：`&`
13. 按位异或：`^`
14. 按位或：`|`
15. 逻辑与：`&&`
16. 逻辑或：`||`
17. 条件运算符：`?:`
18. 赋值运算符：`=` `+=` `-=` `*=` `/=` `%=` `&=` `|=` `^=` `<<=` `>>=`
19. 逗号运算符：`,`

## 14. 语法特性总结

### 14.1 指针与引用语法
- 指针声明：`int^ ptr`
- 取址：`^var`
- 取引用：`&var`
- 解引用：`ptr^`
- 引用声明：`int& ref` / `int!& cref` (不可变引用)
- 万能引用：`T&~` (仅泛型推导上下文)
- 多级指针：`int^^^ ppp`

### 14.2 数组语法
- 固定大小数组：`int[5] arr`
- 动态切片：`int[] slice`
- 栈数组推导：`int[$] arr = [1, 2, 3]`
- 多维数组：`int[,] matrix`
- 数组初始化：`int[5] arr = [1, 2, 3, 4, 5]`
- 范围操作：`arr[0..3]`

### 14.3 函数语法
- 标准函数：`func add(int a, int b) -> int { return a + b; }`
- 单表达式函数：`func add(int a, int b) -> int => a + b;`
- Lambda 表达式：`[=](int a, int b) => a + b` (显式捕获)
- 成员函数：`func print(self!) { ... }`

### 14.4 类语法
- 类声明：`class Person { ... }`
- 继承：`class Student : Person { ... }`
- 约束：`class Student<T> where Comparable<T> { ... }`
- 属性：`public get Age => _age;` / `public set Age(int v) => _age = v;`

### 14.5 其他特性
- 类型推导：`var x = 100; let y = 200;`
- 类型别名：`using string_view = byte![];`
- 模式匹配：`match (code) { case 200 => "OK", default => "Unknown" }`
- 延迟初始化：`lazy Database db;`
- 智能指针：`unique_ptr<int> p = new int(42);`
- 静态反射：`TypeInfo t = @int;`
- 类型获取：`typeof(x) y = x;`
- 协程：`func fetch() -> string { ... }` (自动识别)
- 变参泛型：`func print_all<T...>(T... args) { ... }`
- 编译期循环：`comptime for (var arg : args) { ... }`
- 包方法：`args.map(...).reduce(...)`
- 约束：`requires { {x+y}->T; }` / `where Comparable<T>`
- Concept：`concept Addable<T> = ...;`

## 15. 解析器实现注意事项

1. **符号歧义处理**：
   - `^`：前缀(取址) vs 后缀(指针类型/解引用) vs 二元(异或)
   - `&`：前缀(取引用) vs 二元(按位与)
   - `<` / `>`：关系运算符 vs 泛型括号


2. **数组语法处理**：区分固定大小数组和动态切片，处理数组初始化和访问。

3. **优先级处理**：实现操作符优先级和结合性，特别是 `^` 符号的前后缀优先级。

4. **类型系统**：构建符号表，处理类型推导、泛型和继承关系。

5. **错误处理**：提供清晰的语法错误信息，包括位置和建议。

6. **性能优化**：使用高效的词法分析和语法分析算法，如递归下降或 LR 解析。

7. **代码生成**：为后续的 LLVM IR 生成做准备，构建抽象语法树 (AST)。

8. **内存管理**：正确处理语法树的内存分配和释放，避免内存泄漏。

9. **测试覆盖**：编写全面的测试用例，覆盖所有语法特性和边界情况。

10. **文档维护**：保持 EBNF 文法与实际语法实现的一致性，及时更新文档。

11. **变参泛型处理**：
    - 正确识别和解析参数包声明：`T...`
    - 处理编译期循环：`comptime for`
    - 解析折叠表达式：`(args + ...)`
    - 处理扩展操作符：`args...`
    - 构建参数包的抽象语法树表示

12. **编译期计算**：
    - 实现 `comptime` 关键字的语义，支持编译期常量计算
    - 处理编译期类型信息，支持静态反射
    - 优化编译期循环展开，生成高效的代码

13. **泛型约束检查**：
    - 实现 `where` 子句和 `requires` 表达式的约束检查
    - 在编译期验证泛型参数是否满足约束条件
    - 提供清晰的约束违反错误信息

14. **代码膨胀控制**：
    - 实现泛型具体化的缓存机制，避免重复生成相同的代码
    - 提供编译选项控制代码膨胀程度
    - 优化模板实例化的编译时间
