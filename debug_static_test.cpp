#include "src/parser/Parser.h"
#include "src/semantic/SemanticAnalyzer.h"
#include <iostream>

using namespace c_hat;

int main() {
    // 测试1: 全局 static 变量
    {
        std::string src = "static int g_count = 0;\nfunc main() { g_count = 1; }";
        try {
            parser::Parser p(src);
            auto prog = p.parseProgram();
            if (!prog) { std::cout << "Parse failed (null)\n"; return 1; }
            std::cout << "Parse OK, " << prog->declarations.size() << " decls\n";
            for (auto& d : prog->declarations) {
                std::cout << "  decl type=" << (int)d->getType() << "\n";
            }
            semantic::SemanticAnalyzer analyzer("", false);
            analyzer.analyze(*prog);
            if (analyzer.hasError()) {
                std::cout << "Semantic FAILED\n";
                for (auto& e : analyzer.getErrors()) {
                    std::cout << "  Error: " << e.message << "\n";
                }
            } else {
                std::cout << "Semantic OK\n";
            }
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << "\n";
        } catch (...) {
            std::cout << "Unknown exception\n";
        }
    }
    return 0;
}
