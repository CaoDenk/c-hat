// IntegrationTest.cpp - 端到端编译流水线测试
// 测试 parser → semantic → LLVM codegen → JIT 执行 完整链路
#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include "../src/semantic/ModuleLoader.h"
#include "../src/llvm/LLVMCodeGenerator.h"
#include <catch2/catch_test_macros.hpp>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace c_hat;

static int compileAndRun(const std::string &source) {
  parser::Parser parser(source);
  auto program = parser.parseProgram();
  if (!program) return -1;

  semantic::SemanticAnalyzer analyzer("", false);
  analyzer.analyze(*program);
  if (analyzer.hasError()) return -2;

  llvm_codegen::LLVMCodeGenerator codeGen("test_module");
  codeGen.setSemanticModel(&analyzer.getSemanticModel());
  codeGen.generate(std::move(program));
  if (!codeGen.verifyIR()) return -3;
  if (!codeGen.hasJIT()) return -4;

  codeGen.addExternalSymbol("printf", (void *)&std::printf);
  return codeGen.runJIT("main");
}


static int compileAndRunWithModules(const std::string &source,
                                    const std::string &moduleDir) {
  parser::Parser parser(source);
  auto program = parser.parseProgram();
  if (!program) return -1;

  semantic::ModuleLoader moduleLoader(std::vector<std::string>{moduleDir});
  semantic::loadImportsInto(*program, moduleLoader);

  semantic::SemanticAnalyzer analyzer(std::vector<std::string>{moduleDir}, false);
  analyzer.setModuleFunctionAliases(moduleLoader.getFunctionAliases());
  analyzer.analyze(*program);
  if (analyzer.hasError()) return -2;

  llvm_codegen::LLVMCodeGenerator codeGen("test_module_imports");
  codeGen.setSemanticModel(&analyzer.getSemanticModel());
  codeGen.generate(std::move(program));
  if (!codeGen.verifyIR()) return -3;
  if (!codeGen.hasJIT()) return -4;
  return codeGen.runJIT("main");
}

TEST_CASE("Integration: main returns constant", "[integration][e2e]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    return 42;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: logical short circuit", "[integration][logic]") {
  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "func touch(Box^ b) -> bool { b->value = b->value + 1; return true; }\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    if (false && touch(b)) { return 1; }\n"
      "    if (true || touch(b)) { }\n"
      "    return b->value + 42;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    long a = 4;\n"
      "    long b = 8;\n"
      "    if (a == 4 && b == 8) { return 42; }\n"
      "    return 0;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: sizeof", "[integration][sizeof]") {
  REQUIRE(compileAndRun(
      "struct P { int x; int y; }\n"
      "func main() -> int {\n"
      "    long a = sizeof(int);\n"
      "    long b = sizeof(long);\n"
      "    long c = sizeof(int[3]);\n"
      "    long d = sizeof(P);\n"
      "    if (a != 4) { return 1; }\n"
      "    if (b != 8) { return 2; }\n"
      "    if (c != 12) { return 3; }\n"
      "    if (d != 8) { return 4; }\n"
      "    return 42;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: computed value", "[integration][e2e]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var x = 10;\n"
      "    var y = 32;\n"
      "    return x + y;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: nested expression", "[integration][e2e]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    return (1 + 2) * (3 + 4) - 5;\n"
      "}\n") == 16);
}

TEST_CASE("Integration: if-else", "[integration][e2e]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var x = 5;\n"
      "    if (x > 3) {\n"
      "        return 100;\n"
      "    } else {\n"
      "        return -1;\n"
      "    }\n"
      "}\n") == 100);
}

TEST_CASE("Integration: function call", "[integration][e2e]") {
  REQUIRE(compileAndRun(
      "func add(int a, int b) -> int {\n"
      "    return a + b;\n"
      "}\n"
      "func main() -> int {\n"
      "    var r = add(20, 22);\n"
      "    return r;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: compound assignment and increment", "[integration][ops]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var x = 40;\n"
      "    x += 2;\n"
      "    return x;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var i = 0;\n"
      "    i++;\n"
      "    ++i;\n"
      "    return i;\n"
      "}\n") == 2);
}

TEST_CASE("Integration: constructor receives arguments", "[integration][class]") {
  REQUIRE(compileAndRun(
      "class P {\n"
      "    int x;\n"
      "    P(int v) { x = v; }\n"
      "}\n"
      "func main() -> int {\n"
      "    P^ p = new P(42);\n"
      "    return 0;\n"
      "}\n") == 0);
}

