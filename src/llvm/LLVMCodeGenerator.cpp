#include "LLVMCodeGenerator.h"
#include "../Diagnostics.h"
#include "../ast/AstClone.h"
#include <cctype>
#include <ctime>
#include <functional>
#include <iostream>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <stdexcept>

namespace c_hat {
namespace llvm_codegen {

namespace {

llvm::GlobalVariable *getOrCreateGlobal(llvm::Module *module, const char *name,
                                        llvm::Type *type,
                                        llvm::Constant *initializer) {
  if (auto *existing = module->getNamedGlobal(name)) {
    return existing;
  }
  return new llvm::GlobalVariable(*module, type, false,
                                  llvm::GlobalValue::InternalLinkage,
                                  initializer, name);
}

llvm::GlobalVariable *getExceptionValueGlobal(llvm::Module *module) {
  return getOrCreateGlobal(
      module, "c_hat_exception_value",
      llvm::Type::getInt64Ty(module->getContext()),
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(module->getContext()), 0));
}

llvm::GlobalVariable *getExceptionTypeGlobal(llvm::Module *module) {
  return getOrCreateGlobal(
      module, "c_hat_exception_type",
      llvm::Type::getInt64Ty(module->getContext()),
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(module->getContext()), 0));
}

llvm::GlobalVariable *getExceptionHandlerGlobal(llvm::Module *module) {
  return getOrCreateGlobal(
      module, "c_hat_exception_handler",
      llvm::PointerType::getUnqual(module->getContext()),
      llvm::ConstantPointerNull::get(
          llvm::PointerType::getUnqual(module->getContext())));
}

// 把任意标量压成 i64，存进全局异常槽。
// 递归检查语句树中是否包含 defer。只有包含 defer 的函数才需要安装
// 函数级 setjmp 清理帧，避免给所有函数增加开销。
bool statementContainsDefer(const ast::Statement *stmt) {
  if (!stmt) {
    return false;
  }
  switch (stmt->getType()) {
  case ast::NodeType::DeferStmt:
    return true;
  case ast::NodeType::CompoundStmt: {
    auto *compound = static_cast<const ast::CompoundStmt *>(stmt);
    for (const auto &node : compound->statements) {
      if (auto *child = dynamic_cast<const ast::Statement *>(node.get())) {
        if (statementContainsDefer(child)) {
          return true;
        }
      }
    }
    return false;
  }
  case ast::NodeType::IfStmt: {
    auto *ifStmt = static_cast<const ast::IfStmt *>(stmt);
    return statementContainsDefer(ifStmt->thenBranch.get()) ||
           statementContainsDefer(ifStmt->elseBranch.get());
  }
  case ast::NodeType::WhileStmt: {
    auto *whileStmt = static_cast<const ast::WhileStmt *>(stmt);
    return statementContainsDefer(whileStmt->body.get());
  }
  case ast::NodeType::DoWhileStmt: {
    auto *doWhile = static_cast<const ast::DoWhileStmt *>(stmt);
    return statementContainsDefer(doWhile->body.get());
  }
  case ast::NodeType::ForStmt: {
    auto *forStmt = static_cast<const ast::ForStmt *>(stmt);
    return statementContainsDefer(forStmt->body.get());
  }
  case ast::NodeType::MatchStmt: {
    auto *matchStmt = static_cast<const ast::MatchStmt *>(stmt);
    for (const auto &arm : matchStmt->arms) {
      if (arm && statementContainsDefer(arm->body.get())) {
        return true;
      }
    }
    return false;
  }
  case ast::NodeType::TryStmt: {
    auto *tryStmt = static_cast<const ast::TryStmt *>(stmt);
    if (statementContainsDefer(tryStmt->tryBlock.get())) {
      return true;
    }
    for (const auto &catchStmt : tryStmt->catchStmts) {
      if (catchStmt && statementContainsDefer(catchStmt->body.get())) {
        return true;
      }
    }
    return false;
  }
  default:
    return false;
  }
}

llvm::Value *toExceptionWord(llvm::IRBuilder<> *builder, llvm::Value *value) {
  llvm::Type *i64Ty = builder->getInt64Ty();
  llvm::Type *ty = value->getType();
  if (ty->isIntegerTy()) {
    unsigned bits = ty->getIntegerBitWidth();
    if (bits < 64) {
      return builder->CreateZExt(value, i64Ty, "exc.zext");
    }
    if (bits > 64) {
      return builder->CreateTrunc(value, i64Ty, "exc.trunc");
    }
    return value;
  }
  if (ty->isPointerTy()) {
    return builder->CreatePtrToInt(value, i64Ty, "exc.ptrtoint");
  }
  if (ty->isFloatTy()) {
    llvm::Value *asInt =
        builder->CreateBitCast(value, builder->getInt32Ty(), "exc.f32bits");
    return builder->CreateZExt(asInt, i64Ty, "exc.zext");
  }
  if (ty->isDoubleTy()) {
    return builder->CreateBitCast(value, i64Ty, "exc.f64bits");
  }
  // 结构体等：保存到临时槽，传地址（catch 侧只能按指针/整数处理）。
  llvm::AllocaInst *slot =
      builder->CreateAlloca(ty, nullptr, "exc.struct.slot");
  builder->CreateStore(value, slot);
  return builder->CreatePtrToInt(slot, i64Ty, "exc.ptrtoint");
}

// 把任意标量条件转换成 i1，避免对 double/pointer 使用 ICmp。
llvm::Value *toBoolean(llvm::IRBuilder<> *builder, llvm::Value *value,
                       const char *name) {
  if (!value) {
    return nullptr;
  }
  llvm::Type *type = value->getType();
  if (type->isIntegerTy(1)) {
    return value;
  }
  if (type->isFloatingPointTy()) {
    return builder->CreateFCmpONE(
        value, llvm::ConstantFP::get(type, 0.0), name);
  }
  if (type->isIntegerTy()) {
    return builder->CreateICmpNE(
        value, llvm::ConstantInt::get(type, 0), name);
  }
  if (type->isPointerTy()) {
    return builder->CreateICmpNE(
        value, llvm::ConstantPointerNull::get(
                   llvm::cast<llvm::PointerType>(type)),
        name);
  }
  return value;
}

llvm::Value *fromExceptionWord(llvm::IRBuilder<> *builder,
                               llvm::Value *word, llvm::Type *targetTy) {
  if (targetTy->isIntegerTy()) {
    unsigned bits = targetTy->getIntegerBitWidth();
    if (bits < 64) {
      return builder->CreateTrunc(word, targetTy, "exc.trunc");
    }
    return word;
  }
  if (targetTy->isPointerTy()) {
    return builder->CreateIntToPtr(word, targetTy, "exc.inttoptr");
  }
  if (targetTy->isFloatTy()) {
    llvm::Value *asInt =
        builder->CreateTrunc(word, builder->getInt32Ty(), "exc.trunc");
    return builder->CreateBitCast(asInt, targetTy, "exc.f32");
  }
  if (targetTy->isDoubleTy()) {
    return builder->CreateBitCast(word, targetTy, "exc.f64");
  }
  return llvm::Constant::getNullValue(targetTy);
}

} // namespace

LLVMCodeGenerator::LLVMCodeGenerator(const std::string &moduleName)
    : generator_(moduleName) {}

void LLVMCodeGenerator::generate(std::unique_ptr<ast::Program> program) {
  // 先添加内置类型 literalview 到 structTypes_
  structTypes_["literalview"] =
      static_cast<llvm::StructType *>(getLiteralViewType());
  // 添加 literalview 的成员信息
  std::unordered_map<std::string, unsigned> literalViewInfo;
  literalViewInfo["ptr"] = 0;
  literalViewInfo["len"] = 1;
  structInfo_["literalview"] = literalViewInfo;

  // 先处理类型声明（不处理 VariableDecl）
  for (auto &decl : program->declarations) {
    ast::NodeType type = decl->getType();
    if (type == ast::NodeType::StructDecl) {
      auto *structDecl = static_cast<ast::StructDecl *>(decl.get());
      if (!structDecl->templateParams.empty()) {
        continue; // 泛型声明由语义层单态化后追加的副本生成
      }
      generateDeclaration(std::move(decl));
    } else if (type == ast::NodeType::ClassDecl) {
      auto *classDecl = static_cast<ast::ClassDecl *>(decl.get());
      if (!classDecl->templateParams.empty()) {
        continue;
      }
      generateDeclaration(std::move(decl));
    } else if (type == ast::NodeType::TypeAliasDecl ||
        type == ast::NodeType::ExtensionDecl ||
        type == ast::NodeType::InterfaceDecl) {
      generateDeclaration(std::move(decl));
    } else if (type == ast::NodeType::EnumDecl) {
      // 收集枚举类型名，并生成 Color_Green 形式的全局常量。
      auto *enumDecl = static_cast<ast::EnumDecl *>(decl.get());
      enumTypes_.insert(enumDecl->name);
      generateEnumDecl(std::unique_ptr<ast::EnumDecl>(
          static_cast<ast::EnumDecl *>(decl.release())));
    }
  }

  // 处理 extern 声明
  for (auto &decl : program->declarations) {
    if (decl && decl->getType() == ast::NodeType::ExternDecl) {
      auto externDecl = std::unique_ptr<ast::ExternDecl>(
          static_cast<ast::ExternDecl *>(decl.release()));
      generateExternDecl(std::move(externDecl));
    }
  }

  // 收集函数声明（创建原型）
  std::vector<std::unique_ptr<ast::FunctionDecl>> funcDecls;
  for (auto &decl : program->declarations) {
    if (decl && decl->getType() == ast::NodeType::FunctionDecl) {
      auto funcDecl = std::unique_ptr<ast::FunctionDecl>(
          static_cast<ast::FunctionDecl *>(decl.release()));
      if (!funcDecl->templateParams.empty()) {
        continue; // 泛型声明由语义层单态化后追加的副本生成
      }
      c_hat::debugStream() << "Debug: Creating prototype for function: " << funcDecl->name
                << std::endl;
      createFunctionPrototype(funcDecl.get());
      funcDecls.push_back(std::move(funcDecl));
    }
  }

  // 生成函数体
  for (auto &funcDecl : funcDecls) {
    generateFunctionBody(std::move(funcDecl));
  }

  // 执行循环优化
  // TODO: optimizeLoops() 目前会把循环体误判为循环头，并把 body 中的
  // load 移到 header，破坏 SSA dominance。先禁用，等重写后再启用。
  // optimizeLoops();

  // 执行函数内联优化
  // optimizeFunctionInlining();  // TODO: 函数内联实现有 bug（指令复制的 valueMap 不完整），需要重写
}

llvm::Value *
LLVMCodeGenerator::generateDeclaration(std::unique_ptr<ast::Declaration> decl) {
  switch (decl->getType()) {
  case ast::NodeType::VariableDecl:
    return generateVariableDecl(std::unique_ptr<ast::VariableDecl>(
        static_cast<ast::VariableDecl *>(decl.release())));
  case ast::NodeType::TupleDestructuringDecl:
    return generateTupleDestructuringDecl(
        std::unique_ptr<ast::TupleDestructuringDecl>(
            static_cast<ast::TupleDestructuringDecl *>(decl.release())));
  case ast::NodeType::EnumDecl:
    return generateEnumDecl(std::unique_ptr<ast::EnumDecl>(
        static_cast<ast::EnumDecl *>(decl.release())));
  case ast::NodeType::InterfaceDecl:
    // 接口声明是类型级构造，不生成 LLVM IR
    return nullptr;
  case ast::NodeType::FunctionDecl:
    return generateFunctionDecl(std::unique_ptr<ast::FunctionDecl>(
        static_cast<ast::FunctionDecl *>(decl.release())));
  case ast::NodeType::ClassDecl:
    return generateClassDecl(std::unique_ptr<ast::ClassDecl>(
        static_cast<ast::ClassDecl *>(decl.release())));
  case ast::NodeType::StructDecl:
    return generateStructDecl(std::unique_ptr<ast::StructDecl>(
        static_cast<ast::StructDecl *>(decl.release())));
  case ast::NodeType::TypeAliasDecl:
    return generateTypeAliasDecl(std::unique_ptr<ast::TypeAliasDecl>(
        static_cast<ast::TypeAliasDecl *>(decl.release())));
  case ast::NodeType::GetterDecl:
    return generateGetterDecl(std::unique_ptr<ast::GetterDecl>(
        static_cast<ast::GetterDecl *>(decl.release())));
  case ast::NodeType::SetterDecl:
    return generateSetterDecl(std::unique_ptr<ast::SetterDecl>(
        static_cast<ast::SetterDecl *>(decl.release())));
  case ast::NodeType::ExtensionDecl:
    return generateExtensionDecl(std::unique_ptr<ast::ExtensionDecl>(
        static_cast<ast::ExtensionDecl *>(decl.release())));
  case ast::NodeType::ExternDecl:
    return generateExternDecl(std::unique_ptr<ast::ExternDecl>(
        static_cast<ast::ExternDecl *>(decl.release())));
  case ast::NodeType::NamespaceDecl:
    return generateNamespaceDecl(std::unique_ptr<ast::NamespaceDecl>(
        static_cast<ast::NamespaceDecl *>(decl.release())));
  case ast::NodeType::AttributeDecl:
  case ast::NodeType::ConceptDecl:
  case ast::NodeType::ModuleDecl:
  case ast::NodeType::ImportDecl:
  case ast::NodeType::ExportDecl:
    // 编译期构造，不生成 LLVM IR
    return nullptr;
  default:
    return nullptr;
  }
}

llvm::Value *LLVMCodeGenerator::generateVariableDecl(
    std::unique_ptr<ast::VariableDecl> varDecl) {
  llvm::Type *varType = nullptr;
  bool isSliceType = false;
  llvm::Type *sliceElementType = nullptr;

  // 无显式类型的普通表达式：先求值，再按值的 LLVM 类型分配变量。
  // 这样 `var x = a[0] + a[1]` 不会因为无法静态推断而退化成 int。
  auto *initializerMember =
      dynamic_cast<ast::MemberExpr *>(varDecl->initializer.get());
  if (!varDecl->type && varDecl->initializer &&
      (dynamic_cast<ast::BinaryExpr *>(varDecl->initializer.get()) ||
       dynamic_cast<ast::UnaryExpr *>(varDecl->initializer.get()) ||
       dynamic_cast<ast::SubscriptExpr *>(varDecl->initializer.get()) ||
       dynamic_cast<ast::ConditionalExpr *>(varDecl->initializer.get()) ||
       (initializerMember && initializerMember->member != "len" &&
        initializerMember->member != "ptr"))) {
    llvm::Value *initValue =
        generateExpression(std::move(varDecl->initializer));
    if (!initValue) {
      return nullptr;
    }
    llvm::AllocaInst *inferredAlloca =
        createEntryAlloca(initValue->getType(), varDecl->name);
    builder()->CreateStore(initValue, inferredAlloca);
    namedValues_[varDecl->name] = inferredAlloca;
    return inferredAlloca;
  }

  if (varDecl->type) {
    if (auto *typeNode = dynamic_cast<ast::Type *>(varDecl->type.get())) {
      varType = generateType(typeNode);
      // 检查是否是切片类型
      if (auto *sliceTypeNode =
              dynamic_cast<const ast::SliceType *>(varDecl->type.get())) {
        isSliceType = true;
        sliceElementType = generateType(sliceTypeNode->baseType.get());
      }
    }
  } else if (varDecl->initializer) {
    if (auto *literal =
            dynamic_cast<ast::Literal *>(varDecl->initializer.get())) {
      switch (literal->type) {
      case ast::Literal::Type::Integer:
        varType = llvm::Type::getInt32Ty(context());
        break;
      case ast::Literal::Type::Floating:
        varType = llvm::Type::getDoubleTy(context());
        break;
      case ast::Literal::Type::Boolean:
        varType = llvm::Type::getInt1Ty(context());
        break;
      case ast::Literal::Type::Character:
        varType = llvm::Type::getInt32Ty(context());
        break;
      case ast::Literal::Type::String:
        varType = getLiteralViewType();
        break;
      default:
        throw std::runtime_error(
            "Cannot infer type from literal for variable: " + varDecl->name);
      }
    } else if (auto *callExpr =
                   dynamic_cast<ast::CallExpr *>(varDecl->initializer.get())) {
      // 对于函数调用表达式，尝试从函数名获取函数类型
      if (auto *identifier =
              dynamic_cast<ast::Identifier *>(callExpr->callee.get())) {
        const std::string &funcName = identifier->name;
        // 优先使用语义层解析出的 mangled name；否则按 "name@..." 前缀查找。
        llvm::Function *func = nullptr;
        if (!callExpr->resolvedMangledName.empty()) {
          auto it = functions_.find(callExpr->resolvedMangledName);
          if (it != functions_.end()) {
            func = it->second;
          }
        }
        if (!func) {
          auto it = functions_.find(funcName);
          if (it != functions_.end()) {
            func = it->second;
          }
        }
        if (!func) {
          for (auto &[name, candidate] : functions_) {
            if (name.rfind(funcName + "@", 0) == 0) {
              func = candidate;
              break;
            }
          }
        }
        if (func) {
          varType = func->getReturnType();
        } else {
          // 如果函数未定义，暂时使用 int 作为默认类型
          varType = llvm::Type::getInt32Ty(context());
        }
      } else if (auto *member =
                     dynamic_cast<ast::MemberExpr *>(callExpr->callee.get())) {
        // 成员方法调用：用 Class_method 的返回类型推断变量类型。
        std::string methodName = member->structName + "_" + member->member;
        auto it = functions_.find(methodName);
        if (it != functions_.end()) {
          varType = it->second->getReturnType();
        } else {
          varType = llvm::Type::getInt32Ty(context());
        }
      } else {
        // 对于非标识符调用，暂时使用 int 作为默认类型
        varType = llvm::Type::getInt32Ty(context());
      }
    } else if (auto *identifier = dynamic_cast<ast::Identifier *>(
                   varDecl->initializer.get())) {
      // 对于标识符表达式，尝试从变量名获取变量类型
      const std::string &varName = identifier->name;
      // 检查是否是已定义的变量
      auto it = namedValues_.find(varName);
      if (it != namedValues_.end()) {
        llvm::Value *varValue = it->second;
        if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(varValue)) {
          varType = alloca->getAllocatedType();
        } else {
          // 如果变量不是 AllocaInst，暂时使用 int 作为默认类型
          varType = llvm::Type::getInt32Ty(context());
        }
      } else {
        // 如果变量未定义，暂时使用 int 作为默认类型
        varType = llvm::Type::getInt32Ty(context());
      }
    } else if (auto *memberExpr = dynamic_cast<ast::MemberExpr *>(
                   varDecl->initializer.get())) {
      // 对于成员表达式，推断类型
      std::string memberName = memberExpr->member;
      if (memberName == "len") {
        // len 字段是 i32 类型
        varType = llvm::Type::getInt32Ty(context());
      } else if (memberName == "ptr") {
        // ptr 字段是指针类型
        varType = llvm::PointerType::get(context(), 0);
      } else {
        // 对于其他成员，暂时使用 int 作为默认类型
        varType = llvm::Type::getInt32Ty(context());
      }
    } else if (auto *newExpr =
                   dynamic_cast<ast::NewExpr *>(varDecl->initializer.get())) {
      if (auto *arrayType =
              dynamic_cast<ast::ArrayType *>(newExpr->type.get())) {
        varType = getOrCreateSliceType(generateType(arrayType->baseType.get()));
      } else if (auto *allocatedType =
                     dynamic_cast<ast::Type *>(newExpr->type.get())) {
        llvm::Type *llvmAllocated = generateType(allocatedType);
        varType = llvmAllocated
                      ? static_cast<llvm::Type *>(llvmAllocated->getPointerTo())
                      : llvm::Type::getInt32Ty(context());
      } else {
        varType = llvm::Type::getInt32Ty(context());
      }
    } else if (auto *arrayInitExpr = dynamic_cast<ast::ArrayInitExpr *>(
                   varDecl->initializer.get())) {
      // 对于数组初始化表达式，生成切片类型
      std::string sliceTypeName = "Slice_int";

      // 检查是否已经存在该切片类型
      auto it = sliceTypes_.find(sliceTypeName);
      llvm::StructType *sliceStructType;
      if (it != sliceTypes_.end()) {
        sliceStructType = it->second;
      } else {
        // 生成元素类型
        llvm::Type *elementType = llvm::Type::getInt32Ty(context());

        // 创建切片类型的结构体
        std::vector<llvm::Type *> fields;
        fields.push_back(llvm::PointerType::get(elementType, 0)); // ptr
        fields.push_back(llvm::Type::getInt32Ty(context()));      // len
        sliceStructType =
            llvm::StructType::create(context(), fields, sliceTypeName);

        // 存储到 sliceTypes_ 映射中
        sliceTypes_[sliceTypeName] = sliceStructType;
      }

      varType = sliceStructType;
    } else {
      // 对于其他非字面量初始化器，暂时使用 int 作为默认类型
      varType = llvm::Type::getInt32Ty(context());
    }
  }

  if (varDecl->type) {
    if (auto *pointerType =
            dynamic_cast<ast::PointerType *>(varDecl->type.get())) {
      namedValuePointeeTypes_[varDecl->name] =
          generateType(pointerType->baseType.get());
    }
  }

  if (!varType) {
    // 添加调试信息

    c_hat::debugStream() << "Debug: varDecl->type = "
              << (varDecl->type ? "not null" : "null") << std::endl;
    c_hat::debugStream() << "Debug: varDecl->initializer = "
              << (varDecl->initializer ? "not null" : "null") << std::endl;
    if (varDecl->initializer) {
      c_hat::debugStream() << "Debug: initializer type = "
                << static_cast<int>(varDecl->initializer->getType())
                << std::endl;
    }
    throw std::runtime_error("Unknown variable type: " + varDecl->name);
  }

  if (varDecl->isConst && varDecl->initializer) {
    if (auto *literal =
            dynamic_cast<ast::Literal *>(varDecl->initializer.get())) {
      llvm::Constant *constValue = nullptr;
      switch (literal->type) {
      case ast::Literal::Type::Integer: {
        int64_t val = std::stoll(literal->value);
        constValue = llvm::ConstantInt::get(varType, val, true);
        break;
      }
      case ast::Literal::Type::Floating: {
        double val = std::stod(literal->value);
        constValue = llvm::ConstantFP::get(varType, val);
        break;
      }
      case ast::Literal::Type::Boolean: {
        bool val = (literal->value == "true");
        constValue = llvm::ConstantInt::get(varType, val ? 1 : 0, false);
        break;
      }
      case ast::Literal::Type::String: {
        // 对于字符串字面量，我们不使用 const 优化，因为 createStringLiteral
        // 返回的不是 Constant
        // 所以我们退出这个分支，让它走下面的普通变量分配路径
        goto normal_var_path;
      }
      default:
        throw std::runtime_error("Unsupported const literal type");
      }

      namedValues_[varDecl->name] = constValue;
      return constValue;
    } else {
      throw std::runtime_error(
          "Const declaration must have a literal initializer: " +
          varDecl->name);
    }
  }

normal_var_path:
  // 元组字面量：生成结构体值并存入局部 alloca。
  if (varDecl->initializer) {
    if (dynamic_cast<ast::TupleExpr *>(varDecl->initializer.get())) {
      llvm::Value *initValue =
          generateExpression(std::move(varDecl->initializer));
      if (initValue && initValue->getType()->isStructTy()) {
        llvm::AllocaInst *tupleAlloca =
            createEntryAlloca(initValue->getType(), varDecl->name);
        builder()->CreateStore(initValue, tupleAlloca);
        namedValues_[varDecl->name] = tupleAlloca;
        return tupleAlloca;
      }
    }
  }

  // 检查是否是数组初始化表达式
  if (varDecl->initializer) {
    if (auto *arrayInitExpr =
            dynamic_cast<ast::ArrayInitExpr *>(varDecl->initializer.get())) {
      // 对于数组初始化表达式，直接调用 generateArrayInitExpr
      // 它会返回一个 AllocaInst，我们直接使用它
      llvm::Value *initValue =
          generateExpression(std::move(varDecl->initializer));
      if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(initValue)) {
        namedValues_[varDecl->name] = alloca;
        return alloca;
      }
    }
  }

  llvm::AllocaInst *alloca = createEntryAlloca(varType, varDecl->name);

  // 切片初始化：数组字面量、数组变量、切片变量、new T[n] 或切片区间。
  if (isSliceType && sliceElementType && varDecl->initializer) {
    llvm::StructType *sliceType = getOrCreateSliceType(sliceElementType);
    if (auto *arrayInit =
            dynamic_cast<ast::ArrayInitExpr *>(varDecl->initializer.get())) {
      size_t arraySize = arrayInit->elements.size();
      llvm::ArrayType *arrayType =
          llvm::ArrayType::get(sliceElementType, arraySize);
      llvm::AllocaInst *tempArrayAlloca =
          createEntryAlloca(arrayType, "temp_array");

      for (size_t i = 0; i < arraySize; ++i) {
        auto &element = arrayInit->elements[i];
        llvm::Value *elementValue = generateExpression(std::move(element));
        llvm::Value *elementPtr = builder()->CreateGEP(
            arrayType, tempArrayAlloca,
            {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
             llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), i)},
            "element_ptr");
        builder()->CreateStore(elementValue, elementPtr);
      }

      llvm::Value *arrayPtr = builder()->CreateGEP(
          arrayType, tempArrayAlloca,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
           llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0)});
      llvm::Value *sliceValue = builder()->CreateInsertValue(
          llvm::UndefValue::get(sliceType), arrayPtr, 0);
      sliceValue = builder()->CreateInsertValue(
          sliceValue,
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), arraySize),
          1);
      builder()->CreateStore(sliceValue, alloca);
    } else {
      llvm::Value *initValue =
          generateExpression(std::move(varDecl->initializer));
      if (auto *initAlloca = llvm::dyn_cast<llvm::AllocaInst>(initValue)) {
        llvm::Type *allocatedType = initAlloca->getAllocatedType();
        if (allocatedType->isArrayTy()) {
          auto *arrayType = llvm::cast<llvm::ArrayType>(allocatedType);
          llvm::Value *arrayPtr = builder()->CreateGEP(
              arrayType, initAlloca,
              {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
               llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0)});
          llvm::Value *sliceValue = builder()->CreateInsertValue(
              llvm::UndefValue::get(sliceType), arrayPtr, 0);
          sliceValue = builder()->CreateInsertValue(
              sliceValue,
              llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                     arrayType->getNumElements()),
              1);
          builder()->CreateStore(sliceValue, alloca);
        } else if (isSliceStructType(allocatedType)) {
          llvm::Value *sliceValue =
              builder()->CreateLoad(allocatedType, initAlloca, "slice.load");
          builder()->CreateStore(sliceValue, alloca);
        } else {
          builder()->CreateStore(initValue, alloca);
        }
      } else if (initValue->getType()->isArrayTy()) {
        // int[] s = fixedArray; 数组值先落到临时数组，再取首元素指针。
        auto *arrayType = llvm::cast<llvm::ArrayType>(initValue->getType());
        llvm::AllocaInst *tmp =
            createEntryAlloca(arrayType, "slice.array.tmp");
        builder()->CreateStore(initValue, tmp);
        llvm::Value *arrayPtr = builder()->CreateGEP(
            arrayType, tmp,
            {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
             llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0)});
        llvm::Value *sliceValue = builder()->CreateInsertValue(
            llvm::UndefValue::get(sliceType), arrayPtr, 0);
        sliceValue = builder()->CreateInsertValue(
            sliceValue,
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                   arrayType->getNumElements()),
            1);
        builder()->CreateStore(sliceValue, alloca);
      } else {
        builder()->CreateStore(initValue, alloca);
      }
    }
  } else {
    // 对于非切片类型，正常处理
    if (varDecl->initializer) {
      llvm::Value *initValue =
          generateExpression(std::move(varDecl->initializer));

      // 检查是否是数组初始化表达式返回的切片结构体指针
      if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(initValue)) {
        llvm::Type *allocatedType = alloca->getAllocatedType();
        if (allocatedType->isStructTy()) {
          llvm::StructType *structType =
              dyn_cast<llvm::StructType>(allocatedType);
          if (structType) {
            std::string structName = structType->getName().str();
            // 检查是否是切片类型
            if (structName.find("Slice_") == 0) {
              // 对于切片类型，直接使用返回的 AllocaInst，不需要创建新的
              namedValues_[varDecl->name] = alloca;
              return alloca;
            }
          }
        }
      }

      // 如果初始化值的类型与变量类型不匹配，进行类型转换
      if (initValue->getType() != varType) {
        if (varType->isFloatingPointTy() &&
            initValue->getType()->isIntegerTy()) {
          // 整数转浮点数
          initValue = builder()->CreateSIToFP(initValue, varType, "casttmp");
        } else if (varType->isIntegerTy() &&
                   initValue->getType()->isFloatingPointTy()) {
          // 浮点数转整数
          initValue = builder()->CreateFPToSI(initValue, varType, "casttmp");
        } else if (varType->isIntegerTy() &&
                   initValue->getType()->isIntegerTy()) {
          // 整数类型之间的转换
          initValue =
              builder()->CreateIntCast(initValue, varType, true, "casttmp");
        }
      }

      builder()->CreateStore(initValue, alloca);
    }
  }

  namedValues_[varDecl->name] = alloca;
  return alloca;
}

