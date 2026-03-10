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
  // 先添加内置类型 LiteralView 到 structTypes_
  structTypes_["LiteralView"] =
      static_cast<llvm::StructType *>(getLiteralViewType());
  // 添加 LiteralView 的成员信息
  std::unordered_map<std::string, unsigned> literalViewInfo;
  literalViewInfo["ptr"] = 0;
  literalViewInfo["len"] = 1;
  structInfo_["LiteralView"] = literalViewInfo;

  // 先处理类型声明（不处理 VariableDecl）
  for (auto &decl : program->declarations) {
    ast::NodeType type = decl->getType();
    if (type == ast::NodeType::StructDecl || type == ast::NodeType::ClassDecl ||
        type == ast::NodeType::TypeAliasDecl ||
        type == ast::NodeType::ExtensionDecl) {
      generateDeclaration(std::move(decl));
    }
  }

  // 收集函数声明（创建原型）
  std::vector<std::unique_ptr<ast::FunctionDecl>> funcDecls;
  for (auto &decl : program->declarations) {
    if (decl && decl->getType() == ast::NodeType::FunctionDecl) {
      auto funcDecl = std::unique_ptr<ast::FunctionDecl>(
          static_cast<ast::FunctionDecl *>(decl.release()));
      std::cerr << "Debug: Creating prototype for function: " << funcDecl->name
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
  optimizeLoops();

  // 执行函数内联优化
  optimizeFunctionInlining();
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
  case ast::NodeType::NamespaceDecl:
    return generateNamespaceDecl(std::unique_ptr<ast::NamespaceDecl>(
        static_cast<ast::NamespaceDecl *>(decl.release())));
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
    } else if (auto *callExpr =
                   dynamic_cast<ast::CallExpr *>(varDecl->initializer.get())) {
      // 对于函数调用表达式，尝试从函数名获取函数类型
      if (auto *identifier =
              dynamic_cast<ast::Identifier *>(callExpr->callee.get())) {
        const std::string &funcName = identifier->name;
        // 检查是否是已定义的函数
        auto it = functions_.find(funcName);
        if (it != functions_.end()) {
          llvm::Function *func = it->second;
          varType = func->getReturnType();
        } else {
          // 如果函数未定义，暂时使用 int 作为默认类型
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

  if (!varType) {
    // 添加调试信息

    std::cerr << "Debug: varDecl->type = "
              << (varDecl->type ? "not null" : "null") << std::endl;
    std::cerr << "Debug: varDecl->initializer = "
              << (varDecl->initializer ? "not null" : "null") << std::endl;
    if (varDecl->initializer) {
      std::cerr << "Debug: initializer type = "
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

  llvm::AllocaInst *alloca =
      builder()->CreateAlloca(varType, nullptr, varDecl->name);

  // 检查是否是切片类型且初始化值是数组初始化表达式，进行临时数组生命周期延长
  if (isSliceType && sliceElementType && varDecl->initializer) {
    if (auto *arrayInit =
            dynamic_cast<ast::ArrayInitExpr *>(varDecl->initializer.get())) {
      // 计算数组大小
      size_t arraySize = arrayInit->elements.size();

      // 创建临时数组
      llvm::ArrayType *arrayType =
          llvm::ArrayType::get(sliceElementType, arraySize);
      llvm::AllocaInst *tempArrayAlloca =
          builder()->CreateAlloca(arrayType, nullptr, "temp_array");

      // 填充数组元素
      for (size_t i = 0; i < arraySize; ++i) {
        auto &element = arrayInit->elements[i];
        llvm::Value *elementValue = generateExpression(std::move(element));

        // 计算数组元素的地址
        llvm::Value *indices[] = {
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), i)};
        llvm::Value *elementPtr = builder()->CreateGEP(
            arrayType, tempArrayAlloca, indices, "element_ptr");

        // 存储元素值
        builder()->CreateStore(elementValue, elementPtr);
      }

      // 计算临时数组的指针和长度
      llvm::Value *arrayPtr = builder()->CreateGEP(
          arrayType, tempArrayAlloca,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
           llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0)});

      // 假设切片类型是一个结构体，包含 ptr 和 len 两个成员
      // 这里需要根据实际的切片类型定义来调整
      llvm::Value *sliceValue = builder()->CreateInsertValue(
          llvm::UndefValue::get(varType), arrayPtr, 0);
      sliceValue = builder()->CreateInsertValue(
          sliceValue,
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(context()), arraySize),
          1);

      // 存储切片值到变量
      builder()->CreateStore(sliceValue, alloca);
    } else {
      // 对于其他类型的初始化器，正常处理
      if (varDecl->initializer) {
        llvm::Value *initValue =
            generateExpression(std::move(varDecl->initializer));
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

  // 假设元组是一个结构体，我们需要提取各个元素
  // 这里需要根据实际的元组类型定义来调整
  for (size_t i = 0; i < tupleDecl->names.size(); ++i) {
    const std::string &name = tupleDecl->names[i];

    // 提取元组元素
    llvm::Value *elementValue = builder()->CreateExtractValue(initValue, i);

    // 为元素创建局部变量
    llvm::AllocaInst *alloca =
        builder()->CreateAlloca(elementValue->getType(), nullptr, name);
    builder()->CreateStore(elementValue, alloca);

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

  // 为了支持重载，生成带参数类型的函数名
  std::string uniqueFuncName = funcDecl->name;
  for (auto &paramType : paramTypes) {
    std::string typeName = paramType->getStructName().str();
    if (typeName.empty()) {
      if (paramType->isFloatingPointTy()) {
        typeName = "double";
      } else if (paramType->isIntegerTy()) {
        typeName = "int";
      } else {
        typeName = paramType->getTypeID() == llvm::Type::PointerTyID
                       ? "ptr"
                       : "unknown";
      }
    }
    uniqueFuncName += "_" + typeName;
  }

  // 检查函数是否已经存在
  llvm::Function *function = module()->getFunction(uniqueFuncName);
  if (function) {
    // 函数已经存在，如果当前是函数声明（没有函数体），直接返回
    if (!funcDecl->body) {
      return function;
    }
    // 如果函数已经存在且有函数体，报错
    if (!function->empty()) {
      throw std::runtime_error("Function already defined: " + uniqueFuncName);
    }
    // 函数已经存在但没有函数体，我们需要添加函数体
    // 首先清除personality routine（如果有的话）
    if (function->getPersonalityFn()) {
      function->setPersonalityFn(nullptr);
    }
  } else {
    // 创建新函数
    function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                      uniqueFuncName, module());
  }

  if (funcDecl->body) {
    llvm::BasicBlock *entryBlock =
        llvm::BasicBlock::Create(context(), "entry", function);
    builder()->SetInsertPoint(entryBlock);

    auto prevFunction = currentFunction_;
    currentFunction_ = function;

    namedValues_.clear();
    deferExpressions_.clear();
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

        ++argIt;
        ++paramIndex;
      }
    }

    if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(funcDecl->body.release())));

      llvm::BasicBlock *lastBlock = &function->back();
      if (!lastBlock->getTerminator()) {
        // 为已初始化的 late 变量调用析构函数
        for (const auto &[varName, info] : lateVariables_) {
          if (info.isInitialized) {
            // 这里需要添加析构函数调用的代码
            // 目前暂时跳过，因为析构函数的实现还未完成
          }
        }

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

void LLVMCodeGenerator::createFunctionPrototype(ast::FunctionDecl *funcDecl) {

  std::cerr << "Debug: createFunctionPrototype - function name: "
            << funcDecl->name << std::endl;

  llvm::Type *returnType = nullptr;
  if (funcDecl->returnType) {
    std::cerr << "Debug: createFunctionPrototype - has return type"
              << std::endl;
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(funcDecl->returnType.get())) {
      std::cerr << "Debug: createFunctionPrototype - generating return type"
                << std::endl;
      returnType = generateType(typeNode);
      std::cerr << "Debug: createFunctionPrototype - return type generated"
                << std::endl;
    }
  }

  if (!returnType) {
    returnType = llvm::Type::getVoidTy(context());
    std::cerr << "Debug: createFunctionPrototype - using void return type"
              << std::endl;
  }
  std::cerr << "Debug: createFunctionPrototype - return type: "
            << returnType->getTypeID() << std::endl;

  std::vector<llvm::Type *> paramTypes;
  std::cerr << "Debug: createFunctionPrototype - processing parameters"
            << std::endl;
  for (auto &paramNode : funcDecl->params) {
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      std::cerr << "Debug: createFunctionPrototype - processing parameter: "
                << param->name << std::endl;
      llvm::Type *paramType = generateType(param->type.get());
      if (!paramType) {
        throw std::runtime_error("Unknown parameter type for function: " +
                                 funcDecl->name);
      }
      paramTypes.push_back(paramType);
      std::cerr << "Debug: createFunctionPrototype - parameter type: "
                << paramType->getTypeID() << std::endl;
    }
  }
  std::cerr << "Debug: createFunctionPrototype - parameters processed"
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
      if (auto *structType = llvm::dyn_cast<llvm::StructType>(paramTypes[i])) {
        typeName = structType->getName().str();
      }
      if (typeName.empty()) {
        if (paramTypes[i]->isFloatingPointTy()) {
          typeName = "double";
        } else if (paramTypes[i]->isIntegerTy()) {
          typeName = "int";
        } else {
          typeName = paramTypes[i]->getTypeID() == llvm::Type::PointerTyID
                         ? "ptr"
                         : "unknown";
        }
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

  std::cerr << "Debug: createFunctionPrototype - adding function to map"
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
      if (auto *structType = llvm::dyn_cast<llvm::StructType>(paramTypes[i])) {
        typeName = structType->getName().str();
      }
      if (typeName.empty()) {
        if (paramTypes[i]->isFloatingPointTy()) {
          typeName = "double";
        } else if (paramTypes[i]->isIntegerTy()) {
          typeName = "int";
        } else {
          typeName = paramTypes[i]->getTypeID() == llvm::Type::PointerTyID
                         ? "ptr"
                         : "unknown";
        }
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
    deferExpressions_.clear();
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

    if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(funcDecl->body.release())));

      // 检查函数是否还有基本块
      if (!function->empty()) {
        llvm::BasicBlock *lastBlock = &function->back();
        std::cerr << "Debug: lastBlock = " << lastBlock->getName().str()
                  << std::endl;
        std::cerr << "Debug: lastBlock has terminator = "
                  << (lastBlock->getTerminator() != nullptr) << std::endl;
        if (!lastBlock->getTerminator()) {
          std::cerr << "Debug: Adding default return statement" << std::endl;

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
          while (!deferExpressions_.empty()) {
            auto deferExpr = std::move(deferExpressions_.back());
            deferExpressions_.pop_back();
            generateExpression(std::move(deferExpr));
          }

          if (returnType->isVoidTy()) {
            builder()->CreateRetVoid();
          } else {
            builder()->CreateRet(llvm::Constant::getNullValue(returnType));
          }
        }
      }
    }

    // 清理 catch 块
    catchStmts_.clear();
    currentFunctionHasTry_ = false;

    std::cerr << "Debug: Before setting currentFunction_ back" << std::endl;
    currentFunction_ = prevFunction;
    std::cerr << "Debug: Before clearing deferExpressions_" << std::endl;
    deferExpressions_.clear();
  }

  std::cerr << "Debug: Before returning function" << std::endl;

  // 重置标记
  currentFunctionHasTry_ = false;

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
    deferExpressions_.clear();
    lateVariables_.clear();

    // 处理 this 指针
    auto argIt = function->args().begin();
    llvm::Value *thisArg = &(*argIt++);
    thisArg->setName("this");

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
      }
    }

    // 处理函数体
    if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(funcDecl->body.release())));

      llvm::BasicBlock *lastBlock = &function->back();
      if (!lastBlock->getTerminator()) {
        // 为已初始化的 late 变量调用析构函数
        for (const auto &[varName, info] : lateVariables_) {
          if (info.isInitialized) {
            // 这里需要添加析构函数调用的代码
            // 目前暂时跳过，因为析构函数的实现还未完成
          }
        }

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

  functions_[mangledName] = function;
  return function;
}