TEST_CASE("Integration: while and for loops", "[integration][loop]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var i = 0;\n"
      "    var s = 0;\n"
      "    while (i < 3) {\n"
      "        s = s + i;\n"
      "        i++;\n"
      "    }\n"
      "    return s;\n"
      "}\n") == 3);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var s = 0;\n"
      "    for (int i = 0; i < 3; i++) {\n"
      "        s += i;\n"
      "    }\n"
      "    return s;\n"
      "}\n") == 3);
}

TEST_CASE("Integration: class method and self field", "[integration][class]") {
  REQUIRE(compileAndRun(
      "class P {\n"
      "    int x;\n"
      "    P(int v) { x = v; }\n"
      "    func add(int d) -> int {\n"
      "        x = x + d;\n"
      "        return x;\n"
      "    }\n"
      "    func get_x(self!) -> int { return self.x; }\n"
      "}\n"
      "func main() -> int {\n"
      "    P^ p = new P(40);\n"
      "    p->add(2);\n"
      "    return p->get_x();\n"
      "}\n") == 42);
}

TEST_CASE("Integration: struct field assignment", "[integration][struct]") {
  REQUIRE(compileAndRun(
      "struct P { int x; }\n"
      "func main() -> int {\n"
      "    P^ p = new P();\n"
      "    p->x = 7;\n"
      "    return p->x;\n"
      "}\n") == 7);
}

TEST_CASE("Integration: match statement", "[integration][match]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var total = 0;\n"
      "    for (int i = 0; i < 4; i++) {\n"
      "        match (i) {\n"
      "            0 => { total = total + 1; }\n"
      "            1 => { total = total + 10; }\n"
      "            _ => { total = total + 100; }\n"
      "        }\n"
      "    }\n"
      "    return total;\n"
      "}\n") == 211);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var n = 5;\n"
      "    var r = 0;\n"
      "    match (n) {\n"
      "        x if x > 10 => { r = 1; }\n"
      "        x if x > 3  => { r = 2; }\n"
      "        _           => { r = 3; }\n"
      "    }\n"
      "    return r;\n"
      "}\n") == 2);
}

TEST_CASE("Integration: foreach", "[integration][foreach]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int[3] a = [1, 2, 3];\n"
      "    var s = 0;\n"
      "    foreach (var x : a) { s += x; }\n"
      "    return s;\n"
      "}\n") == 6);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var s = 0;\n"
      "    foreach (var i : 0..4) { s += i; }\n"
      "    return s;\n"
      "}\n") == 6);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var s = 0;\n"
      "    foreach (var i : 0..=4) { s += i; }\n"
      "    return s;\n"
      "}\n") == 10);
}

TEST_CASE("Integration: array and slice properties", "[integration][array]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int[3] a = [0, 0, 0];\n"
      "    a[0] = 20;\n"
      "    a[1] = 22;\n"
      "    int^ p = a.ptr;\n"
      "    long l = a.len;\n"
      "    int extra = 0;\n"
      "    if (l > 0) { extra = 3; }\n"
      "    return p[0] + p[1] + extra;\n"
      "}\n") == 45);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int[4] a = [10, 20, 12, 0];\n"
      "    int[] s = a[1..3];\n"
      "    s[0] = 40;\n"
      "    int^ p = s.ptr;\n"
      "    long l = a.len + s.len;\n"
      "    int extra = 0;\n"
      "    if (l > 0) { extra = 6; }\n"
      "    return p[0] + p[1] + extra;\n"
      "}\n") == 58);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int[3] a = [20, 22, 3];\n"
      "    long l = a.len + 1;\n"
      "    if (l == 4) { return a[0] + a[1] + 3; }\n"
      "    return 0;\n"
      "}\n") == 45);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int[4] a = [10, 20, 12, 0];\n"
      "    int[] s = a[1..=3];\n"
      "    long l = s.len;\n"
      "    int extra = 0;\n"
      "    if (l > 0) { extra = 3; }\n"
      "    return s[0] + s[2] + extra;\n"
      "}\n") == 23);
}

