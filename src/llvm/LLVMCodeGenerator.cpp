#include "LLVMCodeGenerator.h"
#include <iostream>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <stdexcept>

namespace c_hat {
namespace llvm_codegen {

LLVMCodeGenerator::LLVMCodeGenerator(const std::string &moduleName)
    : generator_(moduleName) {}

void LLVMCodeGenerator::generate(std::unique_ptr<ast::Program> program) {
  for (size_t i = 0; i < program->declarations.size(); ++i) {
    auto &decl = program->declarations[i];
    generateDeclaration(std::move(decl));
  }
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
  default:
    return nullptr;
  }
}

llvm::Value *LLVMCodeGenerator::generateVariableDecl(
    std::unique_ptr<ast::VariableDecl> varDecl) {
  llvm::Type *varType = nullptr;
  bool isSliceType = false;
  llvm::Type *sliceElementType = nullptr;

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
    } else {
      throw std::runtime_error(
          "Cannot infer type from non-literal initializer for variable: " +
          varDecl->name);
    }
  }

  if (!varType) {
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
        constValue =
            llvm::cast<llvm::Constant>(createStringLiteral(literal->value));
        break;
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

  llvm::AllocaInst *alloca =
      builder()->CreateAlloca(varType, nullptr, varDecl->name);

  // 检查是否是切片类型且初始化值是数组初始化表达式，进行临时数组生命周期延长
  if (isSliceType && sliceElementType && varDecl->initializer) {
    if (auto *arrayInit =
            dynamic_cast<ast::ArrayInitExpr *>(varDecl->initializer.get())) {
      // 1. 创建临时数组变量
      size_t arraySize = arrayInit->elements.size();
      std::string tempArrayName = "_temp_" + varDecl->name;
      llvm::Type *tempArrayType = getArrayType(sliceElementType, arraySize);
      llvm::AllocaInst *tempArrayAlloca =
          builder()->CreateAlloca(tempArrayType, nullptr, tempArrayName);

      // 2. 生成数组初始化代码
      std::vector<llvm::Constant *> constants;
      for (auto &elem : arrayInit->elements) {
        if (auto *literal = dynamic_cast<ast::Literal *>(elem.get())) {
          llvm::Value *val = generateLiteralExpr(std::unique_ptr<ast::Literal>(
              static_cast<ast::Literal *>(elem.release())));
          if (auto *constVal = llvm::dyn_cast<llvm::Constant>(val)) {
            constants.push_back(constVal);
          } else {
            throw std::runtime_error(
                "Array initializer elements must be literals");
          }
        } else {
          throw std::runtime_error(
              "Array initializer elements must be literals");
        }
      }
      llvm::Constant *arrayConstant = llvm::ConstantArray::get(
          llvm::ArrayType::get(sliceElementType, arraySize), constants);
      builder()->CreateStore(arrayConstant, tempArrayAlloca);

      // 3. 创建切片，指向临时数组
      std::vector<llvm::Value *> indices;
      indices.push_back(
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0));
      indices.push_back(
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0));
      llvm::Value *arrayPtr = builder()->CreateGEP(
          tempArrayType, tempArrayAlloca, indices, "array_ptr");

      // 4. 构建切片结构体
      llvm::Value *sliceValue = llvm::UndefValue::get(varType);
      sliceValue =
          builder()->CreateInsertValue(sliceValue, arrayPtr, 0, "slice_ptr");
      llvm::Constant *lenConstant =
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(context()), arraySize);
      sliceValue =
          builder()->CreateInsertValue(sliceValue, lenConstant, 1, "slice_len");

      // 5. 存储切片到变量
      builder()->CreateStore(sliceValue, alloca);

      namedValues_[varDecl->name] = alloca;
      return alloca;
    }
  }

  // 正常初始化路径
  if (varDecl->initializer) {
    llvm::Value *initValue =
        generateExpression(std::move(varDecl->initializer));
    builder()->CreateStore(initValue, alloca);
  }

  namedValues_[varDecl->name] = alloca;
  return alloca;
}

llvm::Value *LLVMCodeGenerator::generateTupleDestructuringDecl(
    std::unique_ptr<ast::TupleDestructuringDecl> tupleDecl) {
  if (!tupleDecl->initializer) {
    throw std::runtime_error("Tuple destructuring must have initializer");
  }

  // 1. 生成初始化表达式的值
  llvm::Value *tupleValue =
      generateExpression(std::move(tupleDecl->initializer));

  // 2. 为每个变量分配空间并赋值
  for (size_t i = 0; i < tupleDecl->names.size(); ++i) {
    const auto &name = tupleDecl->names[i];
    // 我们现在还没有精确的类型信息，不过先简单实现，假设我们已经知道了类型
    // 这里暂时只做一个占位实现，后续需要类型信息才能正确生成
    // 先假设所有元素都是 int 类型，用于测试
    llvm::Type *elemType = llvm::Type::getInt32Ty(context());
    llvm::AllocaInst *alloca = builder()->CreateAlloca(elemType, nullptr, name);

    // 从 tupleValue 中提取第 i 个元素
    llvm::Value *elemValue =
        builder()->CreateExtractValue(tupleValue, i, name + "_elem");
    builder()->CreateStore(elemValue, alloca);

    namedValues_[name] = alloca;
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
      llvm::Type *paramType = generateType(param->type.get());
      if (!paramType) {
        throw std::runtime_error("Unknown parameter type for function: " +
                                 funcDecl->name);
      }
      paramTypes.push_back(paramType);
    }
  }

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, false);

  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, funcDecl->name, module());

  if (funcDecl->body) {
    llvm::BasicBlock *entryBlock =
        llvm::BasicBlock::Create(context(), "entry", function);
    builder()->SetInsertPoint(entryBlock);

    auto prevFunction = currentFunction_;
    currentFunction_ = function;

    namedValues_.clear();
    deferExpressions_.clear();

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

        ++argIt;
        ++paramIndex;
      }
    }

    if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(funcDecl->body.release())));

      llvm::BasicBlock *lastBlock = &function->back();
      if (!lastBlock->getTerminator()) {
        // 执行 defer 语句（按相反顺序）
        while (!deferExpressions_.empty()) {
          auto deferExpr = std::move(deferExpressions_.back());
          deferExpressions_.pop_back();
          generateExpression(std::move(deferExpr));
        }

        if (returnType->isVoidTy()) {
          builder()->SetInsertPoint(lastBlock);
          builder()->CreateRetVoid();
        } else {
          builder()->SetInsertPoint(lastBlock);
          builder()->CreateRet(llvm::Constant::getNullValue(returnType));
        }
      }
    }

    currentFunction_ = prevFunction;
    deferExpressions_.clear();
  }

  functions_[funcDecl->name] = function;
  return function;
}