// 辅助函数：获取 LiteralView 类型
llvm::Type *LLVMCodeGenerator::getLiteralViewType() {
  if (literalViewType_) {
    return literalViewType_;
  }

  // 创建 LiteralView 类型：包含指针和长度
  std::vector<llvm::Type *> fields;
  fields.push_back(
      llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0)); // ptr
  fields.push_back(llvm::Type::getInt64Ty(context()));              // len
  literalViewType_ = llvm::StructType::create(context(), fields, "LiteralView");

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
  case ast::NodeType::ThisExpr:
    return generateThisExpr(std::unique_ptr<ast::ThisExpr>(
        static_cast<ast::ThisExpr *>(expr.release())));
  case ast::NodeType::SelfExpr:
    return generateSelfExpr(std::unique_ptr<ast::SelfExpr>(
        static_cast<ast::SelfExpr *>(expr.release())));
  case ast::NodeType::SuperExpr:
    return generateSuperExpr(std::unique_ptr<ast::SuperExpr>(
        static_cast<ast::SuperExpr *>(expr.release())));
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
        builder()->CreateCall(constructor, {ptr});
      }
    }
  }

  return ptr;
}

// 生成 delete 表达式
llvm::Value *LLVMCodeGenerator::generateDeleteExpr(
    std::unique_ptr<ast::DeleteExpr> deleteExpr) {
  // 生成表达式
  llvm::Value *exprValue = generateExpression(std::move(deleteExpr->expr));
  if (!exprValue) {
    return nullptr;
  }

  // 如果是类类型，调用析构函数
  // 注意：在新版本的 LLVM 中，指针类型不再包含元素类型信息
  // 需要从其他上下文中获取元素类型，暂时跳过析构函数调用
  // TODO: 实现析构函数调用逻辑

  // 获取 free 函数
  llvm::FunctionType *freeType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(context()),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0)}, false);
  llvm::FunctionCallee freeFunc =
      module()->getOrInsertFunction("free", freeType);

  // 类型转换为 void*
  llvm::Value *voidPtr = builder()->CreatePointerCast(
      exprValue, llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0),
      "void.ptr");

  // 调用 free
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
    // 生成切片类型
    std::string sliceTypeName =
        "Slice_" + getTypeName(sliceType->baseType.get());
    auto sliceIt = sliceTypes_.find(sliceTypeName);
    if (sliceIt != sliceTypes_.end()) {
      llvmType = sliceIt->second;
    } else {
      // 生成元素类型
      llvm::Type *elementType = generateType(sliceType->baseType.get());
      if (!elementType) {
        elementType = llvm::Type::getInt32Ty(context());
      }

      // 创建切片类型的结构体
      std::vector<llvm::Type *> fields;
      fields.push_back(llvm::PointerType::get(elementType, 0)); // ptr
      fields.push_back(llvm::Type::getInt32Ty(context()));      // len
      llvm::StructType *sliceStructType =
          llvm::StructType::create(context(), fields, sliceTypeName);

      // 存储到 sliceTypes_ 映射中
      sliceTypes_[sliceTypeName] = sliceStructType;
      llvmType = sliceStructType;
    }
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
    // 创建全局字符串常量
    llvm::Constant *strConstant =
        llvm::ConstantDataArray::getString(context(), literal->value, true);
    llvm::GlobalVariable *globalStr = new llvm::GlobalVariable(
        *module(), strConstant->getType(), true,
        llvm::GlobalValue::PrivateLinkage, strConstant, ".str");
    // 返回指向字符串的指针
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
  // 查找变量
  auto it = namedValues_.find(ident->name);
  if (it != namedValues_.end()) {
    // TODO: 需要知道变量的实际类型来加载
    // 暂时返回存储的指针
    return it->second;
  }
  error("Unknown variable: " + ident->name);
  return nullptr;
}