TEST_CASE("Integration: struct value construction", "[integration][struct]") {
  REQUIRE(compileAndRun(
      "struct P { int x; int y; }\n"
      "func main() -> int {\n"
      "    P p = P(40, 2);\n"
      "    return p.x + p.y;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "struct Point<T> { T x; T y; }\n"
      "func main() -> int {\n"
      "    Point<int> p = Point<int>(40, 2);\n"
      "    return p.x + p.y;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: stack object destructors", "[integration][raii]") {
  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = 42; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    { C c = C(b); }\n"
      "    return b->value;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = 42; }\n"
      "}\n"
      "func f(Box^ b) -> int {\n"
      "    { C c = C(b); return 0; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    f(b);\n"
      "    return b->value;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = 42; }\n"
      "}\n"
      "func f(Box^ b) -> void {\n"
      "    C c = C(b);\n"
      "    throw 3;\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    try { f(b); } catch (int e) { }\n"
      "    return b->value;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = 42; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    {\n"
      "        late C c;\n"
      "        c = C(b);\n"
      "    }\n"
      "    return b->value;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = p->value + 1; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    { C[3] arr = [C(b), C(b), C(b)]; }\n"
      "    return b->value;\n"
      "}\n") == 3);

  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = p->value + 1; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    {\n"
      "        late C[2] arr;\n"
      "        arr = [C(b), C(b)];\n"
      "    }\n"
      "    return b->value;\n"
      "}\n") == 2);
}

TEST_CASE("Integration: goto defer registration count", "[integration][goto][defer]") {
  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "func f(Box^ b) -> void {\n"
      "    var i = 0;\n"
      "start:\n"
      "    i = i + 1;\n"
      "    defer b->value = b->value + 1;\n"
      "    if (i < 3) { goto start; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    f(b);\n"
      "    return b->value;\n"
      "}\n") == 3);
}

TEST_CASE("Integration: goto cleanup", "[integration][goto][raii]") {
  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "func f(Box^ b) -> void {\n"
      "    {\n"
      "        defer b->value = 42;\n"
      "        goto done;\n"
      "    }\n"
      "done:\n"
      "    return;\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    f(b);\n"
      "    return b->value;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = 42; }\n"
      "}\n"
      "func f(Box^ b) -> void {\n"
      "    {\n"
      "        C c = C(b);\n"
      "        goto done;\n"
      "    }\n"
      "done:\n"
      "    return;\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    f(b);\n"
      "    return b->value;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: out array element destructor", "[integration][out][raii]") {
  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = p->value + 1; }\n"
      "}\n"
      "func fill(out C[2]& dst, Box^ b) { dst = [C(b), C(b)]; }\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    {\n"
      "        late C[2] arr;\n"
      "        fill(&arr, b);\n"
      "    }\n"
      "    return b->value;\n"
      "}\n") == 2);
}

TEST_CASE("Integration: out object destructor", "[integration][out][raii]") {
  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = 42; }\n"
      "}\n"
      "func fill(out C& c, Box^ b) { c = C(b); }\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    {\n"
      "        late C c;\n"
      "        fill(&c, b);\n"
      "    }\n"
      "    return b->value;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: unified cleanup order", "[integration][raii][defer]") {
  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = p->value * 10 + 1; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    {\n"
      "        C c = C(b);\n"
      "        defer b->value = b->value * 10 + 2;\n"
      "    }\n"
      "    return b->value + 20;\n"
      "}\n") == 41);

  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = p->value * 10 + 1; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    {\n"
      "        defer b->value = b->value * 10 + 2;\n"
      "        C c = C(b);\n"
      "    }\n"
      "    return b->value + 20;\n"
      "}\n") == 32);
}