llvm::Value *LLVMCodeGenerator::generateClassDecl(
    std::unique_ptr<ast::ClassDecl> classDecl) {
  std::vector<llvm::Type *> memberTypes;
  std::unordered_map<std::string, unsigned> memberIndices;
  std::vector<std::unique_ptr<ast::Node>> functions;

  for (size_t i = 0; i < classDecl->members.size(); ++i) {
    auto &member = classDecl->members[i];
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
    auto funcDecl = std::unique_ptr<ast::FunctionDecl>(
        static_cast<ast::FunctionDecl *>(funcNode.release()));

    bool isConstructor = (funcDecl->name == classDecl->name);
    bool isDestructor = (!funcDecl->name.empty() && funcDecl->name[0] == '~');

    generateClassMemberFunction(std::move(funcDecl), classDecl->name,
                                isConstructor, isDestructor);
  }

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateStructDecl(
    std::unique_ptr<ast::StructDecl> structDecl) {
  std::vector<llvm::Type *> memberTypes;
  std::unordered_map<std::string, unsigned> memberIndices;

  for (size_t i = 0; i < structDecl->members.size(); ++i) {
    auto &member = structDecl->members[i];
    if (auto *varDecl = dynamic_cast<ast::VariableDecl *>(member.get())) {
      if (auto *typeNode = dynamic_cast<ast::Type *>(varDecl->type.get())) {
        llvm::Type *memberType = generateType(typeNode);
        memberTypes.push_back(memberType);
        memberIndices[varDecl->name] = static_cast<unsigned>(i);
      }
    }
  }

  if (memberTypes.empty()) {
    memberTypes.push_back(llvm::Type::getInt8Ty(context()));
  }

  llvm::StructType *structType =
      llvm::StructType::create(context(), memberTypes, structDecl->name);
  structTypes_[structDecl->name] = structType;
  structInfo_[structDecl->name] = memberIndices;

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateClassMemberFunction(
    std::unique_ptr<ast::FunctionDecl> funcDecl, const std::string &className,
    bool isConstructor, bool isDestructor) {
  llvm::StructType *classType = structTypes_[className];
  llvm::PointerType *thisType = classType->getPointerTo();

  std::vector<llvm::Type *> paramTypes;
  paramTypes.push_back(thisType);

  for (auto &paramNode : funcDecl->params) {
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      if (param->type) {
        llvm::Type *paramType = generateType(param->type.get());
        paramTypes.push_back(paramType);
      }
    }
  }

  llvm::Type *returnType;
  if (isConstructor || isDestructor) {
    returnType = llvm::Type::getVoidTy(context());
  } else if (funcDecl->returnType) {
    if (auto *returnTypeNode =
            dynamic_cast<ast::Type *>(funcDecl->returnType.get())) {
      returnType = generateType(returnTypeNode);
    } else {
      returnType = llvm::Type::getVoidTy(context());
    }
  } else {
    returnType = llvm::Type::getVoidTy(context());
  }

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, false);

  std::string mangledName = className + "_" + funcDecl->name;
  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, mangledName, module());

  llvm::Function::arg_iterator args = function->arg_begin();
  llvm::Argument *selfArg = &*args++;
  selfArg->setName("self");

  int paramIndex = 0;
  for (; args != function->arg_end(); ++args, ++paramIndex) {
    if (auto *param = dynamic_cast<ast::Parameter *>(
            funcDecl->params[paramIndex].get())) {
      args->setName(param->name);
    }
  }

  llvm::BasicBlock *entryBB =
      llvm::BasicBlock::Create(context(), "entry", function);
  builder()->SetInsertPoint(entryBB);

  llvm::Function *prevFunction = currentFunction_;
  currentFunction_ = function;
  namedValues_["self"] = selfArg;

  for (size_t i = 0; i < funcDecl->params.size(); ++i) {
    if (auto *param =
            dynamic_cast<ast::Parameter *>(funcDecl->params[i].get())) {
      namedValues_[param->name] = function->getArg(i + 1);
    }
  }

  if (funcDecl->body) {
    generateStatement(std::unique_ptr<ast::Statement>(
        static_cast<ast::Statement *>(funcDecl->body.release())));

    llvm::BasicBlock *lastBlock = &function->back();
    if (!lastBlock->getTerminator()) {
      while (!deferExpressions_.empty()) {
        auto deferExpr = std::move(deferExpressions_.back());
        deferExpressions_.pop_back();
        generateExpression(std::move(deferExpr));
      }

      if (returnType->isVoidTy()) {
        builder()->SetInsertPoint(lastBlock);
        builder()->CreateRetVoid();
      } else {
        builder()->SetInsertPoint(lastBlock);
        builder()->CreateRet(llvm::Constant::getNullValue(returnType));
      }
    }
  }

  currentFunction_ = prevFunction;
  deferExpressions_.clear();
  functions_[mangledName] = function;

  return function;
}

llvm::Value *LLVMCodeGenerator::generateTypeAliasDecl(
    std::unique_ptr<ast::TypeAliasDecl> typeAliasDecl) {
  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateStatement(std::unique_ptr<ast::Statement> stmt) {
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
  case ast::NodeType::VariableDecl: {
    auto *varStmt = static_cast<ast::VariableStmt *>(stmt.get());
    stmt.release();
    return generateVariableDecl(std::move(varStmt->declaration));
  }
  case ast::NodeType::TupleDestructuringDecl: {
    auto *tupleStmt = static_cast<ast::TupleDestructuringStmt *>(stmt.get());
    stmt.release();
    return generateTupleDestructuringDecl(std::move(tupleStmt->declaration));
  }
  default:
    return nullptr;
  }
}

llvm::Value *
LLVMCodeGenerator::generateExprStmt(std::unique_ptr<ast::ExprStmt> exprStmt) {
  return generateExpression(std::move(exprStmt->expr));
}

llvm::Value *LLVMCodeGenerator::generateCompoundStmt(
    std::unique_ptr<ast::CompoundStmt> compoundStmt) {
  llvm::Value *lastValue = nullptr;
  for (auto &stmt : compoundStmt->statements) {
    if (auto *declaration = dynamic_cast<ast::Declaration *>(stmt.get())) {
      stmt.release();
      lastValue =
          generateDeclaration(std::unique_ptr<ast::Declaration>(declaration));
    } else if (auto *statement = dynamic_cast<ast::Statement *>(stmt.get())) {
      stmt.release();
      lastValue = generateStatement(std::unique_ptr<ast::Statement>(statement));
    }
  }
  return lastValue;
}

llvm::Value *LLVMCodeGenerator::generateReturnStmt(
    std::unique_ptr<ast::ReturnStmt> returnStmt) {
  // 先执行 defer 语句（按相反顺序）
  while (!deferExpressions_.empty()) {
    auto deferExpr = std::move(deferExpressions_.back());
    deferExpressions_.pop_back();
    generateExpression(std::move(deferExpr));
  }

  if (returnStmt->expr) {
    llvm::Value *returnValue = generateExpression(std::move(returnStmt->expr));
    return builder()->CreateRet(returnValue);
  } else {
    return builder()->CreateRetVoid();
  }
}