// 生成二元表达式
llvm::Value *LLVMCodeGenerator::generateBinaryExpr(
    std::unique_ptr<ast::BinaryExpr> binaryExpr) {
  llvm::Value *lhs = generateExpression(std::move(binaryExpr->left));
  llvm::Value *rhs = generateExpression(std::move(binaryExpr->right));

  if (!lhs || !rhs) {
    return nullptr;
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
  llvm::Value *operand = generateExpression(std::move(unaryExpr->expr));
  if (!operand) {
    return nullptr;
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

  // 生成参数
  std::vector<llvm::Value *> args;
  for (auto &arg : callExpr->args) {
    args.push_back(generateExpression(std::move(arg)));
  }

  // 查找函数
  auto it = functions_.find(funcName);
  if (it != functions_.end()) {
    return builder()->CreateCall(it->second, args, "calltmp");
  }

  // 如果找不到函数，尝试使用 extern 函数
  llvm::Function *func = module()->getFunction(funcName);
  if (func) {
    return builder()->CreateCall(func, args, "calltmp");
  }

  error("Unknown function: " + funcName);
  return nullptr;
}

// 生成成员表达式
llvm::Value *LLVMCodeGenerator::generateMemberExpr(
    std::unique_ptr<ast::MemberExpr> memberExpr) {
  llvm::Value *object = generateExpression(std::move(memberExpr->object));
  if (!object) {
    return nullptr;
  }

  // TODO: 实现成员访问表达式生成
  warning("Member expression not fully implemented");
  return nullptr;
}

// 生成下标表达式
llvm::Value *LLVMCodeGenerator::generateSubscriptExpr(
    std::unique_ptr<ast::SubscriptExpr> subscriptExpr, bool isLValue) {
  llvm::Value *object = generateExpression(std::move(subscriptExpr->object));
  llvm::Value *index = generateExpression(std::move(subscriptExpr->index));

  if (!object || !index) {
    return nullptr;
  }

  // TODO: 实现下标表达式生成
  warning("Subscript expression not fully implemented");
  return nullptr;
}

// 生成 this 表达式
llvm::Value *
LLVMCodeGenerator::generateThisExpr(std::unique_ptr<ast::ThisExpr> thisExpr) {
  auto it = namedValues_.find("this");
  if (it != namedValues_.end()) {
    return it->second;
  }
  error("'this' not found in current context");
  return nullptr;
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

// 生成 super 表达式
llvm::Value *LLVMCodeGenerator::generateSuperExpr(
    std::unique_ptr<ast::SuperExpr> superExpr) {
  auto it = namedValues_.find("super");
  if (it != namedValues_.end()) {
    return it->second;
  }
  error("'super' not found in current context");
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

  // 创建数组类型
  llvm::Type *elementType = llvm::Type::getInt32Ty(context());
  llvm::ArrayType *arrayType = llvm::ArrayType::get(elementType, size);

  // 分配数组
  llvm::AllocaInst *alloca =
      builder()->CreateAlloca(arrayType, nullptr, "array");

  // 初始化元素
  for (size_t i = 0; i < size; ++i) {
    llvm::Value *element =
        generateExpression(std::move(arrayInitExpr->elements[i]));
    llvm::Value *indices[] = {
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), i)};
    llvm::Value *ptr =
        builder()->CreateGEP(arrayType, alloca, indices, "element_ptr");
    builder()->CreateStore(element, ptr);
  }

  return alloca;
}

// 生成结构体初始化表达式
llvm::Value *LLVMCodeGenerator::generateStructInitExpr(
    std::unique_ptr<ast::StructInitExpr> structInitExpr) {
  // TODO: 实现结构体初始化表达式生成
  warning("Struct initialization expression not fully implemented");
  return nullptr;
}

// 生成元组表达式
llvm::Value *LLVMCodeGenerator::generateTupleExpr(
    std::unique_ptr<ast::TupleExpr> tupleExpr) {
  // 创建元组类型
  std::vector<llvm::Type *> elementTypes;
  for (auto &element : tupleExpr->elements) {
    llvm::Value *val = generateExpression(std::move(element));
    if (val) {
      elementTypes.push_back(val->getType());
    }
  }

  llvm::StructType *tupleType =
      llvm::StructType::create(context(), elementTypes, "tuple");
  llvm::AllocaInst *alloca =
      builder()->CreateAlloca(tupleType, nullptr, "tuple");

  return alloca;
}

// 生成 lambda 表达式
llvm::Value *LLVMCodeGenerator::generateLambdaExpr(
    std::unique_ptr<ast::LambdaExpr> lambdaExpr) {
  warning("Lambda expression not implemented");
  return nullptr;
}

// 生成反射表达式
llvm::Value *LLVMCodeGenerator::generateReflectionExpr(
    std::unique_ptr<ast::ReflectionExpr> reflectionExpr) {
  warning("Reflection expression not implemented");
  return nullptr;
}

// 生成表达式语句
llvm::Value *
LLVMCodeGenerator::generateExprStmt(std::unique_ptr<ast::ExprStmt> exprStmt) {
  return generateExpression(std::move(exprStmt->expr));
}

// 生成复合语句
llvm::Value *LLVMCodeGenerator::generateCompoundStmt(
    std::unique_ptr<ast::CompoundStmt> compoundStmt) {
  for (auto &stmt : compoundStmt->statements) {
    if (auto *statement = dynamic_cast<ast::Statement *>(stmt.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(stmt.release())));
    }
  }
  return nullptr;
}