TEST_CASE("Integration: FFI out reference", "[integration][ffi][out]") {
  REQUIRE(compileAndRun(
      "struct Timeval { long tv_sec; long tv_usec; }\n"
      "extern \"C\" func gettimeofday(out Timeval& tv, byte^ tz) -> int;\n"
      "func main() -> int {\n"
      "    late Timeval tv;\n"
      "    gettimeofday(&tv, null);\n"
      "    if (tv.tv_sec > 0) { return 42; }\n"
      "    return 0;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: out parameters", "[integration][out]") {
  REQUIRE(compileAndRun(
      "func assign_out(out int& x) { x = 42; }\n"
      "func main() -> int {\n"
      "    late int a;\n"
      "    assign_out(&a);\n"
      "    return a;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func fill(out int[5]& dst) {\n"
      "    dst = [0; 5];\n"
      "    dst[0] = 7;\n"
      "}\n"
      "func main() -> int {\n"
      "    late int[5] a;\n"
      "    fill(&a);\n"
      "    return a[0] + a[1] + 35;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func bad(out int& x) { }\n"
      "func main() -> int {\n"
      "    late int a;\n"
      "    bad(&a);\n"
      "    return 0;\n"
      "}\n") == -2);

  REQUIRE(compileAndRun(
      "func bad_return(out int& x, bool c) -> int {\n"
      "    if (c) { return 0; }\n"
      "    x = 1;\n"
      "    return 0;\n"
      "}\n"
      "func main() -> int {\n"
      "    late int a;\n"
      "    bad_return(&a, true);\n"
      "    return 0;\n"
      "}\n") == -2);

  REQUIRE(compileAndRun(
      "func good(out int& x) -> int {\n"
      "    x = 42;\n"
      "    return 0;\n"
      "}\n"
      "func main() -> int {\n"
      "    late int a;\n"
      "    good(&a);\n"
      "    return a;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "struct P { int x; int y; }\n"
      "func copy_out(out P& dst, P src) { dst = src; }\n"
      "func main() -> int {\n"
      "    P^ s = new P();\n"
      "    s->x = 40;\n"
      "    s->y = 2;\n"
      "    late P dst;\n"
      "    copy_out(&dst, s^);\n"
      "    return dst.x + dst.y;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "struct P { int x; int y; }\n"
      "func bad(out P& dst) { }\n"
      "func main() -> int {\n"
      "    late P p;\n"
      "    bad(&p);\n"
      "    return 0;\n"
      "}\n") == -2);

  REQUIRE(compileAndRun(
      "struct P { int x; int y; }\n"
      "func fill(out P& dst) { dst = P(40, 2); }\n"
      "func main() -> int {\n"
      "    late P p;\n"
      "    fill(&p);\n"
      "    return p.x + p.y;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func assign_out(out int& x) { x = 42; }\n"
      "func main() -> int {\n"
      "    late let int a;\n"
      "    assign_out(&a);\n"
      "    return a;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func assign_out(out int& x) { x = 42; }\n"
      "func main() -> int {\n"
      "    let a = 0;\n"
      "    assign_out(&a);\n"
      "    return a;\n"
      "}\n") == -2);

  REQUIRE(compileAndRun(
      "class C { int x; }\n"
      "func copy_out(out C& dst, C src) { dst = src; }\n"
      "func main() -> int {\n"
      "    C^ s = new C();\n"
      "    s->x = 42;\n"
      "    late C dst;\n"
      "    copy_out(&dst, s^);\n"
      "    return dst.x;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: references", "[integration][reference]") {
  REQUIRE(compileAndRun(
      "func increment(int& x) { x = x + 1; }\n"
      "func main() -> int {\n"
      "    int a = 41;\n"
      "    increment(&a);\n"
      "    return a;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int a = 42;\n"
      "    int& r = &a;\n"
      "    r = r + 1;\n"
      "    return a - 1;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func read(int!& x) -> int { return x; }\n"
      "func main() -> int {\n"
      "    int a = 42;\n"
      "    return read(a);\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func set_element(int[3]& a) { a[1] = 42; }\n"
      "func main() -> int {\n"
      "    int[3] a = [0, 0, 0];\n"
      "    set_element(&a);\n"
      "    return a[1];\n"
      "}\n") == 42);
}

TEST_CASE("Integration: array to slice assignment", "[integration][array]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int[3] a = [10, 20, 12];\n"
      "    late int[] s;\n"
      "    s = a;\n"
      "    s[0] = 40;\n"
      "    if (s.len == 3) { return a[0] + 2; }\n"
      "    return 0;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func bind(out int[]& s, int[] src) { s = src; }\n"
      "func main() -> int {\n"
      "    int[3] a = [10, 20, 12];\n"
      "    late int[] s;\n"
      "    bind(&s, a);\n"
      "    return s[0] + s[2] + 20;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: fixed array value semantics", "[integration][array]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int[3] a = [1, 2, 3];\n"
      "    int[3] b = a;\n"
      "    b[0] = 9;\n"
      "    return a[0] + b[0] + 32;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func change(int[3] x) -> int { x[0] = 9; return x[0]; }\n"
      "func main() -> int {\n"
      "    int[3] a = [1, 2, 3];\n"
      "    int[3] b = [0, 0, 0];\n"
      "    b = a;\n"
      "    b[1] = 8;\n"
      "    var y = change(a);\n"
      "    return a[0] + a[1] + b[1] + y + 22;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: late array whole assignment", "[integration][array][late]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    late int[5] a;\n"
      "    a = [1, 2, 3, 4, 5];\n"
      "    return a[0] + a[4] + 36;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    late int[100] a;\n"
      "    a = [0; 100];\n"
      "    a[0] = 20;\n"
      "    a[99] = 22;\n"
      "    return a[0] + a[99];\n"
      "}\n") == 42);
}

TEST_CASE("Integration: new array element lifecycle", "[integration][array][raii]") {
  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    static Box^ target;\n"
      "    Box^ p;\n"
      "    C() { p = target; }\n"
      "    ~C() { p->value = p->value + 1; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    C.target = b;\n"
      "    C[] a = new C[3];\n"
      "    delete[] a;\n"
      "    return b->value;\n"
      "}\n") == 3);
}