llvm::Value *
LLVMCodeGenerator::generateIfStmt(std::unique_ptr<ast::IfStmt> ifStmt) {
  llvm::Value *condition = generateExpression(std::move(ifStmt->condition));

  if (condition->getType()->isFloatingPointTy()) {
    condition = builder()->CreateFCmpONE(
        condition, llvm::ConstantFP::get(condition->getType(), 0.0), "ifcond");
  } else {
    condition = builder()->CreateICmpNE(
        condition, llvm::ConstantInt::get(condition->getType(), 0), "ifcond");
  }

  llvm::Function *function = builder()->GetInsertBlock()->getParent();

  llvm::BasicBlock *thenBlock =
      llvm::BasicBlock::Create(context(), "then", function);
  llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(context(), "else");
  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(context(), "ifcont");

  builder()->CreateCondBr(condition, thenBlock, elseBlock);

  builder()->SetInsertPoint(thenBlock);
  generateStatement(std::move(ifStmt->thenBranch));
  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(mergeBlock);
  }

  function->insert(function->end(), elseBlock);
  builder()->SetInsertPoint(elseBlock);
  if (ifStmt->elseBranch) {
    generateStatement(std::move(ifStmt->elseBranch));
  }
  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(mergeBlock);
  }

  function->insert(function->end(), mergeBlock);
  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->SetInsertPoint(mergeBlock);
  }

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateWhileStmt(
    std::unique_ptr<ast::WhileStmt> whileStmt) {
  llvm::Function *function = builder()->GetInsertBlock()->getParent();

  llvm::BasicBlock *condBlock =
      llvm::BasicBlock::Create(context(), "whilecond", function);
  llvm::BasicBlock *loopBlock =
      llvm::BasicBlock::Create(context(), "whilebody");
  llvm::BasicBlock *afterBlock =
      llvm::BasicBlock::Create(context(), "whileend");

  auto prevContinueBlock = continueBlock_;
  auto prevBreakBlock = breakBlock_;
  continueBlock_ = condBlock;
  breakBlock_ = afterBlock;

  builder()->CreateBr(condBlock);

  builder()->SetInsertPoint(condBlock);
  llvm::Value *condition = generateExpression(std::move(whileStmt->condition));

  if (condition->getType()->isFloatingPointTy()) {
    condition = builder()->CreateFCmpONE(
        condition, llvm::ConstantFP::get(condition->getType(), 0.0),
        "whilecond");
  } else {
    condition = builder()->CreateICmpNE(
        condition, llvm::ConstantInt::get(condition->getType(), 0),
        "whilecond");
  }

  builder()->CreateCondBr(condition, loopBlock, afterBlock);

  function->insert(function->end(), loopBlock);
  builder()->SetInsertPoint(loopBlock);
  generateStatement(std::move(whileStmt->body));
  builder()->CreateBr(condBlock);

  function->insert(function->end(), afterBlock);
  builder()->SetInsertPoint(afterBlock);

  continueBlock_ = prevContinueBlock;
  breakBlock_ = prevBreakBlock;

  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateForStmt(std::unique_ptr<ast::ForStmt> forStmt) {
  llvm::Function *function = builder()->GetInsertBlock()->getParent();

  llvm::BasicBlock *preLoopBlock =
      llvm::BasicBlock::Create(context(), "preloop", function);
  llvm::BasicBlock *condBlock = llvm::BasicBlock::Create(context(), "forcond");
  llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(context(), "forbody");
  llvm::BasicBlock *updateBlock =
      llvm::BasicBlock::Create(context(), "forupdate");
  llvm::BasicBlock *afterBlock = llvm::BasicBlock::Create(context(), "forend");

  auto prevContinueBlock = continueBlock_;
  auto prevBreakBlock = breakBlock_;
  continueBlock_ = updateBlock;
  breakBlock_ = afterBlock;

  builder()->CreateBr(preLoopBlock);

  builder()->SetInsertPoint(preLoopBlock);
  if (forStmt->init) {
    if (auto *decl = dynamic_cast<ast::Declaration *>(forStmt->init.get())) {
      forStmt->init.release();
      generateDeclaration(std::unique_ptr<ast::Declaration>(decl));
    } else if (auto *expr =
                   dynamic_cast<ast::Expression *>(forStmt->init.get())) {
      forStmt->init.release();
      generateExpression(std::unique_ptr<ast::Expression>(expr));
    }
  }
  builder()->CreateBr(condBlock);

  function->insert(function->end(), condBlock);
  builder()->SetInsertPoint(condBlock);
  llvm::Value *condition = nullptr;
  if (forStmt->condition) {
    condition = generateExpression(std::move(forStmt->condition));

    if (condition->getType()->isFloatingPointTy()) {
      condition = builder()->CreateFCmpONE(
          condition, llvm::ConstantFP::get(condition->getType(), 0.0),
          "forcond");
    } else {
      condition = builder()->CreateICmpNE(
          condition, llvm::ConstantInt::get(condition->getType(), 0),
          "forcond");
    }
  } else {
    condition = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context()), 1);
  }

  builder()->CreateCondBr(condition, loopBlock, afterBlock);

  function->insert(function->end(), loopBlock);
  builder()->SetInsertPoint(loopBlock);
  generateStatement(std::move(forStmt->body));
  builder()->CreateBr(updateBlock);

  function->insert(function->end(), updateBlock);
  builder()->SetInsertPoint(updateBlock);
  if (forStmt->update) {
    generateExpression(std::move(forStmt->update));
  }
  builder()->CreateBr(condBlock);

  function->insert(function->end(), afterBlock);
  builder()->SetInsertPoint(afterBlock);

  continueBlock_ = prevContinueBlock;
  breakBlock_ = prevBreakBlock;

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateBreakStmt(
    std::unique_ptr<ast::BreakStmt> breakStmt) {
  if (breakBlock_) {
    builder()->CreateBr(breakBlock_);
  }
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateContinueStmt(
    std::unique_ptr<ast::ContinueStmt> continueStmt) {
  if (continueBlock_) {
    builder()->CreateBr(continueBlock_);
  }
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateMatchStmt(
    std::unique_ptr<ast::MatchStmt> matchStmt) {
  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateTryStmt(std::unique_ptr<ast::TryStmt> tryStmt) {
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateThrowStmt(
    std::unique_ptr<ast::ThrowStmt> throwStmt) {
  if (throwStmt->expr) {
    generateExpression(std::move(throwStmt->expr));
  }
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateDeferStmt(
    std::unique_ptr<ast::DeferStmt> deferStmt) {
  if (deferStmt->expr) {
    deferExpressions_.push_back(std::move(deferStmt->expr));
  }
  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateExpression(std::unique_ptr<ast::Expression> expr) {
  switch (expr->getType()) {
  case ast::NodeType::BinaryExpr:
    return generateBinaryExpr(std::unique_ptr<ast::BinaryExpr>(
        static_cast<ast::BinaryExpr *>(expr.release())));
  case ast::NodeType::UnaryExpr:
    return generateUnaryExpr(std::unique_ptr<ast::UnaryExpr>(
        static_cast<ast::UnaryExpr *>(expr.release())));
  case ast::NodeType::Identifier:
    return generateIdentifierExpr(std::unique_ptr<ast::Identifier>(
        static_cast<ast::Identifier *>(expr.release())));
  case ast::NodeType::Literal:
    return generateLiteralExpr(std::unique_ptr<ast::Literal>(
        static_cast<ast::Literal *>(expr.release())));
  case ast::NodeType::CallExpr:
    return generateCallExpr(std::unique_ptr<ast::CallExpr>(
        static_cast<ast::CallExpr *>(expr.release())));
  case ast::NodeType::MemberExpr:
    return generateMemberExpr(std::unique_ptr<ast::MemberExpr>(
        static_cast<ast::MemberExpr *>(expr.release())));
  case ast::NodeType::SubscriptExpr: {
    auto *subscript = static_cast<ast::SubscriptExpr *>(expr.release());
    llvm::Value *arrayPtr = getExpressionLValue(
        std::unique_ptr<ast::Expression>(subscript->object.release()));
    llvm::Value *index = generateExpression(
        std::unique_ptr<ast::Expression>(subscript->index.release()));

    llvm::Type *arrayType = nullptr;
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr)) {
      arrayType = alloca->getAllocatedType();
    }

    std::vector<llvm::Value *> indices;
    indices.push_back(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0));
    indices.push_back(index);

    llvm::Value *elemPtr =
        builder()->CreateGEP(arrayType, arrayPtr, indices, "subscripttmp");

    llvm::Type *elemType = nullptr;
    if (arrayType && arrayType->isArrayTy()) {
      elemType = llvm::cast<llvm::ArrayType>(arrayType)->getElementType();
    } else {
      elemType = llvm::Type::getInt32Ty(context());
    }

    return builder()->CreateLoad(elemType, elemPtr, "elemtmp");
  }
  case ast::NodeType::NewExpr:
    return generateNewExpr(std::unique_ptr<ast::NewExpr>(
        static_cast<ast::NewExpr *>(expr.release())));
  case ast::NodeType::DeleteExpr:
    return generateDeleteExpr(std::unique_ptr<ast::DeleteExpr>(
        static_cast<ast::DeleteExpr *>(expr.release())));
  case ast::NodeType::ThisExpr:
    return generateThisExpr(std::unique_ptr<ast::ThisExpr>(
        static_cast<ast::ThisExpr *>(expr.release())));
  case ast::NodeType::SelfExpr:
    return generateSelfExpr(std::unique_ptr<ast::SelfExpr>(
        static_cast<ast::SelfExpr *>(expr.release())));
  case ast::NodeType::SuperExpr:
    return generateSuperExpr(std::unique_ptr<ast::SuperExpr>(
        static_cast<ast::SuperExpr *>(expr.release())));
  case ast::NodeType::ExpansionExpr:
    return generateExpansionExpr(std::unique_ptr<ast::ExpansionExpr>(
        static_cast<ast::ExpansionExpr *>(expr.release())));
  case ast::NodeType::LambdaExpr:
    return generateLambdaExpr(std::unique_ptr<ast::LambdaExpr>(
        static_cast<ast::LambdaExpr *>(expr.release())));
  case ast::NodeType::ArrayInitExpr:
    return generateArrayInitExpr(std::unique_ptr<ast::ArrayInitExpr>(
        static_cast<ast::ArrayInitExpr *>(expr.release())));
  case ast::NodeType::StructInitExpr:
    return generateStructInitExpr(std::unique_ptr<ast::StructInitExpr>(
        static_cast<ast::StructInitExpr *>(expr.release())));
  case ast::NodeType::TupleExpr:
    return generateTupleExpr(std::unique_ptr<ast::TupleExpr>(
        static_cast<ast::TupleExpr *>(expr.release())));
  default:
    return nullptr;
  }
}

llvm::Value *LLVMCodeGenerator::generateBinaryExpr(
    std::unique_ptr<ast::BinaryExpr> binaryExpr) {
  llvm::Value *left = nullptr;
  llvm::Value *leftAddr = nullptr;
  llvm::Value *right = generateExpression(std::move(binaryExpr->right));

  if (!right) {
    return nullptr;
  }

  bool isAssignOp = false;
  bool isSimpleAssign = (binaryExpr->op == ast::BinaryExpr::Op::Assign);
  switch (binaryExpr->op) {
  case ast::BinaryExpr::Op::Assign:
  case ast::BinaryExpr::Op::AddAssign:
  case ast::BinaryExpr::Op::SubAssign:
  case ast::BinaryExpr::Op::MulAssign:
  case ast::BinaryExpr::Op::DivAssign:
  case ast::BinaryExpr::Op::ModAssign:
  case ast::BinaryExpr::Op::AndAssign:
  case ast::BinaryExpr::Op::OrAssign:
  case ast::BinaryExpr::Op::XorAssign:
  case ast::BinaryExpr::Op::ShlAssign:
  case ast::BinaryExpr::Op::ShrAssign:
    isAssignOp = true;
    break;
  default:
    break;
  }

  if (isAssignOp) {
    leftAddr = getExpressionLValue(std::move(binaryExpr->left));
    if (!isSimpleAssign) {
      if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(leftAddr)) {
        left = builder()->CreateLoad(alloca->getAllocatedType(), alloca,
                                     "lefttmp");
      }
    }
  }

  if (!isAssignOp && !left) {
    left = generateExpression(std::move(binaryExpr->left));
  }

  if (!left && !isSimpleAssign) {
    return nullptr;
  }

  bool isFloat = false;
  if (!isSimpleAssign) {
    isFloat = left->getType()->isFloatingPointTy();
  }
  llvm::Value *result = nullptr;

  switch (binaryExpr->op) {
  case ast::BinaryExpr::Op::Add:
    return isFloat ? builder()->CreateFAdd(left, right, "addtmp")
                   : builder()->CreateAdd(left, right, "addtmp");
  case ast::BinaryExpr::Op::Sub:
    return isFloat ? builder()->CreateFSub(left, right, "subtmp")
                   : builder()->CreateSub(left, right, "subtmp");
  case ast::BinaryExpr::Op::Mul:
    return isFloat ? builder()->CreateFMul(left, right, "multmp")
                   : builder()->CreateMul(left, right, "multtmp");
  case ast::BinaryExpr::Op::Div:
    return isFloat ? builder()->CreateFDiv(left, right, "divtmp")
                   : builder()->CreateSDiv(left, right, "divtmp");
  case ast::BinaryExpr::Op::Mod:
    return builder()->CreateSRem(left, right, "modtmp");
  case ast::BinaryExpr::Op::Lt:
    return isFloat ? builder()->CreateFCmpOLT(left, right, "lttmp")
                   : builder()->CreateICmpSLT(left, right, "lttmp");
  case ast::BinaryExpr::Op::Le:
    return isFloat ? builder()->CreateFCmpOLE(left, right, "letmp")
                   : builder()->CreateICmpSLE(left, right, "letmp");
  case ast::BinaryExpr::Op::Gt:
    return isFloat ? builder()->CreateFCmpOGT(left, right, "gttmp")
                   : builder()->CreateICmpSGT(left, right, "gttmp");
  case ast::BinaryExpr::Op::Ge:
    return isFloat ? builder()->CreateFCmpOGE(left, right, "getmp")
                   : builder()->CreateICmpSGE(left, right, "gettmp");
  case ast::BinaryExpr::Op::Eq:
    return isFloat ? builder()->CreateFCmpOEQ(left, right, "eqtmp")
                   : builder()->CreateICmpEQ(left, right, "eqtmp");
  case ast::BinaryExpr::Op::Ne:
    return isFloat ? builder()->CreateFCmpONE(left, right, "netmp")
                   : builder()->CreateICmpNE(left, right, "netmp");
  case ast::BinaryExpr::Op::LogicAnd:
    return builder()->CreateAnd(left, right, "andtmp");
  case ast::BinaryExpr::Op::LogicOr:
    return builder()->CreateOr(left, right, "ortmp");
  case ast::BinaryExpr::Op::And:
    return builder()->CreateAnd(left, right, "andtmp");
  case ast::BinaryExpr::Op::Or:
    return builder()->CreateOr(left, right, "ortmp");
  case ast::BinaryExpr::Op::Xor:
    return builder()->CreateXor(left, right, "xortmp");
  case ast::BinaryExpr::Op::Shl:
    return builder()->CreateShl(left, right, "shltmp");
  case ast::BinaryExpr::Op::Shr:
    return builder()->CreateAShr(left, right, "shrtmp");
  case ast::BinaryExpr::Op::Assign:
    return builder()->CreateStore(right, leftAddr);
  case ast::BinaryExpr::Op::AddAssign:
    result = isFloat ? builder()->CreateFAdd(left, right, "addtmp")
                     : builder()->CreateAdd(left, right, "addtmp");
    builder()->CreateStore(result, leftAddr);
    return result;
  case ast::BinaryExpr::Op::SubAssign:
    result = isFloat ? builder()->CreateFSub(left, right, "subtmp")
                     : builder()->CreateSub(left, right, "subtmp");
    builder()->CreateStore(result, leftAddr);
    return result;
  case ast::BinaryExpr::Op::MulAssign:
    result = isFloat ? builder()->CreateFMul(left, right, "multmp")
                     : builder()->CreateMul(left, right, "multmp");
    builder()->CreateStore(result, leftAddr);
    return result;
  case ast::BinaryExpr::Op::DivAssign:
    result = isFloat ? builder()->CreateFDiv(left, right, "divtmp")
                     : builder()->CreateSDiv(left, right, "divtmp");
    builder()->CreateStore(result, leftAddr);
    return result;
  case ast::BinaryExpr::Op::ModAssign:
    result = builder()->CreateSRem(left, right, "modtmp");
    builder()->CreateStore(result, leftAddr);
    return result;
  case ast::BinaryExpr::Op::AndAssign:
    result = builder()->CreateAnd(left, right, "andtmp");
    builder()->CreateStore(result, leftAddr);
    return result;
  case ast::BinaryExpr::Op::OrAssign:
    result = builder()->CreateOr(left, right, "ortmp");
    builder()->CreateStore(result, leftAddr);
    return result;
  case ast::BinaryExpr::Op::XorAssign:
    result = builder()->CreateXor(left, right, "xortmp");
    builder()->CreateStore(result, leftAddr);
    return result;
  case ast::BinaryExpr::Op::ShlAssign:
    result = builder()->CreateShl(left, right, "shltmp");
    builder()->CreateStore(result, leftAddr);
    return result;
  case ast::BinaryExpr::Op::ShrAssign:
    result = builder()->CreateAShr(left, right, "shrtmp");
    builder()->CreateStore(result, leftAddr);
    return result;
  default:
    return nullptr;
  }
}

llvm::Value *
LLVMCodeGenerator::generateUnaryExpr(std::unique_ptr<ast::UnaryExpr> unaryExpr,
                                     bool isLValue) {
  llvm::Value *operand = nullptr;

  if (unaryExpr->op == ast::UnaryExpr::Op::AddressOf) {
    operand = getExpressionLValue(std::move(unaryExpr->expr));
  } else {
    operand = generateExpression(std::move(unaryExpr->expr));
  }

  if (!operand) {
    return nullptr;
  }

  switch (unaryExpr->op) {
  case ast::UnaryExpr::Op::Plus:
    return operand;
  case ast::UnaryExpr::Op::Minus:
    if (operand->getType()->isFloatingPointTy()) {
      return builder()->CreateFNeg(operand, "negtmp");
    }
    return builder()->CreateNeg(operand, "negtmp");
  case ast::UnaryExpr::Op::Not:
    return builder()->CreateNot(operand, "nottmp");
  case ast::UnaryExpr::Op::BitNot:
    return builder()->CreateNot(operand, "bwnottmp");
  case ast::UnaryExpr::Op::AddressOf:
    return operand;
  case ast::UnaryExpr::Op::Dereference:
    if (isLValue) {
      return operand;
    }
    return builder()->CreateLoad(llvm::Type::getInt32Ty(context()), operand,
                                 "deref");
  default:
    return nullptr;
  }
}

llvm::Value *LLVMCodeGenerator::generateIdentifierExpr(
    std::unique_ptr<ast::Identifier> identifier, bool isLValue) {
  auto it = namedValues_.find(identifier->name);
  if (it != namedValues_.end()) {
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(it->second)) {
      if (isLValue) {
        return alloca;
      }
      return builder()->CreateLoad(alloca->getAllocatedType(), alloca,
                                   identifier->name);
    }
    return it->second;
  }

  auto funcIt = functions_.find(identifier->name);
  if (funcIt != functions_.end()) {
    return funcIt->second;
  }

  throw std::runtime_error("Unknown identifier: " + identifier->name);
}

llvm::Value *
LLVMCodeGenerator::getExpressionLValue(std::unique_ptr<ast::Expression> expr) {
  switch (expr->getType()) {
  case ast::NodeType::UnaryExpr: {
    auto *unaryExpr = static_cast<ast::UnaryExpr *>(expr.release());
    if (unaryExpr->op == ast::UnaryExpr::Op::Dereference) {
      return generateUnaryExpr(std::unique_ptr<ast::UnaryExpr>(unaryExpr),
                               true);
    }
    throw std::runtime_error("Unary expression is not an lvalue");
  }
  case ast::NodeType::Identifier: {
    auto *ident = static_cast<ast::Identifier *>(expr.release());
    return generateIdentifierExpr(std::unique_ptr<ast::Identifier>(ident),
                                  true);
  }
  case ast::NodeType::SubscriptExpr: {
    auto *subscript = static_cast<ast::SubscriptExpr *>(expr.release());
    llvm::Value *arrayPtr = getExpressionLValue(
        std::unique_ptr<ast::Expression>(subscript->object.release()));
    llvm::Value *index = generateExpression(
        std::unique_ptr<ast::Expression>(subscript->index.release()));

    llvm::Type *arrayType = nullptr;
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr)) {
      arrayType = alloca->getAllocatedType();
    }

    std::vector<llvm::Value *> indices;
    indices.push_back(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0));
    indices.push_back(index);

    return builder()->CreateGEP(arrayType, arrayPtr, indices, "subscripttmp");
  }
  case ast::NodeType::MemberExpr: {
    auto *memberExpr = static_cast<ast::MemberExpr *>(expr.release());
    return generateMemberExpr(std::unique_ptr<ast::MemberExpr>(memberExpr),
                              true);
  }
  default:
    throw std::runtime_error("Expression is not an lvalue");
  }
}