llvm::Value *LLVMCodeGenerator::generateTupleDestructuringDecl(
    std::unique_ptr<ast::TupleDestructuringDecl> tupleDecl) {
  // 生成初始化表达式
  llvm::Value *initValue =
      generateExpression(std::move(tupleDecl->initializer));

  if (!initValue) {
    return nullptr;
  }

  // 元组表达式返回 alloca；函数调用可能返回结构体值。
  llvm::Value *tupleValue = initValue;
  llvm::StructType *tupleType = nullptr;
  if (auto *tupleAlloca = llvm::dyn_cast<llvm::AllocaInst>(initValue)) {
    tupleType = llvm::dyn_cast<llvm::StructType>(
        tupleAlloca->getAllocatedType());
    if (tupleType) {
      tupleValue =
          builder()->CreateLoad(tupleType, tupleAlloca, "tuple.load");
    }
  } else if (auto *structType =
                 llvm::dyn_cast<llvm::StructType>(initValue->getType())) {
    tupleType = structType;
  }
  if (!tupleType) {
    error("Tuple destructuring requires a tuple value");
    return nullptr;
  }

  for (size_t i = 0; i < tupleDecl->names.size(); ++i) {
    const std::string &name = tupleDecl->names[i];
    if (i >= tupleType->getNumElements()) {
      error("Tuple destructuring has too many names");
      return nullptr;
    }
    llvm::Value *elementValue =
        builder()->CreateExtractValue(tupleValue, static_cast<unsigned>(i),
                                      name + ".tuple.elem");
    llvm::AllocaInst *alloca =
        createEntryAlloca(elementValue->getType(), name);
    builder()->CreateStore(elementValue, alloca);
    namedValues_[name] = alloca;
  }

  return nullptr;
}

// 生成枚举声明
llvm::Value *LLVMCodeGenerator::generateEnumDecl(
    std::unique_ptr<ast::EnumDecl> enumDecl) {
  // 枚举值作为 int32 全局常量
  int currentValue = 0;
  for (auto &member : enumDecl->members) {
    if (auto *enumMember = dynamic_cast<ast::EnumMember *>(member.get())) {
      std::string globalName = enumDecl->name + "_" + enumMember->name;

      // 如果枚举成员有显式值，解析它
      if (enumMember->value) {
        if (auto *literal =
                dynamic_cast<ast::Literal *>(enumMember->value.get())) {
          if (literal->type == ast::Literal::Type::Integer) {
            currentValue = std::stoi(literal->value);
          }
        }
      }

      // 创建全局 int32 常量
      new llvm::GlobalVariable(
          *module(), llvm::Type::getInt32Ty(context()), true,
          llvm::GlobalValue::InternalLinkage,
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                 currentValue),
          globalName);

      currentValue++;
    }
  }
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateFunctionDecl(
    std::unique_ptr<ast::FunctionDecl> funcDecl) {
  llvm::Type *returnType = nullptr;
  if (funcDecl->returnType) {
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(funcDecl->returnType.get())) {
      returnType = generateType(typeNode);
    }
  }

  if (!returnType) {
    returnType = llvm::Type::getVoidTy(context());
  }

  std::vector<llvm::Type *> paramTypes;
  for (auto &paramNode : funcDecl->params) {
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      if (param->isVariadic) {
        continue;
      }
      llvm::Type *paramType = generateType(param->type.get());
      if (!paramType) {
        throw std::runtime_error("Unknown parameter type for function: " +
                                 funcDecl->name);
      }
      paramTypes.push_back(paramType);
    }
  }

  bool isVariadic = funcDecl->isVariadic;
  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, isVariadic);

  std::string funcName = funcDecl->name;

  // 检查函数是否已经存在
  llvm::Function *function = module()->getFunction(funcName);
  if (function) {
    // 函数已经存在，如果当前是函数声明（没有函数体），直接返回
    if (!funcDecl->body) {
      functions_[funcName] = function;
      return function;
    }
    // 如果函数已经存在且有函数体，报错
    if (!function->empty()) {
      throw std::runtime_error("Function already defined: " + funcName);
    }
    // 函数已经存在但没有函数体，我们需要添加函数体
    // 首先清除personality routine（如果有的话）
    if (function->getPersonalityFn()) {
      function->setPersonalityFn(nullptr);
    }
  } else {
    // 创建新函数
    function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                      funcName, module());
    functions_[funcName] = function;
  }

  // 外部函数没有函数体，直接返回
  if (!funcDecl->body) {
    return function;
  }
  llvm::BasicBlock *entryBlock =
      llvm::BasicBlock::Create(context(), "entry", function);
  builder()->SetInsertPoint(entryBlock);

  auto prevFunction = currentFunction_;
  currentFunction_ = function;

  namedValues_.clear();

  namedValuePointeeTypes_.clear();
  deferExpressions_.clear();
  deferFlags_.clear();
  lateVariables_.clear();

  size_t paramIndex = 0;
  auto argIt = function->args().begin();
  for (auto &paramNode : funcDecl->params) {
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      llvm::Value *arg = &(*argIt);
      arg->setName(param->name);

      llvm::AllocaInst *alloca =
          builder()->CreateAlloca(arg->getType(), nullptr, param->name);
      builder()->CreateStore(arg, alloca);
      namedValues_[param->name] = alloca;
      if (auto *pointerType =
              dynamic_cast<ast::PointerType *>(param->type.get())) {
        namedValuePointeeTypes_[param->name] =
            generateType(pointerType->baseType.get());
      }

      ++argIt;
      ++paramIndex;
    }
  }

  bool hasDefer = false;
  if (auto *bodyStmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
    hasDefer = statementContainsDefer(bodyStmt);
  }
  llvm::Value *functionPrevHandler = nullptr;
  llvm::BasicBlock *functionCleanupBB = nullptr;

  // 含 defer 的函数安装自己的 setjmp 清理帧。这样异常穿过本函数
  // （即使本函数没有 try）时，longjmp 会先回到这里执行 defer，
  // 再继续向外传播。
  if (hasDefer) {
    llvm::PointerType *ptrTy = llvm::PointerType::getUnqual(context());
    auto *excHandler = getExceptionHandlerGlobal(module());
    auto *jmpBuf = builder()->CreateAlloca(
        llvm::ArrayType::get(llvm::Type::getInt8Ty(context()), 256), nullptr,
        "fn.jmpbuf");
    jmpBuf->setAlignment(llvm::Align(16));
    auto *oldHandler =
        builder()->CreateAlloca(ptrTy, nullptr, "fn.oldhandler");
    functionPrevHandler =
        builder()->CreateLoad(ptrTy, excHandler, "fn.prev.handler");
    builder()->CreateStore(functionPrevHandler, oldHandler);
    builder()->CreateStore(jmpBuf, excHandler);
    exceptionHandlerStack_.push_back(oldHandler);

#ifdef _WIN32
    const char *setjmpName = "_setjmp";
#else
    const char *setjmpName = "setjmp";
#endif
    llvm::FunctionCallee setjmpFn = module()->getOrInsertFunction(
        setjmpName, llvm::FunctionType::get(llvm::Type::getInt32Ty(context()),
                                            {ptrTy}, false));
    if (auto *setjmpFunc =
            llvm::dyn_cast<llvm::Function>(setjmpFn.getCallee())) {
      setjmpFunc->addFnAttr(llvm::Attribute::ReturnsTwice);
    }
    llvm::Value *setjmpResult =
        builder()->CreateCall(setjmpFn, {jmpBuf}, "fn.setjmp");
    llvm::Value *thrown = builder()->CreateICmpNE(
        setjmpResult,
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
        "fn.thrown");
    functionCleanupBB =
        llvm::BasicBlock::Create(context(), "fn.cleanup", function);
    llvm::BasicBlock *bodyBB =
        llvm::BasicBlock::Create(context(), "fn.body", function);
    builder()->CreateCondBr(thrown, functionCleanupBB, bodyBB);
    builder()->SetInsertPoint(bodyBB);
  }

  if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
    generateStatement(std::unique_ptr<ast::Statement>(
        static_cast<ast::Statement *>(funcDecl->body.release())));

    llvm::BasicBlock *lastBlock = &function->back();

    // 先为异常路径生成清理块，再生成正常返回路径，避免正常路径
    // 消费掉 defer AST 后异常清理块无节点可生成。
    if (hasDefer) {
      exceptionHandlerStack_.pop_back();
      llvm::PointerType *ptrTy = llvm::PointerType::getUnqual(context());
      auto *excHandler = getExceptionHandlerGlobal(module());
      builder()->SetInsertPoint(functionCleanupBB);
      emitDeferCleanup(false);
      builder()->CreateStore(functionPrevHandler, excHandler);

      llvm::Value *outerHandler =
          builder()->CreateLoad(ptrTy, excHandler, "fn.outer.handler");
      llvm::BasicBlock *rethrowBB =
          llvm::BasicBlock::Create(context(), "fn.rethrow", function);
      llvm::BasicBlock *abortBB =
          llvm::BasicBlock::Create(context(), "fn.abort", function);
      llvm::Value *noHandler = builder()->CreateICmpEQ(
          outerHandler, llvm::ConstantPointerNull::get(ptrTy),
          "fn.no.handler");
      builder()->CreateCondBr(noHandler, abortBB, rethrowBB);

      builder()->SetInsertPoint(rethrowBB);
      llvm::FunctionCallee longjmpFn = module()->getOrInsertFunction(
          "longjmp",
          llvm::FunctionType::get(llvm::Type::getVoidTy(context()),
                                  {ptrTy, llvm::Type::getInt32Ty(context())},
                                  false));
      builder()->CreateCall(longjmpFn,
                            {outerHandler,
                             llvm::ConstantInt::get(
                                 llvm::Type::getInt32Ty(context()), 1)});
      builder()->CreateUnreachable();

      builder()->SetInsertPoint(abortBB);
      llvm::FunctionCallee abortFn = module()->getOrInsertFunction(
          "abort",
          llvm::FunctionType::get(llvm::Type::getVoidTy(context()), false));
      builder()->CreateCall(abortFn);
      builder()->CreateUnreachable();

      builder()->SetInsertPoint(lastBlock);
    }

    if (!lastBlock->getTerminator()) {
      // 为已初始化的 late 变量调用析构函数
      for (const auto &[varName, info] : lateVariables_) {
        if (info.isInitialized) {
          // 这里需要添加析构函数调用的代码
          // 目前暂时跳过，因为析构函数的实现还未完成
        }
      }

      // 执行 defer 语句（按相反顺序）
      emitDeferCleanup(true);
      deferExpressions_.clear();
      deferFlags_.clear();

      if (hasDefer) {
        auto *excHandler = getExceptionHandlerGlobal(module());
        builder()->CreateStore(functionPrevHandler, excHandler);
      }

      if (returnType->isVoidTy()) {
        builder()->CreateRetVoid();
      } else {
        builder()->CreateRet(llvm::Constant::getNullValue(returnType));
      }
    }
  }

  currentFunction_ = prevFunction;
  deferExpressions_.clear();
  deferFlags_.clear();

  return function;
}

void LLVMCodeGenerator::createFunctionPrototype(ast::FunctionDecl *funcDecl) {

  c_hat::debugStream() << "Debug: createFunctionPrototype - function name: "
            << funcDecl->name << std::endl;

  llvm::Type *returnType = nullptr;
  if (funcDecl->returnType) {
    c_hat::debugStream() << "Debug: createFunctionPrototype - has return type"
              << std::endl;
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(funcDecl->returnType.get())) {
      c_hat::debugStream() << "Debug: createFunctionPrototype - generating return type"
                << std::endl;
      returnType = generateType(typeNode);
      c_hat::debugStream() << "Debug: createFunctionPrototype - return type generated"
                << std::endl;
    }
  }

  if (!returnType) {
    returnType = llvm::Type::getVoidTy(context());
    c_hat::debugStream() << "Debug: createFunctionPrototype - using void return type"
              << std::endl;
  }
  c_hat::debugStream() << "Debug: createFunctionPrototype - return type: "
            << returnType->getTypeID() << std::endl;

  std::vector<llvm::Type *> paramTypes;
  c_hat::debugStream() << "Debug: createFunctionPrototype - processing parameters"
            << std::endl;
  for (auto &paramNode : funcDecl->params) {
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      c_hat::debugStream() << "Debug: createFunctionPrototype - processing parameter: "
                << param->name << std::endl;
      llvm::Type *paramType = generateType(param->type.get());
      if (!paramType) {
        throw std::runtime_error("Unknown parameter type for function: " +
                                 funcDecl->name);
      }
      paramTypes.push_back(paramType);
      c_hat::debugStream() << "Debug: createFunctionPrototype - parameter type: "
                << paramType->getTypeID() << std::endl;
    }
  }
  c_hat::debugStream() << "Debug: createFunctionPrototype - parameters processed"
            << std::endl;

  // 检查是否是可变参数函数
  bool isVariadic = false;
  if (!funcDecl->params.empty()) {
    auto *lastParam =
        dynamic_cast<ast::Parameter *>(funcDecl->params.back().get());
    if (lastParam && lastParam->isVariadic) {
      isVariadic = true;
      paramTypes
          .pop_back(); // 移除变参参数，因为 LLVM 函数类型中变参是单独标记的
    }
  }

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, isVariadic);

  // 为了支持重载，生成带参数类型的函数名
  std::string uniqueFuncName;
  // 添加命名空间路径
  auto it = functionNamespaces_.find(funcDecl);
  if (it != functionNamespaces_.end() && !it->second.empty()) {
    // 从 functionNamespaces_ 中获取函数的命名空间路径
    uniqueFuncName = it->second[0];
    for (size_t i = 1; i < it->second.size(); ++i) {
      uniqueFuncName += "." + it->second[i];
    }
  } else {
    // 从当前 namespaceStack_ 中获取命名空间路径
    if (!namespaceStack_.empty()) {
      uniqueFuncName = namespaceStack_[0];
      for (size_t i = 1; i < namespaceStack_.size(); ++i) {
        uniqueFuncName += "." + namespaceStack_[i];
      }
    }
  }
  if (!uniqueFuncName.empty()) {
    uniqueFuncName += ".";
  }
  uniqueFuncName += funcDecl->name;

  // 检查函数是否是 extern 声明的
  bool isExtern = funcDecl->specifiers.find("extern") != std::string::npos;

  // 对于非 extern 声明的函数，生成带参数类型的唯一函数名
  if (!isExtern && !paramTypes.empty()) {
    uniqueFuncName += "@";
    for (size_t i = 0; i < paramTypes.size(); ++i) {
      if (i > 0) {
        uniqueFuncName += "#";
      }
      std::string typeName;
      // 检查是否为结构体类型
      if (llvm::isa<llvm::StructType>(paramTypes[i])) {
        typeName = "struct";
      } else if (paramTypes[i]->isIntegerTy(1)) {
        typeName = "bool";
      } else if (paramTypes[i]->isIntegerTy(8)) {
        typeName = "byte";
      } else if (paramTypes[i]->isIntegerTy(32)) {
        typeName = "int";
      } else if (paramTypes[i]->isIntegerTy(64)) {
        typeName = "long";
      } else if (paramTypes[i]->isFloatTy()) {
        typeName = "float";
      } else if (paramTypes[i]->isDoubleTy()) {
        typeName = "double";
      } else if (paramTypes[i]->isPointerTy()) {
        typeName = "ptr";
      } else {
        typeName = "unknown";
      }
      uniqueFuncName += typeName;
    }
  }

  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, uniqueFuncName, module());

  // 只在函数有函数体时才设置 personality routine
  // 函数声明不应该有 personality routine
  if (funcDecl->body) {
    // 暂时移除个性函数的设置，因为系统中没有安装 LLVM
  }

  c_hat::debugStream() << "Debug: createFunctionPrototype - adding function to map"
            << std::endl;
  functions_[uniqueFuncName] = function;
}

llvm::Value *LLVMCodeGenerator::generateFunctionBody(
    std::unique_ptr<ast::FunctionDecl> funcDecl) {
  // 为了支持重载，生成带参数类型的函数名
  std::string uniqueFuncName;
  // 添加命名空间路径
  auto it = functionNamespaces_.find(funcDecl.get());
  if (it != functionNamespaces_.end() && !it->second.empty()) {
    // 从 functionNamespaces_ 中获取函数的命名空间路径
    uniqueFuncName = it->second[0];
    for (size_t i = 1; i < it->second.size(); ++i) {
      uniqueFuncName += "." + it->second[i];
    }
  } else {
    // 从当前 namespaceStack_ 中获取命名空间路径
    if (!namespaceStack_.empty()) {
      uniqueFuncName = namespaceStack_[0];
      for (size_t i = 1; i < namespaceStack_.size(); ++i) {
        uniqueFuncName += "." + namespaceStack_[i];
      }
    }
  }
  if (!uniqueFuncName.empty()) {
    uniqueFuncName += ".";
  }
  uniqueFuncName += funcDecl->name;
  std::vector<llvm::Type *> paramTypes;
  for (auto &paramNode : funcDecl->params) {
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      llvm::Type *paramType = generateType(param->type.get());
      if (!paramType) {
        throw std::runtime_error("Unknown parameter type for function: " +
                                 funcDecl->name);
      }
      paramTypes.push_back(paramType);
    }
  }
  if (!paramTypes.empty()) {
    uniqueFuncName += "@";
    for (size_t i = 0; i < paramTypes.size(); ++i) {
      if (i > 0) {
        uniqueFuncName += "#";
      }
      std::string typeName;
      // 检查是否为结构体类型
      if (llvm::isa<llvm::StructType>(paramTypes[i])) {
        typeName = "struct";
      } else if (paramTypes[i]->isIntegerTy(1)) {
        typeName = "bool";
      } else if (paramTypes[i]->isIntegerTy(8)) {
        typeName = "byte";
      } else if (paramTypes[i]->isIntegerTy(16)) {
        typeName = "short";
      } else if (paramTypes[i]->isIntegerTy(32)) {
        typeName = "int";
      } else if (paramTypes[i]->isIntegerTy(64)) {
        typeName = "long";
      } else if (paramTypes[i]->isFloatTy()) {
        typeName = "float";
      } else if (paramTypes[i]->isDoubleTy()) {
        typeName = "double";
      } else if (paramTypes[i]->isPointerTy()) {
        typeName = "ptr";
      } else {
        typeName = "unknown";
      }
      uniqueFuncName += typeName;
    }
  }

  llvm::Function *function = nullptr;
  auto funcIt = functions_.find(uniqueFuncName);
  if (funcIt == functions_.end()) {
    // 如果没找到带参数类型的函数名，尝试查找不带参数类型的函数名
    auto simpleFuncIt = functions_.find(funcDecl->name);
    if (simpleFuncIt == functions_.end()) {
      throw std::runtime_error("Function prototype not found for: " +
                               uniqueFuncName + " (or " + funcDecl->name + ")");
    }
    function = simpleFuncIt->second;
  } else {
    function = funcIt->second;
  }
  llvm::Type *returnType = function->getReturnType();

  if (funcDecl->body) {
    llvm::BasicBlock *entryBlock =
        llvm::BasicBlock::Create(context(), "entry", function);
    builder()->SetInsertPoint(entryBlock);

    auto prevFunction = currentFunction_;
    currentFunction_ = function;

    namedValues_.clear();

    namedValuePointeeTypes_.clear();
    deferExpressions_.clear();
    deferFlags_.clear();
    lateVariables_.clear();

    size_t paramIndex = 0;
    auto argIt = function->args().begin();
    for (auto &paramNode : funcDecl->params) {
      if (auto *param = dynamic_cast<ast::Parameter *>(
              funcDecl->params[paramIndex].get())) {
        llvm::Value *arg = &(*argIt);
        arg->setName(param->name);

        llvm::AllocaInst *alloca =
            builder()->CreateAlloca(arg->getType(), nullptr, param->name);
        builder()->CreateStore(arg, alloca);
        namedValues_[param->name] = alloca;

        ++argIt;
        ++paramIndex;
      }
    }

    bool hasDefer = false;
    if (auto *bodyStmt =
            dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      hasDefer = statementContainsDefer(bodyStmt);
    }
    llvm::Value *functionPrevHandler = nullptr;
    llvm::BasicBlock *functionCleanupBB = nullptr;

    if (hasDefer) {
      llvm::PointerType *ptrTy = llvm::PointerType::getUnqual(context());
      auto *excHandler = getExceptionHandlerGlobal(module());
      auto *jmpBuf = builder()->CreateAlloca(
          llvm::ArrayType::get(llvm::Type::getInt8Ty(context()), 256),
          nullptr, "fn.jmpbuf");
      jmpBuf->setAlignment(llvm::Align(16));
      auto *oldHandler =
          builder()->CreateAlloca(ptrTy, nullptr, "fn.oldhandler");
      functionPrevHandler =
          builder()->CreateLoad(ptrTy, excHandler, "fn.prev.handler");
      builder()->CreateStore(functionPrevHandler, oldHandler);
      builder()->CreateStore(jmpBuf, excHandler);
      exceptionHandlerStack_.push_back(oldHandler);

#ifdef _WIN32
      const char *setjmpName = "_setjmp";
#else
      const char *setjmpName = "setjmp";
#endif
      llvm::FunctionCallee setjmpFn = module()->getOrInsertFunction(
          setjmpName,
          llvm::FunctionType::get(llvm::Type::getInt32Ty(context()), {ptrTy},
                                  false));
      if (auto *setjmpFunc =
              llvm::dyn_cast<llvm::Function>(setjmpFn.getCallee())) {
        setjmpFunc->addFnAttr(llvm::Attribute::ReturnsTwice);
      }
      llvm::Value *setjmpResult =
          builder()->CreateCall(setjmpFn, {jmpBuf}, "fn.setjmp");
      llvm::Value *thrown = builder()->CreateICmpNE(
          setjmpResult,
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
          "fn.thrown");
      functionCleanupBB =
          llvm::BasicBlock::Create(context(), "fn.cleanup", function);
      llvm::BasicBlock *bodyBB =
          llvm::BasicBlock::Create(context(), "fn.body", function);
      builder()->CreateCondBr(thrown, functionCleanupBB, bodyBB);
      builder()->SetInsertPoint(bodyBB);
    }

    if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(funcDecl->body.release())));

      // 检查函数是否还有基本块
      if (!function->empty()) {
        llvm::BasicBlock *lastBlock = &function->back();

        if (hasDefer) {
          exceptionHandlerStack_.pop_back();
          llvm::PointerType *ptrTy = llvm::PointerType::getUnqual(context());
          auto *excHandler = getExceptionHandlerGlobal(module());
          builder()->SetInsertPoint(functionCleanupBB);
          emitDeferCleanup(false);
          builder()->CreateStore(functionPrevHandler, excHandler);

          llvm::Value *outerHandler =
              builder()->CreateLoad(ptrTy, excHandler, "fn.outer.handler");
          llvm::BasicBlock *rethrowBB =
              llvm::BasicBlock::Create(context(), "fn.rethrow", function);
          llvm::BasicBlock *abortBB =
              llvm::BasicBlock::Create(context(), "fn.abort", function);
          llvm::Value *noHandler = builder()->CreateICmpEQ(
              outerHandler, llvm::ConstantPointerNull::get(ptrTy),
              "fn.no.handler");
          builder()->CreateCondBr(noHandler, abortBB, rethrowBB);

          builder()->SetInsertPoint(rethrowBB);
          llvm::FunctionCallee longjmpFn = module()->getOrInsertFunction(
              "longjmp",
              llvm::FunctionType::get(llvm::Type::getVoidTy(context()),
                                      {ptrTy,
                                       llvm::Type::getInt32Ty(context())},
                                      false));
          builder()->CreateCall(longjmpFn,
                                {outerHandler,
                                 llvm::ConstantInt::get(
                                     llvm::Type::getInt32Ty(context()), 1)});
          builder()->CreateUnreachable();

          builder()->SetInsertPoint(abortBB);
          llvm::FunctionCallee abortFn = module()->getOrInsertFunction(
              "abort",
              llvm::FunctionType::get(llvm::Type::getVoidTy(context()), false));
          builder()->CreateCall(abortFn);
          builder()->CreateUnreachable();

          builder()->SetInsertPoint(lastBlock);
        }

        c_hat::debugStream() << "Debug: lastBlock = " << lastBlock->getName().str()
                  << std::endl;
        c_hat::debugStream() << "Debug: lastBlock has terminator = "
                  << (lastBlock->getTerminator() != nullptr) << std::endl;
        if (!lastBlock->getTerminator()) {
          builder()->SetInsertPoint(lastBlock);
          c_hat::debugStream() << "Debug: Adding default return statement" << std::endl;

          // 检查是否有异常
          if (currentFunctionHasTry_) {
            llvm::Type *intType = llvm::Type::getInt32Ty(context());

            // 获取全局异常标志变量
            llvm::GlobalVariable *exceptionFlag =
                module()->getNamedGlobal("exception_flag");
            if (exceptionFlag) {
              llvm::Value *flag =
                  builder()->CreateLoad(intType, exceptionFlag, "load_flag");
              llvm::Value *hasException = builder()->CreateICmpNE(
                  flag, llvm::ConstantInt::get(intType, 0), "has_exception");

              // 创建异常处理块
              llvm::BasicBlock *normalReturnBlock = llvm::BasicBlock::Create(
                  context(), "normal_return", function);
              llvm::BasicBlock *catchBlock = llvm::BasicBlock::Create(
                  context(), "exception_catch", function);

              // 条件分支
              builder()->SetInsertPoint(lastBlock);
              builder()->CreateCondBr(hasException, catchBlock,
                                      normalReturnBlock);

              // 生成 catch 块代码
              builder()->SetInsertPoint(catchBlock);

              // 处理 catch 块
              for (auto &catchStmt : catchStmts_) {
                if (catchStmt->param) {
                  // 从全局变量读取异常值
                  llvm::GlobalVariable *exceptionValueVar =
                      module()->getNamedGlobal("exception_value");
                  if (exceptionValueVar) {
                    llvm::AllocaInst *alloca = builder()->CreateAlloca(
                        intType, nullptr, catchStmt->param->name);
                    llvm::Value *loadedException = builder()->CreateLoad(
                        intType, exceptionValueVar, "load_exception");
                    builder()->CreateStore(loadedException, alloca);
                    namedValues_[catchStmt->param->name] = alloca;
                  }
                }
                generateStatement(std::move(catchStmt->body));
              }

              // 生成正常返回块代码
              builder()->SetInsertPoint(normalReturnBlock);
            }
          }

          // 为已初始化的 late 变量调用析构函数
          for (const auto &[varName, info] : lateVariables_) {
            if (info.isInitialized) {
              // 这里需要添加析构函数调用的代码
              // 目前暂时跳过，因为析构函数的实现还未完成
            }
          }

          // 执行 defer 语句（按相反顺序）
          emitDeferCleanup(true);
          deferExpressions_.clear();
          deferFlags_.clear();

          if (hasDefer) {
            auto *excHandler = getExceptionHandlerGlobal(module());
            builder()->CreateStore(functionPrevHandler, excHandler);
          }

          if (returnType->isVoidTy()) {
            builder()->CreateRetVoid();
          } else {
            // 对于 int 类型，返回 0
            if (returnType->isIntegerTy()) {
              builder()->CreateRet(llvm::ConstantInt::get(returnType, 0));
            } else {
              builder()->CreateRet(llvm::Constant::getNullValue(returnType));
            }
          }
        }
      }
    }

    // 清理 catch 块
    catchStmts_.clear();
    currentFunctionHasTry_ = false;

    c_hat::debugStream() << "Debug: Before setting currentFunction_ back" << std::endl;
    currentFunction_ = prevFunction;
    c_hat::debugStream() << "Debug: Before clearing deferExpressions_" << std::endl;
    deferExpressions_.clear();
    deferFlags_.clear();
  }

  c_hat::debugStream() << "Debug: Before returning function" << std::endl;

  // 重置标记
  currentFunctionHasTry_ = false;

  return function;
}