TEST_CASE("Integration: repeat array initializer", "[integration][array]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int[10000] a = [0; 10000];\n"
      "    a[0] = 1;\n"
      "    a[9999] = 2;\n"
      "    return a[0] + a[9999] + 39;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int[3] a = [5; 3];\n"
      "    return a[0] + a[2] + 32;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    int[] s = [5; 4];\n"
      "    if (s.len == 4) { return s[0] + s[3] + 32; }\n"
      "    return 0;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: new array and delete[]", "[integration][array]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var a = new int[3];\n"
      "    a[0] = 20;\n"
      "    a[1] = 22;\n"
      "    long l = a.len;\n"
      "    int extra = 0;\n"
      "    if (l > 0) { extra = 3; }\n"
      "    var r = a[0] + a[1] + extra;\n"
      "    delete[] a;\n"
      "    return r;\n"
      "}\n") == 45);
}

TEST_CASE("Integration: semantic expression types", "[integration][types]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    double[2] a = [1.5, 2.5];\n"
      "    var x = a[0] + a[1];\n"
      "    if (x > 3.9) { return 42; }\n"
      "    return 0;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    double[2] a = [1.5, 2.5];\n"
      "    double^ p = a.ptr;\n"
      "    var x = p[0] + p[1];\n"
      "    if (x > 3.9) { return 42; }\n"
      "    return 0;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "struct P { double x; }\n"
      "func main() -> int {\n"
      "    P^ p = new P();\n"
      "    p->x = 42.0;\n"
      "    var y = p->x;\n"
      "    if (y > 41.0) { return 42; }\n"
      "    return 0;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: tuple access and destructuring", "[integration][tuple]") {
  REQUIRE(compileAndRun(
      "func pair() -> (int, int) { return (20, 22); }\n"
      "func main() -> int {\n"
      "    var (a, b) = pair();\n"
      "    var t = (1, 2);\n"
      "    return a + b + t[0] + t[1];\n"
      "}\n") == 45);
}

TEST_CASE("Integration: do-while goto and enum", "[integration][control]") {
  REQUIRE(compileAndRun(
      "enum Color { Red, Green, Blue }\n"
      "func main() -> int {\n"
      "    var i = 0;\n"
      "    do { i = i + 1; } while (i < 3);\n"
      "    goto done;\n"
      "    i = 99;\n"
      "done:\n"
      "    Color c = Color.Green;\n"
      "    if (c == Color.Green) { return i + 1; }\n"
      "    return 0;\n"
      "}\n") == 4);
}