// 生成返回语句
llvm::Value *LLVMCodeGenerator::generateReturnStmt(
    std::unique_ptr<ast::ReturnStmt> returnStmt) {
  if (returnStmt->expr) {
    llvm::Value *retVal = generateExpression(std::move(returnStmt->expr));
    builder()->CreateRet(retVal);
  } else {
    builder()->CreateRetVoid();
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

  // 转换为 bool
  cond = builder()->CreateICmpNE(
      cond, llvm::ConstantInt::get(cond->getType(), 0), "ifcond");

  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context(), "then", func);
  llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context(), "else");
  llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context(), "ifcont");

  builder()->CreateCondBr(cond, thenBB, elseBB);

  // 生成 then 块
  builder()->SetInsertPoint(thenBB);
  generateStatement(std::move(ifStmt->thenBranch));
  builder()->CreateBr(mergeBB);

  // 生成 else 块
  func->insert(func->end(), elseBB);
  builder()->SetInsertPoint(elseBB);
  if (ifStmt->elseBranch) {
    generateStatement(std::move(ifStmt->elseBranch));
  }
  builder()->CreateBr(mergeBB);

  // 生成 merge 块
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
  cond = builder()->CreateICmpNE(
      cond, llvm::ConstantInt::get(cond->getType(), 0), "whilecond");
  builder()->CreateCondBr(cond, bodyBB, afterBB);

  // 生成循环体
  func->insert(func->end(), bodyBB);
  builder()->SetInsertPoint(bodyBB);
  generateStatement(std::move(whileStmt->body));
  builder()->CreateBr(condBB);

  // 生成 after 块
  func->insert(func->end(), afterBB);
  builder()->SetInsertPoint(afterBB);

  return nullptr;
}