llvm::Value *LLVMCodeGenerator::generateClassDecl(
    std::unique_ptr<ast::ClassDecl> classDecl) {
  std::vector<llvm::Type *> memberTypes;
  std::unordered_map<std::string, unsigned> memberIndices;
  std::vector<std::unique_ptr<ast::Node>> functions;

  classBaseNames_[classDecl->name] = classDecl->baseClass;
  if (!classDecl->baseClass.empty()) {
    auto baseIt = structTypes_.find(classDecl->baseClass);
    if (baseIt != structTypes_.end()) {
      llvm::StructType *baseType = baseIt->second;
      for (unsigned i = 0; i < baseType->getNumElements(); ++i) {
        memberTypes.push_back(baseType->getElementType(i));
      }
      auto baseInfoIt = structInfo_.find(classDecl->baseClass);
      if (baseInfoIt != structInfo_.end()) {
        for (const auto &[name, index] : baseInfoIt->second) {
          memberIndices[name] = index;
        }
      }
    }
  }

  for (size_t i = 0; i < classDecl->members.size(); ++i) {
    auto &member = classDecl->members[i];
    if (auto *varDecl = dynamic_cast<ast::VariableDecl *>(member.get())) {
      if (auto *typeNode = dynamic_cast<ast::Type *>(varDecl->type.get())) {
        llvm::Type *memberType = generateType(typeNode);
        bool isStatic =
            varDecl->specifiers.find("static") != std::string::npos;
        if (isStatic) {
          llvm::Constant *initializer =
              llvm::Constant::getNullValue(memberType);
          if (auto *literal = dynamic_cast<ast::Literal *>(
                  varDecl->initializer.get())) {
            switch (literal->type) {
            case ast::Literal::Type::Integer:
              initializer = llvm::ConstantInt::get(
                  memberType, std::stoll(literal->value), true);
              break;
            case ast::Literal::Type::Floating:
              initializer =
                  llvm::ConstantFP::get(memberType,
                                        std::stod(literal->value));
              break;
            case ast::Literal::Type::Boolean:
              initializer = llvm::ConstantInt::get(
                  memberType, literal->value == "true" ? 1 : 0);
              break;
            case ast::Literal::Type::Character:
              initializer = llvm::ConstantInt::get(
                  memberType,
                  literal->value.empty() ? 0 : literal->value[0]);
              break;
            default:
              break;
            }
          }
          new llvm::GlobalVariable(
              *module(), memberType, false,
              llvm::GlobalValue::InternalLinkage, initializer,
              classDecl->name + "_" + varDecl->name);
          continue;
        }
        memberTypes.push_back(memberType);
        memberIndices[varDecl->name] =
            static_cast<unsigned>(memberTypes.size() - 1);
      }
    } else if (auto *funcDecl =
                   dynamic_cast<ast::FunctionDecl *>(member.get())) {
      functions.push_back(std::move(member));
    } else if (dynamic_cast<ast::GetterDecl *>(member.get()) ||
               dynamic_cast<ast::SetterDecl *>(member.get())) {
      functions.push_back(std::move(member));
    }
  }

  if (memberTypes.empty()) {
    memberTypes.push_back(llvm::Type::getInt8Ty(context()));
  }

  llvm::StructType *classType =
      llvm::StructType::create(context(), memberTypes, classDecl->name);
  structTypes_[classDecl->name] = classType;
  structInfo_[classDecl->name] = memberIndices;

  for (auto &funcNode : functions) {
    if (dynamic_cast<ast::FunctionDecl *>(funcNode.get())) {
      auto funcDecl = std::unique_ptr<ast::FunctionDecl>(
          static_cast<ast::FunctionDecl *>(funcNode.release()));
      bool isConstructor = (funcDecl->name == classDecl->name);
      bool isDestructor = (!funcDecl->name.empty() && funcDecl->name[0] == '~');
      generateClassMemberFunction(std::move(funcDecl), classDecl->name,
                                  isConstructor, isDestructor);
    } else if (dynamic_cast<ast::GetterDecl *>(funcNode.get())) {
      auto getterDecl = std::unique_ptr<ast::GetterDecl>(
          static_cast<ast::GetterDecl *>(funcNode.release()));
      generateGetterDecl(std::move(getterDecl), classDecl->name);
    } else if (dynamic_cast<ast::SetterDecl *>(funcNode.get())) {
      auto setterDecl = std::unique_ptr<ast::SetterDecl>(
          static_cast<ast::SetterDecl *>(funcNode.release()));
      generateSetterDecl(std::move(setterDecl), classDecl->name);
    }
  }

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateStructDecl(
    std::unique_ptr<ast::StructDecl> structDecl) {
  std::vector<llvm::Type *> memberTypes;
  std::unordered_map<std::string, unsigned> memberIndices;
  std::vector<std::unique_ptr<ast::Node>> functions;

  for (auto &member : structDecl->members) {
    if (auto *varDecl = dynamic_cast<ast::VariableDecl *>(member.get())) {
      if (auto *typeNode = dynamic_cast<ast::Type *>(varDecl->type.get())) {
        llvm::Type *memberType = generateType(typeNode);
        memberTypes.push_back(memberType);
        memberIndices[varDecl->name] =
            static_cast<unsigned>(memberTypes.size() - 1);
      }
    } else if (auto *funcDecl =
                   dynamic_cast<ast::FunctionDecl *>(member.get())) {
      functions.push_back(std::move(member));
    } else if (dynamic_cast<ast::GetterDecl *>(member.get()) ||
               dynamic_cast<ast::SetterDecl *>(member.get())) {
      functions.push_back(std::move(member));
    }
  }

  if (memberTypes.empty()) {
    memberTypes.push_back(llvm::Type::getInt8Ty(context()));
  }

  llvm::StructType *structType =
      llvm::StructType::create(context(), memberTypes, structDecl->name);
  structTypes_[structDecl->name] = structType;
  structInfo_[structDecl->name] = memberIndices;

  for (auto &funcNode : functions) {
    if (auto *funcDecl =
            dynamic_cast<ast::FunctionDecl *>(funcNode.get())) {
      auto owned = std::unique_ptr<ast::FunctionDecl>(
          static_cast<ast::FunctionDecl *>(funcNode.release()));
      bool isConstructor = (owned->name == structDecl->name);
      bool isDestructor =
          (!owned->name.empty() && owned->name[0] == '~');
      generateClassMemberFunction(std::move(owned), structDecl->name,
                                  isConstructor, isDestructor);
    } else if (auto *getterDecl =
                   dynamic_cast<ast::GetterDecl *>(funcNode.get())) {
      auto owned = std::unique_ptr<ast::GetterDecl>(
          static_cast<ast::GetterDecl *>(funcNode.release()));
      generateGetterDecl(std::move(owned), structDecl->name);
    } else if (auto *setterDecl =
                   dynamic_cast<ast::SetterDecl *>(funcNode.get())) {
      auto owned = std::unique_ptr<ast::SetterDecl>(
          static_cast<ast::SetterDecl *>(funcNode.release()));
      generateSetterDecl(std::move(owned), structDecl->name);
    }
  }

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateClassMemberFunction(
    std::unique_ptr<ast::FunctionDecl> funcDecl, const std::string &className,
    bool isConstructor, bool isDestructor) {
  llvm::StructType *classType = structTypes_[className];
  llvm::PointerType *thisType = classType->getPointerTo();

  std::vector<llvm::Type *> paramTypes;
  if (!funcDecl->isStatic) {
    paramTypes.push_back(thisType);
  }

  for (auto &paramNode : funcDecl->params) {
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      if (param->type) {
        if (auto *typeNode = dynamic_cast<ast::Type *>(param->type.get())) {
          llvm::Type *paramType = generateType(typeNode);
          paramTypes.push_back(paramType);
        }
      }
    }
  }

  llvm::Type *returnType = llvm::Type::getVoidTy(context());
  if (!isConstructor && !isDestructor && funcDecl->returnType) {
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(funcDecl->returnType.get())) {
      returnType = generateType(typeNode);
    }
  }

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, false);

  std::string mangledName = className + "_" + funcDecl->name;
  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, mangledName, module());

  if (funcDecl->body) {
    llvm::BasicBlock *entryBlock =
        llvm::BasicBlock::Create(context(), "entry", function);
    builder()->SetInsertPoint(entryBlock);

    auto prevFunction = currentFunction_;
    currentFunction_ = function;

    namedValues_.clear();

    namedValuePointeeTypes_.clear();
    deferExpressions_.clear();
    deferFlags_.clear();
    lateVariables_.clear();

    auto prevClassName = currentClassName_;
    currentClassName_ = className;

    // 处理 self 指针（静态方法没有 self）。
    auto argIt = function->args().begin();
    llvm::Value *thisArg = nullptr;
    if (!funcDecl->isStatic) {
      thisArg = &(*argIt++);
      thisArg->setName("self");
      // 成员函数体内通过 self 访问当前对象；单继承下 base 与 self 同址。
      namedValues_["self"] = thisArg;
      namedValues_["base"] = thisArg;
    }

    // 处理其他参数
    size_t paramIndex = 0;
    for (; argIt != function->args().end(); ++argIt, ++paramIndex) {
      if (auto *param = dynamic_cast<ast::Parameter *>(
              funcDecl->params[paramIndex].get())) {
        llvm::Value *arg = &(*argIt);
        arg->setName(param->name);

        llvm::AllocaInst *alloca =
            builder()->CreateAlloca(arg->getType(), nullptr, param->name);
        builder()->CreateStore(arg, alloca);
        namedValues_[param->name] = alloca;
        if (auto *pointerType =
                dynamic_cast<ast::PointerType *>(param->type.get())) {
          namedValuePointeeTypes_[param->name] =
              generateType(pointerType->baseType.get());
        }
      }
    }

    // 派生类构造函数：调用基类构造函数。
    if (isConstructor) {
      auto baseIt = classBaseNames_.find(className);
      if (baseIt != classBaseNames_.end() && !baseIt->second.empty()) {
        std::string baseClassName = baseIt->second;
        auto baseCtorIt =
            functions_.find(baseClassName + "_" + baseClassName);
        if (baseCtorIt != functions_.end() &&
            (funcDecl->baseCall ||
             baseCtorIt->second->arg_size() == 1)) {
          std::vector<llvm::Value *> baseArgs = {thisArg};
          if (funcDecl->baseCall) {
            llvm::Value *baseArg =
                generateExpression(std::move(funcDecl->baseCall));
            if (baseArg) {
              baseArgs.push_back(baseArg);
            }
          }
          builder()->CreateCall(baseCtorIt->second, baseArgs);
        }
      }
    }

    bool hasDefer = false;
    if (auto *bodyStmt =
            dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      hasDefer = statementContainsDefer(bodyStmt);
    }
    llvm::Value *functionPrevHandler = nullptr;
    llvm::BasicBlock *functionCleanupBB = nullptr;

    if (hasDefer) {
      llvm::PointerType *ptrTy = llvm::PointerType::getUnqual(context());
      auto *excHandler = getExceptionHandlerGlobal(module());
      auto *jmpBuf = builder()->CreateAlloca(
          llvm::ArrayType::get(llvm::Type::getInt8Ty(context()), 256),
          nullptr, "fn.jmpbuf");
      jmpBuf->setAlignment(llvm::Align(16));
      auto *oldHandler =
          builder()->CreateAlloca(ptrTy, nullptr, "fn.oldhandler");
      functionPrevHandler =
          builder()->CreateLoad(ptrTy, excHandler, "fn.prev.handler");
      builder()->CreateStore(functionPrevHandler, oldHandler);
      builder()->CreateStore(jmpBuf, excHandler);
      exceptionHandlerStack_.push_back(oldHandler);

#ifdef _WIN32
      const char *setjmpName = "_setjmp";
#else
      const char *setjmpName = "setjmp";
#endif
      llvm::FunctionCallee setjmpFn = module()->getOrInsertFunction(
          setjmpName,
          llvm::FunctionType::get(llvm::Type::getInt32Ty(context()), {ptrTy},
                                  false));
      if (auto *setjmpFunc =
              llvm::dyn_cast<llvm::Function>(setjmpFn.getCallee())) {
        setjmpFunc->addFnAttr(llvm::Attribute::ReturnsTwice);
      }
      llvm::Value *setjmpResult =
          builder()->CreateCall(setjmpFn, {jmpBuf}, "fn.setjmp");
      llvm::Value *thrown = builder()->CreateICmpNE(
          setjmpResult,
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
          "fn.thrown");
      functionCleanupBB =
          llvm::BasicBlock::Create(context(), "fn.cleanup", function);
      llvm::BasicBlock *bodyBB =
          llvm::BasicBlock::Create(context(), "fn.body", function);
      builder()->CreateCondBr(thrown, functionCleanupBB, bodyBB);
      builder()->SetInsertPoint(bodyBB);
    }

    // 处理函数体
    if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(funcDecl->body.release())));

      llvm::BasicBlock *lastBlock = &function->back();

      if (hasDefer) {
        exceptionHandlerStack_.pop_back();
        llvm::PointerType *ptrTy = llvm::PointerType::getUnqual(context());
        auto *excHandler = getExceptionHandlerGlobal(module());
        builder()->SetInsertPoint(functionCleanupBB);
        emitDeferCleanup(false);
        builder()->CreateStore(functionPrevHandler, excHandler);

        llvm::Value *outerHandler =
            builder()->CreateLoad(ptrTy, excHandler, "fn.outer.handler");
        llvm::BasicBlock *rethrowBB =
            llvm::BasicBlock::Create(context(), "fn.rethrow", function);
        llvm::BasicBlock *abortBB =
            llvm::BasicBlock::Create(context(), "fn.abort", function);
        llvm::Value *noHandler = builder()->CreateICmpEQ(
            outerHandler, llvm::ConstantPointerNull::get(ptrTy),
            "fn.no.handler");
        builder()->CreateCondBr(noHandler, abortBB, rethrowBB);

        builder()->SetInsertPoint(rethrowBB);
        llvm::FunctionCallee longjmpFn = module()->getOrInsertFunction(
            "longjmp",
            llvm::FunctionType::get(llvm::Type::getVoidTy(context()),
                                    {ptrTy,
                                     llvm::Type::getInt32Ty(context())},
                                    false));
        builder()->CreateCall(longjmpFn,
                              {outerHandler,
                               llvm::ConstantInt::get(
                                   llvm::Type::getInt32Ty(context()), 1)});
        builder()->CreateUnreachable();

        builder()->SetInsertPoint(abortBB);
        llvm::FunctionCallee abortFn = module()->getOrInsertFunction(
            "abort",
            llvm::FunctionType::get(llvm::Type::getVoidTy(context()), false));
        builder()->CreateCall(abortFn);
        builder()->CreateUnreachable();

        builder()->SetInsertPoint(lastBlock);
      }

      if (!lastBlock->getTerminator()) {
        builder()->SetInsertPoint(lastBlock);
        // 为已初始化的 late 变量调用析构函数
        for (const auto &[varName, info] : lateVariables_) {
          if (info.isInitialized) {
            // 这里需要添加析构函数调用的代码
            // 目前暂时跳过，因为析构函数的实现还未完成
          }
        }

        // 执行 defer 语句（按相反顺序）
        emitDeferCleanup(true);
        deferExpressions_.clear();
        deferFlags_.clear();

        if (hasDefer) {
          auto *excHandler = getExceptionHandlerGlobal(module());
          builder()->CreateStore(functionPrevHandler, excHandler);
        }

        if (returnType->isVoidTy()) {
          builder()->CreateRetVoid();
        } else {
          builder()->CreateRet(llvm::Constant::getNullValue(returnType));
        }
      }
    }

    currentFunction_ = prevFunction;
    currentClassName_ = prevClassName;
    deferExpressions_.clear();
    deferFlags_.clear();
  }

  functions_[mangledName] = function;
  return function;
}

llvm::AllocaInst *
LLVMCodeGenerator::createEntryAlloca(llvm::Type *type,
                                     const std::string &name) {
  llvm::Function *func = currentFunction_;
  if (!func) {
    llvm::BasicBlock *block = builder()->GetInsertBlock();
    if (!block) {
      return builder()->CreateAlloca(type, nullptr, name);
    }
    func = block->getParent();
  }
  llvm::IRBuilder<> entryBuilder(&func->getEntryBlock(),
                                 func->getEntryBlock().begin());
  return entryBuilder.CreateAlloca(type, nullptr, name);
}

llvm::StructType *
LLVMCodeGenerator::getOrCreateSliceType(llvm::Type *elementType) {
  if (!elementType) {
    elementType = llvm::Type::getInt32Ty(context());
  }
  auto existing = sliceTypesByElement_.find(elementType);
  if (existing != sliceTypesByElement_.end()) {
    return existing->second;
  }

  std::string suffix = "int";
  if (elementType->isIntegerTy(1)) {
    suffix = "bool";
  } else if (elementType->isIntegerTy(8)) {
    suffix = "byte";
  } else if (elementType->isIntegerTy(16)) {
    suffix = "short";
  } else if (elementType->isIntegerTy(32)) {
    suffix = "int";
  } else if (elementType->isIntegerTy(64)) {
    suffix = "long";
  } else if (elementType->isFloatTy()) {
    suffix = "float";
  } else if (elementType->isDoubleTy()) {
    suffix = "double";
  } else if (elementType->isPointerTy()) {
    suffix = "ptr";
  } else if (auto *structType = llvm::dyn_cast<llvm::StructType>(elementType)) {
    suffix = structType->hasName() ? structType->getName().str() : "struct";
    for (char &c : suffix) {
      if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
        c = '_';
      }
    }
  }

  std::string name = "Slice_" + suffix;
  if (sliceTypes_.find(name) != sliceTypes_.end()) {
    name += "_" + std::to_string(sliceTypes_.size());
  }
  llvm::StructType *sliceType = llvm::StructType::create(
      context(),
      {llvm::PointerType::getUnqual(context()),
       llvm::Type::getInt32Ty(context())},
      name);
  sliceTypes_[name] = sliceType;
  sliceTypesByElement_[elementType] = sliceType;
  sliceElementTypes_[sliceType] = elementType;
  return sliceType;
}

llvm::Type *
LLVMCodeGenerator::getSliceElementType(llvm::StructType *sliceType) {
  if (!sliceType) {
    return llvm::Type::getInt32Ty(context());
  }
  auto it = sliceElementTypes_.find(sliceType);
  if (it != sliceElementTypes_.end()) {
    return it->second;
  }
  std::string name = sliceType->hasName() ? sliceType->getName().str() : "";
  if (name.find("byte") != std::string::npos) {
    return llvm::Type::getInt8Ty(context());
  }
  if (name.find("double") != std::string::npos) {
    return llvm::Type::getDoubleTy(context());
  }
  if (name.find("float") != std::string::npos) {
    return llvm::Type::getFloatTy(context());
  }
  if (name.find("long") != std::string::npos) {
    return llvm::Type::getInt64Ty(context());
  }
  if (name.find("short") != std::string::npos) {
    return llvm::Type::getInt16Ty(context());
  }
  if (name.find("bool") != std::string::npos) {
    return llvm::Type::getInt1Ty(context());
  }
  if (name.find("ptr") != std::string::npos) {
    return llvm::PointerType::getUnqual(context());
  }
  return llvm::Type::getInt32Ty(context());
}

bool LLVMCodeGenerator::isSliceStructType(llvm::Type *type) {
  auto *structType = llvm::dyn_cast_or_null<llvm::StructType>(type);
  if (!structType) {
    return false;
  }
  if (sliceElementTypes_.find(structType) != sliceElementTypes_.end()) {
    return true;
  }
  if (!structType->hasName()) {
    return false;
  }
  return structType->getName().str().rfind("Slice_", 0) == 0;
}

// 辅助函数：获取 literalview 类型
llvm::Type *LLVMCodeGenerator::getLiteralViewType() {
  if (literalViewType_) {
    return literalViewType_;
  }

  // 创建 literalview 类型：包含指针和长度
  std::vector<llvm::Type *> fields;
  fields.push_back(
      llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0)); // ptr
  fields.push_back(llvm::Type::getInt64Ty(context()));              // len
  literalViewType_ = llvm::StructType::create(context(), fields, "literalview");

  return literalViewType_;
}

// 生成命名空间声明
llvm::Value *LLVMCodeGenerator::generateNamespaceDecl(
    std::unique_ptr<ast::NamespaceDecl> namespaceDecl) {
  // 将命名空间名称添加到命名空间栈
  namespaceStack_.push_back(namespaceDecl->name);

  // 生成命名空间中的声明
  for (auto &decl : namespaceDecl->members) {
    if (auto *declaration = dynamic_cast<ast::Declaration *>(decl.get())) {
      generateDeclaration(std::unique_ptr<ast::Declaration>(
          static_cast<ast::Declaration *>(decl.release())));
    }
  }

  // 从命名空间栈中弹出
  namespaceStack_.pop_back();

  return nullptr;
}

// 生成语句
llvm::Value *
LLVMCodeGenerator::generateStatement(std::unique_ptr<ast::Statement> stmt) {
  if (!stmt) {
    return nullptr;
  }

  if (auto *variableStmt = dynamic_cast<ast::VariableStmt *>(stmt.get())) {
    auto declaration = std::move(variableStmt->declaration);
    return generateVariableDecl(std::move(declaration));
  }

  if (auto *tupleStmt =
          dynamic_cast<ast::TupleDestructuringStmt *>(stmt.get())) {
    auto declaration = std::move(tupleStmt->declaration);
    return generateTupleDestructuringDecl(std::move(declaration));
  }

  switch (stmt->getType()) {
  case ast::NodeType::ExprStmt:
    return generateExprStmt(std::unique_ptr<ast::ExprStmt>(
        static_cast<ast::ExprStmt *>(stmt.release())));
  case ast::NodeType::CompoundStmt:
    return generateCompoundStmt(std::unique_ptr<ast::CompoundStmt>(
        static_cast<ast::CompoundStmt *>(stmt.release())));
  case ast::NodeType::ReturnStmt:
    return generateReturnStmt(std::unique_ptr<ast::ReturnStmt>(
        static_cast<ast::ReturnStmt *>(stmt.release())));
  case ast::NodeType::IfStmt:
    return generateIfStmt(std::unique_ptr<ast::IfStmt>(
        static_cast<ast::IfStmt *>(stmt.release())));
  case ast::NodeType::WhileStmt:
    return generateWhileStmt(std::unique_ptr<ast::WhileStmt>(
        static_cast<ast::WhileStmt *>(stmt.release())));
  case ast::NodeType::DoWhileStmt:
    return generateDoWhileStmt(std::unique_ptr<ast::DoWhileStmt>(
        static_cast<ast::DoWhileStmt *>(stmt.release())));
  case ast::NodeType::ForStmt:
    return generateForStmt(std::unique_ptr<ast::ForStmt>(
        static_cast<ast::ForStmt *>(stmt.release())));
  case ast::NodeType::BreakStmt:
    return generateBreakStmt(std::unique_ptr<ast::BreakStmt>(
        static_cast<ast::BreakStmt *>(stmt.release())));
  case ast::NodeType::ContinueStmt:
    return generateContinueStmt(std::unique_ptr<ast::ContinueStmt>(
        static_cast<ast::ContinueStmt *>(stmt.release())));
  case ast::NodeType::MatchStmt:
    return generateMatchStmt(std::unique_ptr<ast::MatchStmt>(
        static_cast<ast::MatchStmt *>(stmt.release())));
  case ast::NodeType::TryStmt:
    return generateTryStmt(std::unique_ptr<ast::TryStmt>(
        static_cast<ast::TryStmt *>(stmt.release())));
  case ast::NodeType::ThrowStmt:
    return generateThrowStmt(std::unique_ptr<ast::ThrowStmt>(
        static_cast<ast::ThrowStmt *>(stmt.release())));
  case ast::NodeType::DeferStmt:
    return generateDeferStmt(std::unique_ptr<ast::DeferStmt>(
        static_cast<ast::DeferStmt *>(stmt.release())));
  case ast::NodeType::YieldStmt:
    return generateYieldStmt(std::unique_ptr<ast::YieldStmt>(
        static_cast<ast::YieldStmt *>(stmt.release())));
  case ast::NodeType::GotoStmt:
    return generateGotoStmt(std::unique_ptr<ast::GotoStmt>(
        static_cast<ast::GotoStmt *>(stmt.release())));
  case ast::NodeType::LabelStmt:
    return generateLabelStmt(std::unique_ptr<ast::LabelStmt>(
        static_cast<ast::LabelStmt *>(stmt.release())));
  case ast::NodeType::ComptimeStmt: {
    auto comptime = std::unique_ptr<ast::ComptimeStmt>(
        static_cast<ast::ComptimeStmt *>(stmt.release()));
    if (comptime->stmt) {
      // 检查是否是 ComptimeForStmt
      if (auto *comptimeFor = dynamic_cast<ast::ComptimeForStmt *>(comptime->stmt.get())) {
        if (comptimeFor->forStmt) {
          return generateForStmt(std::unique_ptr<ast::ForStmt>(
              static_cast<ast::ForStmt *>(comptimeFor->forStmt.release())));
        }
      }
      return generateStatement(std::move(comptime->stmt));
    }
    return nullptr;
  }
  case ast::NodeType::ComptimeForStmt: {
    auto comptimeFor = std::unique_ptr<ast::ComptimeForStmt>(
        static_cast<ast::ComptimeForStmt *>(stmt.release()));
    if (comptimeFor->forStmt) {
      return generateForStmt(std::unique_ptr<ast::ForStmt>(
          static_cast<ast::ForStmt *>(comptimeFor->forStmt.release())));
    }
    return nullptr;
  }
  case ast::NodeType::ComptimeIfStmt: {
    auto comptimeIf = std::unique_ptr<ast::ComptimeIfStmt>(
        static_cast<ast::ComptimeIfStmt *>(stmt.release()));
    if (comptimeIf->condition && comptimeIf->thenBranch) {
      return generateIfStmt(std::make_unique<ast::IfStmt>(
          std::move(comptimeIf->condition),
          std::move(comptimeIf->thenBranch),
          std::move(comptimeIf->elseBranch)));
    }
    return nullptr;
  }
  default:
    return nullptr;
  }
}

