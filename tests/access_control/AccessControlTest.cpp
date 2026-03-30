#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>

using namespace c_hat;

bool analyzeSource(const std::string &source) {
  try {
    // 添加一个简单的 main 函数
    std::string sourceWithMain = source + "\nfunc main() { }\n";
    std::cerr << "Testing source:\n" << sourceWithMain << std::endl;
    std::cerr.flush();
    parser::Parser parser(sourceWithMain);
    auto program = parser.parseProgram();
    if (!program) {
      std::cerr << "Parser failed to parse program" << std::endl;
      std::cerr.flush();
      return false;
    }

    // 打印解析结果
    std::cerr << "Parser succeeded. Number of declarations: "
              << program->declarations.size() << std::endl;
    std::cerr.flush();
    for (size_t i = 0; i < program->declarations.size(); ++i) {
      std::cerr << "Declaration " << i << ": "
                << program->declarations[i]->toString() << std::endl;
      std::cerr.flush();
    }

    std::cerr << "Starting semantic analysis..." << std::endl;
    std::cerr.flush();
    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(*program);
    bool hasError = analyzer.hasError();
    std::cerr << "Semantic analysis result: "
              << (hasError ? "FAILED" : "PASSED") << std::endl;
    std::cerr.flush();
    return !hasError;
  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    std::cerr.flush();
    return false;
  }
}

TEST_CASE("Access Control - Public members", "[access_control]") {
  // 测试公共成员可以在任何地方访问
  const char *source = R"(
    class Test {
    public:
      int publicVar;
      public void publicMethod() {}
    };
    
    void testFunction() {
      Test t;
      t.publicVar = 10; // 应该可以访问
      t.publicMethod(); // 应该可以访问
    }
  )";

  // 应该成功，没有访问控制错误
  REQUIRE(analyzeSource(source) == true);
}

TEST_CASE("Access Control - Private members", "[access_control]") {
  // 测试私有成员只能在类内部访问
  const char *source = R"(
    class Test {
    private:
      int privateVar;
      private void privateMethod() {}
    
    public:
      void accessPrivate() {
        privateVar = 10; // 类内部可以访问
        privateMethod(); // 类内部可以访问
      }
    };
    
    void testFunction() {
      Test t;
      t.privateVar = 10; // 应该报错：无法访问私有成员
      t.privateMethod(); // 应该报错：无法访问私有成员
    }
  )";

  // 应该失败，因为在类外部访问了私有成员
  REQUIRE(analyzeSource(source) == false);
}

TEST_CASE("Access Control - Protected members", "[access_control]") {
  // 测试保护成员可以在类内部和子类中访问
  const char *source = R"(
    class Base {
    protected:
      int protectedVar;
      protected void protectedMethod() {}
    
    public:
      void accessProtected() {
        protectedVar = 10; // 类内部可以访问
        protectedMethod(); // 类内部可以访问
      }
    };
    
    class Derived : Base {
    public:
      void accessProtected() {
        protectedVar = 10; // 子类中可以访问
        protectedMethod(); // 子类中可以访问
      }
    };
    
    void testFunction() {
      Base b;
      b.protectedVar = 10; // 应该报错：无法访问保护成员
      b.protectedMethod(); // 应该报错：无法访问保护成员
    }
  )";

  // 应该失败，因为在类外部访问了保护成员
  REQUIRE(analyzeSource(source) == false);
}