llvm::Value *
LLVMCodeGenerator::generateLiteralExpr(std::unique_ptr<ast::Literal> literal) {
  if (literal->type == ast::Literal::Type::Integer) {
    int64_t value = std::stoll(literal->value);
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                  static_cast<uint32_t>(value));
  } else if (literal->type == ast::Literal::Type::Floating) {
    double value = std::stod(literal->value);
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context()), value);
  } else if (literal->type == ast::Literal::Type::Boolean) {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context()),
                                  literal->value == "true" ? 1 : 0);
  } else if (literal->type == ast::Literal::Type::String) {
    return createStringLiteral(literal->value);
  } else if (literal->type == ast::Literal::Type::Character) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                  literal->value[0]);
  }

  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateCallExpr(std::unique_ptr<ast::CallExpr> callExpr) {
  std::vector<llvm::Value *> args;
  std::string funcName;
  llvm::Value *objectPtr = nullptr;

  // Check if callee is a MemberExpr (method call)
  if (auto *memberExpr =
          dynamic_cast<ast::MemberExpr *>(callExpr->callee.get())) {
    objectPtr = getExpressionLValue(
        std::unique_ptr<ast::Expression>(memberExpr->object.release()));

    // Get struct name
    std::string structName;
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(objectPtr)) {
      if (auto *structType =
              llvm::dyn_cast<llvm::StructType>(alloca->getAllocatedType())) {
        structName = structType->getName().str();
      }
    } else if (auto *gep = llvm::dyn_cast<llvm::GetElementPtrInst>(objectPtr)) {
      if (auto *structType =
              llvm::dyn_cast<llvm::StructType>(gep->getResultElementType())) {
        structName = structType->getName().str();
      }
    }

    if (!structName.empty()) {
      funcName = structName + "_" + memberExpr->member;
      args.push_back(objectPtr);
    }
  } else if (auto *ident =
                 dynamic_cast<ast::Identifier *>(callExpr->callee.get())) {
    funcName = ident->name;
  }

  for (auto &arg : callExpr->args) {
    args.push_back(generateExpression(std::move(arg)));
  }

  if (!funcName.empty() && functions_.find(funcName) != functions_.end()) {
    llvm::Function *func = functions_[funcName];
    return builder()->CreateCall(func, args, "calltmp");
  }

  throw std::runtime_error("Function not found: " + funcName);
}