// 生成表达式
llvm::Value *
LLVMCodeGenerator::generateExpression(std::unique_ptr<ast::Expression> expr) {
  if (!expr) {
    return nullptr;
  }

  switch (expr->getType()) {
  case ast::NodeType::Literal:
    return generateLiteral(std::unique_ptr<ast::Literal>(
        static_cast<ast::Literal *>(expr.release())));
  case ast::NodeType::Identifier:
    return generateIdentifier(std::unique_ptr<ast::Identifier>(
        static_cast<ast::Identifier *>(expr.release())));
  case ast::NodeType::BinaryExpr:
    return generateBinaryExpr(std::unique_ptr<ast::BinaryExpr>(
        static_cast<ast::BinaryExpr *>(expr.release())));
  case ast::NodeType::UnaryExpr:
    return generateUnaryExpr(std::unique_ptr<ast::UnaryExpr>(
        static_cast<ast::UnaryExpr *>(expr.release())));
  case ast::NodeType::CallExpr:
    return generateCallExpr(std::unique_ptr<ast::CallExpr>(
        static_cast<ast::CallExpr *>(expr.release())));
  case ast::NodeType::MemberExpr:
    return generateMemberExpr(std::unique_ptr<ast::MemberExpr>(
        static_cast<ast::MemberExpr *>(expr.release())));
  case ast::NodeType::SubscriptExpr:
    return generateSubscriptExpr(std::unique_ptr<ast::SubscriptExpr>(
        static_cast<ast::SubscriptExpr *>(expr.release())));
  case ast::NodeType::SelfExpr:
    return generateSelfExpr(std::unique_ptr<ast::SelfExpr>(
        static_cast<ast::SelfExpr *>(expr.release())));
  case ast::NodeType::BaseExpr:
    return generateBaseExpr(std::unique_ptr<ast::BaseExpr>(
        static_cast<ast::BaseExpr *>(expr.release())));
  case ast::NodeType::NewExpr:
    return generateNewExpr(std::unique_ptr<ast::NewExpr>(
        static_cast<ast::NewExpr *>(expr.release())));
  case ast::NodeType::DeleteExpr:
    return generateDeleteExpr(std::unique_ptr<ast::DeleteExpr>(
        static_cast<ast::DeleteExpr *>(expr.release())));
  case ast::NodeType::FoldExpr:
    return generateFoldExpr(std::unique_ptr<ast::FoldExpr>(
        static_cast<ast::FoldExpr *>(expr.release())));
  case ast::NodeType::ExpansionExpr:
    return generateExpansionExpr(std::unique_ptr<ast::ExpansionExpr>(
        static_cast<ast::ExpansionExpr *>(expr.release())));
  case ast::NodeType::ArrayInitExpr:
    return generateArrayInitExpr(std::unique_ptr<ast::ArrayInitExpr>(
        static_cast<ast::ArrayInitExpr *>(expr.release())));
  case ast::NodeType::StructInitExpr:
    return generateStructInitExpr(std::unique_ptr<ast::StructInitExpr>(
        static_cast<ast::StructInitExpr *>(expr.release())));
  case ast::NodeType::TupleExpr:
    return generateTupleExpr(std::unique_ptr<ast::TupleExpr>(
        static_cast<ast::TupleExpr *>(expr.release())));
  case ast::NodeType::LambdaExpr:
    return generateLambdaExpr(std::unique_ptr<ast::LambdaExpr>(
        static_cast<ast::LambdaExpr *>(expr.release())));
  case ast::NodeType::ReflectionExpr:
    return generateReflectionExpr(std::unique_ptr<ast::ReflectionExpr>(
        static_cast<ast::ReflectionExpr *>(expr.release())));
  case ast::NodeType::ConditionalExpr:
    return generateConditionalExpr(std::unique_ptr<ast::ConditionalExpr>(
        static_cast<ast::ConditionalExpr *>(expr.release())));
  case ast::NodeType::TypeIsExpr:
    return generateTypeIsExpr(std::unique_ptr<ast::TypeIsExpr>(
        static_cast<ast::TypeIsExpr *>(expr.release())));
  case ast::NodeType::TypeofExpr:
    return generateTypeofExpr(std::unique_ptr<ast::TypeofExpr>(
        static_cast<ast::TypeofExpr *>(expr.release())));
  case ast::NodeType::MetaFieldAccessExpr:
    return generateMetaFieldAccessExpr(std::unique_ptr<ast::MetaFieldAccessExpr>(
        static_cast<ast::MetaFieldAccessExpr *>(expr.release())));
  case ast::NodeType::BuiltinVarExpr:
    return generateBuiltinVarExpr(std::unique_ptr<ast::BuiltinVarExpr>(
        static_cast<ast::BuiltinVarExpr *>(expr.release())));
  default:
    return nullptr;
  }
}

// 辅助函数：获取 AST 类型的名称
std::string LLVMCodeGenerator::getTypeName(ast::Type *type) {
  if (!type) {
    return "int";
  }

  if (auto *primitiveType = dynamic_cast<ast::PrimitiveType *>(type)) {
    switch (primitiveType->kind) {
    case ast::PrimitiveType::Kind::Int:
      return "int";
    case ast::PrimitiveType::Kind::Double:
      return "double";
    case ast::PrimitiveType::Kind::Float:
      return "float";
    case ast::PrimitiveType::Kind::Bool:
      return "bool";
    case ast::PrimitiveType::Kind::Byte:
      return "byte";
    case ast::PrimitiveType::Kind::Char:
      return "char";
    case ast::PrimitiveType::Kind::Void:
      return "void";
    default:
      return "int";
    }
  } else if (auto *namedType = dynamic_cast<ast::NamedType *>(type)) {
    return namedType->name;
  } else if (auto *sliceType = dynamic_cast<ast::SliceType *>(type)) {
    return "Slice_" + getTypeName(sliceType->baseType.get());
  }

  return "int";
}

// 辅助函数：获取 LLVM 类型的名称
std::string LLVMCodeGenerator::getLLVMTypeName(llvm::Type *type) {
  if (!type) {
    return "int";
  }

  if (type->isIntegerTy()) {
    return "int";
  } else if (type->isDoubleTy()) {
    return "double";
  } else if (type->isFloatTy()) {
    return "float";
  } else if (type->isPointerTy()) {
    return "ptr";
  } else if (type->isStructTy()) {
    return "struct";
  } else if (type->isArrayTy()) {
    return "array";
  }

  return "int";
}

// 辅助函数：获取函数签名
std::string LLVMCodeGenerator::mangleFunctionName(
    const std::string &funcName, const std::vector<ast::Type *> &paramTypes) {
  std::string uniqueName = funcName;
  for (auto *paramType : paramTypes) {
    uniqueName += "_" + getTypeName(paramType);
  }
  return uniqueName;
}

// 循环不变式外提辅助函数
bool LLVMCodeGenerator::isLoopInvariant(llvm::Instruction *inst,
                                        llvm::BasicBlock *loopHeader) {
  // 检查指令是否是循环不变式
  for (unsigned i = 0; i < inst->getNumOperands(); ++i) {
    if (llvm::Value *op = inst->getOperand(i)) {
      if (llvm::Instruction *opInst = llvm::dyn_cast<llvm::Instruction>(op)) {
        if (opInst->getParent() != loopHeader &&
            !llvm::isa<llvm::Argument>(op) &&
            !llvm::isa<llvm::GlobalValue>(op)) {
          // 操作数在循环内定义，不是不变式
          return false;
        }
      }
    }
  }
  return true;
}

// 循环优化方法
void LLVMCodeGenerator::optimizeLoops() {
  // 遍历所有函数
  for (llvm::Function &func : *module()) {
    if (func.isDeclaration())
      continue;

    // 简单的循环检测和不变式外提
    for (llvm::BasicBlock &bb : func) {
      // 检查是否是循环头
      auto predBegin = llvm::pred_begin(&bb);
      auto predEnd = llvm::pred_end(&bb);
      size_t numPredecessors = std::distance(predBegin, predEnd);

      // 循环头至少有一个前驱，且其中一个前驱是循环体
      bool hasBackEdge = false;
      for (auto it = predBegin; it != predEnd; ++it) {
        llvm::BasicBlock *pred = *it;
        if (pred->getTerminator() &&
            pred->getTerminator()->getSuccessor(0) == &bb) {
          hasBackEdge = true;
          break;
        }
      }

      if (hasBackEdge) {
        // 这是一个循环头
        llvm::BasicBlock *loopHeader = &bb;
        llvm::BasicBlock *preHeader = nullptr;

        // 寻找前置块
        for (auto it = predBegin; it != predEnd; ++it) {
          llvm::BasicBlock *pred = *it;
          if (pred != loopHeader && pred->getTerminator() &&
              pred->getTerminator()->getSuccessor(0) == loopHeader) {
            preHeader = pred;
            break;
          }
        }

        if (preHeader) {
          // 收集循环不变式指令
          std::vector<llvm::Instruction *> invariants;
          for (llvm::Instruction &inst : *loopHeader) {
            if (!inst.isTerminator() && isLoopInvariant(&inst, loopHeader)) {
              invariants.push_back(&inst);
            }
          }

          // 将不变式指令移动到前置块
          for (llvm::Instruction *inst : invariants) {
            inst->moveBefore(preHeader->getTerminator());
          }
        }
      }
    }
  }
}

// 函数内联优化方法
void LLVMCodeGenerator::optimizeFunctionInlining() {
  // 简单的函数内联实现
  // 遍历所有函数
  for (auto &func : *module()) {
    // 遍历所有基本块
    for (auto &block : func) {
      // 遍历所有指令
      for (auto &inst : block) {
        // 检查是否是函数调用指令
        if (auto *callInst = dyn_cast<llvm::CallInst>(&inst)) {
          // 检查是否是直接函数调用
          if (auto *callee = callInst->getCalledFunction()) {
            // 简单的内联判断：只内联小函数（指令数少于10）
            if (!callee->isDeclaration() && callee->size() < 10) {
              // 开始内联过程
              llvm::BasicBlock *callBlock = callInst->getParent();
              llvm::BasicBlock *insertBlock =
                  callBlock->splitBasicBlock(callInst);

              // 为内联代码创建新的基本块
              llvm::BasicBlock *inlineBlock = llvm::BasicBlock::Create(
                  context(), "inline", &func, insertBlock);

              // 跳转到内联块
              callBlock->getTerminator()->setSuccessor(0, inlineBlock);

              // 创建新的 IRBuilder 用于内联代码
              llvm::IRBuilder<> inlineBuilder(inlineBlock);

              // 映射参数到实际值
              std::map<llvm::Value *, llvm::Value *> valueMap;
              for (unsigned i = 0; i < callee->arg_size(); ++i) {
                valueMap[callee->getArg(i)] = callInst->getOperand(i);
              }

              // 复制函数体指令
              for (const llvm::BasicBlock &calleeBlock : *callee) {
                if (calleeBlock.isEntryBlock())
                  continue;

                // 创建新的基本块
                llvm::BasicBlock *newBlock = llvm::BasicBlock::Create(
                    context(), calleeBlock.getName(), &func);

                // 创建新的 IRBuilder 用于这个块
                llvm::IRBuilder<> blockBuilder(newBlock);

                // 复制指令
                for (const llvm::Instruction &calleeInst : calleeBlock) {
                  llvm::Instruction *newInst = calleeInst.clone();

                  // 替换操作数
                  for (unsigned i = 0; i < newInst->getNumOperands(); ++i) {
                    if (llvm::Value *op = newInst->getOperand(i)) {
                      if (valueMap.count(op)) {
                        newInst->setOperand(i, valueMap[op]);
                      }
                    }
                  }

                  // 插入新指令
                  blockBuilder.Insert(newInst);
                }
              }

              // 连接内联块到插入块
              inlineBuilder.CreateBr(insertBlock);

              // 删除原始的函数调用指令
              callInst->eraseFromParent();
            }
          }
        }
      }
    }
  }
}

// 生成 new 表达式
llvm::Value *
LLVMCodeGenerator::generateNewExpr(std::unique_ptr<ast::NewExpr> newExpr) {
  // 生成类型 - 需要将 Node* 转换为 Type*
  ast::Type *typeNode = dynamic_cast<ast::Type *>(newExpr->type.get());
  if (!typeNode) {
    error("Invalid type in new expression");
    return nullptr;
  }

  // new T[n]：分配连续数组并返回 { ptr, len } 切片。
  if (auto *arrayType = dynamic_cast<ast::ArrayType *>(typeNode)) {
    llvm::Type *elementType = generateType(arrayType->baseType.get());
    if (!elementType) {
      elementType = llvm::Type::getInt32Ty(context());
    }
    if (!arrayType->size) {
      error("Array new expression requires a size");
      return nullptr;
    }
    llvm::Value *count = generateExpression(arrayType->size->clone());
    if (!count) {
      return nullptr;
    }
    llvm::Value *count64 = count;
    if (!count64->getType()->isIntegerTy(64)) {
      count64 = builder()->CreateIntCast(
          count64, llvm::Type::getInt64Ty(context()), false, "new.count64");
    }
    llvm::Value *count32 = count;
    if (count32->getType() != llvm::Type::getInt32Ty(context())) {
      count32 = builder()->CreateIntCast(
          count32, llvm::Type::getInt32Ty(context()), false, "new.count32");
    }

    uint64_t elementSize =
        module()->getDataLayout().getTypeAllocSize(elementType).getFixedValue();
    llvm::Value *bytes = builder()->CreateMul(
        count64,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context()), elementSize),
        "new.bytes");
    llvm::FunctionCallee mallocFn = module()->getOrInsertFunction(
        "malloc", llvm::FunctionType::get(
                      llvm::PointerType::getUnqual(context()),
                      {llvm::Type::getInt64Ty(context())}, false));
    llvm::Value *raw =
        builder()->CreateCall(mallocFn, {bytes}, "malloc.result");
    llvm::Value *ptr = builder()->CreatePointerCast(
        raw, llvm::PointerType::getUnqual(context()), "array.ptr");

    llvm::StructType *sliceType = getOrCreateSliceType(elementType);
    llvm::Value *sliceValue =
        builder()->CreateInsertValue(llvm::UndefValue::get(sliceType), ptr, 0);
    sliceValue = builder()->CreateInsertValue(sliceValue, count32, 1);
    return sliceValue;
  }

  llvm::Type *type = generateType(typeNode);
  if (!type) {
    error("Unknown type in new expression");
    return nullptr;
  }

  // 计算类型大小
  llvm::Type *int64Type = llvm::Type::getInt64Ty(context());
  llvm::Constant *sizeConstant = llvm::ConstantInt::get(
      int64Type, module()->getDataLayout().getTypeSizeInBits(type) / 8);

  // 获取 malloc 函数
  llvm::FunctionType *mallocType = llvm::FunctionType::get(
      llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0), {int64Type},
      false);
  llvm::FunctionCallee mallocFunc =
      module()->getOrInsertFunction("malloc", mallocType);

  // 调用 malloc
  llvm::Value *ptr =
      builder()->CreateCall(mallocFunc, {sizeConstant}, "malloc.result");

  // 类型转换
  ptr = builder()->CreatePointerCast(ptr, type->getPointerTo(), "cast.ptr");

  // 如果是类类型，调用构造函数
  if (type->isStructTy()) {
    std::string structName = type->getStructName().str();
    if (!structName.empty()) {
      // 查找构造函数
      std::string constructorName = structName + "_" + structName;
      auto it = functions_.find(constructorName);
      if (it != functions_.end()) {
        llvm::Function *constructor = it->second;

        // 构造函数签名是 (this, arg0, arg1, ...)。这里必须把 new 表达式的
        // 实参一起传进去，否则 LLVM 会报 "Incorrect number of arguments"。
        std::vector<llvm::Value *> ctorArgs;
        ctorArgs.push_back(ptr);
        for (auto &arg : newExpr->args) {
          llvm::Value *argValue = generateExpression(std::move(arg));
          if (!argValue) {
            error("Failed to generate constructor argument");
            return nullptr;
          }
          ctorArgs.push_back(argValue);
        }
        builder()->CreateCall(constructor, ctorArgs);
      }
    }
  }

  return ptr;
}

// 生成 delete 表达式
llvm::Value *LLVMCodeGenerator::generateDeleteExpr(
    std::unique_ptr<ast::DeleteExpr> deleteExpr) {
  llvm::Value *exprValue = generateExpression(std::move(deleteExpr->expr));
  if (!exprValue) {
    return nullptr;
  }

  // 切片值 { ptr, len } 释放底层 ptr；指针值直接释放。
  llvm::Value *ptrValue = exprValue;
  llvm::Type *valueType = exprValue->getType();
  if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(exprValue)) {
    llvm::Type *allocatedType = alloca->getAllocatedType();
    if (isSliceStructType(allocatedType)) {
      llvm::Value *sliceValue =
          builder()->CreateLoad(allocatedType, alloca, "delete.slice");
      ptrValue = builder()->CreateExtractValue(sliceValue, 0, "delete.ptr");
    } else if (allocatedType->isPointerTy()) {
      ptrValue =
          builder()->CreateLoad(allocatedType, alloca, "delete.ptr.load");
    }
  } else if (valueType->isStructTy() && isSliceStructType(valueType)) {
    ptrValue = builder()->CreateExtractValue(exprValue, 0, "delete.ptr");
  }

  // 类指针：调用 ClassName_~ClassName(this)。
  if (!deleteExpr->typeName.empty()) {
    std::string dtorName =
        deleteExpr->typeName + "_~" + deleteExpr->typeName;
    auto it = functions_.find(dtorName);
    if (it != functions_.end()) {
      builder()->CreateCall(it->second, {ptrValue});
    }
  }

  llvm::FunctionType *freeType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(context()),
      {llvm::PointerType::getUnqual(context())}, false);
  llvm::FunctionCallee freeFunc =
      module()->getOrInsertFunction("free", freeType);
  llvm::Value *voidPtr = builder()->CreatePointerCast(
      ptrValue, llvm::PointerType::getUnqual(context()), "void.ptr");
  builder()->CreateCall(freeFunc, {voidPtr});
  return nullptr;
}

// 生成类型
llvm::Type *LLVMCodeGenerator::generateType(ast::Type *type) {
  if (!type) {
    return nullptr;
  }

  // 检查类型缓存
  auto it = typeCache_.find(type);
  if (it != typeCache_.end()) {
    return it->second;
  }

  llvm::Type *llvmType = nullptr;

  if (auto *primitiveType = dynamic_cast<ast::PrimitiveType *>(type)) {
    switch (primitiveType->kind) {
    case ast::PrimitiveType::Kind::Int:
      llvmType = llvm::Type::getInt32Ty(context());
      break;
    case ast::PrimitiveType::Kind::Double:
      llvmType = llvm::Type::getDoubleTy(context());
      break;
    case ast::PrimitiveType::Kind::Float:
      llvmType = llvm::Type::getFloatTy(context());
      break;
    case ast::PrimitiveType::Kind::Bool:
      llvmType = llvm::Type::getInt1Ty(context());
      break;
    case ast::PrimitiveType::Kind::Byte:
      llvmType = llvm::Type::getInt8Ty(context());
      break;
    case ast::PrimitiveType::Kind::Char:
      llvmType = llvm::Type::getInt32Ty(context());
      break;
    case ast::PrimitiveType::Kind::Void:
      llvmType = llvm::Type::getVoidTy(context());
      break;
    default:
      llvmType = llvm::Type::getInt32Ty(context());
      break;
    }
  } else if (auto *genericType = dynamic_cast<ast::GenericType *>(type)) {
    // 泛型单态化后的结构体名：Box<int> -> Box__int
    std::string specializedName = genericType->name + "__";
    for (size_t i = 0; i < genericType->arguments.size(); ++i) {
      if (i > 0) {
        specializedName += "_";
      }
      auto *argType =
          dynamic_cast<ast::Type *>(genericType->arguments[i].get());
      std::string argName = argType ? argType->toString() : "unknown";
      for (char c : argName) {
        specializedName +=
            (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
      }
    }
    auto structIt = structTypes_.find(specializedName);
    if (structIt != structTypes_.end()) {
      llvmType = structIt->second;
    } else {
      llvmType = llvm::Type::getInt32Ty(context());
    }
  } else if (auto *namedType = dynamic_cast<ast::NamedType *>(type)) {
    // 检查是否是结构体类型
    auto structIt = structTypes_.find(namedType->name);
    if (structIt != structTypes_.end()) {
      llvmType = structIt->second;
    } else {
      // 如果不是结构体类型，暂时使用 int 作为默认类型
      llvmType = llvm::Type::getInt32Ty(context());
    }
  } else if (auto *sliceType = dynamic_cast<ast::SliceType *>(type)) {
    // 生成切片类型：{ element*, i32 len }
    llvm::Type *elementType = generateType(sliceType->baseType.get());
    llvmType = getOrCreateSliceType(elementType);
  } else if (auto *arrayType = dynamic_cast<ast::ArrayType *>(type)) {
    // 固定长度数组：只支持字面量长度；动态长度只用于 new T[n]。
    llvm::Type *elementType = generateType(arrayType->baseType.get());
    size_t arraySize = 0;
    if (auto *sizeLiteral =
            dynamic_cast<ast::Literal *>(arrayType->size.get())) {
      if (sizeLiteral->type == ast::Literal::Type::Integer) {
        arraySize = static_cast<size_t>(std::stoull(sizeLiteral->value));
      }
    }
    llvmType = llvm::ArrayType::get(elementType, arraySize);
  } else if (auto *tupleType = dynamic_cast<ast::TupleType *>(type)) {
    std::vector<llvm::Type *> elementTypes;
    for (const auto &element : tupleType->elementTypes) {
      llvm::Type *elementType = generateType(element.get());
      if (elementType) {
        elementTypes.push_back(elementType);
      }
    }
    llvmType = llvm::StructType::get(context(), elementTypes);
  } else if (auto *pointerType = dynamic_cast<ast::PointerType *>(type)) {
    // 生成指针类型
    llvm::Type *pointeeType = generateType(pointerType->baseType.get());
    if (!pointeeType) {
      pointeeType = llvm::Type::getInt8Ty(context());
    }
    llvmType = llvm::PointerType::get(pointeeType, 0);
  } else {
    // 对于其他类型，暂时使用 int 作为默认类型
    llvmType = llvm::Type::getInt32Ty(context());
  }

  // 将类型添加到缓存中
  typeCache_[type] = llvmType;

  return llvmType;
}

// 错误处理方法
void LLVMCodeGenerator::error(const std::string &message, ast::Node *node) {
  std::cerr << "Error: " << message;
  if (node) {
    // 这里可以添加行号和列号信息
    // 假设 Node 类有 getLocation() 方法
    // auto loc = node->getLocation();
    // std::cerr << " at line " << loc.line << ", column " << loc.column;
  }
  std::cerr << std::endl;
  errorCount_++;
  hasErrors_ = true;
}

// 警告处理方法
void LLVMCodeGenerator::warning(const std::string &message, ast::Node *node) {
  std::cerr << "Warning: " << message;
  if (node) {
    // 这里可以添加行号和列号信息
    // 假设 Node 类有 getLocation() 方法
    // auto loc = node->getLocation();
    // std::cerr << " at line " << loc.line << ", column " << loc.column;
  }
  std::cerr << std::endl;
}

// 生成字面量
llvm::Value *
LLVMCodeGenerator::generateLiteral(std::unique_ptr<ast::Literal> literal) {
  switch (literal->type) {
  case ast::Literal::Type::Integer:
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                  std::stoll(literal->value));
  case ast::Literal::Type::Floating:
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context()),
                                 std::stod(literal->value));
  case ast::Literal::Type::Boolean:
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context()),
                                  literal->value == "true" ? 1 : 0);
  case ast::Literal::Type::Character:
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                  literal->value.empty() ? 0
                                                         : literal->value[0]);
  case ast::Literal::Type::String: {
    std::string strValue = literal->value;
    if (strValue.size() >= 2 && strValue.front() == '"' &&
        strValue.back() == '"') {
      strValue = strValue.substr(1, strValue.size() - 2);
    }
    std::string processedStr;
    for (size_t i = 0; i < strValue.size(); ++i) {
      if (strValue[i] == '\\' && i + 1 < strValue.size()) {
        switch (strValue[i + 1]) {
        case 'n':
          processedStr += '\n';
          break;
        case 't':
          processedStr += '\t';
          break;
        case 'r':
          processedStr += '\r';
          break;
        case '\\':
          processedStr += '\\';
          break;
        case '"':
          processedStr += '"';
          break;
        case '0':
          processedStr += '\0';
          break;
        default:
          processedStr += strValue[i + 1];
          break;
        }
        ++i;
      } else {
        processedStr += strValue[i];
      }
    }
    llvm::Constant *strConstant =
        llvm::ConstantDataArray::getString(context(), processedStr, true);
    llvm::GlobalVariable *globalStr = new llvm::GlobalVariable(
        *module(), strConstant->getType(), true,
        llvm::GlobalValue::PrivateLinkage, strConstant, ".str");
    llvm::Value *zero =
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
    llvm::Value *indices[] = {zero, zero};
    return builder()->CreateInBoundsGEP(
        strConstant->getType(), globalStr,
        llvm::ArrayRef<llvm::Value *>(indices, 2));
  }
  default:
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
  }
}

// 生成标识符
llvm::Value *
LLVMCodeGenerator::generateIdentifier(std::unique_ptr<ast::Identifier> ident) {
  auto it = namedValues_.find(ident->name);
  if (it != namedValues_.end()) {
    llvm::Value *value = it->second;
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(value)) {
      return builder()->CreateLoad(alloca->getAllocatedType(), alloca,
                                   ident->name + ".load");
    }
    return value;
  }

  // 静态字段：ClassName_field。
  if (!currentClassName_.empty()) {
    if (auto *global = module()->getNamedGlobal(
            currentClassName_ + "_" + ident->name)) {
      return builder()->CreateLoad(global->getValueType(), global,
                                   ident->name + ".static");
    }
  }

  // 成员函数体内的裸字段名（如 self.x 写成 x）按当前类的字段解析。
  if (!currentClassName_.empty()) {
    auto classIt = structInfo_.find(currentClassName_);
    auto thisIt = namedValues_.find("self");
    if (classIt != structInfo_.end() && thisIt != namedValues_.end()) {
      auto fieldIt = classIt->second.find(ident->name);
      if (fieldIt != classIt->second.end()) {
        llvm::StructType *structTy = structTypes_[currentClassName_];
        llvm::Value *fieldPtr = builder()->CreateStructGEP(
            structTy, thisIt->second, fieldIt->second, ident->name + ".ptr");
        return builder()->CreateLoad(structTy->getElementType(fieldIt->second),
                                     fieldPtr, ident->name + ".load");
      }
    }
  }

  error("Unknown variable: " + ident->name);
  return nullptr;
}