TEST_CASE("Integration: properties", "[integration][class]") {
  REQUIRE(compileAndRun(
      "class Counter {\n"
      "    private int _count;\n"
      "    public set count(value: int) { _count = value; }\n"
      "    public get count -> int { return _count; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Counter^ c = new Counter();\n"
      "    c->count = 42;\n"
      "    return c->count;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: inheritance", "[integration][class]") {
  REQUIRE(compileAndRun(
      "class Animal {\n"
      "    int x;\n"
      "    Animal(int v) { x = v; }\n"
      "    func speak() -> int { return x; }\n"
      "}\n"
      "class Dog : Animal {\n"
      "    Dog(int v) : base(v) { }\n"
      "    override func speak() -> int { return base.speak() + 1; }\n"
      "    func base_x() -> int { return base.x; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Dog^ d = new Dog(41);\n"
      "    return d->speak() + d->base_x() - 41;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: static members", "[integration][class]") {
  REQUIRE(compileAndRun(
      "class Counter {\n"
      "    public static int count = 40;\n"
      "    public static func get_count() -> int { return count; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Counter.count = 40;\n"
      "    return Counter.get_count() + 2;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: explicit delete calls destructor", "[integration][class]") {
  REQUIRE(compileAndRun(
      "class Box { int value; Box(int v) { value = v; } }\n"
      "class R {\n"
      "    Box^ box;\n"
      "    R(Box^ b) { box = b; }\n"
      "    ~R() { box->value = 42; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box(0);\n"
      "    R^ r = new R(b);\n"
      "    delete r;\n"
      "    return b->value;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: try/catch", "[integration][exception]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    try { throw 7; } catch (int e) { return e; }\n"
      "    return 0;\n"
      "}\n") == 7);

  REQUIRE(compileAndRun(
      "func boom() -> void { throw 9; }\n"
      "func main() -> int {\n"
      "    try { boom(); } catch (int e) { return e; }\n"
      "    return 0;\n"
      "}\n") == 9);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    try {\n"
      "        try { throw 3; } catch (int e) { r = e; }\n"
      "        throw 4;\n"
      "    } catch (int e) { r = r + e; }\n"
      "    return r;\n"
      "}\n") == 7);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    try { throw 1; } catch (...) { return 42; }\n"
      "    return 0;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: catch type matching", "[integration][exception]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    try { throw 42; }\n"
      "    catch (double e) { r = 100; }\n"
      "    catch (int e) { r = e; }\n"
      "    catch (...) { r = 200; }\n"
      "    return r;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    try { throw 2.5; }\n"
      "    catch (int e) { r = 1; }\n"
      "    catch (double e) { if (e > 2.0) { r = 7; } }\n"
      "    return r;\n"
      "}\n") == 7);

  REQUIRE(compileAndRun(
      "class ErrorA { int code; ErrorA(int c) { code = c; } }\n"
      "class ErrorB { int code; ErrorB(int c) { code = c; } }\n"
      "func main() -> int {\n"
      "    try { throw new ErrorB(5); }\n"
      "    catch (ErrorA^ e) { return 1; }\n"
      "    catch (ErrorB^ e) { return e->code + 2; }\n"
      "    return 0;\n"
      "}\n") == 7);
}

TEST_CASE("Integration: catch fallthrough and rethrow", "[integration][exception]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    try {\n"
      "        try { throw 3.5; }\n"
      "        catch (int e) { r = 10; }\n"
      "        catch (...) { r = 7; }\n"
      "    } catch (...) { r = 99; }\n"
      "    return r;\n"
      "}\n") == 7);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    try {\n"
      "        try { throw 8; }\n"
      "        catch (double e) { r = 10; }\n"
      "    } catch (int e) { r = e; }\n"
      "    return r;\n"
      "}\n") == 8);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    try {\n"
      "        try { throw 8; }\n"
      "        catch (int e) { throw; }\n"
      "    } catch (int e) { r = e; }\n"
      "    return r;\n"
      "}\n") == 8);
}

TEST_CASE("Integration: try catch defer timing", "[integration][exception][defer]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    defer r = r + 100;\n"
      "    try {\n"
      "        defer r = r + 1;\n"
      "        throw 3;\n"
      "    } catch (int e) {\n"
      "        r = r + 10;\n"
      "    }\n"
      "    return r;\n"
      "}\n") == 11);

  REQUIRE(compileAndRun(
      "class Box { int value; }\n"
      "class C {\n"
      "    Box^ p;\n"
      "    C(Box^ q) { p = q; }\n"
      "    ~C() { p->value = 42; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box();\n"
      "    b->value = 0;\n"
      "    {\n"
      "        C c = C(b);\n"
      "        try { throw 3; } catch (int e) {\n"
      "            if (b->value != 0) { return 1; }\n"
      "        }\n"
      "    }\n"
      "    return b->value;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: defer on throw and return", "[integration][exception][defer]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    try {\n"
      "        defer r = 7;\n"
      "        throw 3;\n"
      "    } catch (int e) { r = r + e; }\n"
      "    return r;\n"
      "}\n") == 10);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    try {\n"
      "        defer r = r * 10 + 1;\n"
      "        defer r = r * 10 + 2;\n"
      "        throw 3;\n"
      "    } catch (int e) { r = r + 0; }\n"
      "    return r;\n"
      "}\n") == 21);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    try {\n"
      "        if (false) { defer r = 99; }\n"
      "        defer r = 5;\n"
      "        throw 3;\n"
      "    } catch (int e) { r = r + e; }\n"
      "    return r;\n"
      "}\n") == 8);
}