llvm::Value *LLVMCodeGenerator::generateMemberExpr(
    std::unique_ptr<ast::MemberExpr> memberExpr, bool isLValue) {
  llvm::Value *objectPtr = getExpressionLValue(std::move(memberExpr->object));

  std::string structName;
  unsigned idx = 0;
  bool isLiteralView = false;
  llvm::Type *structType = nullptr;

  if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(objectPtr)) {
    structType = alloca->getAllocatedType();
    if (auto *structTypeNode = llvm::dyn_cast<llvm::StructType>(structType)) {
      structName = structTypeNode->getName().str();
      if (structName == "LiteralView") {
        isLiteralView = true;
        if (memberExpr->member == "ptr") {
          idx = 0;
        } else if (memberExpr->member == "len") {
          idx = 1;
        }
      }
    }
  } else if (auto *gep = llvm::dyn_cast<llvm::GetElementPtrInst>(objectPtr)) {
    structType = gep->getResultElementType();
    if (auto *structTypeNode = llvm::dyn_cast<llvm::StructType>(structType)) {
      structName = structTypeNode->getName().str();
      if (structName == "LiteralView") {
        isLiteralView = true;
        if (memberExpr->member == "ptr") {
          idx = 0;
        } else if (memberExpr->member == "len") {
          idx = 1;
        }
      }
    }
  }

  if (!isLiteralView) {
    auto it = structInfo_.find(structName);
    if (it != structInfo_.end()) {
      auto idxIt = it->second.find(memberExpr->member);
      if (idxIt != it->second.end()) {
        idx = idxIt->second;
      }
    }
  }

  std::vector<llvm::Value *> indices;
  indices.push_back(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0));
  indices.push_back(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), idx));

  llvm::Value *gep =
      builder()->CreateGEP(structType, objectPtr, indices, "membertmp");

  if (isLValue) {
    return gep;
  } else {
    llvm::Type *memberType = nullptr;
    if (structType && structType->isStructTy()) {
      auto *llvmStructType = llvm::cast<llvm::StructType>(structType);
      if (idx < llvmStructType->getNumElements()) {
        memberType = llvmStructType->getElementType(idx);
      }
    }
    if (!memberType) {
      memberType = llvm::Type::getInt32Ty(context());
    }
    return builder()->CreateLoad(memberType, gep, "memberloadtmp");
  }
}