// 生成二元表达式
llvm::Value *LLVMCodeGenerator::generateBinaryExpr(
    std::unique_ptr<ast::BinaryExpr> binaryExpr) {
  switch (binaryExpr->op) {
  case ast::BinaryExpr::Op::AddAssign:
  case ast::BinaryExpr::Op::SubAssign:
  case ast::BinaryExpr::Op::MulAssign:
  case ast::BinaryExpr::Op::DivAssign:
  case ast::BinaryExpr::Op::ModAssign:
  case ast::BinaryExpr::Op::AndAssign:
  case ast::BinaryExpr::Op::OrAssign:
  case ast::BinaryExpr::Op::XorAssign:
  case ast::BinaryExpr::Op::ShlAssign:
  case ast::BinaryExpr::Op::ShrAssign: {
    // 复合赋值：lhs op= rhs  等价于  lhs = lhs op rhs。
    // 这里先取左值地址，再 load / 运算 / store，避免把左值当成普通值。
    auto op = binaryExpr->op;
    llvm::Value *lhsPtr = getExpressionLValue(std::move(binaryExpr->left));
    llvm::Value *rhs = generateExpression(std::move(binaryExpr->right));
    if (!lhsPtr || !rhs) {
      return nullptr;
    }

    llvm::Type *valueType = rhs->getType();
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(lhsPtr)) {
      valueType = alloca->getAllocatedType();
    }

    llvm::Value *lhs =
        builder()->CreateLoad(valueType, lhsPtr, "compound.load");
    if (lhs->getType() != rhs->getType()) {
      if (lhs->getType()->isFloatingPointTy() && rhs->getType()->isIntegerTy()) {
        rhs = builder()->CreateSIToFP(rhs, lhs->getType(), "compound.cast");
      } else if (lhs->getType()->isIntegerTy() &&
                 rhs->getType()->isFloatingPointTy()) {
        rhs = builder()->CreateFPToSI(rhs, lhs->getType(), "compound.cast");
      } else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
        rhs = builder()->CreateIntCast(rhs, lhs->getType(), true,
                                       "compound.cast");
      }
    }

    llvm::Value *result = nullptr;
    switch (op) {
    case ast::BinaryExpr::Op::AddAssign:
      result = lhs->getType()->isFloatingPointTy()
                   ? builder()->CreateFAdd(lhs, rhs, "compound.add")
                   : builder()->CreateAdd(lhs, rhs, "compound.add");
      break;
    case ast::BinaryExpr::Op::SubAssign:
      result = lhs->getType()->isFloatingPointTy()
                   ? builder()->CreateFSub(lhs, rhs, "compound.sub")
                   : builder()->CreateSub(lhs, rhs, "compound.sub");
      break;
    case ast::BinaryExpr::Op::MulAssign:
      result = lhs->getType()->isFloatingPointTy()
                   ? builder()->CreateFMul(lhs, rhs, "compound.mul")
                   : builder()->CreateMul(lhs, rhs, "compound.mul");
      break;
    case ast::BinaryExpr::Op::DivAssign:
      result = lhs->getType()->isFloatingPointTy()
                   ? builder()->CreateFDiv(lhs, rhs, "compound.div")
                   : builder()->CreateSDiv(lhs, rhs, "compound.div");
      break;
    case ast::BinaryExpr::Op::ModAssign:
      result = builder()->CreateSRem(lhs, rhs, "compound.rem");
      break;
    case ast::BinaryExpr::Op::AndAssign:
      result = builder()->CreateAnd(lhs, rhs, "compound.and");
      break;
    case ast::BinaryExpr::Op::OrAssign:
      result = builder()->CreateOr(lhs, rhs, "compound.or");
      break;
    case ast::BinaryExpr::Op::XorAssign:
      result = builder()->CreateXor(lhs, rhs, "compound.xor");
      break;
    case ast::BinaryExpr::Op::ShlAssign:
      result = builder()->CreateShl(lhs, rhs, "compound.shl");
      break;
    case ast::BinaryExpr::Op::ShrAssign:
      result = builder()->CreateAShr(lhs, rhs, "compound.shr");
      break;
    default:
      break;
    }

    if (!result) {
      error("Unknown compound assignment operator");
      return nullptr;
    }
    builder()->CreateStore(result, lhsPtr);
    return result;
  }
  case ast::BinaryExpr::Op::Assign: {
    // 属性 setter：ClassName_set_prop(this, value)。
    if (auto *memberExpr =
            dynamic_cast<ast::MemberExpr *>(binaryExpr->left.get())) {
      std::string propertyOwner = memberExpr->structName;
      if (propertyOwner.empty()) {
        propertyOwner = currentClassName_;
      }
      if (!propertyOwner.empty()) {
        auto setterIt =
            functions_.find(propertyOwner + "_set_" + memberExpr->member);
        std::string setterOwner = propertyOwner;
        while (setterIt == functions_.end()) {
          auto baseIt = classBaseNames_.find(setterOwner);
          if (baseIt == classBaseNames_.end() || baseIt->second.empty()) {
            break;
          }
          setterOwner = baseIt->second;
          setterIt = functions_.find(setterOwner + "_set_" +
                                     memberExpr->member);
        }
        if (setterIt != functions_.end()) {
          llvm::Value *object =
              generateExpression(std::move(memberExpr->object));
          llvm::Value *rhs =
              generateExpression(std::move(binaryExpr->right));
          if (!object || !rhs) {
            return nullptr;
          }
          llvm::Value *thisPtr = object;
          if (auto *objectAlloca = llvm::dyn_cast<llvm::AllocaInst>(object)) {
            if (objectAlloca->getAllocatedType()->isStructTy()) {
              thisPtr = objectAlloca;
            } else if (objectAlloca->getAllocatedType()->isPointerTy()) {
              thisPtr = builder()->CreateLoad(
                  objectAlloca->getAllocatedType(), objectAlloca,
                  "pset.this.load");
            }
          } else if (object->getType()->isStructTy()) {
            llvm::AllocaInst *tmp =
                createEntryAlloca(object->getType(), "pset.this");
            builder()->CreateStore(object, tmp);
            thisPtr = tmp;
          }
          builder()->CreateCall(setterIt->second, {thisPtr, rhs});
          return rhs;
        }
      }
    }

    llvm::Value *lhsPtr = getExpressionLValue(std::move(binaryExpr->left));
    llvm::Value *rhs = generateExpression(std::move(binaryExpr->right));
    if (!lhsPtr || !rhs) {
      return nullptr;
    }

    llvm::Type *targetType = nullptr;
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(lhsPtr)) {
      targetType = alloca->getAllocatedType();
    } else if (lhsPtr->getType()->isPointerTy()) {
      // LLVM 15+ uses opaque pointers: a GEP result no longer carries the
      // pointee type.  The RHS type is the best available approximation for
      // the store type here; typed conversions are handled by the callers
      // that know the AST type.
      targetType = rhs->getType();
    }

    if (!targetType) {
      error("Assignment target is not addressable");
      return nullptr;
    }

    if (rhs->getType() != targetType) {
      if (targetType->isFloatingPointTy() && rhs->getType()->isIntegerTy()) {
        rhs = builder()->CreateSIToFP(rhs, targetType, "assign.cast");
      } else if (targetType->isIntegerTy() &&
                 rhs->getType()->isFloatingPointTy()) {
        rhs = builder()->CreateFPToSI(rhs, targetType, "assign.cast");
      } else if (targetType->isIntegerTy() && rhs->getType()->isIntegerTy()) {
        rhs = builder()->CreateIntCast(rhs, targetType, true, "assign.cast");
      }
    }

    builder()->CreateStore(rhs, lhsPtr);
    return rhs;
  }
  default:
    break;
  }

  llvm::Value *lhs = generateExpression(std::move(binaryExpr->left));
  llvm::Value *rhs = generateExpression(std::move(binaryExpr->right));

  if (!lhs || !rhs) {
    return nullptr;
  }

  // 查找操作符重载
  auto tryOperatorOverload = [&](const std::string &opName) -> llvm::Value * {
    llvm::Type *lhsType = lhs->getType();
    for (auto &[name, func] : functions_) {
      if (name.size() > opName.size() + 1 &&
          name.substr(name.size() - opName.size()) == opName &&
          name[name.size() - opName.size() - 1] == '_') {
        std::string className = name.substr(0, name.size() - opName.size() - 1);
        auto structIt = structTypes_.find(className);
        if (structIt != structTypes_.end()) {
          llvm::StructType *structType = structIt->second;
          if (lhsType == structType->getPointerTo() ||
              lhsType == structType) {
            std::vector<llvm::Value *> args;
            if (lhsType == structType) {
              llvm::AllocaInst *tmp =
                  builder()->CreateAlloca(structType, nullptr, "tmp");
              builder()->CreateStore(lhs, tmp);
              args.push_back(tmp);
            } else {
              args.push_back(lhs);
            }
            args.push_back(rhs);
            return builder()->CreateCall(func, args, "opoverload");
          }
        }
      }
    }
    return nullptr;
  };

  std::string opName;
  switch (binaryExpr->op) {
  case ast::BinaryExpr::Op::Add: opName = "operator+"; break;
  case ast::BinaryExpr::Op::Sub: opName = "operator-"; break;
  case ast::BinaryExpr::Op::Mul: opName = "operator*"; break;
  case ast::BinaryExpr::Op::Div: opName = "operator/"; break;
  case ast::BinaryExpr::Op::Mod: opName = "operator%"; break;
  case ast::BinaryExpr::Op::Eq: opName = "operator=="; break;
  case ast::BinaryExpr::Op::Ne: opName = "operator!="; break;
  case ast::BinaryExpr::Op::Lt: opName = "operator<"; break;
  case ast::BinaryExpr::Op::Le: opName = "operator<="; break;
  case ast::BinaryExpr::Op::Gt: opName = "operator>"; break;
  case ast::BinaryExpr::Op::Ge: opName = "operator>="; break;
  case ast::BinaryExpr::Op::And: opName = "operator&&"; break;
  case ast::BinaryExpr::Op::Or: opName = "operator||"; break;
  default: break;
  }

  if (!opName.empty()) {
    if (llvm::Value *result = tryOperatorOverload(opName)) {
      return result;
    }
  }

  switch (binaryExpr->op) {
  case ast::BinaryExpr::Op::Add:
    if (lhs->getType()->isFloatingPointTy()) {
      return builder()->CreateFAdd(lhs, rhs, "addtmp");
    }
    return builder()->CreateAdd(lhs, rhs, "addtmp");
  case ast::BinaryExpr::Op::Sub:
    if (lhs->getType()->isFloatingPointTy()) {
      return builder()->CreateFSub(lhs, rhs, "subtmp");
    }
    return builder()->CreateSub(lhs, rhs, "subtmp");
  case ast::BinaryExpr::Op::Mul:
    if (lhs->getType()->isFloatingPointTy()) {
      return builder()->CreateFMul(lhs, rhs, "multmp");
    }
    return builder()->CreateMul(lhs, rhs, "multmp");
  case ast::BinaryExpr::Op::Div:
    if (lhs->getType()->isFloatingPointTy()) {
      return builder()->CreateFDiv(lhs, rhs, "divtmp");
    }
    return builder()->CreateSDiv(lhs, rhs, "divtmp");
  case ast::BinaryExpr::Op::Mod:
    return builder()->CreateSRem(lhs, rhs, "modtmp");
  case ast::BinaryExpr::Op::Eq:
    if (lhs->getType()->isFloatingPointTy()) {
      return builder()->CreateFCmpOEQ(lhs, rhs, "eqtmp");
    }
    return builder()->CreateICmpEQ(lhs, rhs, "eqtmp");
  case ast::BinaryExpr::Op::Ne:
    if (lhs->getType()->isFloatingPointTy()) {
      return builder()->CreateFCmpONE(lhs, rhs, "netmp");
    }
    return builder()->CreateICmpNE(lhs, rhs, "netmp");
  case ast::BinaryExpr::Op::Lt:
    if (lhs->getType()->isFloatingPointTy()) {
      return builder()->CreateFCmpOLT(lhs, rhs, "lttmp");
    }
    return builder()->CreateICmpSLT(lhs, rhs, "lttmp");
  case ast::BinaryExpr::Op::Le:
    if (lhs->getType()->isFloatingPointTy()) {
      return builder()->CreateFCmpOLE(lhs, rhs, "letmp");
    }
    return builder()->CreateICmpSLE(lhs, rhs, "letmp");
  case ast::BinaryExpr::Op::Gt:
    if (lhs->getType()->isFloatingPointTy()) {
      return builder()->CreateFCmpOGT(lhs, rhs, "gttmp");
    }
    return builder()->CreateICmpSGT(lhs, rhs, "gttmp");
  case ast::BinaryExpr::Op::Ge:
    if (lhs->getType()->isFloatingPointTy()) {
      return builder()->CreateFCmpOGE(lhs, rhs, "getmp");
    }
    return builder()->CreateICmpSGE(lhs, rhs, "getmp");
  case ast::BinaryExpr::Op::And:
    return builder()->CreateAnd(lhs, rhs, "andtmp");
  case ast::BinaryExpr::Op::Or:
    return builder()->CreateOr(lhs, rhs, "ortmp");
  default:
    error("Unknown binary operator");
    return nullptr;
  }
}

// 生成一元表达式
llvm::Value *
LLVMCodeGenerator::generateUnaryExpr(std::unique_ptr<ast::UnaryExpr> unaryExpr,
                                     bool isLValue) {
  switch (unaryExpr->op) {
  case ast::UnaryExpr::Op::Await:
    return generateAwaitExpr(std::move(unaryExpr));
  case ast::UnaryExpr::Op::PreIncrement:
  case ast::UnaryExpr::Op::PreDecrement:
  case ast::UnaryExpr::Op::PostIncrement:
  case ast::UnaryExpr::Op::PostDecrement: {
    auto op = unaryExpr->op;
    llvm::Value *ptr = getExpressionLValue(std::move(unaryExpr->expr));
    if (!ptr) {
      error("Increment/decrement target is not an lvalue");
      return nullptr;
    }

    llvm::Type *valueType = llvm::Type::getInt32Ty(context());
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(ptr)) {
      valueType = alloca->getAllocatedType();
    }

    llvm::Value *oldValue =
        builder()->CreateLoad(valueType, ptr, "incdec.load");
    llvm::Value *one = llvm::ConstantInt::get(valueType, 1);
    llvm::Value *newValue =
        (op == ast::UnaryExpr::Op::PreIncrement ||
         op == ast::UnaryExpr::Op::PostIncrement)
            ? builder()->CreateAdd(oldValue, one, "incdec.new")
            : builder()->CreateSub(oldValue, one, "incdec.new");
    builder()->CreateStore(newValue, ptr);

    bool isPrefix = (op == ast::UnaryExpr::Op::PreIncrement ||
                     op == ast::UnaryExpr::Op::PreDecrement);
    return isPrefix ? newValue : oldValue;
  }
  default:
    break;
  }

  llvm::Value *operand = generateExpression(std::move(unaryExpr->expr));
  if (!operand) {
    return nullptr;
  }

  // 查找一元操作符重载
  std::string unaryOpName;
  switch (unaryExpr->op) {
  case ast::UnaryExpr::Op::Minus: unaryOpName = "operator-"; break;
  case ast::UnaryExpr::Op::Not: unaryOpName = "operator!"; break;
  case ast::UnaryExpr::Op::BitNot: unaryOpName = "operator~"; break;
  default: break;
  }

  if (!unaryOpName.empty()) {
    llvm::Type *opType = operand->getType();
    for (auto &[name, func] : functions_) {
      if (name.size() > unaryOpName.size() + 1 &&
          name.substr(name.size() - unaryOpName.size()) == unaryOpName &&
          name[name.size() - unaryOpName.size() - 1] == '_') {
        std::string className =
            name.substr(0, name.size() - unaryOpName.size() - 1);
        auto structIt = structTypes_.find(className);
        if (structIt != structTypes_.end()) {
          llvm::StructType *structType = structIt->second;
          if (opType == structType->getPointerTo() || opType == structType) {
            std::vector<llvm::Value *> args;
            if (opType == structType) {
              llvm::AllocaInst *tmp =
                  builder()->CreateAlloca(structType, nullptr, "tmp");
              builder()->CreateStore(operand, tmp);
              args.push_back(tmp);
            } else {
              args.push_back(operand);
            }
            return builder()->CreateCall(func, args, "unaryopoverload");
          }
        }
      }
    }
  }

  switch (unaryExpr->op) {
  case ast::UnaryExpr::Op::Minus:
    if (operand->getType()->isFloatingPointTy()) {
      return builder()->CreateFNeg(operand, "negtmp");
    }
    return builder()->CreateNeg(operand, "negtmp");
  case ast::UnaryExpr::Op::Not:
    return builder()->CreateNot(operand, "nottmp");
  case ast::UnaryExpr::Op::Dereference:
    return builder()->CreateLoad(operand->getType(), operand, "deref");
  case ast::UnaryExpr::Op::AddressOf:
    return operand; // 返回地址
  case ast::UnaryExpr::Op::PreIncrement: {
    llvm::Value *one = llvm::ConstantInt::get(operand->getType(), 1);
    llvm::Value *result = builder()->CreateAdd(operand, one, "inctmp");
    if (isLValue) {
      builder()->CreateStore(result, operand);
    }
    return result;
  }
  case ast::UnaryExpr::Op::PreDecrement: {
    llvm::Value *one = llvm::ConstantInt::get(operand->getType(), 1);
    llvm::Value *result = builder()->CreateSub(operand, one, "dectmp");
    if (isLValue) {
      builder()->CreateStore(result, operand);
    }
    return result;
  }
  default:
    error("Unknown unary operator");
    return nullptr;
  }
}

// 生成调用表达式
llvm::Value *
LLVMCodeGenerator::generateCallExpr(std::unique_ptr<ast::CallExpr> callExpr) {
  // 获取函数名
  std::string funcName;
  if (auto *ident = dynamic_cast<ast::Identifier *>(callExpr->callee.get())) {
    funcName = ident->name;
  }

  // 成员方法调用：obj.method(args) / ptr->method(args)
  if (auto *member =
          dynamic_cast<ast::MemberExpr *>(callExpr->callee.get())) {
    // 静态方法：ClassName.method(...)，没有 this 参数。
    if (auto *objectIdent =
            dynamic_cast<ast::Identifier *>(member->object.get())) {
      auto staticIt =
          functions_.find(objectIdent->name + "_" + member->member);
      if (staticIt != functions_.end() &&
          staticIt->second->arg_size() == callExpr->args.size()) {
        std::vector<llvm::Value *> args;
        for (auto &arg : callExpr->args) {
          args.push_back(generateExpression(std::move(arg)));
        }
        if (staticIt->second->getReturnType()->isVoidTy()) {
          return builder()->CreateCall(staticIt->second, args);
        }
        return builder()->CreateCall(staticIt->second, args, "scall");
      }
    }

    llvm::Value *object = generateExpression(std::move(member->object));
    if (!object) {
      return nullptr;
    }

    std::string className = member->structName;
    if (className.empty()) {
      className = currentClassName_;
    }
    if (className.empty()) {
      if (auto *structTy = llvm::dyn_cast<llvm::StructType>(object->getType())) {
        className = structTy->getName().str();
      }
    }

    // 方法第一个参数是 this；把对象统一成指向结构体的指针。
    llvm::Value *thisPtr = object;
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(object)) {
      if (alloca->getAllocatedType()->isStructTy()) {
        thisPtr = alloca;
      } else if (alloca->getAllocatedType()->isPointerTy()) {
        thisPtr = builder()->CreateLoad(alloca->getAllocatedType(), alloca,
                                       "mcall.this.load");
      }
    } else if (object->getType()->isStructTy()) {
      llvm::AllocaInst *tmp =
          builder()->CreateAlloca(object->getType(), nullptr, "mcall.this");
      builder()->CreateStore(object, tmp);
      thisPtr = tmp;
    }

    std::vector<llvm::Value *> args;
    args.push_back(thisPtr);
    for (auto &arg : callExpr->args) {
      llvm::Value *argValue = generateExpression(std::move(arg));
      if (!argValue) {
        error("Failed to generate method argument");
        return nullptr;
      }
      args.push_back(argValue);
    }

    std::string methodName = className + "_" + member->member;
    auto it = functions_.find(methodName);
    // 继承方法：在基类中查找 Class_method。
    std::string ownerClass = className;
    while (it == functions_.end()) {
      auto baseIt = classBaseNames_.find(ownerClass);
      if (baseIt == classBaseNames_.end() || baseIt->second.empty()) {
        break;
      }
      ownerClass = baseIt->second;
      it = functions_.find(ownerClass + "_" + member->member);
    }
    if (it != functions_.end()) {
      if (it->second->getReturnType()->isVoidTy()) {
        return builder()->CreateCall(it->second, args);
      }
      return builder()->CreateCall(it->second, args, "mcall");
    }

    // 扩展方法/普通函数回退：把对象作为第一个参数
    it = functions_.find(member->member);
    if (it != functions_.end()) {
      if (it->second->getReturnType()->isVoidTy()) {
        return builder()->CreateCall(it->second, args);
      }
      return builder()->CreateCall(it->second, args, "mcall");
    }

    error("Unknown method: " + methodName);
    return nullptr;
  }

  // 生成参数
  std::vector<llvm::Value *> args;
  for (auto &arg : callExpr->args) {
    args.push_back(generateExpression(std::move(arg)));
  }

  // 结构体值构造：Point(...) / Point<int>(...)
  if (!funcName.empty()) {
    auto structIt = structTypes_.find(funcName);
    if (structIt != structTypes_.end()) {
      auto ctorIt = functions_.find(funcName + "_" + funcName);
      if (ctorIt != functions_.end()) {
        llvm::StructType *structTy = structIt->second;
        llvm::AllocaInst *tmp =
            builder()->CreateAlloca(structTy, nullptr, funcName + ".tmp");
        std::vector<llvm::Value *> ctorArgs;
        ctorArgs.push_back(tmp);
        ctorArgs.insert(ctorArgs.end(), args.begin(), args.end());
        builder()->CreateCall(ctorIt->second, ctorArgs);
        return builder()->CreateLoad(structTy, tmp, funcName + ".value");
      }
    }
  }

  // 优先使用语义分析器预计算的重载解析结果（精确类型 mangled name）
  if (!callExpr->resolvedMangledName.empty()) {
    auto it = functions_.find(callExpr->resolvedMangledName);
    if (it != functions_.end()) {
      return builder()->CreateCall(it->second, args);
    }
  }

  // 根据实参 LLVM 类型构造 mangled 函数名（备用路径）
  std::string mangledName = funcName;
  if (!args.empty()) {
    mangledName += "@";
    for (size_t i = 0; i < args.size(); ++i) {
      if (i > 0) mangledName += "#";
      llvm::Type *argType = args[i]->getType();
      if (argType->isIntegerTy()) {
        mangledName += "int";
      } else if (argType->isFloatingPointTy()) {
        mangledName += "double";
      } else if (argType->isPointerTy()) {
        mangledName += "ptr";
      } else if (argType->isStructTy()) {
        mangledName += "struct";
      } else if (argType->isArrayTy()) {
        mangledName += "array";
      } else {
        mangledName += "unknown";
      }
    }
  }

  // 用 LLVM 类型 mangled 名查找
  auto it = functions_.find(mangledName);
  if (it != functions_.end()) {
    return builder()->CreateCall(it->second, args);
  }

  // 回退到简单名查找（无参函数或 extern 函数）
  it = functions_.find(funcName);
  if (it != functions_.end()) {
    return builder()->CreateCall(it->second, args);
  }

  // 最后尝试使用 LLVM module 中的函数（extern 声明）
  llvm::Function *func = module()->getFunction(funcName);
  if (func) {
    return builder()->CreateCall(func, args);
  }

  error("Unknown function: " + funcName + " (mangled: " + mangledName + ")");
  return nullptr;
}

// 生成成员表达式
llvm::Value *LLVMCodeGenerator::generateMemberExpr(
    std::unique_ptr<ast::MemberExpr> memberExpr) {
  // 枚举成员：Color.Green 对应全局常量 Color_Green。
  if (auto *objectIdent =
          dynamic_cast<ast::Identifier *>(memberExpr->object.get())) {
    std::string enumGlobalName =
        objectIdent->name + "_" + memberExpr->member;
    if (auto *global = module()->getNamedGlobal(enumGlobalName)) {
      return builder()->CreateLoad(global->getValueType(), global,
                                   "enum.value");
    }
  }

  llvm::Value *object = nullptr;
  // 裸字段名如果本身是指针（如 out->value 中的 out），先加载字段值，
  // 否则会把字段地址误当成对象指针。
  if (auto *ident = dynamic_cast<ast::Identifier *>(
          memberExpr->object.get())) {
    if (!currentClassName_.empty()) {
      auto classIt = structInfo_.find(currentClassName_);
      auto thisIt = namedValues_.find("self");
      if (classIt != structInfo_.end() && thisIt != namedValues_.end()) {
        auto fieldIt = classIt->second.find(ident->name);
        if (fieldIt != classIt->second.end()) {
          auto *structType = structTypes_[currentClassName_];
          llvm::Type *fieldType =
              structType->getElementType(fieldIt->second);
          if (fieldType->isPointerTy()) {
            llvm::Value *fieldPtr = builder()->CreateStructGEP(
                structType, thisIt->second, fieldIt->second,
                ident->name + ".ptr");
            object = builder()->CreateLoad(fieldType, fieldPtr,
                                           ident->name + ".load");
          }
        }
      }
    }
  }
  if (!object) {
    object = generateExpression(std::move(memberExpr->object));
  }
  if (!object) {
    return nullptr;
  }

  unsigned idx = memberExpr->memberIndex;
  const std::string &memberName = memberExpr->member;

  // 属性 getter：ClassName_get_prop(this)。
  std::string propertyOwner = memberExpr->structName;
  if (propertyOwner.empty()) {
    propertyOwner = currentClassName_;
  }
  if (propertyOwner.empty()) {
    llvm::Type *objectType = object->getType();
    if (auto *objectAlloca = llvm::dyn_cast<llvm::AllocaInst>(object)) {
      objectType = objectAlloca->getAllocatedType();
    }
    if (auto *structType = llvm::dyn_cast<llvm::StructType>(objectType)) {
      if (structType->hasName()) {
        propertyOwner = structType->getName().str();
      }
    }
  }
  if (!propertyOwner.empty()) {
    auto getterIt = functions_.find(propertyOwner + "_get_" + memberName);
    std::string getterOwner = propertyOwner;
    while (getterIt == functions_.end()) {
      auto baseIt = classBaseNames_.find(getterOwner);
      if (baseIt == classBaseNames_.end() || baseIt->second.empty()) {
        break;
      }
      getterOwner = baseIt->second;
      getterIt = functions_.find(getterOwner + "_get_" + memberName);
    }
    if (getterIt != functions_.end()) {
      llvm::Value *thisPtr = object;
      if (auto *objectAlloca = llvm::dyn_cast<llvm::AllocaInst>(object)) {
        if (objectAlloca->getAllocatedType()->isStructTy()) {
          thisPtr = objectAlloca;
        } else if (objectAlloca->getAllocatedType()->isPointerTy()) {
          thisPtr = builder()->CreateLoad(
              objectAlloca->getAllocatedType(), objectAlloca,
              "pget.this.load");
        }
      } else if (object->getType()->isStructTy()) {
        llvm::AllocaInst *tmp =
            createEntryAlloca(object->getType(), "pget.this");
        builder()->CreateStore(object, tmp);
        thisPtr = tmp;
      }
      return builder()->CreateCall(getterIt->second, {thisPtr}, "pget");
    }
  }

  // 数组/切片的内建属性 len / ptr。
  if (memberName == "len" || memberName == "ptr") {
    llvm::Type *objectType = object->getType();
    auto *objectAlloca = llvm::dyn_cast<llvm::AllocaInst>(object);
    if (objectAlloca) {
      objectType = objectAlloca->getAllocatedType();
    }

    if (objectType->isArrayTy()) {
      auto *arrayType = llvm::cast<llvm::ArrayType>(objectType);
      llvm::Value *arrayPtr = object;
      if (!objectAlloca) {
        llvm::AllocaInst *tmp =
            createEntryAlloca(objectType, memberName + ".array.tmp");
        builder()->CreateStore(object, tmp);
        arrayPtr = tmp;
      }
      if (memberName == "len") {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                      arrayType->getNumElements());
      }
      return builder()->CreateGEP(
          arrayType, arrayPtr,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
           llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0)},
          "array.ptr");
    }

    if (isSliceStructType(objectType)) {
      llvm::Value *sliceValue = object;
      if (objectAlloca) {
        sliceValue =
            builder()->CreateLoad(objectType, objectAlloca, "slice.value");
      }
      if (memberName == "len") {
        llvm::Value *len = builder()->CreateExtractValue(sliceValue, 1,
                                                         "slice.len");
        if (len->getType() != llvm::Type::getInt32Ty(context())) {
          len = builder()->CreateIntCast(len,
                                         llvm::Type::getInt32Ty(context()),
                                         false, "slice.len32");
        }
        return len;
      }
      return builder()->CreateExtractValue(sliceValue, 0, "slice.ptr");
    }

    if (objectType->isPointerTy()) {
      llvm::Value *ptr = object;
      if (objectAlloca) {
        ptr = builder()->CreateLoad(objectType, objectAlloca, "ptr.value");
      }
      if (memberName == "len") {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
      }
      return ptr;
    }
  }

  // 语义层可能没有填 memberIndex；用当前类/结构体的字段表补上。
  std::string structName = memberExpr->structName;
  if (structName.empty()) {
    structName = currentClassName_;
  }
  if (!structName.empty()) {
    auto infoIt = structInfo_.find(structName);
    if (infoIt != structInfo_.end()) {
      auto fieldIt = infoIt->second.find(memberName);
      if (fieldIt != infoIt->second.end()) {
        idx = fieldIt->second;
      }
    }
  }

  // case 1: AllocaInst — know the allocated type
  if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(object)) {
    llvm::Type *allocatedTy = alloca->getAllocatedType();
    llvm::Value *ptr = builder()->CreateStructGEP(allocatedTy, alloca, idx,
                                                   memberName + ".ptr");
    llvm::Type *fieldTy = allocatedTy->getStructElementType(idx);
    return builder()->CreateLoad(fieldTy, ptr, memberName + ".load");
  }

  // case 2: the value itself is a struct — alloca, store, GEP, load
  llvm::Type *valTy = object->getType();
  if (valTy->isStructTy()) {
    llvm::AllocaInst *tmpAlloca = builder()->CreateAlloca(valTy, nullptr,
                                                           memberName + ".alloca");
    builder()->CreateStore(object, tmpAlloca);
    llvm::Value *ptr = builder()->CreateStructGEP(valTy, tmpAlloca, idx,
                                                   memberName + ".ptr");
    llvm::Type *fieldTy = valTy->getStructElementType(idx);
    return builder()->CreateLoad(fieldTy, ptr, memberName + ".load");
  }

  // case 3: opaque pointer (e.g. this) — look up struct type by name
  if (!structName.empty()) {
    llvm::StructType *structTy = nullptr;
    auto typeIt = structTypes_.find(structName);
    if (typeIt != structTypes_.end()) {
      structTy = typeIt->second;
    } else {
      structTy = llvm::StructType::getTypeByName(context(), structName);
    }
    if (structTy) {
      llvm::Value *ptr = builder()->CreateStructGEP(structTy, object, idx,
                                                     memberName + ".ptr");
      llvm::Type *fieldTy = structTy->getElementType(idx);
      return builder()->CreateLoad(fieldTy, ptr, memberName + ".load");
    }
  }

  warning("Member expression: cannot determine struct type for '" +
          memberName + "'");
  return nullptr;
}