TEST_CASE("Integration: block-scoped defer", "[integration][defer]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    {\n"
      "        defer r = r + 1;\n"
      "        r = 10;\n"
      "    }\n"
      "    if (r == 11) { return 42; }\n"
      "    return 0;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    foreach (var i : 0..3) {\n"
      "        defer r = r + 1;\n"
      "    }\n"
      "    return r + 39;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    foreach (var i : 0..10) {\n"
      "        defer r = r + 1;\n"
      "        if (i == 2) { break; }\n"
      "    }\n"
      "    return r + 39;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    foreach (var i : 0..3) {\n"
      "        defer r = r + 1;\n"
      "        if (i == 1) { continue; }\n"
      "    }\n"
      "    return r + 39;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    {\n"
      "        defer {\n"
      "            r = r + 1;\n"
      "            r = r + 10;\n"
      "        }\n"
      "        r = 1;\n"
      "    }\n"
      "    return r + 30;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func main() -> int {\n"
      "    var r = 0;\n"
      "    try {\n"
      "        defer { r = r + 1; r = r + 10; }\n"
      "        throw 3;\n"
      "    } catch (int e) { r = r + e; }\n"
      "    return r + 28;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "class Box {\n"
      "    int value;\n"
      "    Box(int v) { value = v; }\n"
      "}\n"
      "func f(Box^ b) -> int {\n"
      "    {\n"
      "        defer b->value = 7;\n"
      "        return 0;\n"
      "    }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box(0);\n"
      "    var x = f(b);\n"
      "    return b->value + x;\n"
      "}\n") == 7);
}

TEST_CASE("Integration: defer unwinds intermediate frames", "[integration][exception][defer]") {
  REQUIRE(compileAndRun(
      "class Box {\n"
      "    int value;\n"
      "    Box(int v) { value = v; }\n"
      "}\n"
      "func deep() -> void { throw 9; }\n"
      "func middle(Box^ b) -> void {\n"
      "    defer b->value = 5;\n"
      "    deep();\n"
      "}\n"
      "func main() -> int {\n"
      "    Box^ b = new Box(0);\n"
      "    try { middle(b); }\n"
      "    catch (int e) { return b->value + e; }\n"
      "    return 0;\n"
      "}\n") == 14);
}