llvm::Value *LLVMCodeGenerator::generateSubscriptExpr(
    std::unique_ptr<ast::SubscriptExpr> subscriptExpr) {
  llvm::Value *arrayPtr = getExpressionLValue(std::move(subscriptExpr->object));
  llvm::Value *index = generateExpression(std::move(subscriptExpr->index));

  std::vector<llvm::Value *> indices;
  indices.push_back(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0));
  indices.push_back(index);

  return builder()->CreateGEP(nullptr, arrayPtr, indices, "subscripttmp");
}

llvm::Value *
LLVMCodeGenerator::generateNewExpr(std::unique_ptr<ast::NewExpr> newExpr) {
  llvm::Type *type = nullptr;
  if (auto *typeNode = dynamic_cast<ast::Type *>(newExpr->type.get())) {
    type = generateType(typeNode);
  }

  if (!type) {
    throw std::runtime_error("Cannot generate type for new expression");
  }

  llvm::Type *i8Type = llvm::Type::getInt8Ty(context());
  llvm::Type *sizeType = llvm::Type::getInt64Ty(context());

  llvm::FunctionType *mallocType =
      llvm::FunctionType::get(i8Type->getPointerTo(), {sizeType}, false);
  llvm::FunctionCallee mallocFunc =
      module()->getOrInsertFunction("malloc", mallocType);

  llvm::DataLayout dataLayout(module());
  uint64_t size = dataLayout.getTypeAllocSize(type);

  llvm::Value *sizeVal = llvm::ConstantInt::get(sizeType, size);
  llvm::Value *allocPtr =
      builder()->CreateCall(mallocFunc, {sizeVal}, "newtmp");

  llvm::Value *typedPtr =
      builder()->CreateBitCast(allocPtr, type->getPointerTo(), "typedtmp");

  std::string typeName;
  if (auto *namedType = dynamic_cast<ast::NamedType *>(newExpr->type.get())) {
    typeName = namedType->name;
  }

  if (!typeName.empty() && structTypes_.find(typeName) != structTypes_.end()) {
    std::string ctorName = typeName + "_" + typeName;
    if (functions_.find(ctorName) != functions_.end()) {
      llvm::Function *ctor = functions_[ctorName];

      std::vector<llvm::Value *> args;
      args.push_back(typedPtr);
      for (auto &arg : newExpr->args) {
        args.push_back(generateExpression(std::move(arg)));
      }

      builder()->CreateCall(ctor, args);
    }
  }

  return typedPtr;
}

llvm::Value *LLVMCodeGenerator::generateDeleteExpr(
    std::unique_ptr<ast::DeleteExpr> deleteExpr) {
  llvm::Value *ptr = generateExpression(std::move(deleteExpr->expr));

  llvm::Type *i8Type = llvm::Type::getInt8Ty(context());
  llvm::FunctionType *freeType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(context()), {i8Type->getPointerTo()}, false);
  llvm::FunctionCallee freeFunc =
      module()->getOrInsertFunction("free", freeType);

  llvm::Value *i8Ptr =
      builder()->CreateBitCast(ptr, i8Type->getPointerTo(), "freetmp");
  builder()->CreateCall(freeFunc, {i8Ptr});

  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateThisExpr(std::unique_ptr<ast::ThisExpr> thisExpr) {
  return namedValues_["self"];
}

llvm::Value *
LLVMCodeGenerator::generateSelfExpr(std::unique_ptr<ast::SelfExpr> selfExpr) {
  return namedValues_["self"];
}