// 生成下标表达式
llvm::Value *LLVMCodeGenerator::generateSubscriptExpr(
    std::unique_ptr<ast::SubscriptExpr> subscriptExpr, bool isLValue) {
  struct ArrayBase {
    llvm::Value *ptr = nullptr;
    llvm::Type *elementType = nullptr;
    llvm::ArrayType *arrayType = nullptr;
    llvm::StructType *tupleType = nullptr;
    llvm::Value *length = nullptr;
  };

  std::function<ArrayBase(ast::Expression *)> getBase;
  std::function<llvm::Value *(ArrayBase &, llvm::Value *)> getElementPtr;

  getElementPtr = [&](ArrayBase &base, llvm::Value *index) -> llvm::Value * {
    if (!index) {
      return nullptr;
    }
    if (index->getType() != llvm::Type::getInt32Ty(context())) {
      if (index->getType()->isIntegerTy()) {
        index = builder()->CreateIntCast(index,
                                         llvm::Type::getInt32Ty(context()),
                                         true, "index32");
      }
    }
    if (base.tupleType) {
      auto *constantIndex = llvm::dyn_cast<llvm::ConstantInt>(index);
      if (!constantIndex) {
        error("Tuple subscript must be a compile-time constant");
        return nullptr;
      }
      return builder()->CreateStructGEP(
          base.tupleType, base.ptr,
          static_cast<unsigned>(constantIndex->getZExtValue()),
          "tuple.elem.ptr");
    }
    if (base.arrayType) {
      return builder()->CreateGEP(
          base.arrayType, base.ptr,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
           index},
          "element_ptr");
    }
    return builder()->CreateGEP(base.elementType, base.ptr, index,
                                "element_ptr");
  };

  getBase = [&](ast::Expression *expr) -> ArrayBase {
    ArrayBase result;

    // 标识符必须取 alloca 本身，否则 a[0] = x 会写到临时数组。
    if (auto *ident = dynamic_cast<ast::Identifier *>(expr)) {
      auto it = namedValues_.find(ident->name);
      if (it == namedValues_.end()) {
        error("Unknown variable: " + ident->name);
        return result;
      }
      llvm::Value *value = it->second;
      if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(value)) {
        llvm::Type *allocatedType = alloca->getAllocatedType();
        if (allocatedType->isArrayTy()) {
          result.ptr = alloca;
          result.arrayType = llvm::cast<llvm::ArrayType>(allocatedType);
          result.elementType = result.arrayType->getElementType();
          result.length = llvm::ConstantInt::get(
              llvm::Type::getInt32Ty(context()),
              result.arrayType->getNumElements());
          return result;
        }
        if (isSliceStructType(allocatedType)) {
          auto *structType = llvm::cast<llvm::StructType>(allocatedType);
          llvm::Value *sliceValue =
              builder()->CreateLoad(structType, alloca, "slice.value");
          result.ptr = builder()->CreateExtractValue(sliceValue, 0, "slice.ptr");
          result.length =
              builder()->CreateExtractValue(sliceValue, 1, "slice.len");
          if (result.length->getType() !=
              llvm::Type::getInt32Ty(context())) {
            result.length = builder()->CreateIntCast(
                result.length, llvm::Type::getInt32Ty(context()), false,
                "slice.len32");
          }
          result.elementType = getSliceElementType(structType);
          return result;
        }
        if (allocatedType->isPointerTy()) {
          result.ptr =
              builder()->CreateLoad(allocatedType, alloca, "ptr.value");
          auto pointeeIt = namedValuePointeeTypes_.find(ident->name);
          result.elementType =
              pointeeIt != namedValuePointeeTypes_.end() && pointeeIt->second
                  ? pointeeIt->second
                  : llvm::Type::getInt32Ty(context());
          return result;
        }
        if (allocatedType->isStructTy()) {
          result.ptr = alloca;
          result.tupleType = llvm::cast<llvm::StructType>(allocatedType);
          return result;
        }
      } else {
        if (value->getType()->isStructTy() &&
            isSliceStructType(value->getType())) {
          auto *structType = llvm::cast<llvm::StructType>(value->getType());
          result.ptr = builder()->CreateExtractValue(value, 0, "slice.ptr");
          result.length =
              builder()->CreateExtractValue(value, 1, "slice.len");
          if (result.length->getType() !=
              llvm::Type::getInt32Ty(context())) {
            result.length = builder()->CreateIntCast(
                result.length, llvm::Type::getInt32Ty(context()), false,
                "slice.len32");
          }
          result.elementType = getSliceElementType(structType);
          return result;
        }
        if (value->getType()->isStructTy() &&
            !isSliceStructType(value->getType())) {
          auto *structType = llvm::cast<llvm::StructType>(value->getType());
          llvm::AllocaInst *tmp =
              createEntryAlloca(structType, "tuple.base.tmp");
          builder()->CreateStore(value, tmp);
          result.ptr = tmp;
          result.tupleType = structType;
          return result;
        }
        if (value->getType()->isArrayTy()) {
          auto *arrayType = llvm::cast<llvm::ArrayType>(value->getType());
          llvm::AllocaInst *tmp =
              createEntryAlloca(arrayType, "array.base.tmp");
          builder()->CreateStore(value, tmp);
          result.ptr = tmp;
          result.arrayType = arrayType;
          result.elementType = arrayType->getElementType();
          result.length = llvm::ConstantInt::get(
              llvm::Type::getInt32Ty(context()), arrayType->getNumElements());
          return result;
        }
        if (value->getType()->isPointerTy()) {
          result.ptr = value;
          result.elementType = llvm::Type::getInt32Ty(context());
          return result;
        }
      }
      return result;
    }

    // 嵌套下标：a[i][j] 必须让内层返回元素地址，而不是加载成临时值。
    if (auto *sub = dynamic_cast<ast::SubscriptExpr *>(expr)) {
      ArrayBase inner = getBase(sub->object.get());
      llvm::Value *innerIndex =
          generateExpression(sub->index->clone());
      llvm::Value *innerPtr = getElementPtr(inner, innerIndex);
      result.ptr = innerPtr;
      if (inner.tupleType) {
        auto *constantIndex = llvm::dyn_cast<llvm::ConstantInt>(innerIndex);
        if (!constantIndex) {
          error("Tuple subscript must be a compile-time constant");
          return result;
        }
        unsigned tupleIndex =
            static_cast<unsigned>(constantIndex->getZExtValue());
        if (tupleIndex >= inner.tupleType->getNumElements()) {
          error("Tuple subscript out of range");
          return result;
        }
        llvm::Type *tupleElementType =
            inner.tupleType->getElementType(tupleIndex);
        if (tupleElementType->isStructTy()) {
          result.tupleType = llvm::cast<llvm::StructType>(tupleElementType);
        } else {
          result.elementType = tupleElementType;
        }
      } else if (inner.elementType && inner.elementType->isArrayTy()) {
        result.arrayType = llvm::cast<llvm::ArrayType>(inner.elementType);
        result.elementType = result.arrayType->getElementType();
        result.length = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(context()),
            result.arrayType->getNumElements());
      } else {
        result.elementType = inner.elementType;
        result.length = inner.length;
      }
      return result;
    }

    llvm::Value *object = generateExpression(expr->clone());
    if (!object) {
      return result;
    }
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(object)) {
      llvm::Type *allocatedType = alloca->getAllocatedType();
      if (allocatedType->isArrayTy()) {
        result.ptr = alloca;
        result.arrayType = llvm::cast<llvm::ArrayType>(allocatedType);
        result.elementType = result.arrayType->getElementType();
        result.length = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(context()),
            result.arrayType->getNumElements());
        return result;
      }
      if (isSliceStructType(allocatedType)) {
        auto *structType = llvm::cast<llvm::StructType>(allocatedType);
        llvm::Value *sliceValue =
            builder()->CreateLoad(structType, alloca, "slice.value");
        result.ptr = builder()->CreateExtractValue(sliceValue, 0, "slice.ptr");
        result.length = builder()->CreateExtractValue(sliceValue, 1, "slice.len");
        if (result.length->getType() != llvm::Type::getInt32Ty(context())) {
          result.length = builder()->CreateIntCast(
              result.length, llvm::Type::getInt32Ty(context()), false,
              "slice.len32");
        }
        result.elementType = getSliceElementType(structType);
        return result;
      }
      if (allocatedType->isPointerTy()) {
        result.ptr = builder()->CreateLoad(allocatedType, alloca, "ptr.value");
        result.elementType = llvm::Type::getInt32Ty(context());
        return result;
      }
    }

    if (object->getType()->isStructTy() &&
        isSliceStructType(object->getType())) {
      auto *structType = llvm::cast<llvm::StructType>(object->getType());
      result.ptr = builder()->CreateExtractValue(object, 0, "slice.ptr");
      result.length = builder()->CreateExtractValue(object, 1, "slice.len");
      if (result.length->getType() != llvm::Type::getInt32Ty(context())) {
        result.length = builder()->CreateIntCast(
            result.length, llvm::Type::getInt32Ty(context()), false,
            "slice.len32");
      }
      result.elementType = getSliceElementType(structType);
      return result;
    }
    if (object->getType()->isStructTy() &&
        !isSliceStructType(object->getType())) {
      auto *structType = llvm::cast<llvm::StructType>(object->getType());
      llvm::AllocaInst *tmp =
          createEntryAlloca(structType, "tuple.base.tmp");
      builder()->CreateStore(object, tmp);
      result.ptr = tmp;
      result.tupleType = structType;
      return result;
    }
    if (object->getType()->isArrayTy()) {
      auto *arrayType = llvm::cast<llvm::ArrayType>(object->getType());
      llvm::AllocaInst *tmp = createEntryAlloca(arrayType, "array.base.tmp");
      builder()->CreateStore(object, tmp);
      result.ptr = tmp;
      result.arrayType = arrayType;
      result.elementType = arrayType->getElementType();
      result.length = llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(context()), arrayType->getNumElements());
      return result;
    }
    if (object->getType()->isPointerTy()) {
      result.ptr = object;
      result.elementType = llvm::Type::getInt32Ty(context());
      return result;
    }
    return result;
  };

  // 切片区间：a[start..end]，半开区间。
  if (auto *range = dynamic_cast<ast::BinaryExpr *>(subscriptExpr->index.get());
      range && range->op == ast::BinaryExpr::Op::Range) {
    ArrayBase base = getBase(subscriptExpr->object.get());
    if (!base.ptr || !base.elementType) {
      error("Invalid array base in slice expression");
      return nullptr;
    }
    llvm::Value *start = generateExpression(range->left->clone());
    llvm::Value *end = generateExpression(range->right->clone());
    if (!start || !end) {
      return nullptr;
    }
    if (start->getType() != llvm::Type::getInt32Ty(context())) {
      start = builder()->CreateIntCast(start,
                                       llvm::Type::getInt32Ty(context()), true,
                                       "slice.start32");
    }
    if (end->getType() != llvm::Type::getInt32Ty(context())) {
      end = builder()->CreateIntCast(end, llvm::Type::getInt32Ty(context()),
                                     true, "slice.end32");
    }
    llvm::Value *ptr = getElementPtr(base, start);
    llvm::Value *len =
        builder()->CreateSub(end, start, "slice.len");
    llvm::StructType *sliceType = getOrCreateSliceType(base.elementType);
    llvm::Value *sliceValue =
        builder()->CreateInsertValue(llvm::UndefValue::get(sliceType), ptr, 0);
    sliceValue = builder()->CreateInsertValue(sliceValue, len, 1);
    return sliceValue;
  }

  ArrayBase base = getBase(subscriptExpr->object.get());
  if (!base.ptr || (!base.elementType && !base.tupleType)) {
    error("Invalid array base in subscript");
    return nullptr;
  }
  llvm::Value *index = generateExpression(std::move(subscriptExpr->index));
  if (base.tupleType) {
    auto *constantIndex = llvm::dyn_cast<llvm::ConstantInt>(index);
    if (!constantIndex) {
      error("Tuple subscript must be a compile-time constant");
      return nullptr;
    }
    unsigned tupleIndex =
        static_cast<unsigned>(constantIndex->getZExtValue());
    if (tupleIndex >= base.tupleType->getNumElements()) {
      error("Tuple subscript out of range");
      return nullptr;
    }
    llvm::Value *elementPtr = builder()->CreateStructGEP(
        base.tupleType, base.ptr, tupleIndex, "tuple.elem.ptr");
    if (isLValue) {
      return elementPtr;
    }
    return builder()->CreateLoad(base.tupleType->getElementType(tupleIndex),
                                 elementPtr, "tuple.elem");
  }
  llvm::Value *elementPtr = getElementPtr(base, index);
  if (!elementPtr) {
    return nullptr;
  }
  if (isLValue) {
    return elementPtr;
  }
  return builder()->CreateLoad(base.elementType, elementPtr, "element");
}

// 生成 self 表达式
llvm::Value *
LLVMCodeGenerator::generateSelfExpr(std::unique_ptr<ast::SelfExpr> selfExpr) {
  auto it = namedValues_.find("self");
  if (it != namedValues_.end()) {
    return it->second;
  }
  error("'self' not found in current context");
  return nullptr;
}

// 生成 base 表达式
llvm::Value *LLVMCodeGenerator::generateBaseExpr(
    std::unique_ptr<ast::BaseExpr> baseExpr) {
  auto it = namedValues_.find("base");
  if (it != namedValues_.end()) {
    return it->second;
  }
  error("'base' not found in current context");
  return nullptr;
}

// 生成 fold 表达式
llvm::Value *
LLVMCodeGenerator::generateFoldExpr(std::unique_ptr<ast::FoldExpr> foldExpr) {
  warning("Fold expression not implemented");
  return nullptr;
}

// 生成展开表达式
llvm::Value *LLVMCodeGenerator::generateExpansionExpr(
    std::unique_ptr<ast::ExpansionExpr> expansionExpr) {
  warning("Expansion expression not implemented");
  return nullptr;
}

// 生成数组初始化表达式
llvm::Value *LLVMCodeGenerator::generateArrayInitExpr(
    std::unique_ptr<ast::ArrayInitExpr> arrayInitExpr) {
  // 计算数组大小
  size_t size = arrayInitExpr->elements.size();

  // 分析第一个元素的类型
  llvm::Type *elementType = nullptr;
  if (size > 0) {
    // 复制第一个元素用于类型分析
    auto firstElement = arrayInitExpr->elements[0]->clone();
    llvm::Value *firstValue = generateExpression(std::move(firstElement));
    if (firstValue) {
      elementType = firstValue->getType();
    }
  }

  // 如果无法确定元素类型，默认使用 int32
  if (!elementType) {
    elementType = llvm::Type::getInt32Ty(context());
  }

  // 创建临时数组
  llvm::ArrayType *arrayType = llvm::ArrayType::get(elementType, size);
  llvm::AllocaInst *tempArrayAlloca =
      createEntryAlloca(arrayType, "temp_array");

  // 初始化元素
  for (size_t i = 0; i < size; ++i) {
    auto element = arrayInitExpr->elements[i]->clone();
    llvm::Value *elementValue = generateExpression(std::move(element));
    if (elementValue) {
      llvm::Value *indices[] = {
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                 static_cast<int>(i))};
      llvm::Value *ptr = builder()->CreateGEP(arrayType, tempArrayAlloca,
                                              indices, "element_ptr");
      builder()->CreateStore(elementValue, ptr);
    }
  }

  // 创建切片类型
  llvm::StructType *sliceStructType = getOrCreateSliceType(elementType);

  // 分配切片变量
  llvm::AllocaInst *sliceAlloca =
      createEntryAlloca(sliceStructType, "slice");

  // 计算临时数组的指针
  llvm::Value *arrayPtr = builder()->CreateGEP(
      arrayType, tempArrayAlloca,
      {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
       llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0)});

  // 构建切片值
  llvm::Value *sliceValue = builder()->CreateInsertValue(
      llvm::UndefValue::get(sliceStructType), arrayPtr, 0);
  sliceValue = builder()->CreateInsertValue(
      sliceValue,
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), size), 1);

  // 存储切片值
  builder()->CreateStore(sliceValue, sliceAlloca);

  return sliceAlloca;
}

// 生成结构体初始化表达式
llvm::Value *LLVMCodeGenerator::generateStructInitExpr(
    std::unique_ptr<ast::StructInitExpr> structInitExpr) {
  // 从类型节点中提取结构体名称
  std::string structName;
  if (structInitExpr->type) {
    if (auto *namedTy = dynamic_cast<ast::NamedType *>(structInitExpr->type.get())) {
      structName = namedTy->name;
    }
  }

  llvm::StructType *structTy = nullptr;
  if (!structName.empty()) {
    auto it = structTypes_.find(structName);
    if (it != structTypes_.end()) {
      structTy = it->second;
    }
    if (!structTy) {
      structTy = llvm::StructType::getTypeByName(context(), structName);
    }
  }

  if (!structTy) {
    // 尝试从 field 类型推断：创建匿名结构体
    std::vector<llvm::Type *> fieldTypes;
    for (auto &field : structInitExpr->fields) {
      llvm::Value *val = generateExpression(std::move(field.second));
      if (!val) {
        warning("Struct init field could not be generated");
        return nullptr;
      }
      fieldTypes.push_back(val->getType());
    }
    structTy = llvm::StructType::create(context(), fieldTypes, "anon.struct");
  }

  // 分配结构体
  llvm::AllocaInst *alloca = createEntryAlloca(
      structTy, structName.empty() ? "anon" : structName);

  // 填充字段
  for (size_t i = 0; i < structInitExpr->fields.size(); ++i) {
    auto &field = structInitExpr->fields[i];
    llvm::Value *val = generateExpression(std::move(field.second));
    if (!val) {
      return nullptr;
    }
    llvm::Value *ptr = builder()->CreateStructGEP(structTy, alloca, i,
                                                   structName + ".field" + std::to_string(i));
    builder()->CreateStore(val, ptr);
  }

  return alloca;
}

// 生成元组表达式
llvm::Value *LLVMCodeGenerator::generateTupleExpr(
    std::unique_ptr<ast::TupleExpr> tupleExpr) {
  std::vector<llvm::Value *> values;
  std::vector<llvm::Type *> elementTypes;
  for (auto &element : tupleExpr->elements) {
    llvm::Value *value = generateExpression(std::move(element));
    if (!value) {
      return nullptr;
    }
    values.push_back(value);
    elementTypes.push_back(value->getType());
  }

  llvm::StructType *tupleType =
      llvm::StructType::get(context(), elementTypes);
  llvm::Value *tupleValue = llvm::UndefValue::get(tupleType);
  for (size_t i = 0; i < values.size(); ++i) {
    tupleValue = builder()->CreateInsertValue(
        tupleValue, values[i], static_cast<unsigned>(i), "tuple.insert");
  }
  return tupleValue;
}

// 生成 lambda 表达式
llvm::Value *LLVMCodeGenerator::generateLambdaExpr(
    std::unique_ptr<ast::LambdaExpr> lambdaExpr) {
  static int lambdaCounter = 0;
  std::string funcName = "__lambda_" + std::to_string(lambdaCounter++);

  std::vector<llvm::Type *> paramTypes;
  for (auto &paramNode : lambdaExpr->params) {
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      if (param->type) {
        llvm::Type *pt = generateType(param->type.get());
        if (pt) {
          paramTypes.push_back(pt);
        }
      }
    }
  }
  if (paramTypes.empty()) {
    paramTypes.push_back(llvm::Type::getInt32Ty(context()));
  }

  llvm::FunctionType *funcType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(context()), paramTypes, false);
  llvm::Function *func = llvm::Function::Create(
      funcType, llvm::Function::InternalLinkage, funcName, module());

  llvm::BasicBlock *entryBB =
      llvm::BasicBlock::Create(context(), "entry", func);
  builder()->SetInsertPoint(entryBB);

  auto prevFunction = currentFunction_;
  currentFunction_ = func;

  namedValues_.clear();

  namedValuePointeeTypes_.clear();
  size_t paramIndex = 0;
  for (auto argIt = func->args().begin();
       argIt != func->args().end() && paramIndex < lambdaExpr->params.size();
       ++argIt, ++paramIndex) {
    if (auto *param =
            dynamic_cast<ast::Parameter *>(
                lambdaExpr->params[paramIndex].get())) {
      argIt->setName(param->name);
      llvm::AllocaInst *alloca =
          builder()->CreateAlloca(argIt->getType(), nullptr, param->name);
      builder()->CreateStore(argIt, alloca);
      namedValues_[param->name] = alloca;
      if (auto *pointerType =
              dynamic_cast<ast::PointerType *>(param->type.get())) {
        namedValuePointeeTypes_[param->name] =
            generateType(pointerType->baseType.get());
      }
    }
  }

  if (auto *stmt = lambdaExpr->body.get()) {
    ast::Expression *bodyExpr = nullptr;
    if (auto *exprStmt = dynamic_cast<ast::ExprStmt *>(stmt)) {
      bodyExpr = exprStmt->expr.get();
    }

    if (bodyExpr) {
      llvm::Value *retVal = generateExpression(bodyExpr->clone());
      if (retVal) {
        builder()->CreateRet(retVal);
      } else {
        builder()->CreateRet(llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(context()), 0));
      }
    } else {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(lambdaExpr->body.release())));
      if (!func->empty()) {
        llvm::BasicBlock *lastBlock = &func->back();
        if (!lastBlock->getTerminator()) {
          builder()->SetInsertPoint(lastBlock);
          builder()->CreateRet(llvm::ConstantInt::get(
              llvm::Type::getInt32Ty(context()), 0));
        }
      }
    }
  }

  currentFunction_ = prevFunction;
  return builder()->CreatePointerCast(func, llvm::PointerType::getUnqual(context()),
                                      funcName + "_ptr");
}

// 生成反射表达式
llvm::Value *LLVMCodeGenerator::generateReflectionExpr(
    std::unique_ptr<ast::ReflectionExpr> reflectionExpr) {
  warning("Reflection expression not implemented");
  return nullptr;
}

// 生成条件表达式 (?:)
llvm::Value *LLVMCodeGenerator::generateConditionalExpr(
    std::unique_ptr<ast::ConditionalExpr> condExpr) {
  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::Value *cond = generateExpression(std::move(condExpr->condition));
  if (!cond) return nullptr;
  cond = toBoolean(builder(), cond, "ifcond");

  llvm::BasicBlock *thenBB =
      llvm::BasicBlock::Create(context(), "condthen", func);
  llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context(), "condelse");
  llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context(), "condmerge");

  builder()->CreateCondBr(cond, thenBB, elseBB);

  builder()->SetInsertPoint(thenBB);
  llvm::Value *thenVal = generateExpression(std::move(condExpr->thenExpr));
  if (!thenVal) return nullptr;
  builder()->CreateBr(mergeBB);
  thenBB = builder()->GetInsertBlock();

  func->insert(func->end(), elseBB);
  builder()->SetInsertPoint(elseBB);
  llvm::Value *elseVal = generateExpression(std::move(condExpr->elseExpr));
  if (!elseVal) return nullptr;
  builder()->CreateBr(mergeBB);
  elseBB = builder()->GetInsertBlock();

  func->insert(func->end(), mergeBB);
  builder()->SetInsertPoint(mergeBB);

  llvm::PHINode *phi = builder()->CreatePHI(thenVal->getType(), 2, "condphi");
  phi->addIncoming(thenVal, thenBB);
  phi->addIncoming(elseVal, elseBB);
  return phi;
}

// 生成 is 类型检查表达式
llvm::Value *LLVMCodeGenerator::generateTypeIsExpr(
    std::unique_ptr<ast::TypeIsExpr> typeIsExpr) {
  // 编译期求值：检查类型是否匹配
  // 简化实现：总是返回 true（后续可扩展为实际类型检查）
  return llvm::ConstantInt::getTrue(context());
}

// 生成 typeof 表达式
llvm::Value *LLVMCodeGenerator::generateTypeofExpr(
    std::unique_ptr<ast::TypeofExpr> typeofExpr) {
  // typeof 在编译期求值，返回类型信息
  // 简化实现：返回一个占位值
  if (typeofExpr->expr) {
    generateExpression(std::move(typeofExpr->expr));
  }
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
}

// 生成编译期字段访问表达式
llvm::Value *LLVMCodeGenerator::generateMetaFieldAccessExpr(
    std::unique_ptr<ast::MetaFieldAccessExpr> metaFieldExpr) {
  // 生成对象
  llvm::Value *object = nullptr;
  if (metaFieldExpr->object) {
    object = generateExpression(std::move(metaFieldExpr->object));
  }

  // 生成字段
  llvm::Value *field = nullptr;
  if (metaFieldExpr->field) {
    field = generateExpression(std::move(metaFieldExpr->field));
  }

  // 简化实现：返回一个占位值
  // 实际实现需要根据字段元对象进行编译期字段访问
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
}

// 生成内置变量表达式
llvm::Value *LLVMCodeGenerator::generateBuiltinVarExpr(
    std::unique_ptr<ast::BuiltinVarExpr> builtinVarExpr) {
  const std::string &name = builtinVarExpr->name;

  if (name == "__line__" || name == "__column__") {
    // 返回行号/列号（编译期常量）
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
  } else if (name == "__timestamp__") {
    // 返回时间戳（编译期常量，当前时间作为 long）
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(context()),
        static_cast<long>(std::time(nullptr)));
  } else if (name == "__build_mode__") {
    // 返回构建模式（0=debug）
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
  } else if (name == "__file__" || name == "__function__" ||
             name == "__module__" || name == "__compiler_version__") {
    // 返回字符串常量（char*）
    std::string strValue;
    if (name == "__function__" && currentFunction_)
      strValue = currentFunction_->getName().str();
    else if (name == "__compiler_version__")
      strValue = "C^ Compiler 0.1";
    else
      strValue = "unknown";

    return builder()->CreateGlobalStringPtr(strValue, name);
  }

  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
}

// 生成表达式语句
llvm::Value *
LLVMCodeGenerator::generateExprStmt(std::unique_ptr<ast::ExprStmt> exprStmt) {
  return generateExpression(std::move(exprStmt->expr));
}

// 生成复合语句
llvm::Value *LLVMCodeGenerator::generateCompoundStmt(
    std::unique_ptr<ast::CompoundStmt> compoundStmt) {
  for (size_t i = 0; i < compoundStmt->statements.size(); ++i) {
    auto &stmt = compoundStmt->statements[i];
    if (!stmt) {
      continue;
    }

    if (auto *statement = dynamic_cast<ast::Statement *>(stmt.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(stmt.release())));
    }

    // return/throw/goto 之后可能还有 label 语句；不能直接停止生成，
    // 否则 label 目标块会缺少内容或 terminator。把后续语句放进
    // 一个不可达块，label 语句会自行切换到目标块。
    if (builder()->GetInsertBlock() &&
        builder()->GetInsertBlock()->getTerminator() &&
        i + 1 < compoundStmt->statements.size()) {
      llvm::BasicBlock *deadBlock = llvm::BasicBlock::Create(
          context(), "unreachable", currentFunction_);
      builder()->SetInsertPoint(deadBlock);
    }
  }
  return nullptr;
}

