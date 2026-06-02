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