llvm::Value *LLVMCodeGenerator::generateSuperExpr(
    std::unique_ptr<ast::SuperExpr> superExpr) {
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateExpansionExpr(
    std::unique_ptr<ast::ExpansionExpr> expansionExpr) {
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateLambdaExpr(
    std::unique_ptr<ast::LambdaExpr> lambdaExpr) {
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateArrayInitExpr(
    std::unique_ptr<ast::ArrayInitExpr> arrayInitExpr) {
  std::vector<llvm::Constant *> constants;
  for (auto &elem : arrayInitExpr->elements) {
    if (auto *literal = dynamic_cast<ast::Literal *>(elem.get())) {
      llvm::Value *val = generateLiteralExpr(std::unique_ptr<ast::Literal>(
          static_cast<ast::Literal *>(elem.release())));
      if (auto *constVal = llvm::dyn_cast<llvm::Constant>(val)) {
        constants.push_back(constVal);
      } else {
        throw std::runtime_error("Array initializer elements must be literals");
      }
    } else {
      throw std::runtime_error("Array initializer elements must be literals");
    }
  }

  if (constants.empty()) {
    return nullptr;
  }

  llvm::Type *elemType = constants[0]->getType();
  llvm::ArrayType *arrayType = llvm::ArrayType::get(elemType, constants.size());
  return llvm::ConstantArray::get(arrayType, constants);
}

llvm::Value *LLVMCodeGenerator::generateStructInitExpr(
    std::unique_ptr<ast::StructInitExpr> structInitExpr) {
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateTupleExpr(
    std::unique_ptr<ast::TupleExpr> tupleExpr) {
  std::vector<llvm::Type *> elementTypes;
  std::vector<llvm::Value *> elementValues;

  // 生成每个元素的值并收集类型
  for (auto &elem : tupleExpr->elements) {
    llvm::Value *val = generateExpression(std::move(elem));
    elementValues.push_back(val);
    elementTypes.push_back(val->getType());
  }

  // 创建一个 StructType 来表示元组
  llvm::StructType *tupleType = llvm::StructType::get(context(), elementTypes);

  // 创建一个 UndefValue 作为初始值
  llvm::Value *tupleValue = llvm::UndefValue::get(tupleType);

  // 将每个元素插入到 tupleValue 中
  for (size_t i = 0; i < elementValues.size(); ++i) {
    tupleValue = builder()->CreateInsertValue(
        tupleValue, elementValues[i], i, "tuple_elem_" + std::to_string(i));
  }

  return tupleValue;
}

llvm::Type *LLVMCodeGenerator::generateType(const ast::Type *type) {
  if (!type) {
    return llvm::Type::getVoidTy(context());
  }

  if (auto *primitiveType = dynamic_cast<const ast::PrimitiveType *>(type)) {
    return generatePrimitiveType(primitiveType);
  } else if (auto *pointerType = dynamic_cast<const ast::PointerType *>(type)) {
    auto baseType = generateType(pointerType->baseType.get());
    return getPointerType(baseType, pointerType->isNullable);
  } else if (auto *arrayType = dynamic_cast<const ast::ArrayType *>(type)) {
    auto elementType = generateType(arrayType->baseType.get());
    if (arrayType->size) {
      if (auto *intLiteral =
              dynamic_cast<const ast::Literal *>(arrayType->size.get())) {
        int64_t size = std::stoll(intLiteral->value);
        return getArrayType(elementType, size);
      }
    }
    return getArrayType(elementType, 0);
  } else if (auto *rectArrayType =
                 dynamic_cast<const ast::RectangularArrayType *>(type)) {
    auto elementType = generateType(rectArrayType->baseType.get());
    llvm::Type *resultType = elementType;
    for (auto &sizeExpr : rectArrayType->sizes) {
      int64_t size = 0;
      if (sizeExpr) {
        if (auto *intLiteral =
                dynamic_cast<const ast::Literal *>(sizeExpr.get())) {
          size = std::stoll(intLiteral->value);
        }
      }
      resultType = getArrayType(resultType, size);
    }
    return resultType;
  } else if (auto *sliceType = dynamic_cast<const ast::SliceType *>(type)) {
    auto elementType = generateType(sliceType->baseType.get());
    return getSliceType(elementType);
  } else if (auto *rectSliceType =
                 dynamic_cast<const ast::RectangularSliceType *>(type)) {
    auto elementType = generateType(rectSliceType->baseType.get());
    return getSliceType(elementType);
  } else if (auto *namedType = dynamic_cast<const ast::NamedType *>(type)) {
    auto it = structTypes_.find(namedType->name);
    if (it != structTypes_.end()) {
      return it->second;
    }
  } else if (auto *funcType = dynamic_cast<const ast::FunctionType *>(type)) {
    // 生成函数类型
    std::vector<llvm::Type *> paramTypes;
    for (const auto &paramType : funcType->parameterTypes) {
      paramTypes.push_back(generateType(paramType.get()));
    }
    auto returnType = generateType(funcType->returnType.get());
    // 创建 LLVM 函数类型
    auto llvmFuncType = llvm::FunctionType::get(returnType, paramTypes, false);
    // 返回函数指针类型（不透明指针）
    return llvm::PointerType::get(llvmFuncType, 0);
  } else if (auto *tupleType = dynamic_cast<const ast::TupleType *>(type)) {
    std::vector<llvm::Type *> elementTypes;
    for (const auto &elementType : tupleType->elementTypes) {
      elementTypes.push_back(generateType(elementType.get()));
    }
    if (elementTypes.empty()) {
      elementTypes.push_back(llvm::Type::getInt8Ty(context()));
    }
    return llvm::StructType::create(context(), elementTypes);
  }

  return getPrimitiveType(type->toString());
}

llvm::Type *LLVMCodeGenerator::generatePrimitiveType(
    const ast::PrimitiveType *primitiveType) {
  switch (primitiveType->kind) {
  case ast::PrimitiveType::Kind::Void:
    return llvm::Type::getVoidTy(context());
  case ast::PrimitiveType::Kind::Bool:
    return llvm::Type::getInt1Ty(context());
  case ast::PrimitiveType::Kind::Byte:
  case ast::PrimitiveType::Kind::SByte:
    return llvm::Type::getInt8Ty(context());
  case ast::PrimitiveType::Kind::Char:
    return llvm::Type::getInt32Ty(context());
  case ast::PrimitiveType::Kind::Short:
  case ast::PrimitiveType::Kind::UShort:
    return llvm::Type::getInt16Ty(context());
  case ast::PrimitiveType::Kind::Int:
  case ast::PrimitiveType::Kind::UInt:
    return llvm::Type::getInt32Ty(context());
  case ast::PrimitiveType::Kind::Long:
  case ast::PrimitiveType::Kind::ULong:
    return llvm::Type::getInt64Ty(context());
  case ast::PrimitiveType::Kind::Float:
    return llvm::Type::getFloatTy(context());
  case ast::PrimitiveType::Kind::Double:
    return llvm::Type::getDoubleTy(context());
  case ast::PrimitiveType::Kind::Fp16:
    return llvm::Type::getHalfTy(context());
  case ast::PrimitiveType::Kind::Bf16:
    return llvm::Type::getBFloatTy(context());
  default:
    return llvm::Type::getInt32Ty(context());
  }
}

llvm::Type *LLVMCodeGenerator::getPrimitiveType(const std::string &typeName) {
  if (typeName == "void") {
    return llvm::Type::getVoidTy(context());
  } else if (typeName == "bool") {
    return llvm::Type::getInt1Ty(context());
  } else if (typeName == "byte" || typeName == "sbyte") {
    return llvm::Type::getInt8Ty(context());
  } else if (typeName == "char") {
    return llvm::Type::getInt32Ty(context());
  } else if (typeName == "short" || typeName == "ushort") {
    return llvm::Type::getInt16Ty(context());
  } else if (typeName == "int" || typeName == "uint") {
    return llvm::Type::getInt32Ty(context());
  } else if (typeName == "long" || typeName == "ulong") {
    return llvm::Type::getInt64Ty(context());
  } else if (typeName == "float") {
    return llvm::Type::getFloatTy(context());
  } else if (typeName == "double") {
    return llvm::Type::getDoubleTy(context());
  }

  return nullptr;
}

llvm::Type *LLVMCodeGenerator::getPointerType(llvm::Type *baseType,
                                              bool nullable) {
  return llvm::PointerType::get(context(), 0);
}

llvm::Type *LLVMCodeGenerator::getArrayType(llvm::Type *elementType,
                                            int64_t size) {
  return llvm::ArrayType::get(elementType, size);
}

llvm::Type *LLVMCodeGenerator::getSliceType(llvm::Type *elementType) {
  static std::unordered_map<llvm::Type *, llvm::StructType *> sliceTypes;
  auto it = sliceTypes.find(elementType);
  if (it != sliceTypes.end()) {
    return it->second;
  }

  std::vector<llvm::Type *> elements;
  // ptr: elementType*
  elements.push_back(llvm::PointerType::get(elementType, 0));
  // len: long (int64)
  elements.push_back(llvm::Type::getInt64Ty(context()));
  auto sliceType = llvm::StructType::create(context(), elements, "Slice");
  sliceTypes[elementType] = sliceType;
  return sliceType;
}

llvm::Type *LLVMCodeGenerator::getLiteralViewType() {
  static llvm::StructType *type = nullptr;
  if (!type) {
    std::vector<llvm::Type *> elements;
    // ptr: byte!^ (int8*)
    elements.push_back(
        llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0));
    // len: long (int64)
    elements.push_back(llvm::Type::getInt64Ty(context()));
    type = llvm::StructType::create(context(), elements, "LiteralView");
  }
  return type;
}

llvm::Value *LLVMCodeGenerator::createStringLiteral(const std::string &str) {
  llvm::Constant *strConstant =
      llvm::ConstantDataArray::getString(context(), str, true);
  llvm::GlobalVariable *globalStr = new llvm::GlobalVariable(
      *module(), strConstant->getType(), true,
      llvm::GlobalValue::PrivateLinkage, strConstant, ".str");
  globalStr->setAlignment(llvm::Align(1));

  // 获取字符串指针（指向第一个字符）
  std::vector<llvm::Value *> indices;
  indices.push_back(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0));
  indices.push_back(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0));
  llvm::Value *strPtr = builder()->CreateGEP(strConstant->getType(), globalStr,
                                             indices, "strptr");

  // 创建 LiteralView 结构体
  llvm::Type *literalViewType = getLiteralViewType();
  llvm::Value *literalView = llvm::UndefValue::get(literalViewType);

  // 设置 ptr 字段
  literalView =
      builder()->CreateInsertValue(literalView, strPtr, 0, "literalview_ptr");

  // 设置 len 字段（字符串长度，不包含 null 终止符）
  llvm::Constant *lenConstant =
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(context()), str.size());
  literalView = builder()->CreateInsertValue(literalView, lenConstant, 1,
                                             "literalview_len");

  return literalView;
}