// 生成返回语句
llvm::Value *LLVMCodeGenerator::generateReturnStmt(
    std::unique_ptr<ast::ReturnStmt> returnStmt) {
  llvm::Type *returnType = currentFunction_ ? currentFunction_->getReturnType()
                                            : llvm::Type::getVoidTy(context());

  // 如果在 try 体内 return，必须先恢复外层异常 handler，否则栈帧销毁后
  // 全局 handler 仍指向失效的 jmp_buf。
  auto restoreExceptionHandlers = [&]() {
    if (exceptionHandlerStack_.empty()) {
      return;
    }
    llvm::PointerType *ptrTy = llvm::PointerType::getUnqual(context());
    auto *excHandler = getExceptionHandlerGlobal(module());
    for (auto it = exceptionHandlerStack_.rbegin();
         it != exceptionHandlerStack_.rend(); ++it) {
      llvm::Value *prev =
          builder()->CreateLoad(ptrTy, *it, "ret.prev.handler");
      builder()->CreateStore(prev, excHandler);
    }
  };

  if (returnStmt->expr) {
    llvm::Value *retVal = generateExpression(std::move(returnStmt->expr));
    if (!retVal) {
      return nullptr;
    }

    // 元组/结构体字面量目前以 alloca 形式返回；函数 ABI 返回结构体值。
    if (returnType->isStructTy() && retVal->getType()->isPointerTy()) {
      if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(retVal)) {
        if (alloca->getAllocatedType() == returnType) {
          retVal = builder()->CreateLoad(returnType, alloca, "ret.struct");
        }
      }
    }

    if (returnType->isVoidTy()) {
      emitDeferCleanup(false);
      restoreExceptionHandlers();
      builder()->CreateRetVoid();
      return nullptr;
    }

    if (retVal->getType() != returnType) {
      if (returnType->isFloatingPointTy() && retVal->getType()->isIntegerTy()) {
        retVal = builder()->CreateSIToFP(retVal, returnType, "ret.cast");
      } else if (returnType->isIntegerTy() &&
                 retVal->getType()->isFloatingPointTy()) {
        retVal = builder()->CreateFPToSI(retVal, returnType, "ret.cast");
      } else if (returnType->isIntegerTy() &&
                 retVal->getType()->isIntegerTy()) {
        retVal = builder()->CreateIntCast(retVal, returnType, true, "ret.cast");
      }
    }

    emitDeferCleanup(false);
    restoreExceptionHandlers();
    builder()->CreateRet(retVal);
  } else {
    if (returnType->isVoidTy()) {
      emitDeferCleanup(false);
      restoreExceptionHandlers();
      builder()->CreateRetVoid();
    } else if (returnType->isIntegerTy()) {
      emitDeferCleanup(false);
      restoreExceptionHandlers();
      builder()->CreateRet(llvm::ConstantInt::get(returnType, 0));
    } else {
      emitDeferCleanup(false);
      restoreExceptionHandlers();
      builder()->CreateRet(llvm::Constant::getNullValue(returnType));
    }
  }
  return nullptr;
}

// 生成 if 语句
llvm::Value *
LLVMCodeGenerator::generateIfStmt(std::unique_ptr<ast::IfStmt> ifStmt) {
  llvm::Value *cond = generateExpression(std::move(ifStmt->condition));
  if (!cond) {
    return nullptr;
  }

  cond = toBoolean(builder(), cond, "ifcond");

  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context(), "then", func);
  llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context(), "else");
  llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context(), "ifcont");

  builder()->CreateCondBr(cond, thenBB, elseBB);

  builder()->SetInsertPoint(thenBB);
  generateStatement(std::move(ifStmt->thenBranch));
  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(mergeBB);
  }

  func->insert(func->end(), elseBB);
  builder()->SetInsertPoint(elseBB);
  if (ifStmt->elseBranch) {
    generateStatement(std::move(ifStmt->elseBranch));
  }
  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(mergeBB);
  }

  func->insert(func->end(), mergeBB);
  builder()->SetInsertPoint(mergeBB);

  return nullptr;
}

// 生成 while 语句
llvm::Value *LLVMCodeGenerator::generateWhileStmt(
    std::unique_ptr<ast::WhileStmt> whileStmt) {
  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::BasicBlock *condBB =
      llvm::BasicBlock::Create(context(), "whilecond", func);
  llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context(), "whilebody");
  llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context(), "whileafter");

  builder()->CreateBr(condBB);

  // 生成条件块
  builder()->SetInsertPoint(condBB);
  llvm::Value *cond = generateExpression(std::move(whileStmt->condition));
  if (!cond) {
    return nullptr;
  }
  cond = toBoolean(builder(), cond, "whilecond");
  builder()->CreateCondBr(cond, bodyBB, afterBB);

  // 生成循环体
  func->insert(func->end(), bodyBB);
  builder()->SetInsertPoint(bodyBB);
  breakTargets_.push_back(afterBB);
  continueTargets_.push_back(condBB);
  generateStatement(std::move(whileStmt->body));
  breakTargets_.pop_back();
  continueTargets_.pop_back();
  builder()->CreateBr(condBB);


  // 生成 after 块
  func->insert(func->end(), afterBB);
  builder()->SetInsertPoint(afterBB);

  return nullptr;
}

// 生成 do-while 语句
llvm::Value *LLVMCodeGenerator::generateDoWhileStmt(
    std::unique_ptr<ast::DoWhileStmt> doWhileStmt) {
  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::BasicBlock *bodyBB =
      llvm::BasicBlock::Create(context(), "dowhilebody", func);
  llvm::BasicBlock *condBB =
      llvm::BasicBlock::Create(context(), "dowhilecond");
  llvm::BasicBlock *afterBB =
      llvm::BasicBlock::Create(context(), "dowhileafter");

  // 先跳转到循环体
  builder()->CreateBr(bodyBB);

  // 生成循环体
  builder()->SetInsertPoint(bodyBB);
  breakTargets_.push_back(afterBB);
  continueTargets_.push_back(condBB);
  generateStatement(std::move(doWhileStmt->body));
  breakTargets_.pop_back();
  continueTargets_.pop_back();
  if (builder()->GetInsertBlock() &&
      !builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(condBB);
  }

  // 生成条件块
  func->insert(func->end(), condBB);
  builder()->SetInsertPoint(condBB);
  llvm::Value *cond = generateExpression(std::move(doWhileStmt->condition));
  if (cond) {
    cond = toBoolean(builder(), cond, "dowhilecond");
    builder()->CreateCondBr(cond, bodyBB, afterBB);
  } else {
    builder()->CreateBr(afterBB);
  }

  // 生成 after 块
  func->insert(func->end(), afterBB);
  builder()->SetInsertPoint(afterBB);

  return nullptr;
}

// 生成 foreach 语句（数组、切片、范围）
llvm::Value *LLVMCodeGenerator::generateForeachStmt(
    std::unique_ptr<ast::ForStmt> forStmt) {
  llvm::Function *func = builder()->GetInsertBlock()->getParent();

  auto *iterVar = dynamic_cast<ast::VariableDecl *>(forStmt->init.get());
  auto *indexVar =
      dynamic_cast<ast::VariableDecl *>(forStmt->indexVar.get());
  if (!iterVar) {
    error("foreach requires an iteration variable");
    return nullptr;
  }

  llvm::Type *elemTy = llvm::Type::getInt32Ty(context());
  llvm::Value *dataPtr = nullptr;
  llvm::Value *count = nullptr;
  llvm::Value *rangeStart = nullptr;
  llvm::Value *rangeEnd = nullptr;
  bool isRange = false;

  // 情况 1：范围  start..end（当前实现按闭区间处理）
  if (auto *range = dynamic_cast<ast::BinaryExpr *>(forStmt->condition.get());
      range != nullptr && range->op == ast::BinaryExpr::Op::Range) {
    rangeStart = generateExpression(std::move(range->left));
    rangeEnd = generateExpression(std::move(range->right));
    if (!rangeStart || !rangeEnd) {
      return nullptr;
    }
    elemTy = rangeStart->getType();
    isRange = true;
  } else {
    // 情况 2：数组 / 切片
    llvm::Value *collection = nullptr;
    if (auto *ident =
            dynamic_cast<ast::Identifier *>(forStmt->condition.get())) {
      auto it = namedValues_.find(ident->name);
      if (it != namedValues_.end()) {
        collection = it->second;
      }
    }
    if (!collection) {
      collection = generateExpression(std::move(forStmt->condition));
    }
    if (!collection) {
      error("Invalid foreach collection");
      return nullptr;
    }

    llvm::Type *collTy = collection->getType();
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(collection)) {
      collTy = alloca->getAllocatedType();
    }

    if (collTy->isArrayTy()) {
      auto *arrayTy = llvm::cast<llvm::ArrayType>(collTy);
      elemTy = arrayTy->getElementType();
      count = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                     arrayTy->getNumElements());
      llvm::Value *base = collection;
      if (llvm::isa<llvm::AllocaInst>(collection)) {
        base = builder()->CreateGEP(
            arrayTy, collection,
            {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
             llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0)},
            "foreach.base");
      }
      dataPtr = base;
    } else if (collTy->isStructTy()) {
      // 切片：{ ptr, len }
      llvm::Value *sliceValue = collection;
      if (llvm::isa<llvm::AllocaInst>(collection)) {
        sliceValue = builder()->CreateLoad(collTy, collection, "foreach.slice");
      }
      dataPtr = builder()->CreateExtractValue(sliceValue, 0, "foreach.ptr");
      count = builder()->CreateExtractValue(sliceValue, 1, "foreach.len");
      if (count->getType() != llvm::Type::getInt32Ty(context())) {
        count = builder()->CreateIntCast(
            count, llvm::Type::getInt32Ty(context()), false, "foreach.len32");
      }

      // 从切片结构体名推断元素类型（Slice_int / Slice_byte / ...）
      std::string name = collTy->getStructName().str();
      if (name.find("byte") != std::string::npos) {
        elemTy = llvm::Type::getInt8Ty(context());
      } else if (name.find("double") != std::string::npos) {
        elemTy = llvm::Type::getDoubleTy(context());
      } else if (name.find("float") != std::string::npos) {
        elemTy = llvm::Type::getFloatTy(context());
      } else if (name.find("long") != std::string::npos) {
        elemTy = llvm::Type::getInt64Ty(context());
      } else {
        elemTy = llvm::Type::getInt32Ty(context());
      }
    } else {
      error("foreach can only iterate over arrays, slices or ranges");
      return nullptr;
    }
  }

  llvm::BasicBlock *condBB =
      llvm::BasicBlock::Create(context(), "foreachcond", func);
  llvm::BasicBlock *bodyBB =
      llvm::BasicBlock::Create(context(), "foreachbody");
  llvm::BasicBlock *stepBB =
      llvm::BasicBlock::Create(context(), "foreachstep");
  llvm::BasicBlock *afterBB =
      llvm::BasicBlock::Create(context(), "foreachafter");

  llvm::AllocaInst *indexAlloca =
      createEntryAlloca(llvm::Type::getInt32Ty(context()), "foreach.index");
  llvm::AllocaInst *iterAlloca =
      createEntryAlloca(elemTy, iterVar->name + ".iter");

  if (isRange) {
    builder()->CreateStore(rangeStart, indexAlloca);
  } else {
    builder()->CreateStore(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
        indexAlloca);
  }

  builder()->CreateBr(condBB);

  // 条件块
  builder()->SetInsertPoint(condBB);
  llvm::Value *idx = builder()->CreateLoad(
      llvm::Type::getInt32Ty(context()), indexAlloca, "foreach.idx");
  llvm::Value *cond = nullptr;
  if (isRange) {
    cond = builder()->CreateICmpSLE(idx, rangeEnd, "foreach.rangecond");
  } else {
    cond = builder()->CreateICmpSLT(idx, count, "foreach.cond");
  }
  builder()->CreateCondBr(cond, bodyBB, afterBB);

  // 循环体
  func->insert(func->end(), bodyBB);
  builder()->SetInsertPoint(bodyBB);
  if (indexVar) {
    namedValues_[indexVar->name] = indexAlloca;
  }
  if (isRange) {
    builder()->CreateStore(idx, iterAlloca);
  } else {
    llvm::Value *elemPtr =
        builder()->CreateGEP(elemTy, dataPtr, idx, "foreach.elemptr");
    llvm::Value *elem = builder()->CreateLoad(elemTy, elemPtr, "foreach.elem");
    builder()->CreateStore(elem, iterAlloca);
  }
  namedValues_[iterVar->name] = iterAlloca;

  breakTargets_.push_back(afterBB);
  continueTargets_.push_back(stepBB);
  generateStatement(std::move(forStmt->body));
  breakTargets_.pop_back();
  continueTargets_.pop_back();
  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(stepBB);
  }

  // 步进块
  func->insert(func->end(), stepBB);
  builder()->SetInsertPoint(stepBB);
  llvm::Value *stepIdx = builder()->CreateLoad(
      llvm::Type::getInt32Ty(context()), indexAlloca, "foreach.stepidx");
  llvm::Value *nextIdx = builder()->CreateAdd(
      stepIdx, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 1),
      "foreach.next");
  builder()->CreateStore(nextIdx, indexAlloca);
  builder()->CreateBr(condBB);

  // 结束块
  func->insert(func->end(), afterBB);
  builder()->SetInsertPoint(afterBB);
  namedValues_.erase(iterVar->name);
  if (indexVar) {
    namedValues_.erase(indexVar->name);
  }
  return nullptr;
}

// 生成 for 语句
llvm::Value *
LLVMCodeGenerator::generateForStmt(std::unique_ptr<ast::ForStmt> forStmt) {
  if (forStmt->isForeach) {
    return generateForeachStmt(std::move(forStmt));
  }
  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::BasicBlock *condBB =
      llvm::BasicBlock::Create(context(), "forcond", func);
  llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context(), "forbody");
  llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context(), "forafter");

  // 生成初始化
  if (forStmt->init) {
    if (auto *decl = dynamic_cast<ast::Declaration *>(forStmt->init.get())) {
      generateDeclaration(std::unique_ptr<ast::Declaration>(
          static_cast<ast::Declaration *>(forStmt->init.release())));
    } else if (auto *stmt =
                   dynamic_cast<ast::Statement *>(forStmt->init.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(forStmt->init.release())));
    }
  }
  builder()->CreateBr(condBB);

  // 生成条件块
  builder()->SetInsertPoint(condBB);
  llvm::Value *cond = nullptr;
  if (forStmt->condition) {
    cond = generateExpression(std::move(forStmt->condition));
    if (cond) {
      cond = toBoolean(builder(), cond, "forcond");
    }
  }
  if (cond) {
    builder()->CreateCondBr(cond, bodyBB, afterBB);
  } else {
    builder()->CreateBr(bodyBB);
  }

  // 生成循环体
  func->insert(func->end(), bodyBB);
  builder()->SetInsertPoint(bodyBB);
  breakTargets_.push_back(afterBB);
  continueTargets_.push_back(condBB);
  generateStatement(std::move(forStmt->body));
  breakTargets_.pop_back();
  continueTargets_.pop_back();
  if (forStmt->update) {
    generateExpression(std::move(forStmt->update));
  }
  builder()->CreateBr(condBB);


  // 生成 after 块
  func->insert(func->end(), afterBB);
  builder()->SetInsertPoint(afterBB);

  return nullptr;
}

// 生成 break 语句
llvm::Value *LLVMCodeGenerator::generateBreakStmt(
    std::unique_ptr<ast::BreakStmt> breakStmt) {
  if (breakTargets_.empty()) {
    error("break outside loop");
    return nullptr;
  }
  builder()->CreateBr(breakTargets_.back());
  llvm::BasicBlock *deadBB = llvm::BasicBlock::Create(
      context(), "afterbreak", builder()->GetInsertBlock()->getParent());
  builder()->SetInsertPoint(deadBB);
  return nullptr;
}

// 生成 continue 语句
llvm::Value *LLVMCodeGenerator::generateContinueStmt(
    std::unique_ptr<ast::ContinueStmt> continueStmt) {
  if (continueTargets_.empty()) {
    error("continue outside loop");
    return nullptr;
  }
  builder()->CreateBr(continueTargets_.back());
  llvm::BasicBlock *deadBB = llvm::BasicBlock::Create(
      context(), "aftercontinue", builder()->GetInsertBlock()->getParent());
  builder()->SetInsertPoint(deadBB);
  return nullptr;
}

// 生成 match 语句
llvm::Value *LLVMCodeGenerator::generateMatchStmt(
    std::unique_ptr<ast::MatchStmt> matchStmt) {
  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::Value *matchVal = generateExpression(std::move(matchStmt->expr));
  if (!matchVal) {
    return nullptr;
  }

  const size_t armCount = matchStmt->arms.size();
  std::vector<llvm::BasicBlock *> checkBBs;
  std::vector<llvm::BasicBlock *> bodyBBs;
  checkBBs.reserve(armCount);
  bodyBBs.reserve(armCount);
  for (size_t i = 0; i < armCount; ++i) {
    checkBBs.push_back(llvm::BasicBlock::Create(
        context(), "matchcheck" + std::to_string(i)));
    bodyBBs.push_back(llvm::BasicBlock::Create(
        context(), "matchbody" + std::to_string(i)));
  }
  llvm::BasicBlock *endBB =
      llvm::BasicBlock::Create(context(), "matchend");
  llvm::BasicBlock *mergeBB =
      llvm::BasicBlock::Create(context(), "matchmerge");

  // 从当前块跳到第一个检查块；所有检查/分支块按顺序插入函数末尾。
  builder()->CreateBr(armCount > 0 ? checkBBs[0] : endBB);

  auto isAlwaysMatch = [](ast::Pattern *pattern) {
    return pattern != nullptr &&
           (pattern->isDefault ||
            (pattern->value &&
             dynamic_cast<ast::Identifier *>(pattern->value.get()) != nullptr));
  };

  for (size_t i = 0; i < armCount; ++i) {
    auto &arm = matchStmt->arms[i];
    ast::Pattern *pattern = arm->pattern.get();

    func->insert(func->end(), checkBBs[i]);
    builder()->SetInsertPoint(checkBBs[i]);

    llvm::BasicBlock *nextBB =
        (i + 1 < armCount) ? checkBBs[i + 1] : endBB;
    bool always = isAlwaysMatch(pattern) && !arm->guard;

    // 标识符模式是绑定：把匹配值存到同名临时变量，供守卫和分支体使用。
    std::string boundName;
    if (pattern && pattern->value) {
      if (auto *ident =
              dynamic_cast<ast::Identifier *>(pattern->value.get())) {
        boundName = ident->name;
        llvm::AllocaInst *binding =
            createEntryAlloca(matchVal->getType(), boundName + ".bind");
        builder()->CreateStore(matchVal, binding);
        namedValues_[boundName] = binding;
      }
    }

    llvm::Value *cond = nullptr;
    if (!always && pattern && pattern->value) {
      auto patternClone = pattern->value->clone();
      llvm::Value *patternVal = generateExpression(std::move(patternClone));

      bool isStringPattern = false;
      if (auto *literal =
              dynamic_cast<ast::Literal *>(pattern->value.get())) {
        isStringPattern = literal->type == ast::Literal::Type::String;
      }

      if (patternVal && isStringPattern) {
        // 字符串字面量在 codegen 里是 byte^；如果 match 值是 literalview
        // 结构体，取它的 ptr 字段再比较。
        llvm::Value *matchPtr = matchVal;
        if (matchVal->getType()->isStructTy() &&
            matchVal->getType()->getStructNumElements() >= 1) {
          matchPtr = builder()->CreateExtractValue(matchVal, 0,
                                                   "match.str.ptr");
        }
        llvm::Value *patternPtr = patternVal;
        if (patternVal->getType()->isStructTy() &&
            patternVal->getType()->getStructNumElements() >= 1) {
          patternPtr = builder()->CreateExtractValue(patternVal, 0,
                                                     "pattern.str.ptr");
        }
        if (matchPtr->getType()->isPointerTy() &&
            patternPtr->getType()->isPointerTy()) {
          llvm::FunctionType *strcmpType = llvm::FunctionType::get(
              llvm::Type::getInt32Ty(context()),
              {llvm::PointerType::get(context(), 0),
               llvm::PointerType::get(context(), 0)},
              false);
          llvm::FunctionCallee strcmpFunc =
              module()->getOrInsertFunction("strcmp", strcmpType);
          llvm::Value *cmpResult = builder()->CreateCall(
              strcmpFunc, {matchPtr, patternPtr}, "match.strcmp");
          cond = builder()->CreateICmpEQ(
              cmpResult,
              llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
              "matchcmp");
        }
      } else if (patternVal && patternVal->getType() == matchVal->getType()) {
        if (matchVal->getType()->isIntegerTy()) {
          cond = builder()->CreateICmpEQ(matchVal, patternVal, "matchcmp");
        } else if (matchVal->getType()->isFloatingPointTy()) {
          cond = builder()->CreateFCmpOEQ(matchVal, patternVal, "matchcmp");
        }
      }
    }

    if (arm->guard) {
      llvm::Value *guardVal = generateExpression(arm->guard->clone());
      if (guardVal) {
        guardVal = toBoolean(builder(), guardVal, "guard");
        cond = cond ? builder()->CreateAnd(cond, guardVal, "matchguard")
                    : guardVal;
      }
    }

    if (always) {
      builder()->CreateBr(bodyBBs[i]);
    } else if (cond) {
      builder()->CreateCondBr(cond, bodyBBs[i], nextBB);
    } else {
      builder()->CreateBr(nextBB);
    }

    func->insert(func->end(), bodyBBs[i]);
    builder()->SetInsertPoint(bodyBBs[i]);
    generateStatement(std::move(arm->body));
    if (!builder()->GetInsertBlock()->getTerminator()) {
      builder()->CreateBr(mergeBB);
    }

    if (!boundName.empty()) {
      namedValues_.erase(boundName);
    }
  }

  func->insert(func->end(), endBB);
  builder()->SetInsertPoint(endBB);
  builder()->CreateBr(mergeBB);

  func->insert(func->end(), mergeBB);
  builder()->SetInsertPoint(mergeBB);
  return nullptr;
}

// 生成 try 语句
//
// 异常传播仍使用 setjmp/longjmp：throw 把值/类型标签写入全局槽，
// longjmp 到最近的 handler。catch 根据类型标签顺序匹配；未匹配的
// 异常会继续向外交给上一层 handler。
llvm::Value *
LLVMCodeGenerator::generateTryStmt(std::unique_ptr<ast::TryStmt> tryStmt) {
  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  auto *excValue = getExceptionValueGlobal(module());
  auto *excType = getExceptionTypeGlobal(module());
  auto *excHandler = getExceptionHandlerGlobal(module());
  llvm::PointerType *ptrTy = llvm::PointerType::getUnqual(context());
  llvm::Type *i64Ty = llvm::Type::getInt64Ty(context());

  llvm::BasicBlock *tryBB =
      llvm::BasicBlock::Create(context(), "trybody", func);
  llvm::BasicBlock *catchBB =
      llvm::BasicBlock::Create(context(), "catchdispatch");
  llvm::BasicBlock *finallyBB =
      llvm::BasicBlock::Create(context(), "tryfinally");
  llvm::BasicBlock *endBB =
      llvm::BasicBlock::Create(context(), "tryend");

  // 栈上的 jmp_buf；256 字节足够覆盖 glibc/MSVC 的 jmp_buf。
  llvm::AllocaInst *jmpBuf = builder()->CreateAlloca(
      llvm::ArrayType::get(llvm::Type::getInt8Ty(context()), 256), nullptr,
      "jmpbuf");
  jmpBuf->setAlignment(llvm::Align(16));

  llvm::AllocaInst *oldHandler =
      builder()->CreateAlloca(ptrTy, nullptr, "oldhandler");
  llvm::Value *prevHandler =
      builder()->CreateLoad(ptrTy, excHandler, "prev.handler");
  builder()->CreateStore(prevHandler, oldHandler);
  builder()->CreateStore(jmpBuf, excHandler);

#ifdef _WIN32
  const char *setjmpName = "_setjmp";
#else
  const char *setjmpName = "setjmp";
#endif
  llvm::FunctionCallee setjmpFn =
      module()->getOrInsertFunction(setjmpName,
                                    llvm::FunctionType::get(
                                        llvm::Type::getInt32Ty(context()),
                                        {ptrTy}, false));
  if (auto *setjmpFunc =
          llvm::dyn_cast<llvm::Function>(setjmpFn.getCallee())) {
    setjmpFunc->addFnAttr(llvm::Attribute::ReturnsTwice);
  }
  llvm::Value *setjmpResult =
      builder()->CreateCall(setjmpFn, {jmpBuf}, "setjmp.result");
  llvm::Value *thrown = builder()->CreateICmpNE(
      setjmpResult,
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
      "thrown");
  builder()->CreateCondBr(thrown, catchBB, tryBB);

  // try 体
  exceptionHandlerStack_.push_back(oldHandler);
  builder()->SetInsertPoint(tryBB);
  generateStatement(std::move(tryStmt->tryBlock));
  exceptionHandlerStack_.pop_back();
  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(finallyBB);
  }

  // 正常路径：恢复外层 handler
  func->insert(func->end(), finallyBB);
  builder()->SetInsertPoint(finallyBB);
  builder()->CreateStore(prevHandler, excHandler);
  builder()->CreateBr(endBB);

  // 异常路径：先把全局 handler 恢复为外层 handler。这样 catch 体内部
  // 再次 throw 时会正确传播，而不是跳回已经失效的当前 jmp_buf。
  func->insert(func->end(), catchBB);
  builder()->SetInsertPoint(catchBB);
  builder()->CreateStore(prevHandler, excHandler);

  // 生成“没有 catch 匹配”时的向外传播代码。
  auto emitRethrow = [&]() {
    llvm::Value *outerHandler =
        builder()->CreateLoad(ptrTy, excHandler, "outer.handler");
    llvm::BasicBlock *rethrowBB =
        llvm::BasicBlock::Create(context(), "rethrow", func);
    llvm::BasicBlock *abortBB =
        llvm::BasicBlock::Create(context(), "exc.abort", func);
    llvm::Value *noHandler = builder()->CreateICmpEQ(
        outerHandler, llvm::ConstantPointerNull::get(ptrTy), "no.handler");
    builder()->CreateCondBr(noHandler, abortBB, rethrowBB);

    builder()->SetInsertPoint(rethrowBB);
    llvm::FunctionCallee longjmpFn = module()->getOrInsertFunction(
        "longjmp", llvm::FunctionType::get(llvm::Type::getVoidTy(context()),
                                           {ptrTy,
                                            llvm::Type::getInt32Ty(context())},
                                           false));
    builder()->CreateCall(longjmpFn, {outerHandler,
                                      llvm::ConstantInt::get(
                                          llvm::Type::getInt32Ty(context()), 1)});
    builder()->CreateUnreachable();

    builder()->SetInsertPoint(abortBB);
    llvm::FunctionCallee abortFn = module()->getOrInsertFunction(
        "abort", llvm::FunctionType::get(llvm::Type::getVoidTy(context()),
                                         false));
    builder()->CreateCall(abortFn);
    builder()->CreateUnreachable();
  };

  if (tryStmt->catchStmts.empty()) {
    emitRethrow();
  } else {
    // 一次性读取异常值；类型标签在每个 catch 的 dispatch 块读取。
    llvm::Value *excWord =
        builder()->CreateLoad(i64Ty, excValue, "exc.word");

    llvm::BasicBlock *dispatchBB = catchBB;
    for (size_t i = 0; i < tryStmt->catchStmts.size(); ++i) {
      auto &catchStmt = *tryStmt->catchStmts[i];
      builder()->SetInsertPoint(dispatchBB);

      llvm::BasicBlock *bodyBB =
          llvm::BasicBlock::Create(context(), "catch.body", func);
      llvm::BasicBlock *nextBB =
          llvm::BasicBlock::Create(context(), "catch.next", func);

      if (catchStmt.catchTag == 0) {
        // catch(...) 或无法确定类型的 catch：接受所有异常。
        builder()->CreateBr(bodyBB);
      } else {
        llvm::Value *throwTag =
            builder()->CreateLoad(i64Ty, excType, "exc.tag");
        llvm::Value *matches = builder()->CreateICmpEQ(
            throwTag,
            llvm::ConstantInt::get(i64Ty, catchStmt.catchTag),
            "catch.match");
        builder()->CreateCondBr(matches, bodyBB, nextBB);
      }

      builder()->SetInsertPoint(bodyBB);
      std::string paramName;
      llvm::Value *previousBinding = nullptr;
      bool hadPreviousBinding = false;
      if (catchStmt.param && !catchStmt.param->name.empty() &&
          catchStmt.param->name != "...") {
        paramName = catchStmt.param->name;
        auto binding = namedValues_.find(paramName);
        if (binding != namedValues_.end()) {
          previousBinding = binding->second;
          hadPreviousBinding = true;
        }

        llvm::Type *paramType = llvm::Type::getInt32Ty(context());
        if (catchStmt.param->type) {
          llvm::Type *generated = generateType(catchStmt.param->type.get());
          if (generated) {
            paramType = generated;
          }
        }
        llvm::AllocaInst *catchAlloca =
            createEntryAlloca(paramType, catchStmt.param->name);
        builder()->CreateStore(
            fromExceptionWord(builder(), excWord, paramType), catchAlloca);
        namedValues_[catchStmt.param->name] = catchAlloca;
      }

      generateStatement(std::move(catchStmt.body));

      // catch 参数只在 catch 体内部可见，避免泄漏到后续语句。
      if (!paramName.empty()) {
        if (hadPreviousBinding) {
          namedValues_[paramName] = previousBinding;
        } else {
          namedValues_.erase(paramName);
        }
      }

      if (!builder()->GetInsertBlock()->getTerminator()) {
        builder()->CreateBr(endBB);
      }
      dispatchBB = nextBB;
    }

    // 所有 catch 都不匹配：继续向外抛。
    builder()->SetInsertPoint(dispatchBB);
    emitRethrow();
  }

  func->insert(func->end(), endBB);
  builder()->SetInsertPoint(endBB);
  return nullptr;
}

