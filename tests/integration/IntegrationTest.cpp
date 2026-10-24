// IntegrationTest.cpp - 端到端编译流水线测试
// 测试 parser → semantic → LLVM codegen → JIT 执行 完整链路
#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include "../src/llvm/LLVMCodeGenerator.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <string>

using namespace c_hat;

static int compileAndRun(const std::string &source) {
  parser::Parser parser(source);
  auto program = parser.parseProgram();
  if (!program) return -1;

  semantic::SemanticAnalyzer analyzer("", false);
  analyzer.analyze(*program);
  if (analyzer.hasError()) return -2;

  llvm_codegen::LLVMCodeGenerator codeGen("test_module");
  codeGen.generate(std::move(program));
  if (!codeGen.verifyIR()) return -3;
  if (!codeGen.hasJIT()) return -4;

  codeGen.addExternalSymbol("printf", (void *)&std::printf);
  return codeGen.runJIT("main");
}

TEST_CASE("Integration: main returns constant", "[integration][e2e]") {
  REQUIRE(compileAndRun(
      "func main() -> int {\n"
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
      "    P p;\n"
      "    p.x = 7;\n"
      "    return p.x;\n"
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
      "}\n") == 10);
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