llvm::Value *LLVMCodeGenerator::generateGetterDecl(
    std::unique_ptr<ast::GetterDecl> getterDecl) {
  llvm::Type *returnType = nullptr;
  if (getterDecl->returnType) {
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(getterDecl->returnType.get())) {
      returnType = generateType(typeNode);
    }
  }

  if (!returnType) {
    returnType = llvm::Type::getVoidTy(context());
  }

  std::vector<llvm::Type *> paramTypes;
  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, false);
  std::string funcName = "get_" + getterDecl->name;
  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, funcName, module());

  if (getterDecl->body) {
    llvm::BasicBlock *entryBlock =
        llvm::BasicBlock::Create(context(), "entry", function);
    builder()->SetInsertPoint(entryBlock);

    auto prevFunction = currentFunction_;
    currentFunction_ = function;
    namedValues_.clear();

    if (auto *stmt = dynamic_cast<ast::Statement *>(getterDecl->body.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(getterDecl->body.release())));

      llvm::BasicBlock *lastBlock = &function->back();
      if (!lastBlock->getTerminator()) {
        if (returnType->isVoidTy()) {
          builder()->SetInsertPoint(lastBlock);
          builder()->CreateRetVoid();
        } else {
          builder()->SetInsertPoint(lastBlock);
          builder()->CreateRet(llvm::Constant::getNullValue(returnType));
        }
      }
    }

    currentFunction_ = prevFunction;
  }

  functions_[funcName] = function;
  return function;
}

llvm::Value *LLVMCodeGenerator::generateSetterDecl(
    std::unique_ptr<ast::SetterDecl> setterDecl) {
  llvm::Type *paramType = nullptr;
  if (setterDecl->param && setterDecl->param->type) {
    paramType = generateType(setterDecl->param->type.get());
  }

  if (!paramType) {
    paramType = llvm::Type::getInt32Ty(context());
  }

  std::vector<llvm::Type *> paramTypes;
  paramTypes.push_back(paramType);

  llvm::FunctionType *funcType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(context()), paramTypes, false);
  std::string funcName = "set_" + setterDecl->name;
  llvm::Function *function = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, funcName, module());

  if (setterDecl->body) {
    llvm::BasicBlock *entryBlock =
        llvm::BasicBlock::Create(context(), "entry", function);
    builder()->SetInsertPoint(entryBlock);

    auto prevFunction = currentFunction_;
    currentFunction_ = function;
    namedValues_.clear();

    size_t paramIndex = 0;
    auto argIt = function->args().begin();

    llvm::Value *arg = &(*argIt);
    arg->setName(setterDecl->param->name);

    llvm::AllocaInst *alloca = builder()->CreateAlloca(arg->getType(), nullptr,
                                                       setterDecl->param->name);
    builder()->CreateStore(arg, alloca);
    namedValues_[setterDecl->param->name] = alloca;

    if (auto *stmt = dynamic_cast<ast::Statement *>(setterDecl->body.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(setterDecl->body.release())));

      llvm::BasicBlock *lastBlock = &function->back();
      if (!lastBlock->getTerminator()) {
        builder()->SetInsertPoint(lastBlock);
        builder()->CreateRetVoid();
      }
    }

    currentFunction_ = prevFunction;
  }

  functions_[funcName] = function;
  return function;
}

llvm::Value *LLVMCodeGenerator::generateExtensionDecl(
    std::unique_ptr<ast::ExtensionDecl> extensionDecl) {
  for (auto &member : extensionDecl->members) {
    if (auto *decl = dynamic_cast<ast::Declaration *>(member.get())) {
      generateDeclaration(std::unique_ptr<ast::Declaration>(
          static_cast<ast::Declaration *>(member.release())));
    }
  }
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateExternDecl(
    std::unique_ptr<ast::ExternDecl> externDecl) {
  for (auto &declaration : externDecl->declarations) {
    generateDeclaration(std::move(declaration));
  }
  return nullptr;
}

} // namespace llvm_codegen
} // namespace c_hat