// 生成 for 语句
llvm::Value *
LLVMCodeGenerator::generateForStmt(std::unique_ptr<ast::ForStmt> forStmt) {
  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::BasicBlock *condBB =
      llvm::BasicBlock::Create(context(), "forcond", func);
  llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context(), "forbody");
  llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context(), "forafter");

  // 生成初始化
  if (forStmt->init) {
    if (auto *stmt = dynamic_cast<ast::Statement *>(forStmt->init.get())) {
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
      cond = builder()->CreateICmpNE(
          cond, llvm::ConstantInt::get(cond->getType(), 0), "forcond");
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
  generateStatement(std::move(forStmt->body));
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
  // TODO: 实现 break 语句
  warning("Break statement not fully implemented");
  return nullptr;
}

// 生成 continue 语句
llvm::Value *LLVMCodeGenerator::generateContinueStmt(
    std::unique_ptr<ast::ContinueStmt> continueStmt) {
  // TODO: 实现 continue 语句
  warning("Continue statement not fully implemented");
  return nullptr;
}

// 生成 match 语句
llvm::Value *LLVMCodeGenerator::generateMatchStmt(
    std::unique_ptr<ast::MatchStmt> matchStmt) {
  warning("Match statement not implemented");
  return nullptr;
}

// 生成 try 语句
llvm::Value *
LLVMCodeGenerator::generateTryStmt(std::unique_ptr<ast::TryStmt> tryStmt) {
  warning("Try statement not implemented");
  return nullptr;
}

// 生成 throw 语句
llvm::Value *LLVMCodeGenerator::generateThrowStmt(
    std::unique_ptr<ast::ThrowStmt> throwStmt) {
  warning("Throw statement not implemented");
  return nullptr;
}

// 生成 defer 语句
llvm::Value *LLVMCodeGenerator::generateDeferStmt(
    std::unique_ptr<ast::DeferStmt> deferStmt) {
  deferExpressions_.push_back(std::move(deferStmt->expr));
  return nullptr;
}

// 生成 getter 声明
llvm::Value *LLVMCodeGenerator::generateGetterDecl(
    std::unique_ptr<ast::GetterDecl> getterDecl) {
  warning("Getter declaration not implemented");
  return nullptr;
}

// 生成 setter 声明
llvm::Value *LLVMCodeGenerator::generateSetterDecl(
    std::unique_ptr<ast::SetterDecl> setterDecl) {
  warning("Setter declaration not implemented");
  return nullptr;
}

// 生成扩展声明
llvm::Value *LLVMCodeGenerator::generateExtensionDecl(
    std::unique_ptr<ast::ExtensionDecl> extensionDecl) {
  warning("Extension declaration not implemented");
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
  warning("Extension member function not implemented");
  return nullptr;
}

// 获取表达式左值
llvm::Value *
LLVMCodeGenerator::getExpressionLValue(std::unique_ptr<ast::Expression> expr) {
  if (auto *ident = dynamic_cast<ast::Identifier *>(expr.get())) {
    auto it = namedValues_.find(ident->name);
    if (it != namedValues_.end()) {
      return it->second;
    }
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
  // TODO: 实现枚举类型检查
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
  // TODO: 实现异常检查
}

} // namespace llvm_codegen
} // namespace c_hat