TEST_CASE("Integration: generic function monomorphization", "[integration][generics]") {
  REQUIRE(compileAndRun(
      "func id<T>(T x) -> T { return x; }\n"
      "func main() -> int {\n"
      "    var i = id<int>(42);\n"
      "    var d = id<double>(3.5);\n"
      "    if (d > 3.0) { return i; }\n"
      "    return 0;\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "func max_of<T>(T a, T b) -> T where Comparable<T> {\n"
      "    if (a > b) { return a; }\n"
      "    return b;\n"
      "}\n"
      "func main() -> int { return max_of<int>(40, 42); }\n") == 42);

  REQUIRE(compileAndRun(
      "func first<T>(T[] xs) -> T { return xs[0]; }\n"
      "func main() -> int {\n"
      "    int[3] a = [7, 8, 9];\n"
      "    return first(a);\n"
      "}\n") == 7);
}

TEST_CASE("Integration: generic class monomorphization", "[integration][generics]") {
  REQUIRE(compileAndRun(
      "class Box<T> {\n"
      "    T value;\n"
      "    Box(T v) { value = v; }\n"
      "    func get_value() -> T { return value; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box<int>^ b = new Box<int>(42);\n"
      "    return b->get_value();\n"
      "}\n") == 42);
}

TEST_CASE("Integration: concept rejection", "[integration][generics]") {
  REQUIRE(compileAndRun(
      "concept Addable<T> = requires (T a, T b) {\n"
      "    { a + b } -> T;\n"
      "};\n"
      "func add<T>(T a, T b) -> T where Addable<T> { return a + b; }\n"
      "func main() -> int { return add<bool>(true, false); }\n") == -2);
}

TEST_CASE("Integration: generic struct monomorphization", "[integration][generics]") {
  REQUIRE(compileAndRun(
      "struct Point<T> {\n"
      "    T x;\n"
      "    T y;\n"
      "    func sum() -> T { return x + y; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Point<int>^ a = new Point<int>();\n"
      "    Point<double>^ b = new Point<double>();\n"
      "    a->x = 40;\n"
      "    a->y = 2;\n"
      "    b->x = 1.5;\n"
      "    b->y = 1.0;\n"
      "    if (b->sum() > 2.0) { return a->sum(); }\n"
      "    return 0;\n"
      "}\n") == 42);
}

TEST_CASE("Integration: generic class where constraint", "[integration][generics]") {
  REQUIRE(compileAndRun(
      "class Box<T> where Comparable<T> {\n"
      "    T value;\n"
      "    Box(T v) { value = v; }\n"
      "    func get_value() -> T { return value; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box<int>^ b = new Box<int>(42);\n"
      "    return b->get_value();\n"
      "}\n") == 42);

  REQUIRE(compileAndRun(
      "class Box<T> where Comparable<T> {\n"
      "    T value;\n"
      "    Box(T v) { value = v; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Box<bool>^ b = new Box<bool>(true);\n"
      "    return 0;\n"
      "}\n") == -2);
}

TEST_CASE("Integration: struct value constructor", "[integration][generics]") {
  REQUIRE(compileAndRun(
      "struct Point<T> {\n"
      "    T x;\n"
      "    T y;\n"
      "    Point(T a, T b) { x = a; y = b; }\n"
      "    func sum() -> T { return x + y; }\n"
      "}\n"
      "func main() -> int {\n"
      "    Point<int> p = Point<int>(40, 2);\n"
      "    return p.sum();\n"
      "}\n") == 42);
}

TEST_CASE("Integration: module imports", "[integration][module]") {
  namespace fs = std::filesystem;
  fs::path dir = fs::temp_directory_path() / "c_hat_module_test";
  fs::remove_all(dir);
  fs::create_directories(dir);
  {
    std::ofstream a(dir / "A.ch");
    a << "module A;\npublic func foo() -> int { return 42; }\n";
  }
  {
    std::ofstream b(dir / "B.ch");
    b << "module B;\nimport A;\n"
         "public func bar() -> int { return foo(); }\n";
  }
  {
    std::ofstream c(dir / "C.ch");
    c << "module C;\npublic func foo() -> int { return 10; }\n";
  }
  int result = compileAndRunWithModules(
      "import B;\nfunc main() -> int { return bar(); }\n", dir.string());
  REQUIRE(result == 42);

  result = compileAndRunWithModules(
      "import A;\nfunc main() -> int { return A.foo(); }\n",
      dir.string());
  REQUIRE(result == 42);

  result = compileAndRunWithModules(
      "import A as X;\nfunc main() -> int { return X.foo(); }\n",
      dir.string());
  REQUIRE(result == 42);

  result = compileAndRunWithModules(
      "import A;\nimport C;\n"
      "func main() -> int { return A.foo() + C.foo() - 10; }\n",
      dir.string());
  fs::remove_all(dir);
  REQUIRE(result == 42);
}

#ifndef _WIN32
static void expectAbort(const std::string &source) {
  pid_t pid = fork();
  REQUIRE(pid >= 0);
  if (pid == 0) {
    std::signal(SIGABRT, SIG_DFL);
    int result = compileAndRun(source);
    _exit(result);
  }
  int status = 0;
  REQUIRE(waitpid(pid, &status, 0) == pid);
  REQUIRE(WIFSIGNALED(status));
  REQUIRE(WTERMSIG(status) == SIGABRT);
}

TEST_CASE("Integration: runtime bounds abort", "[integration][bounds]") {
  expectAbort(
      "func main() -> int {\n"
      "    int[3] a = [1, 2, 3];\n"
      "    var i = 3;\n"
      "    return a[i];\n"
      "}\n");

  expectAbort(
      "func main() -> int {\n"
      "    int[4] a = [1, 2, 3, 4];\n"
      "    var e = 5;\n"
      "    int[] s = a[1..e];\n"
      "    return s[0];\n"
      "}\n");
}
#endif