// 生成 throw 语句
llvm::Value *LLVMCodeGenerator::generateThrowStmt(
    std::unique_ptr<ast::ThrowStmt> throwStmt) {
  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  auto *excValue = getExceptionValueGlobal(module());
  auto *excType = getExceptionTypeGlobal(module());
  auto *excHandler = getExceptionHandlerGlobal(module());
  llvm::PointerType *ptrTy = llvm::PointerType::getUnqual(context());

  // 无表达式的 `throw;` 是重新抛出：保留当前异常值和类型标签，
  // 这样外层具体类型的 catch 仍能匹配原始异常。
  if (throwStmt->expr) {
    llvm::Value *value = generateExpression(std::move(throwStmt->expr));
    if (!value) {
      value = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
    }
    llvm::Value *word = toExceptionWord(builder(), value);
    builder()->CreateStore(word, excValue);
    builder()->CreateStore(
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context()),
                               throwStmt->exceptionTag),
        excType);
  }

  // 当前函数的 defer 必须在本帧被 longjmp 丢弃前执行。用运行时 flag
  // 判断 defer 是否真的被执行过，避免执行未进入分支里的 defer。
  emitDeferCleanup(false);

  llvm::Value *handler =
      builder()->CreateLoad(ptrTy, excHandler, "handler");
  llvm::BasicBlock *throwBB =
      llvm::BasicBlock::Create(context(), "throw", func);
  llvm::BasicBlock *abortBB =
      llvm::BasicBlock::Create(context(), "throw.abort", func);
  llvm::Value *noHandler = builder()->CreateICmpEQ(
      handler, llvm::ConstantPointerNull::get(ptrTy), "no.handler");
  builder()->CreateCondBr(noHandler, abortBB, throwBB);

  builder()->SetInsertPoint(throwBB);
  llvm::FunctionCallee longjmpFn = module()->getOrInsertFunction(
      "longjmp", llvm::FunctionType::get(llvm::Type::getVoidTy(context()),
                                         {ptrTy,
                                          llvm::Type::getInt32Ty(context())},
                                         false));
  builder()->CreateCall(longjmpFn, {handler,
                                    llvm::ConstantInt::get(
                                        llvm::Type::getInt32Ty(context()), 1)});
  builder()->CreateUnreachable();

  builder()->SetInsertPoint(abortBB);
  llvm::FunctionCallee abortFn = module()->getOrInsertFunction(
      "abort", llvm::FunctionType::get(llvm::Type::getVoidTy(context()),
                                       false));
  builder()->CreateCall(abortFn);
  builder()->CreateUnreachable();
  return nullptr;
}

// 生成 defer 语句。defer 表达式在函数级收集；额外记录一个 i1 标志，
// 表示该 defer 语句在运行时是否真的执行到了。这样在 return/throw
// 的清理路径上不会误执行未进入分支中的 defer。
llvm::Value *LLVMCodeGenerator::generateDeferStmt(
    std::unique_ptr<ast::DeferStmt> deferStmt) {
  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::IRBuilder<> entryBuilder(&func->getEntryBlock(),
                                 func->getEntryBlock().begin());
  llvm::AllocaInst *flag = entryBuilder.CreateAlloca(
      llvm::Type::getInt1Ty(context()), nullptr, "defer.flag");
  entryBuilder.CreateStore(llvm::ConstantInt::getFalse(context()), flag);
  builder()->CreateStore(llvm::ConstantInt::getTrue(context()), flag);

  deferExpressions_.push_back(std::move(deferStmt->expr));
  deferFlags_.push_back(flag);
  return nullptr;
}

// 生成按 LIFO 顺序执行 defer 的代码。每个 defer 都受其运行时 flag
// 保护；consumeExpressions 为 true 时移动原始 AST（函数正常结束），
// 否则克隆 AST（return/throw 路径，后续路径仍需使用原始节点）。
void LLVMCodeGenerator::emitDeferCleanup(bool consumeExpressions) {
  if (deferExpressions_.empty()) {
    return;
  }
  llvm::Function *func = currentFunction_;
  if (!func) {
    return;
  }
  llvm::Type *i1Ty = llvm::Type::getInt1Ty(context());

  for (size_t i = deferExpressions_.size(); i-- > 0;) {
    if (!deferExpressions_[i]) {
      continue;
    }
    llvm::AllocaInst *flag =
        i < deferFlags_.size() ? deferFlags_[i] : nullptr;
    llvm::BasicBlock *thenBB =
        llvm::BasicBlock::Create(context(), "defer.then", func);
    llvm::BasicBlock *mergeBB =
        llvm::BasicBlock::Create(context(), "defer.cont", func);

    if (flag) {
      llvm::Value *enabled =
          builder()->CreateLoad(i1Ty, flag, "defer.enabled");
      builder()->CreateCondBr(enabled, thenBB, mergeBB);
    } else {
      builder()->CreateBr(thenBB);
    }

    builder()->SetInsertPoint(thenBB);
    if (consumeExpressions) {
      generateExpression(std::move(deferExpressions_[i]));
    } else {
      auto cloned = ast::cloneExprWithSubst(deferExpressions_[i].get(), {});
      if (cloned) {
        generateExpression(std::move(cloned));
      }
      // 清理路径已经执行过该 defer；清除 flag，避免同一函数随后
      // 正常结束（例如 throw 被同函数 catch 捕获）时重复执行。
      if (flag && builder()->GetInsertBlock() &&
          !builder()->GetInsertBlock()->getTerminator()) {
        builder()->CreateStore(llvm::ConstantInt::getFalse(context()), flag);
      }
    }
    if (builder()->GetInsertBlock() &&
        !builder()->GetInsertBlock()->getTerminator()) {
      builder()->CreateBr(mergeBB);
    }
    builder()->SetInsertPoint(mergeBB);
  }
}

// 生成 getter 声明（独立，无 class 上下文）
llvm::Value *LLVMCodeGenerator::generateGetterDecl(
    std::unique_ptr<ast::GetterDecl> getterDecl) {
  llvm::Type *returnType = llvm::Type::getInt32Ty(context());
  if (getterDecl->returnType) {
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(getterDecl->returnType.get())) {
      llvm::Type *rt = generateType(typeNode);
      if (rt) returnType = rt;
    }
  }
  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, {}, false);
  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage,
      "get_" + getterDecl->name, module());
  llvm::BasicBlock *entryBB =
      llvm::BasicBlock::Create(context(), "entry", function);
  builder()->SetInsertPoint(entryBB);
  builder()->CreateRet(llvm::Constant::getNullValue(returnType));
  return function;
}

// 生成 getter 声明（class 上下文）
llvm::Value *LLVMCodeGenerator::generateGetterDecl(
    std::unique_ptr<ast::GetterDecl> getterDecl,
    const std::string &className) {
  llvm::StructType *classType = structTypes_[className];
  if (!classType) {
    warning("Class type not found: " + className);
    return nullptr;
  }

  std::vector<llvm::Type *> paramTypes = {classType->getPointerTo()};

  llvm::Type *returnType = llvm::Type::getInt32Ty(context());
  if (getterDecl->returnType) {
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(getterDecl->returnType.get())) {
      llvm::Type *rt = generateType(typeNode);
      if (rt) returnType = rt;
    }
  }

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, false);
  std::string mangledName = className + "_get_" + getterDecl->name;
  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, mangledName, module());

  llvm::BasicBlock *entryBB =
      llvm::BasicBlock::Create(context(), "entry", function);
  builder()->SetInsertPoint(entryBB);

  auto prevFunction = currentFunction_;
  currentFunction_ = function;
  namedValues_.clear();
  namedValuePointeeTypes_.clear();

  auto argIt = function->args().begin();
  llvm::Value *thisArg = &(*argIt++);
  thisArg->setName("self");
  namedValues_["self"] = thisArg;
  namedValues_["base"] = thisArg;
  auto prevClassName = currentClassName_;
  currentClassName_ = className;

  if (getterDecl->arrowExpr) {
    llvm::Value *val = generateExpression(getterDecl->arrowExpr->clone());
    if (val) {
      builder()->CreateRet(val);
    } else {
      builder()->CreateRet(llvm::Constant::getNullValue(returnType));
    }
  } else if (getterDecl->body) {
    if (auto *stmt =
            dynamic_cast<ast::Statement *>(getterDecl->body.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(getterDecl->body.release())));
      if (!function->empty()) {
        llvm::BasicBlock *lastBlock = &function->back();
        if (!lastBlock->getTerminator()) {
          if (returnType->isVoidTy()) {
            builder()->CreateRetVoid();
          } else {
            builder()->CreateRet(llvm::Constant::getNullValue(returnType));
          }
        }
      }
    }
  }

  currentClassName_ = prevClassName;
  currentFunction_ = prevFunction;
  return function;
}

// 生成 setter 声明（独立，无 class 上下文）
llvm::Value *LLVMCodeGenerator::generateSetterDecl(
    std::unique_ptr<ast::SetterDecl> setterDecl) {
  llvm::FunctionType *funcType =
      llvm::FunctionType::get(llvm::Type::getVoidTy(context()), {}, false);
  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage,
      "set_" + setterDecl->name, module());
  llvm::BasicBlock *entryBB =
      llvm::BasicBlock::Create(context(), "entry", function);
  builder()->SetInsertPoint(entryBB);
  builder()->CreateRetVoid();
  return function;
}

// 生成 setter 声明（class 上下文）
llvm::Value *LLVMCodeGenerator::generateSetterDecl(
    std::unique_ptr<ast::SetterDecl> setterDecl,
    const std::string &className) {
  llvm::StructType *classType = structTypes_[className];
  if (!classType) {
    warning("Class type not found: " + className);
    return nullptr;
  }

  llvm::Type *valueType = llvm::Type::getInt32Ty(context());
  if (setterDecl->param && setterDecl->param->type) {
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(setterDecl->param->type.get())) {
      llvm::Type *vt = generateType(typeNode);
      if (vt) valueType = vt;
    }
  }

  std::vector<llvm::Type *> paramTypes = {classType->getPointerTo(),
                                          valueType};
  llvm::FunctionType *funcType =
      llvm::FunctionType::get(llvm::Type::getVoidTy(context()), paramTypes, false);
  std::string mangledName = className + "_set_" + setterDecl->name;
  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, mangledName, module());

  llvm::BasicBlock *entryBB =
      llvm::BasicBlock::Create(context(), "entry", function);
  builder()->SetInsertPoint(entryBB);

  auto prevFunction = currentFunction_;
  currentFunction_ = function;
  namedValues_.clear();
  namedValuePointeeTypes_.clear();

  auto argIt = function->args().begin();
  llvm::Value *thisArg = &(*argIt++);
  thisArg->setName("self");
  namedValues_["self"] = thisArg;
  namedValues_["base"] = thisArg;
  auto prevClassName = currentClassName_;
  currentClassName_ = className;

  if (setterDecl->param) {
    llvm::Value *valArg = &(*argIt++);
    valArg->setName(setterDecl->param->name);
    llvm::AllocaInst *alloca = builder()->CreateAlloca(
        valArg->getType(), nullptr, setterDecl->param->name);
    builder()->CreateStore(valArg, alloca);
    namedValues_[setterDecl->param->name] = alloca;
  }

  if (setterDecl->arrowExpr) {
    generateExpression(setterDecl->arrowExpr->clone());
    builder()->CreateRetVoid();
  } else if (setterDecl->body) {
    if (auto *stmt =
            dynamic_cast<ast::Statement *>(setterDecl->body.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(setterDecl->body.release())));
      if (!function->empty()) {
        llvm::BasicBlock *lastBlock = &function->back();
        if (!lastBlock->getTerminator()) {
          builder()->CreateRetVoid();
        }
      }
    }
  }

  currentClassName_ = prevClassName;
  currentFunction_ = prevFunction;
  return function;
}

// 生成扩展声明
llvm::Value *LLVMCodeGenerator::generateExtensionDecl(
    std::unique_ptr<ast::ExtensionDecl> extensionDecl) {
  if (!extensionDecl->extendedType) {
    warning("Extension without extended type");
    return nullptr;
  }

  std::string structName = getTypeName(extensionDecl->extendedType.get());

  for (auto &member : extensionDecl->members) {
    if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(member.get())) {
      auto ownedFunc = std::unique_ptr<ast::FunctionDecl>(
          static_cast<ast::FunctionDecl *>(member.release()));
      generateExtensionMemberFunction(std::move(ownedFunc), structName);
    }
  }
  return nullptr;
}

// 生成 extern 声明
llvm::Value *LLVMCodeGenerator::generateExternDecl(
    std::unique_ptr<ast::ExternDecl> externDecl) {
  // 处理 extern 块中的所有声明
  for (auto &decl : externDecl->declarations) {
    generateDeclaration(std::move(decl));
  }
  return nullptr;
}

// 生成类型别名声明
llvm::Value *LLVMCodeGenerator::generateTypeAliasDecl(
    std::unique_ptr<ast::TypeAliasDecl> typeAliasDecl) {
  // 类型别名在 LLVM 中不需要生成代码
  return nullptr;
}

// 生成扩展成员函数
llvm::Value *LLVMCodeGenerator::generateExtensionMemberFunction(
    std::unique_ptr<ast::FunctionDecl> funcDecl,
    const std::string &structName) {
  llvm::Type *selfType = nullptr;
  if (auto *structTy = structTypes_[structName]) {
    selfType = structTy->getPointerTo();
  } else {
    selfType = llvm::Type::getInt32Ty(context());
  }

  std::vector<llvm::Type *> paramTypes = {selfType};

  for (auto &paramNode : funcDecl->params) {
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      if (param->type) {
        if (auto *typeNode = dynamic_cast<ast::Type *>(param->type.get())) {
          llvm::Type *pt = generateType(typeNode);
          if (pt) paramTypes.push_back(pt);
        }
      }
    }
  }

  llvm::Type *returnType = llvm::Type::getVoidTy(context());
  if (funcDecl->returnType) {
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(funcDecl->returnType.get())) {
      llvm::Type *rt = generateType(typeNode);
      if (rt) returnType = rt;
    }
  }

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, false);
  std::string mangledName = "ext_" + structName + "_" + funcDecl->name;
  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, mangledName, module());

  if (!funcDecl->body) return function;

  llvm::BasicBlock *entryBB =
      llvm::BasicBlock::Create(context(), "entry", function);
  builder()->SetInsertPoint(entryBB);

  auto prevFunction = currentFunction_;
  currentFunction_ = function;
  namedValues_.clear();
  namedValuePointeeTypes_.clear();
  deferExpressions_.clear();
  deferFlags_.clear();

  auto argIt = function->args().begin();
  llvm::Value *selfArg = &(*argIt++);
  selfArg->setName("self");
  llvm::AllocaInst *selfAlloca =
      builder()->CreateAlloca(selfArg->getType(), nullptr, "self");
  builder()->CreateStore(selfArg, selfAlloca);
  namedValues_["self"] = selfAlloca;

  size_t paramIndex = 0;
  for (; argIt != function->args().end(); ++argIt, ++paramIndex) {
    if (auto *param = dynamic_cast<ast::Parameter *>(
            funcDecl->params[paramIndex].get())) {
      llvm::Value *arg = &(*argIt);
      arg->setName(param->name);
      llvm::AllocaInst *alloca =
          builder()->CreateAlloca(arg->getType(), nullptr, param->name);
      builder()->CreateStore(arg, alloca);
      namedValues_[param->name] = alloca;
      if (auto *pointerType =
              dynamic_cast<ast::PointerType *>(param->type.get())) {
        namedValuePointeeTypes_[param->name] =
            generateType(pointerType->baseType.get());
      }
    }
  }

  if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
    generateStatement(std::unique_ptr<ast::Statement>(
        static_cast<ast::Statement *>(funcDecl->body.release())));
    if (!function->empty()) {
      llvm::BasicBlock *lastBlock = &function->back();
      if (!lastBlock->getTerminator()) {
        builder()->SetInsertPoint(lastBlock);
        emitDeferCleanup(true);
        deferExpressions_.clear();
        deferFlags_.clear();
        if (returnType->isVoidTy()) {
          builder()->CreateRetVoid();
        } else {
          builder()->CreateRet(llvm::Constant::getNullValue(returnType));
        }
      }
    }
  }

  currentFunction_ = prevFunction;
  deferExpressions_.clear();
  deferFlags_.clear();
  return function;
}

// 获取表达式左值
llvm::Value *
LLVMCodeGenerator::getExpressionLValue(std::unique_ptr<ast::Expression> expr) {
  if (auto *ident = dynamic_cast<ast::Identifier *>(expr.get())) {
    auto it = namedValues_.find(ident->name);
    if (it != namedValues_.end()) {
      return it->second;
    }

    // 裸字段名作为赋值目标：x = v（等价于 self.x = v）
    if (!currentClassName_.empty()) {
      auto classIt = structInfo_.find(currentClassName_);
      auto thisIt = namedValues_.find("self");
      if (classIt != structInfo_.end() && thisIt != namedValues_.end()) {
        auto fieldIt = classIt->second.find(ident->name);
        if (fieldIt != classIt->second.end()) {
          llvm::StructType *structTy = structTypes_[currentClassName_];
          return builder()->CreateStructGEP(
              structTy, thisIt->second, fieldIt->second, ident->name + ".ptr");
        }
      }
    }
  }

  if (auto *subscriptExpr = dynamic_cast<ast::SubscriptExpr *>(expr.get())) {
    return generateSubscriptExpr(
        std::unique_ptr<ast::SubscriptExpr>(
            static_cast<ast::SubscriptExpr *>(expr.release())),
        true);
  }

  if (auto *selfExpr = dynamic_cast<ast::SelfExpr *>(expr.get())) {
    auto it = namedValues_.find("self");
    if (it != namedValues_.end()) {
      return it->second;
    }
    return nullptr;
  }

  if (auto *memberExpr = dynamic_cast<ast::MemberExpr *>(expr.get())) {
    if (auto *objectIdent =
            dynamic_cast<ast::Identifier *>(memberExpr->object.get())) {
      if (auto *global = module()->getNamedGlobal(
              objectIdent->name + "_" + memberExpr->member)) {
        return global;
      }
    }
    std::string structName = memberExpr->structName;
    if (structName.empty()) {
      structName = currentClassName_;
    }
    if (structName.empty()) {
      return nullptr;
    }

    unsigned idx = memberExpr->memberIndex;
    auto infoIt = structInfo_.find(structName);
    if (infoIt != structInfo_.end()) {
      auto fieldIt = infoIt->second.find(memberExpr->member);
      if (fieldIt != infoIt->second.end()) {
        idx = fieldIt->second;
      }
    }

    llvm::StructType *structTy = nullptr;
    auto typeIt = structTypes_.find(structName);
    if (typeIt != structTypes_.end()) {
      structTy = typeIt->second;
    } else {
      structTy = llvm::StructType::getTypeByName(context(), structName);
    }
    if (!structTy) {
      return nullptr;
    }

    std::string objectIdentName;
    if (auto *objectIdent =
            dynamic_cast<ast::Identifier *>(memberExpr->object.get())) {
      objectIdentName = objectIdent->name;
    }
    llvm::Value *objectPtr =
        getExpressionLValue(std::move(memberExpr->object));
    if (!objectPtr) {
      return nullptr;
    }
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(objectPtr)) {
      if (alloca->getAllocatedType()->isPointerTy()) {
        objectPtr = builder()->CreateLoad(alloca->getAllocatedType(), alloca,
                                          "member.obj.load");
      }
    } else if (!objectIdentName.empty()) {
      // out.value 中 out 是指针字段：objectPtr 是字段地址，需要先 load
      // 出真正的对象指针。
      if (!currentClassName_.empty()) {
        auto classIt = structInfo_.find(currentClassName_);
        auto thisIt = namedValues_.find("self");
        if (classIt != structInfo_.end() && thisIt != namedValues_.end()) {
          auto fieldIt = classIt->second.find(objectIdentName);
          if (fieldIt != classIt->second.end()) {
            llvm::StructType *ownerType = structTypes_[currentClassName_];
            llvm::Type *fieldType =
                ownerType->getElementType(fieldIt->second);
            if (fieldType->isPointerTy()) {
              objectPtr = builder()->CreateLoad(fieldType, objectPtr,
                                                "member.obj.load");
            }
          }
        }
      }
    }

    return builder()->CreateStructGEP(structTy, objectPtr, idx,
                                      memberExpr->member + ".ptr");
  }

  return nullptr;
}

// 生成类型表达式
llvm::Value *
LLVMCodeGenerator::generateTypeExpr(std::unique_ptr<ast::Type> type) {
  return nullptr;
}

// 类型辅助函数
bool LLVMCodeGenerator::isSliceType(ast::Type *type) {
  return dynamic_cast<ast::SliceType *>(type) != nullptr;
}

bool LLVMCodeGenerator::isArrayType(ast::Type *type) {
  return dynamic_cast<ast::ArrayType *>(type) != nullptr;
}

bool LLVMCodeGenerator::isPointerType(ast::Type *type) {
  return dynamic_cast<ast::PointerType *>(type) != nullptr;
}

bool LLVMCodeGenerator::isReferenceType(ast::Type *type) {
  return dynamic_cast<ast::ReferenceType *>(type) != nullptr;
}

bool LLVMCodeGenerator::isFunctionType(ast::Type *type) {
  return dynamic_cast<ast::FunctionType *>(type) != nullptr;
}

bool LLVMCodeGenerator::isClassType(ast::Type *type) {
  if (auto *namedType = dynamic_cast<ast::NamedType *>(type)) {
    return structTypes_.find(namedType->name) != structTypes_.end();
  }
  return false;
}

bool LLVMCodeGenerator::isStructType(ast::Type *type) {
  return isClassType(type);
}

bool LLVMCodeGenerator::isEnumType(ast::Type *type) {
  if (auto *namedType = dynamic_cast<ast::NamedType *>(type)) {
    return enumTypes_.find(namedType->name) != enumTypes_.end();
  }
  return false;
}

bool LLVMCodeGenerator::isTupleType(ast::Type *type) {
  return dynamic_cast<ast::TupleType *>(type) != nullptr;
}

bool LLVMCodeGenerator::isGenericType(ast::Type *type) {
  return dynamic_cast<ast::GenericType *>(type) != nullptr;
}

bool LLVMCodeGenerator::isReadonlyType(ast::Type *type) { return false; }

void LLVMCodeGenerator::checkExceptionAfterCall() {
  // 异常检查：验证最近一次 call 指令是否可能抛出异常
  // 在 LLVM IR 层面，异常传播通过 invoke 指令 + landingpad 实现
  // 当前实现中，try 块内的函数调用通过基本块结构完成异常路由
  // 扩展思路：如果需要细粒度的 invoke 替代 call，可在 generateCallExpr 中
  // 检查是否在 try 块上下文内，然后使用 CreateInvoke 替代 CreateCall
  // 传递 tryEndBB 作为正常目标，catchBBs 作为异常目标
}

llvm::Value *LLVMCodeGenerator::generateYieldStmt(
    std::unique_ptr<ast::YieldStmt> yieldStmt) {
  if (!currentFunctionIsCoroutine_) {
    error("'yield' statement can only be used in coroutine functions",
          yieldStmt.get());
    return nullptr;
  }

  if (yieldStmt->expr) {
    llvm::Value *value = generateExpression(std::move(yieldStmt->expr));
    if (!value) {
      return nullptr;
    }

    if (coroutinePromiseAlloca_) {
      llvm::StructType *promiseType = llvm::dyn_cast<llvm::StructType>(
          coroutinePromiseAlloca_->getAllocatedType());
      if (promiseType) {
        llvm::Value *zero =
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
        llvm::Value *idx =
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0);
        llvm::IRBuilder<>::InsertPointGuard guard(*builder());
        builder()->SetInsertPoint(
            coroutinePromiseAlloca_->getParent()->getFirstNonPHI());
        llvm::Value *valuePtr =
            builder()->CreateGEP(promiseType, coroutinePromiseAlloca_,
                                 {zero, idx}, "yield_value_ptr");
        builder()->CreateStore(value, valuePtr);
      }
    }
  }

  coroutineStateIndex_++;

  llvm::BasicBlock *suspendBlock =
      llvm::BasicBlock::Create(context(), "yield_suspend", currentFunction_);
  llvm::BasicBlock *resumeBlock =
      llvm::BasicBlock::Create(context(), "yield_resume", currentFunction_);

  builder()->CreateBr(suspendBlock);
  builder()->SetInsertPoint(suspendBlock);

  if (coroutineStateAlloca_) {
    llvm::Value *stateValue = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context()), coroutineStateIndex_);
    builder()->CreateStore(stateValue, coroutineStateAlloca_);
  }

  builder()->CreateRetVoid();

  builder()->SetInsertPoint(resumeBlock);

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateAwaitExpr(
    std::unique_ptr<ast::UnaryExpr> awaitExpr) {
  if (!currentFunctionIsCoroutine_) {
    error("'await' expression can only be used in coroutine functions",
          awaitExpr.get());
    return nullptr;
  }

  llvm::Value *operand = generateExpression(std::move(awaitExpr->expr));
  if (!operand) {
    return nullptr;
  }

  coroutineStateIndex_++;

  llvm::BasicBlock *suspendBlock =
      llvm::BasicBlock::Create(context(), "await_suspend", currentFunction_);
  llvm::BasicBlock *resumeBlock =
      llvm::BasicBlock::Create(context(), "await_resume", currentFunction_);

  builder()->CreateBr(suspendBlock);
  builder()->SetInsertPoint(suspendBlock);

  if (coroutineStateAlloca_) {
    llvm::Value *stateValue = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context()), coroutineStateIndex_);
    builder()->CreateStore(stateValue, coroutineStateAlloca_);
  }

  builder()->CreateRetVoid();

  builder()->SetInsertPoint(resumeBlock);

  return operand;
}

llvm::Value *
LLVMCodeGenerator::generateGotoStmt(std::unique_ptr<ast::GotoStmt> gotoStmt) {
  if (!gotoStmt->label.empty()) {
    auto it = labelBlocks_.find(gotoStmt->label);
    if (it != labelBlocks_.end()) {
      builder()->CreateBr(it->second);
    } else {
      llvm::BasicBlock *targetBlock = llvm::BasicBlock::Create(
          context(), "label_" + gotoStmt->label, currentFunction_);
      labelBlocks_[gotoStmt->label] = targetBlock;
      builder()->CreateBr(targetBlock);
    }
  }
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateLabelStmt(
    std::unique_ptr<ast::LabelStmt> labelStmt) {
  if (!labelStmt->label.empty()) {
    llvm::BasicBlock *labelBlock = nullptr;
    auto it = labelBlocks_.find(labelStmt->label);
    if (it != labelBlocks_.end()) {
      labelBlock = it->second;
      if (labelBlock->size() == 0) {
        builder()->SetInsertPoint(labelBlock);
      } else {
        llvm::BasicBlock *newBlock =
            llvm::BasicBlock::Create(context(), "", currentFunction_);
        builder()->SetInsertPoint(newBlock);
        labelBlocks_[labelStmt->label] = newBlock;
      }
    } else {
      labelBlock = llvm::BasicBlock::Create(
          context(), "label_" + labelStmt->label, currentFunction_);
      labelBlocks_[labelStmt->label] = labelBlock;

      llvm::BasicBlock *prevBlock = builder()->GetInsertBlock();
      if (prevBlock && !prevBlock->getTerminator()) {
        builder()->CreateBr(labelBlock);
      }
      builder()->SetInsertPoint(labelBlock);
    }
  }
  return nullptr;
}

} // namespace llvm_codegen
} // namespace c_hat