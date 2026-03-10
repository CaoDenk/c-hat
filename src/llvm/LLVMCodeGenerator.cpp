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

llvm::Value *LLVMCodeGenerator::generateExtensionMemberFunction(
    std::unique_ptr<ast::FunctionDecl> funcDecl,
    const std::string &structName) {
  auto it = structTypes_.find(structName);
  if (it == structTypes_.end()) {
    std::cerr << "Error: structType not found for: " << structName << std::endl;
    return nullptr;
  }
  llvm::StructType *structType = it->second;
  llvm::PointerType *selfType = structType->getPointerTo();

  std::vector<llvm::Type *> paramTypes;
  paramTypes.push_back(selfType);

  for (auto &paramNode : funcDecl->params) {
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      if (param->type) {
        llvm::Type *paramType = generateType(param->type.get());
        paramTypes.push_back(paramType);
      }
    }
  }

  llvm::Type *returnType;
  if (funcDecl->returnType) {
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

  std::string mangledName = structName + "_" + funcDecl->name;
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

  if (funcDecl->body) {
    llvm::BasicBlock *entryBlock =
        llvm::BasicBlock::Create(context(), "entry", function);
    builder()->SetInsertPoint(entryBlock);

    auto prevFunction = currentFunction_;
    currentFunction_ = function;

    namedValues_.clear();
    deferExpressions_.clear();
    lateVariables_.clear();

    // 为 self 参数创建 alloca
    llvm::AllocaInst *selfAlloca =
        builder()->CreateAlloca(selfType, nullptr, "self");
    builder()->CreateStore(selfArg, selfAlloca);
    namedValues_["self"] = selfAlloca;

    // 处理其他参数
    args = function->arg_begin();
    ++args; // 跳过 self 参数
    paramIndex = 0;
    for (; args != function->arg_end(); ++args, ++paramIndex) {
      if (auto *param = dynamic_cast<ast::Parameter *>(
              funcDecl->params[paramIndex].get())) {
        llvm::Value *arg = &*args;
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

llvm::Value *LLVMCodeGenerator::generateTypeAliasDecl(
    std::unique_ptr<ast::TypeAliasDecl> typeAliasDecl) {
  // 类型别名在语义分析阶段已经处理，这里不需要生成代码
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateGetterDecl(
    std::unique_ptr<ast::GetterDecl> getterDecl) {
  // Getter 方法在语义分析阶段已经处理，这里不需要生成代码
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateSetterDecl(
    std::unique_ptr<ast::SetterDecl> setterDecl) {
  // Setter 方法在语义分析阶段已经处理，这里不需要生成代码
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateExtensionDecl(
    std::unique_ptr<ast::ExtensionDecl> extensionDecl) {
  // 处理扩展中的成员函数
  for (auto &member : extensionDecl->members) {
    if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(member.get())) {
      auto funcDeclCopy = std::unique_ptr<ast::FunctionDecl>(
          static_cast<ast::FunctionDecl *>(member.release()));
      if (auto *namedType = dynamic_cast<ast::NamedType *>(
              extensionDecl->extendedType.get())) {
        generateExtensionMemberFunction(std::move(funcDeclCopy),
                                        namedType->name);
      }
    }
  }

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateExternDecl(
    std::unique_ptr<ast::ExternDecl> externDecl) {
  // 处理外部函数声明
  for (auto &decl : externDecl->declarations) {
    if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(decl.get())) {
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
          if (param->type) {
            if (auto *typeNode = dynamic_cast<ast::Type *>(param->type.get())) {
              llvm::Type *paramType = generateType(typeNode);
              paramTypes.push_back(paramType);
            }
          }
        }
      }

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

      // 创建外部函数声明
      llvm::Function *function = llvm::Function::Create(
          funcType, llvm::Function::ExternalLinkage, funcDecl->name, module());

      // 设置调用约定（如果是 C 函数）
      if (externDecl->abi == "C") {
        function->setCallingConv(llvm::CallingConv::C);
      }

      functions_[funcDecl->name] = function;
    }
  }

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateNamespaceDecl(
    std::unique_ptr<ast::NamespaceDecl> namespaceDecl) {
  // 进入命名空间
  namespaceStack_.push_back(namespaceDecl->name);

  // 处理命名空间中的成员
  for (size_t i = 0; i < namespaceDecl->members.size(); ++i) {
    auto &member = namespaceDecl->members[i];
    if (auto *decl = dynamic_cast<ast::Declaration *>(member.get())) {
      if (decl->getType() == ast::NodeType::FunctionDecl) {
        // 如果是函数声明，将其添加到 funcDecls_ 中
        if (funcDecls_) {
          auto funcDecl = static_cast<ast::FunctionDecl *>(member.release());
          // 存储函数的命名空间路径
          functionNamespaces_[funcDecl] = namespaceStack_;
          funcDecls_->push_back(std::unique_ptr<ast::FunctionDecl>(funcDecl));
        }
      } else {
        // 否则，直接处理
        generateDeclaration(std::unique_ptr<ast::Declaration>(
            static_cast<ast::Declaration *>(member.release())));
      }
    }
  }

  // 退出命名空间
  namespaceStack_.pop_back();

  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateStatement(std::unique_ptr<ast::Statement> stmt) {
  switch (stmt->getType()) {
  case ast::NodeType::ExprStmt:
    return generateExprStmt(std::unique_ptr<ast::ExprStmt>(
        static_cast<ast::ExprStmt *>(stmt.release())));
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
  case ast::NodeType::Statement: {
    // 处理 VariableStmt
    if (auto *varStmt = dynamic_cast<ast::VariableStmt *>(stmt.get())) {
      stmt.release();
      return generateVariableDecl(std::move(varStmt->declaration));
    }
    // 处理 TupleDestructuringStmt
    else if (auto *tupleStmt =
                 dynamic_cast<ast::TupleDestructuringStmt *>(stmt.get())) {
      stmt.release();
      return generateTupleDestructuringDecl(std::move(tupleStmt->declaration));
    }
    return nullptr;
  }
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
  case ast::NodeType::CompoundStmt:
    return generateCompoundStmt(std::unique_ptr<ast::CompoundStmt>(
        static_cast<ast::CompoundStmt *>(stmt.release())));
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

    // 检查当前基本块是否已经有终止指令
    // 如果有，停止处理后续语句
    llvm::BasicBlock *currentBlock = builder()->GetInsertBlock();
    if (currentBlock && currentBlock->getTerminator()) {
      break;
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

    // 获取当前函数的返回类型
    llvm::Function *currentFunc = builder()->GetInsertBlock()->getParent();
    if (!currentFunc) {
      builder()->CreateRetVoid();
      return nullptr;
    }
    llvm::Type *expectedReturnType = currentFunc->getReturnType();

    if (!returnValue) {
      // 如果没有返回值，根据函数返回类型生成默认值
      if (expectedReturnType->isVoidTy()) {
        builder()->CreateRetVoid();
      } else if (expectedReturnType->isIntegerTy()) {
        builder()->CreateRet(llvm::ConstantInt::get(expectedReturnType, 0));
      } else if (expectedReturnType->isFloatingPointTy()) {
        builder()->CreateRet(llvm::ConstantFP::get(expectedReturnType, 0.0));
      } else {
        // 对于其他类型，使用 nullptr
        builder()->CreateRet(
            llvm::ConstantPointerNull::get(expectedReturnType->getPointerTo()));
      }
      return nullptr;
    }

    // 如果返回值的类型与函数返回类型不匹配，进行类型转换
    if (returnValue->getType() != expectedReturnType) {
      if (expectedReturnType->isFloatingPointTy() &&
          returnValue->getType()->isIntegerTy()) {
        // 整数转浮点数
        returnValue =
            builder()->CreateSIToFP(returnValue, expectedReturnType, "rettmp");
      } else if (expectedReturnType->isIntegerTy() &&
                 returnValue->getType()->isFloatingPointTy()) {
        // 浮点数转整数
        returnValue =
            builder()->CreateFPToSI(returnValue, expectedReturnType, "rettmp");
      } else if (expectedReturnType->isIntegerTy() &&
                 returnValue->getType()->isIntegerTy()) {
        // 整数类型之间的转换
        returnValue = builder()->CreateIntCast(returnValue, expectedReturnType,
                                               true, "rettmp");
      }
    }

    builder()->CreateRet(returnValue);
    return returnValue;
  } else {
    // 如果没有返回表达式，根据函数返回类型生成默认值
    llvm::Function *currentFunc = builder()->GetInsertBlock()->getParent();
    if (currentFunc) {
      llvm::Type *expectedReturnType = currentFunc->getReturnType();
      if (expectedReturnType->isVoidTy()) {
        builder()->CreateRetVoid();
      } else if (expectedReturnType->isIntegerTy()) {
        builder()->CreateRet(llvm::ConstantInt::get(expectedReturnType, 0));
      } else if (expectedReturnType->isFloatingPointTy()) {
        builder()->CreateRet(llvm::ConstantFP::get(expectedReturnType, 0.0));
      } else {
        // 对于其他类型，使用 nullptr
        builder()->CreateRet(
            llvm::ConstantPointerNull::get(expectedReturnType->getPointerTo()));
      }
    } else {
      builder()->CreateRetVoid();
    }
    return nullptr;
  }
}

llvm::Value *
LLVMCodeGenerator::generateIfStmt(std::unique_ptr<ast::IfStmt> ifStmt) {
  llvm::Value *condValue = generateExpression(std::move(ifStmt->condition));
  // 确保条件表达式是整数类型
  if (!condValue->getType()->isIntegerTy()) {
    condValue = builder()->CreateTruncOrBitCast(
        condValue, llvm::Type::getInt32Ty(context()));
  }
  condValue = builder()->CreateICmpNE(
      condValue, llvm::ConstantInt::get(condValue->getType(), 0), "ifcond");

  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::BasicBlock *thenBlock =
      llvm::BasicBlock::Create(context(), "then", func);
  llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(context(), "else");
  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(context(), "ifcont");

  builder()->CreateCondBr(condValue, thenBlock, elseBlock);

  builder()->SetInsertPoint(thenBlock);
  generateStatement(std::move(ifStmt->thenBranch));
  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(mergeBlock);
  }

  elseBlock->insertInto(func);
  builder()->SetInsertPoint(elseBlock);
  if (ifStmt->elseBranch) {
    generateStatement(std::move(ifStmt->elseBranch));
  }
  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(mergeBlock);
  }

  mergeBlock->insertInto(func);
  builder()->SetInsertPoint(mergeBlock);

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateWhileStmt(
    std::unique_ptr<ast::WhileStmt> whileStmt) {
  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(context(), "loop");
  llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(context(), "loopbody");
  llvm::BasicBlock *afterBlock =
      llvm::BasicBlock::Create(context(), "afterloop");

  builder()->CreateBr(loopBlock);
  builder()->SetInsertPoint(loopBlock);

  llvm::Value *condValue = generateExpression(std::move(whileStmt->condition));
  condValue = builder()->CreateICmpNE(
      condValue, llvm::ConstantInt::get(condValue->getType(), 0), "whilecond");
  builder()->CreateCondBr(condValue, bodyBlock, afterBlock);

  bodyBlock->insertInto(func);
  builder()->SetInsertPoint(bodyBlock);
  generateStatement(std::move(whileStmt->body));
  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(loopBlock);
  }

  afterBlock->insertInto(func);
  builder()->SetInsertPoint(afterBlock);

  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateForStmt(std::unique_ptr<ast::ForStmt> forStmt) {
  // 处理初始化语句
  if (forStmt->init) {
    if (auto *stmt = dynamic_cast<ast::Statement *>(forStmt->init.get())) {
      generateStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(forStmt->init.release())));
    } else if (auto *expr =
                   dynamic_cast<ast::Expression *>(forStmt->init.get())) {
      generateExpression(std::unique_ptr<ast::Expression>(
          static_cast<ast::Expression *>(forStmt->init.release())));
    }
  }

  llvm::Function *func = builder()->GetInsertBlock()->getParent();
  llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(context(), "loop");
  llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(context(), "loopbody");
  llvm::BasicBlock *afterBlock =
      llvm::BasicBlock::Create(context(), "afterloop");

  loopBlock->insertInto(func);

  builder()->CreateBr(loopBlock);
  builder()->SetInsertPoint(loopBlock);

  // 处理条件表达式
  llvm::Value *condValue = generateExpression(std::move(forStmt->condition));
  condValue = builder()->CreateICmpNE(
      condValue, llvm::ConstantInt::get(condValue->getType(), 0), "forcond");
  builder()->CreateCondBr(condValue, bodyBlock, afterBlock);

  bodyBlock->insertInto(func);
  builder()->SetInsertPoint(bodyBlock);
  generateStatement(std::move(forStmt->body));

  // 处理更新语句
  if (forStmt->update) {
    generateExpression(std::move(forStmt->update));
  }

  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(loopBlock);
  }

  afterBlock->insertInto(func);
  builder()->SetInsertPoint(afterBlock);

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateBreakStmt(
    std::unique_ptr<ast::BreakStmt> breakStmt) {
  // 找到当前循环的结束块
  // 这里需要更复杂的实现，暂时简单处理
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateContinueStmt(
    std::unique_ptr<ast::ContinueStmt> continueStmt) {
  // 找到当前循环的条件块
  // 这里需要更复杂的实现，暂时简单处理
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateMatchStmt(
    std::unique_ptr<ast::MatchStmt> matchStmt) {
  // 这里需要实现 match 语句的代码生成
  // 暂时简单处理
  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateTryStmt(std::unique_ptr<ast::TryStmt> tryStmt) {
  llvm::Function *func = currentFunction_;
  if (!func) {
    return nullptr;
  }

  llvm::Type *intType = llvm::Type::getInt32Ty(context());
  llvm::PointerType *int8PtrType = llvm::PointerType::get(context(), 0);

  llvm::GlobalVariable *exceptionFlag =
      module()->getNamedGlobal("exception_flag");
  if (!exceptionFlag) {
    exceptionFlag = new llvm::GlobalVariable(
        *module(), intType, false, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(intType, 0), "exception_flag");
  }

  llvm::GlobalVariable *exceptionValueVar =
      module()->getNamedGlobal("exception_value");
  if (!exceptionValueVar) {
    exceptionValueVar = new llvm::GlobalVariable(
        *module(), intType, false, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(intType, 0), "exception_value");
  }

  llvm::GlobalVariable *exceptionTypeVar =
      module()->getNamedGlobal("exception_type");
  if (!exceptionTypeVar) {
    exceptionTypeVar = new llvm::GlobalVariable(
        *module(), int8PtrType, false, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantPointerNull::get(int8PtrType), "exception_type");
  }

  llvm::BasicBlock *tryBlock =
      llvm::BasicBlock::Create(context(), "try.body", func);
  llvm::BasicBlock *endBlock =
      llvm::BasicBlock::Create(context(), "try.end", func);

  std::vector<llvm::BasicBlock *> catchBlocks;
  std::vector<llvm::BasicBlock *> typeCheckBlocks;

  for (size_t i = 0; i < tryStmt->catchStmts.size(); ++i) {
    typeCheckBlocks.push_back(llvm::BasicBlock::Create(
        context(), "catch.check." + std::to_string(i), func));
    catchBlocks.push_back(llvm::BasicBlock::Create(
        context(), "catch.body." + std::to_string(i), func));
  }

  llvm::BasicBlock *currentBlock = builder()->GetInsertBlock();
  builder()->CreateBr(tryBlock);

  builder()->SetInsertPoint(tryBlock);
  generateStatement(std::move(tryStmt->tryBlock));

  if (!builder()->GetInsertBlock()->getTerminator()) {
    builder()->CreateBr(endBlock);
  }

  for (size_t i = 0; i < tryStmt->catchStmts.size(); ++i) {
    auto &catchStmt = tryStmt->catchStmts[i];

    builder()->SetInsertPoint(typeCheckBlocks[i]);

    bool hasTypeCheck = false;
    if (catchStmt->param && catchStmt->param->type) {
      std::string catchTypeName = catchStmt->param->type->toString();
      llvm::Value *exceptionType = builder()->CreateLoad(
          int8PtrType, exceptionTypeVar, "load_exception_type");

      llvm::Constant *catchTypeConst =
          llvm::ConstantDataArray::getString(context(), catchTypeName, true);
      llvm::GlobalVariable *catchTypeGlobal = new llvm::GlobalVariable(
          *module(), catchTypeConst->getType(), true,
          llvm::GlobalValue::PrivateLinkage, catchTypeConst,
          "catch.type." + std::to_string(i));

      llvm::Value *catchTypePtr =
          builder()->CreateBitCast(catchTypeGlobal, int8PtrType);

      llvm::Function *strcmpFunc = module()->getFunction("strcmp");
      if (!strcmpFunc) {
        std::vector<llvm::Type *> strcmpArgs = {int8PtrType, int8PtrType};
        llvm::FunctionType *strcmpType =
            llvm::FunctionType::get(intType, strcmpArgs, false);
        strcmpFunc = llvm::Function::Create(
            strcmpType, llvm::Function::ExternalLinkage, "strcmp", module());
      }

      llvm::Value *cmpResult =
          builder()->CreateCall(strcmpFunc, {exceptionType, catchTypePtr});
      llvm::Value *typeMatch = builder()->CreateICmpEQ(
          cmpResult, llvm::ConstantInt::get(intType, 0));

      llvm::BasicBlock *nextBlock = (i + 1 < tryStmt->catchStmts.size())
                                        ? typeCheckBlocks[i + 1]
                                        : endBlock;
      builder()->CreateCondBr(typeMatch, catchBlocks[i], nextBlock);
      hasTypeCheck = true;
    }

    if (!hasTypeCheck) {
      builder()->CreateBr(catchBlocks[i]);
    }

    builder()->SetInsertPoint(catchBlocks[i]);

    if (catchStmt->param) {
      llvm::Type *paramType = intType;
      if (catchStmt->param->type) {
        paramType = generateType(catchStmt->param->type.get());
      }

      llvm::AllocaInst *alloca =
          builder()->CreateAlloca(paramType, nullptr, catchStmt->param->name);

      llvm::Value *exceptionValue =
          builder()->CreateLoad(paramType, exceptionValueVar, "load_exception");
      builder()->CreateStore(exceptionValue, alloca);
      namedValues_[catchStmt->param->name] = alloca;
    }

    generateStatement(std::move(catchStmt->body));

    if (!builder()->GetInsertBlock()->getTerminator()) {
      builder()->CreateBr(endBlock);
    }
  }

  for (auto &block : *func) {
    if (block.getTerminator() &&
        block.getTerminator()->getOpcode() == llvm::Instruction::Unreachable) {
      builder()->SetInsertPoint(block.getTerminator());

      if (!typeCheckBlocks.empty()) {
        builder()->CreateBr(typeCheckBlocks[0]);
      } else {
        builder()->CreateBr(endBlock);
      }
      block.getTerminator()->eraseFromParent();
    }
  }

  builder()->SetInsertPoint(endBlock);

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateThrowStmt(
    std::unique_ptr<ast::ThrowStmt> throwStmt) {
  llvm::Value *exceptionValue = generateExpression(std::move(throwStmt->expr));
  llvm::Type *intType = llvm::Type::getInt32Ty(context());
  llvm::PointerType *int8PtrType = llvm::PointerType::get(context(), 0);

  llvm::GlobalVariable *exceptionFlag =
      module()->getNamedGlobal("exception_flag");
  if (!exceptionFlag) {
    exceptionFlag = new llvm::GlobalVariable(
        *module(), intType, false, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(intType, 0), "exception_flag");
  }

  llvm::GlobalVariable *exceptionValueVar =
      module()->getNamedGlobal("exception_value");
  if (!exceptionValueVar) {
    exceptionValueVar = new llvm::GlobalVariable(
        *module(), intType, false, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(intType, 0), "exception_value");
  }

  llvm::GlobalVariable *exceptionTypeVar =
      module()->getNamedGlobal("exception_type");
  if (!exceptionTypeVar) {
    exceptionTypeVar = new llvm::GlobalVariable(
        *module(), int8PtrType, false, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantPointerNull::get(int8PtrType), "exception_type");
  }

  if (exceptionValue) {
    builder()->CreateStore(exceptionValue, exceptionValueVar);

    std::string typeName = "int";
    if (exceptionValue->getType()->isFloatingPointTy()) {
      typeName = "double";
    } else if (exceptionValue->getType()->isIntegerTy()) {
      typeName = "int";
    }

    llvm::Constant *typeConst =
        llvm::ConstantDataArray::getString(context(), typeName, true);
    llvm::GlobalVariable *typeGlobal = new llvm::GlobalVariable(
        *module(), typeConst->getType(), true,
        llvm::GlobalValue::PrivateLinkage, typeConst, "throw.type");

    llvm::Value *typePtr = builder()->CreateBitCast(typeGlobal, int8PtrType);
    builder()->CreateStore(typePtr, exceptionTypeVar);
  }

  builder()->CreateStore(llvm::ConstantInt::get(intType, 1), exceptionFlag);
  // 不要使用 unreachable，而是返回一个默认值
  // builder()->CreateUnreachable();

  // 获取当前函数的返回类型
  llvm::Function *currentFunc = builder()->GetInsertBlock()->getParent();
  if (currentFunc) {
    llvm::Type *returnType = currentFunc->getReturnType();
    if (returnType->isVoidTy()) {
      builder()->CreateRetVoid();
    } else if (returnType->isIntegerTy()) {
      builder()->CreateRet(llvm::ConstantInt::get(returnType, 0));
    } else if (returnType->isFloatingPointTy()) {
      builder()->CreateRet(llvm::ConstantFP::get(returnType, 0.0));
    } else {
      // 对于其他类型，使用 nullptr
      builder()->CreateRet(
          llvm::ConstantPointerNull::get(returnType->getPointerTo()));
    }
  }

  return nullptr;
}

void LLVMCodeGenerator::checkExceptionAfterCall() {
  llvm::Type *intType = llvm::Type::getInt32Ty(context());

  llvm::GlobalVariable *exceptionFlag =
      module()->getNamedGlobal("exception_flag");
  if (!exceptionFlag) {
    exceptionFlag = new llvm::GlobalVariable(
        *module(), intType, false, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(intType, 0), "exception_flag");
  }

  // 检查异常标志
  llvm::Value *flagValue =
      builder()->CreateLoad(intType, exceptionFlag, "check_exception");
  llvm::Value *hasException =
      builder()->CreateICmpNE(flagValue, llvm::ConstantInt::get(intType, 0));

  // 创建一个基本块用于处理异常
  llvm::BasicBlock *currentBlock = builder()->GetInsertBlock();
  if (!currentBlock)
    return;

  llvm::Function *currentFunc = currentBlock->getParent();
  if (!currentFunc)
    return;

  // 创建一个异常处理块
  llvm::BasicBlock *exceptionBlock =
      llvm::BasicBlock::Create(context(), "exception.handle", currentFunc);
  llvm::BasicBlock *nextBlock =
      llvm::BasicBlock::Create(context(), "continue", currentFunc);

  // 创建条件分支
  builder()->CreateCondBr(hasException, exceptionBlock, nextBlock);

  // 在异常处理块中，我们需要跳转到最近的异常处理点
  // 这里我们使用 unreachable，让后端处理
  builder()->SetInsertPoint(exceptionBlock);
  builder()->CreateUnreachable();

  // 设置插入点到继续执行的块
  builder()->SetInsertPoint(nextBlock);
}

llvm::Value *LLVMCodeGenerator::generateDeferStmt(
    std::unique_ptr<ast::DeferStmt> deferStmt) {
  // 将 defer 表达式添加到 deferExpressions_ 中，在函数返回前执行
  deferExpressions_.push_back(std::move(deferStmt->expr));
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
  case ast::NodeType::Literal:
    return generateLiteral(std::unique_ptr<ast::Literal>(
        static_cast<ast::Literal *>(expr.release())));
  case ast::NodeType::Identifier:
    return generateIdentifier(std::unique_ptr<ast::Identifier>(
        static_cast<ast::Identifier *>(expr.release())));
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

llvm::Value *
LLVMCodeGenerator::getExpressionLValue(std::unique_ptr<ast::Expression> expr) {
  // 处理可以作为左值的表达式
  if (auto *identifier = dynamic_cast<ast::Identifier *>(expr.get())) {
    std::cerr << "Debug: getExpressionLValue - identifier name: "
              << identifier->name << std::endl;
    auto it = namedValues_.find(identifier->name);
    std::cerr << "Debug: namedValues_ size: " << namedValues_.size()
              << std::endl;
    if (it != namedValues_.end()) {
      std::cerr << "Debug: Found LValue for identifier: " << identifier->name
                << std::endl;
      expr.release();
      return it->second;
    } else {
      std::cerr << "Debug: LValue not found for identifier: "
                << identifier->name << std::endl;
    }
  } else if (auto *memberExpr = dynamic_cast<ast::MemberExpr *>(expr.get())) {
    // 处理成员表达式
    llvm::Value *object = getExpressionLValue(
        std::unique_ptr<ast::Expression>(memberExpr->object.release()));
    if (object) {
      // 处理成员表达式 - 假设我们已经知道结构体类型
      // 这里简化处理，使用第一个结构体类型
      for (const auto &[structName, members] : structInfo_) {
        std::string memberName = memberExpr->member;
        auto it = members.find(memberName);
        if (it != members.end()) {
          unsigned index = it->second;
          // 假设是结构体指针
          llvm::Value *indices[] = {
              llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
              llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), index)};
          expr.release();
          return builder()->CreateGEP(llvm::Type::getInt8Ty(context()), object,
                                      indices, memberName + "_ptr");
        }
      }
    }
  } else if (auto *subscriptExpr =
                 dynamic_cast<ast::SubscriptExpr *>(expr.get())) {
    // 处理下标表达式
    llvm::Value *array = getExpressionLValue(
        std::unique_ptr<ast::Expression>(subscriptExpr->object.release()));
    if (array) {
      llvm::Value *index = generateExpression(
          std::unique_ptr<ast::Expression>(subscriptExpr->index.release()));
      if (index) {
        expr.release();

        // 检查是否是切片类型
        if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(array)) {
          llvm::Type *allocatedType = alloca->getAllocatedType();
          if (allocatedType->isStructTy()) {
            llvm::StructType *structType =
                dyn_cast<llvm::StructType>(allocatedType);
            if (structType) {
              std::string structName = structType->getName().str();
              // 检查是否是切片类型
              if (structName.find("Slice_") == 0) {
                // 生成切片类型名称
                std::string sliceTypeName =
                    "Slice_" +
                    getLLVMTypeName(llvm::Type::getInt32Ty(context()));

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
                  fields.push_back(
                      llvm::PointerType::get(elementType, 0));         // ptr
                  fields.push_back(llvm::Type::getInt32Ty(context())); // len
                  sliceStructType = llvm::StructType::create(context(), fields,
                                                             sliceTypeName);

                  // 存储到 sliceTypes_ 映射中
                  sliceTypes_[sliceTypeName] = sliceStructType;
                }

                // 获取切片的 ptr 字段
                llvm::Value *ptrFieldPtr = builder()->CreateStructGEP(
                    sliceStructType, array, 0, "slice.ptr");
                // 加载 ptr 字段的值
                // 使用与 generateArrayInitExpr 函数相同的指针类型
                llvm::Type *ptrType = llvm::PointerType::get(
                    llvm::Type::getInt32Ty(context()), 0);
                llvm::Value *ptr =
                    builder()->CreateLoad(ptrType, ptrFieldPtr, "slice.ptr");

                // 假设元素类型是 int32
                llvm::Type *elementType = builder()->getInt32Ty();

                // 计算元素地址
                return builder()->CreateGEP(elementType, ptr, index,
                                            "element.ptr");
              }
            }
          }
        }

        // 对于其他类型，使用int8类型作为元素类型
        return builder()->CreateGEP(llvm::Type::getInt8Ty(context()), array,
                                    index, "array_idx");
      }
    }
  }

  // 如果不是左值，返回 nullptr
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateBinaryExpr(
    std::unique_ptr<ast::BinaryExpr> binaryExpr) {
  std::cerr << "Debug: generateBinaryExpr - op: "
            << static_cast<int>(binaryExpr->op) << std::endl;

  // 特殊处理赋值操作符
  if (binaryExpr->op == ast::BinaryExpr::Op::Assign) {
    std::cerr << "Debug: Processing assignment" << std::endl;
    llvm::Value *rhs = generateExpression(std::move(binaryExpr->right));
    llvm::Value *lhsLValue = getExpressionLValue(std::move(binaryExpr->left));
    if (lhsLValue) {
      std::cerr << "Debug: Assignment LValue found, storing value" << std::endl;
      builder()->CreateStore(rhs, lhsLValue);
      return rhs;
    } else {
      std::cerr << "Debug: Assignment LValue not found" << std::endl;
    }
    return nullptr;
  }

  llvm::Value *lhs = generateExpression(std::move(binaryExpr->left));
  llvm::Value *rhs = generateExpression(std::move(binaryExpr->right));

  // 检查是否需要类型转换
  bool lhsIsFloat = lhs->getType()->isFloatingPointTy();
  bool rhsIsFloat = rhs->getType()->isFloatingPointTy();
  bool lhsIsInt = lhs->getType()->isIntegerTy();
  bool rhsIsInt = rhs->getType()->isIntegerTy();
  bool isFloat = lhsIsFloat || rhsIsFloat;
  bool isInt = lhsIsInt && rhsIsInt;

  // 如果一个是浮点数，另一个需要转换为浮点数
  if (isFloat) {
    if (!lhsIsFloat && lhsIsInt) {
      lhs = builder()->CreateSIToFP(lhs, llvm::Type::getDoubleTy(context()),
                                    "casttmp");
    }
    if (!rhsIsFloat && rhsIsInt) {
      rhs = builder()->CreateSIToFP(rhs, llvm::Type::getDoubleTy(context()),
                                    "casttmp");
    }
  }

  switch (binaryExpr->op) {
  case ast::BinaryExpr::Op::Add:
    if (isFloat) {
      return builder()->CreateFAdd(lhs, rhs, "addtmp");
    } else if (isInt) {
      return builder()->CreateAdd(lhs, rhs, "addtmp");
    } else {
      throw std::runtime_error("Unsupported type for addition operation");
    }
  case ast::BinaryExpr::Op::Sub:
    if (isFloat) {
      return builder()->CreateFSub(lhs, rhs, "subtmp");
    } else if (isInt) {
      return builder()->CreateSub(lhs, rhs, "subtmp");
    } else {
      throw std::runtime_error("Unsupported type for subtraction operation");
    }
  case ast::BinaryExpr::Op::Mul:
    if (isFloat) {
      return builder()->CreateFMul(lhs, rhs, "multmp");
    } else if (isInt) {
      return builder()->CreateMul(lhs, rhs, "multmp");
    } else {
      throw std::runtime_error("Unsupported type for multiplication operation");
    }
  case ast::BinaryExpr::Op::Div:
    if (isFloat) {
      return builder()->CreateFDiv(lhs, rhs, "divtmp");
    } else if (isInt) {
      return builder()->CreateSDiv(lhs, rhs, "divtmp");
    } else {
      throw std::runtime_error("Unsupported type for division operation");
    }
  case ast::BinaryExpr::Op::Mod:
    if (isFloat) {
      return builder()->CreateFRem(lhs, rhs, "modtmp");
    } else if (isInt) {
      return builder()->CreateSRem(lhs, rhs, "modtmp");
    } else {
      throw std::runtime_error("Unsupported type for modulo operation");
    }
  case ast::BinaryExpr::Op::Eq:
    if (isFloat) {
      return builder()->CreateFCmpOEQ(lhs, rhs, "eqtmp");
    } else if (isInt) {
      return builder()->CreateICmpEQ(lhs, rhs, "eqtmp");
    } else {
      throw std::runtime_error("Unsupported type for equality operation");
    }
  case ast::BinaryExpr::Op::Ne:
    if (isFloat) {
      return builder()->CreateFCmpONE(lhs, rhs, "netmp");
    } else if (isInt) {
      return builder()->CreateICmpNE(lhs, rhs, "netmp");
    } else {
      throw std::runtime_error("Unsupported type for inequality operation");
    }
  case ast::BinaryExpr::Op::Lt:
    if (isFloat) {
      return builder()->CreateFCmpOLT(lhs, rhs, "lttmp");
    } else if (isInt) {
      return builder()->CreateICmpSLT(lhs, rhs, "lttmp");
    } else {
      throw std::runtime_error("Unsupported type for less than operation");
    }
  case ast::BinaryExpr::Op::Le:
    if (isFloat) {
      return builder()->CreateFCmpOLE(lhs, rhs, "letmp");
    } else if (isInt) {
      return builder()->CreateICmpSLE(lhs, rhs, "letmp");
    } else {
      throw std::runtime_error(
          "Unsupported type for less than or equal operation");
    }
  case ast::BinaryExpr::Op::Gt:
    if (isFloat) {
      return builder()->CreateFCmpOGT(lhs, rhs, "gttmp");
    } else if (isInt) {
      return builder()->CreateICmpSGT(lhs, rhs, "gttmp");
    } else {
      throw std::runtime_error("Unsupported type for greater than operation");
    }
  case ast::BinaryExpr::Op::Ge:
    if (isFloat) {
      return builder()->CreateFCmpOGE(lhs, rhs, "getmp");
    } else if (isInt) {
      return builder()->CreateICmpSGE(lhs, rhs, "getmp");
    } else {
      throw std::runtime_error(
          "Unsupported type for greater than or equal operation");
    }
  case ast::BinaryExpr::Op::And:
    if (isInt) {
      return builder()->CreateAnd(lhs, rhs, "andtmp");
    } else {
      throw std::runtime_error("Unsupported type for bitwise and operation");
    }
  case ast::BinaryExpr::Op::Or:
    if (isInt) {
      return builder()->CreateOr(lhs, rhs, "ortmp");
    } else {
      throw std::runtime_error("Unsupported type for bitwise or operation");
    }
  case ast::BinaryExpr::Op::Xor:
    if (isInt) {
      return builder()->CreateXor(lhs, rhs, "xortmp");
    } else {
      throw std::runtime_error("Unsupported type for bitwise xor operation");
    }
  case ast::BinaryExpr::Op::Shl:
    if (isInt) {
      return builder()->CreateShl(lhs, rhs, "shltmp");
    } else {
      throw std::runtime_error("Unsupported type for shift left operation");
    }
  case ast::BinaryExpr::Op::Shr:
    if (isInt) {
      return builder()->CreateAShr(lhs, rhs, "shrtmp");
    } else {
      throw std::runtime_error("Unsupported type for shift right operation");
    }
  default:
    throw std::runtime_error("Unsupported binary operator");
  }

  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateUnaryExpr(std::unique_ptr<ast::UnaryExpr> unaryExpr,
                                     bool isLValue) {
  llvm::Value *expr = generateExpression(std::move(unaryExpr->expr));

  bool isFloat = expr->getType()->isFloatingPointTy();

  switch (unaryExpr->op) {
  case ast::UnaryExpr::Op::Plus:
    return expr;
  case ast::UnaryExpr::Op::Minus:
    return isFloat ? builder()->CreateFNeg(expr, "negtmp")
                   : builder()->CreateNeg(expr, "negtmp");
  case ast::UnaryExpr::Op::Not:
    return builder()->CreateNot(expr, "nottmp");
  case ast::UnaryExpr::Op::BitNot:
    return builder()->CreateNot(expr, "bwnottmp");
  case ast::UnaryExpr::Op::Dereference: {
    // 处理解引用操作
    // 对于不透明指针，使用int8类型作为元素类型
    return builder()->CreateLoad(llvm::Type::getInt8Ty(context()), expr,
                                 "dereftmp");
  }
  case ast::UnaryExpr::Op::AddressOf: {
    // 处理取地址操作
    if (isLValue) {
      // 如果是左值，直接返回其地址
      return expr;
    } else {
      // 如果不是左值，需要创建一个临时变量
      llvm::AllocaInst *alloca =
          builder()->CreateAlloca(expr->getType(), nullptr, "tmpaddr");
      builder()->CreateStore(expr, alloca);
      return alloca;
    }
  }
  default:
    throw std::runtime_error("Unsupported unary operator");
  }
}

llvm::Value *
LLVMCodeGenerator::generateCallExpr(std::unique_ptr<ast::CallExpr> callExpr) {
  std::vector<llvm::Value *> args;
  std::string funcName;
  llvm::Value *objectPtr = nullptr;

  // Check if callee is a MemberExpr (method call or namespace access)
  bool isFunctionPointer = false;
  if (auto *memberExpr =
          dynamic_cast<ast::MemberExpr *>(callExpr->callee.get())) {
    // 处理嵌套的 MemberExpr，构建完整的命名空间路径
    std::vector<std::string> namespaces;
    ast::Expression *currentExpr = memberExpr->object.get();

    // 递归获取所有命名空间
    while (auto *nestedMemberExpr =
               dynamic_cast<ast::MemberExpr *>(currentExpr)) {
      namespaces.insert(namespaces.begin(), nestedMemberExpr->member);
      currentExpr = nestedMemberExpr->object.get();
    }

    // 处理最内层的标识符
    if (auto *identifier = dynamic_cast<ast::Identifier *>(currentExpr)) {
      namespaces.insert(namespaces.begin(), identifier->name);
    }

    // 构建完整的函数名，包括命名空间路径
    funcName = namespaces[0];
    for (size_t i = 1; i < namespaces.size(); ++i) {
      funcName += "." + namespaces[i];
    }
    funcName += "." + memberExpr->member;
  } else if (auto *identifier =
                 dynamic_cast<ast::Identifier *>(callExpr->callee.get())) {
    // Regular function call
    funcName = identifier->name;
  }

  // Generate arguments
  for (auto &arg : callExpr->args) {
    llvm::Value *argValue = generateExpression(std::move(arg));
    args.push_back(argValue);
  }

  // Handle implicit conversion for string literals (LiteralView to byte!^)
  // 检查参数是否为 LiteralView 结构体，如果是，则转换为 byte^（char*）
  for (size_t i = 0; i < args.size(); ++i) {
    llvm::Value *arg = args[i];
    if (!arg)
      continue;
    // 检查是否是 alloca 指令，并且分配的是 LiteralView 结构体
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(arg)) {
      if (auto *structType =
              llvm::dyn_cast<llvm::StructType>(alloca->getAllocatedType())) {
        if (structType->getName().str() == "LiteralView") {
          // 获取 LiteralView 结构体的 ptr 字段
          llvm::Value *ptrField =
              builder()->CreateStructGEP(structType, arg, 0, "ptr");
          llvm::Value *ptr = builder()->CreateLoad(
              llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0),
              ptrField, "strptr");
          args[i] = ptr;
        }
      }
    }
  }

  // Find or create the function
  llvm::Function *function = nullptr;

  // 尝试找到匹配的函数
  bool found = false;

  // 首先尝试直接使用函数名（对于 extern 函数）
  auto it = functions_.find(funcName);
  if (it != functions_.end()) {
    function = it->second;
    found = true;
  } else {
    // 对于非 extern 函数，生成带参数类型的唯一函数名
    std::string uniqueFuncName = funcName;
    if (!args.empty()) {
      uniqueFuncName += "@";
      for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
          uniqueFuncName += "#";
        }
        std::string typeName;

        // 检查参数是否为 null
        if (!args[i]) {
          typeName = "unknown";
        } else {
          // 检查是否为结构体类型
          if (auto *structType =
                  llvm::dyn_cast<llvm::StructType>(args[i]->getType())) {
            typeName = structType->getName().str();
          }
          if (typeName.empty()) {
            if (args[i]->getType()->isFloatingPointTy()) {
              typeName = "double";
            } else if (args[i]->getType()->isIntegerTy()) {
              typeName = "int";
            } else {
              typeName =
                  args[i]->getType()->getTypeID() == llvm::Type::PointerTyID
                      ? "ptr"
                      : "unknown";
            }
          }
        }
        uniqueFuncName += typeName;
      }
    }

    // 查找带参数类型的函数名（已经包含命名空间路径）
    it = functions_.find(uniqueFuncName);
    if (it != functions_.end()) {
      function = it->second;
      found = true;
    } else {
      // 如果没找到，再尝试前缀匹配
      for (const auto &[name, func] : functions_) {
        if (name == funcName || name.find(funcName + "_") == 0) {
          if (func->arg_size() == args.size()) {
            bool typeMatch = true;
            auto argIt = args.begin();
            for (auto &funcArg : func->args()) {
              if (argIt == args.end()) {
                typeMatch = false;
                break;
              }
              if ((*argIt)->getType() != funcArg.getType()) {
                typeMatch = false;
                break;
              }
              ++argIt;
            }
            if (typeMatch) {
              function = func;
              found = true;
              break;
            }
          }
        }
      }
    }
  }

  // 特殊处理 printf 函数
  if (funcName == "printf") {
    // 确保我们有参数
    if (args.empty()) {
      return nullptr;
    }
    // 检查第一个参数是否是字符串类型
    if (args[0]->getType() !=
        llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0)) {
      // 如果不是，尝试转换
      args[0] = builder()->CreatePointerCast(
          args[0], llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0),
          "fmtptr");
    }
    // 创建 printf 函数类型
    llvm::FunctionType *printfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context()),
        {llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0)},
        true); // 可变参数
    // 获取或创建 printf 函数
    llvm::FunctionCallee printfFunc =
        module()->getOrInsertFunction("printf", printfType);
    // 调用 printf
    return builder()->CreateCall(printfFunc, args, "printf.result");
  }

  // 特殊处理 puts 函数
  if (funcName == "puts") {
    // 确保我们有参数
    if (args.empty()) {
      return nullptr;
    }
    // 检查第一个参数是否是字符串类型
    if (args[0]->getType() !=
        llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0)) {
      // 如果不是，尝试转换
      args[0] = builder()->CreatePointerCast(
          args[0], llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0),
          "strptr");
    }
    // 创建 puts 函数类型
    llvm::FunctionType *putsType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context()),
        {llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0)},
        false); // 非可变参数
    // 获取或创建 puts 函数
    llvm::FunctionCallee putsFunc =
        module()->getOrInsertFunction("puts", putsType);
    // 调用 puts
    return builder()->CreateCall(putsFunc, args, "puts.result");
  }

  // 对于其他函数，使用默认处理
  // 确保找到函数
  if (!function) {
    // 如果仍然没有找到，创建一个新的函数声明
    // 这里简化处理，假设返回类型为int
    std::vector<llvm::Type *> argTypes;
    for (auto *arg : args) {
      if (arg) {
        argTypes.push_back(arg->getType());
      } else {
        // 如果参数为 null，使用 int 类型作为默认类型
        argTypes.push_back(llvm::Type::getInt32Ty(context()));
      }
    }
    // 检查是否是可变参数函数（如printf）
    bool isVarArg = false;
    if (funcName == "printf") {
      isVarArg = true;
    }
    llvm::FunctionType *funcType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context()), argTypes, isVarArg);
    // 对于 extern 函数，使用 C 风格的函数名
    function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                      funcName, module());
    functions_[funcName] = function;
  }

  // 确保参数数量与函数签名匹配
  if (args.size() != function->arg_size() && !function->isVarArg()) {
    // 参数数量不匹配，调整参数列表
    std::vector<llvm::Value *> adjustedArgs;
    for (size_t i = 0; i < function->arg_size(); ++i) {
      if (i < args.size() && args[i]) {
        adjustedArgs.push_back(args[i]);
      } else {
        // 如果参数不足，使用默认值
        adjustedArgs.push_back(
            llvm::Constant::getNullValue(function->getArg(i)->getType()));
      }
    }
    llvm::Value *callResult =
        builder()->CreateCall(function, adjustedArgs, "calltmp");

    // 检查异常
    checkExceptionAfterCall();

    return callResult;
  }

  llvm::Value *callResult = builder()->CreateCall(function, args, "calltmp");

  // 检查异常
  checkExceptionAfterCall();

  return callResult;
}

// Missing function implementations
llvm::Value *
LLVMCodeGenerator::generateLiteral(std::unique_ptr<ast::Literal> literal) {
  // Handle different types of literals
  if (literal->type == ast::Literal::Type::String) {
    // Create a LiteralView struct
    llvm::Type *literalViewType = getLiteralViewType();
    llvm::AllocaInst *alloca =
        builder()->CreateAlloca(literalViewType, nullptr, "literal");

    // Get the string value
    const std::string &strValue = literal->value;

    // Create a global string constant
    llvm::Constant *strConstant =
        llvm::ConstantDataArray::getString(context(), strValue);
    llvm::GlobalVariable *globalStr = new llvm::GlobalVariable(
        *module(), strConstant->getType(), true,
        llvm::GlobalValue::PrivateLinkage, strConstant, "str");

    // Get the pointer to the string
    llvm::Value *strPtr = builder()->CreatePointerCast(
        globalStr, llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0),
        "strptr");

    // Get the length of the string
    llvm::Value *strLen = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context()), strValue.size());

    // Store the pointer and length in the LiteralView struct
    llvm::Value *ptrField =
        builder()->CreateStructGEP(literalViewType, alloca, 0, "ptr");
    builder()->CreateStore(strPtr, ptrField);

    llvm::Value *lenField =
        builder()->CreateStructGEP(literalViewType, alloca, 1, "len");
    builder()->CreateStore(strLen, lenField);

    return alloca;
  } else if (literal->type == ast::Literal::Type::Integer) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()),
                                  std::stoi(literal->value));
  } else if (literal->type == ast::Literal::Type::Floating) {
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context()),
                                 std::stod(literal->value));
  } else if (literal->type == ast::Literal::Type::Boolean) {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context()),
                                  literal->value == "true" ? 1 : 0);
  }
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateIdentifier(
    std::unique_ptr<ast::Identifier> identifier) {
  auto it = namedValues_.find(identifier->name);
  if (it != namedValues_.end()) {
    llvm::Value *value = it->second;
    // 如果值是AllocaInst（即变量的地址），需要加载其值
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(value)) {
      // 检查是否是切片类型
      llvm::Type *allocatedType = alloca->getAllocatedType();
      if (allocatedType->isStructTy()) {
        llvm::StructType *structType =
            dyn_cast<llvm::StructType>(allocatedType);
        if (structType) {
          std::string structName = structType->getName().str();
          // 检查是否是切片类型
          if (structName.find("Slice_") == 0) {
            // 对于切片类型，返回变量的地址
            return alloca;
          }
        }
      }
      // 对于其他类型，加载其值
      return builder()->CreateLoad(alloca->getAllocatedType(), alloca,
                                   identifier->name);
    }
    return value;
  }
  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateExpansionExpr(
    std::unique_ptr<ast::ExpansionExpr> expansionExpr) {
  // 生成内部表达式
  llvm::Value *exprValue = generateExpression(std::move(expansionExpr->expr));
  if (!exprValue) {
    return nullptr;
  }

  // 对于展开表达式，我们直接返回内部表达式的值
  // 注意：实际实现可能需要根据语言的具体语义进行调整
  return exprValue;
}

llvm::Value *LLVMCodeGenerator::generateArrayInitExpr(
    std::unique_ptr<ast::ArrayInitExpr> arrayInitExpr) {
  // 检查 arrayInitExpr 是否为空
  if (!arrayInitExpr) {
    return nullptr;
  }

  // 获取数组大小
  size_t arraySize = arrayInitExpr->elements.size();
  if (arraySize == 0) {
    // 空数组，返回 null
    return nullptr;
  }

  // 从第一个元素推断类型
  llvm::Type *llvmElementType = llvm::Type::getInt32Ty(context()); // 默认类型

  if (!arrayInitExpr->elements.empty()) {
    // 生成第一个元素的表达式，获取其类型
    // 注意：这里不能移动原始元素，因为后面还要使用
    // 所以我们先克隆一个副本
    auto firstElementCopy = arrayInitExpr->elements[0]->clone();
    llvm::Value *firstElement = generateExpression(std::move(firstElementCopy));
    if (firstElement) {
      llvmElementType = firstElement->getType();
    }
  }

  // 创建数组类型
  llvm::ArrayType *arrayType = llvm::ArrayType::get(llvmElementType, arraySize);

  // 创建切片类型
  // 生成切片类型名称
  std::string sliceTypeName = "Slice_" + getLLVMTypeName(llvmElementType);

  // 检查是否已经存在该切片类型
  auto it = sliceTypes_.find(sliceTypeName);
  llvm::StructType *sliceStructType;
  if (it != sliceTypes_.end()) {
    sliceStructType = it->second;
  } else {
    // 生成元素类型
    llvm::Type *elementType = llvmElementType;

    // 创建切片类型的结构体
    std::vector<llvm::Type *> fields;
    fields.push_back(llvm::PointerType::get(elementType, 0)); // ptr
    fields.push_back(llvm::Type::getInt32Ty(context()));      // len
    sliceStructType =
        llvm::StructType::create(context(), fields, sliceTypeName);

    // 存储到 sliceTypes_ 映射中
    sliceTypes_[sliceTypeName] = sliceStructType;
  }

  // 在堆上分配数组
  // 计算数组大小（以字节为单位）
  llvm::Value *arraySizeValue = llvm::ConstantInt::get(
      llvm::Type::getInt32Ty(context()), static_cast<uint32_t>(arraySize));

  // 对于 int32 类型，大小是 4 字节
  llvm::Value *elementSizeValue =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 4);
  llvm::Value *totalSizeValue =
      builder()->CreateMul(arraySizeValue, elementSizeValue, "total.size");

  // 调用 malloc 分配内存
  llvm::FunctionType *mallocType =
      llvm::FunctionType::get(llvm::PointerType::get(context(), 0),
                              {llvm::Type::getInt32Ty(context())}, false);
  llvm::Function *mallocFunc = module()->getFunction("malloc");
  if (!mallocFunc) {
    mallocFunc = llvm::Function::Create(
        mallocType, llvm::Function::ExternalLinkage, "malloc", module());
  }
  llvm::Value *arrayAlloca =
      builder()->CreateCall(mallocFunc, {totalSizeValue}, "array");

  // 将 void* 转换为元素类型的指针
  arrayAlloca = builder()->CreatePointerCast(
      arrayAlloca, llvm::PointerType::get(llvmElementType, 0), "array.cast");

  // 初始化数组元素
  for (size_t i = 0; i < arraySize; ++i) {
    if (i >= arrayInitExpr->elements.size()) {
      // 防止越界访问
      break;
    }

    // 生成元素表达式
    // 注意：这里不能移动原始元素，因为后面可能还要使用
    // 所以我们先克隆一个副本
    auto elementCopy = arrayInitExpr->elements[i]->clone();
    llvm::Value *elementValue = generateExpression(std::move(elementCopy));
    if (!elementValue) {
      // 如果元素生成失败，使用默认值
      if (llvmElementType->isIntegerTy()) {
        elementValue = llvm::ConstantInt::get(llvmElementType, 0);
      } else if (llvmElementType->isFloatingPointTy()) {
        elementValue = llvm::ConstantFP::get(llvmElementType, 0.0);
      } else {
        continue;
      }
    }

    // 确保元素类型匹配
    if (elementValue->getType() != llvmElementType) {
      // 尝试类型转换
      if (llvmElementType->isIntegerTy() &&
          elementValue->getType()->isFloatingPointTy()) {
        elementValue =
            builder()->CreateFPToSI(elementValue, llvmElementType, "casttmp");
      } else if (llvmElementType->isFloatingPointTy() &&
                 elementValue->getType()->isIntegerTy()) {
        elementValue =
            builder()->CreateSIToFP(elementValue, llvmElementType, "casttmp");
      } else if (llvmElementType->isPointerTy() &&
                 elementValue->getType()->isPointerTy()) {
        elementValue = builder()->CreatePointerCast(elementValue,
                                                    llvmElementType, "casttmp");
      } else {
        // 类型不匹配，使用默认值
        if (llvmElementType->isIntegerTy()) {
          elementValue = llvm::ConstantInt::get(llvmElementType, 0);
        } else if (llvmElementType->isFloatingPointTy()) {
          elementValue = llvm::ConstantFP::get(llvmElementType, 0.0);
        } else {
          continue;
        }
      }
    }

    // 计算元素的索引
    llvm::Value *index = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context()), static_cast<uint32_t>(i));

    // 获取元素的地址
    llvm::Value *elementPtr = builder()->CreateGEP(llvmElementType, arrayAlloca,
                                                   index, "element.ptr");

    // 存储元素值
    builder()->CreateStore(elementValue, elementPtr);
  }

  // 在栈上分配切片结构体
  llvm::AllocaInst *sliceAlloca =
      builder()->CreateAlloca(sliceStructType, nullptr, "slice");

  // 存储 ptr 字段
  llvm::Value *ptrFieldPtr =
      builder()->CreateStructGEP(sliceStructType, sliceAlloca, 0, "slice.ptr");
  builder()->CreateStore(arrayAlloca, ptrFieldPtr);

  // 存储 len 字段
  llvm::Value *lenFieldPtr =
      builder()->CreateStructGEP(sliceStructType, sliceAlloca, 1, "slice.len");
  llvm::Value *lenValue = llvm::ConstantInt::get(
      llvm::Type::getInt32Ty(context()), static_cast<uint32_t>(arraySize));
  builder()->CreateStore(lenValue, lenFieldPtr);

  // 返回切片结构体
  return sliceAlloca;
}

llvm::Value *LLVMCodeGenerator::generateStructInitExpr(
    std::unique_ptr<ast::StructInitExpr> structInitExpr) {
  // 检查是否有类型信息
  if (!structInitExpr->type) {
    return nullptr;
  }

  // 生成结构体类型
  llvm::Type *llvmType = generateType(structInitExpr->type.get());
  if (!llvmType) {
    return nullptr;
  }

  // 检查是否是结构体类型
  if (!llvmType->isStructTy()) {
    return nullptr;
  }

  // 分配结构体实例
  llvm::Value *structPtr =
      builder()->CreateAlloca(llvmType, nullptr, "struct.ptr");

  // 生成每个字段的表达式并存储到结构体中
  for (auto &field : structInitExpr->fields) {
    const std::string &fieldName = field.first;
    std::unique_ptr<ast::Expression> fieldExpr =
        std::move(const_cast<std::unique_ptr<ast::Expression> &>(field.second));

    // 生成字段表达式
    llvm::Value *fieldValue = generateExpression(std::move(fieldExpr));
    if (!fieldValue) {
      return nullptr;
    }

    // 查找字段在结构体中的索引
    // 注意：这里简化处理，假设字段名与结构体中的字段顺序一致
    // 实际实现需要根据结构体的字段信息进行查找
    unsigned int fieldIndex = 0; // 临时实现，需要根据实际情况修改

    // 计算字段地址
    llvm::Value *fieldPtr = builder()->CreateStructGEP(llvmType, structPtr,
                                                       fieldIndex, "field.ptr");

    // 存储字段值
    builder()->CreateStore(fieldValue, fieldPtr);
  }

  return structPtr;
}

llvm::Value *LLVMCodeGenerator::generateTupleExpr(
    std::unique_ptr<ast::TupleExpr> tupleExpr) {
  // 检查元组是否为空
  if (tupleExpr->elements.empty()) {
    return nullptr;
  }

  // 生成每个元素的表达式
  std::vector<llvm::Value *> elementValues;
  std::vector<llvm::Type *> elementTypes;

  for (auto &element : tupleExpr->elements) {
    // 生成元素表达式
    llvm::Value *elementValue = generateExpression(std::move(element));
    if (!elementValue) {
      return nullptr;
    }
    elementValues.push_back(elementValue);
    elementTypes.push_back(elementValue->getType());
  }

  // 创建元组类型（结构体类型）
  llvm::StructType *tupleType =
      llvm::StructType::create(context(), elementTypes, "Tuple");

  // 分配元组实例
  llvm::Value *tuplePtr =
      builder()->CreateAlloca(tupleType, nullptr, "tuple.ptr");

  // 存储每个元素到元组中
  for (unsigned int i = 0; i < elementValues.size(); i++) {
    // 计算元素地址
    llvm::Value *elementPtr =
        builder()->CreateStructGEP(tupleType, tuplePtr, i, "element.ptr");
    // 存储元素值
    builder()->CreateStore(elementValues[i], elementPtr);
  }

  return tuplePtr;
}

llvm::Value *LLVMCodeGenerator::generateLambdaExpr(
    std::unique_ptr<ast::LambdaExpr> lambdaExpr) {
  // 生成参数类型
  std::vector<llvm::Type *> paramTypes;
  for (const auto &param : lambdaExpr->params) {
    // 生成参数类型
    llvm::Type *paramType = generateType(param->type.get());
    if (!paramType) {
      return nullptr;
    }
    paramTypes.push_back(paramType);
  }

  // 假设返回类型为 void
  // 实际实现需要根据函数体的返回值类型进行推断
  llvm::Type *returnType = llvm::Type::getVoidTy(context());

  // 创建函数类型
  llvm::FunctionType *functionType =
      llvm::FunctionType::get(returnType, paramTypes, false);

  // 创建函数
  llvm::Function *lambdaFunction = llvm::Function::Create(
      functionType, llvm::Function::InternalLinkage, "lambda", module());

  // 设置参数名称
  unsigned int i = 0;
  for (auto &arg : lambdaFunction->args()) {
    arg.setName(lambdaExpr->params[i]->name);
    i++;
  }

  // 保存当前的插入点
  llvm::BasicBlock::iterator currentInsertPoint = builder()->GetInsertPoint();

  // 创建函数体的基本块
  llvm::BasicBlock *entryBlock =
      llvm::BasicBlock::Create(context(), "entry", lambdaFunction);
  builder()->SetInsertPoint(entryBlock);

  // 处理捕获的变量
  // 注意：这里简化处理，实际实现需要根据捕获方式（值捕获或引用捕获）进行处理
  for (const auto &capture : lambdaExpr->captures) {
    // 查找捕获的变量
    auto it = namedValues_.find(capture.name);
    if (it != namedValues_.end()) {
      // 将捕获的变量添加到 lambda 函数的命名值中
      // 注意：这里简化处理，实际实现需要根据捕获方式进行处理
      namedValues_[capture.name] = it->second;
    }
  }

  // 生成函数体
  generateStatement(std::move(lambdaExpr->body));

  // 如果函数体没有返回语句，添加一个默认的返回语句
  if (!entryBlock->getTerminator()) {
    builder()->CreateRetVoid();
  }

  // 恢复插入点到之前的位置
  builder()->SetInsertPoint(currentInsertPoint);

  // 创建函数指针
  llvm::Value *functionPtr = llvm::ConstantExpr::getBitCast(
      lambdaFunction, llvm::PointerType::get(functionType, 0));

  return functionPtr;
}

llvm::Value *LLVMCodeGenerator::generateReflectionExpr(
    std::unique_ptr<ast::ReflectionExpr> reflectionExpr) {
  // 反射表达式在 LLVM 中比较复杂，需要更多的上下文信息
  // 暂时返回 nullptr，实际实现需要根据反射的具体语义进行处理
  return nullptr;
}

llvm::Type *LLVMCodeGenerator::generateType(ast::Type *type) {
  if (!type) {
    return llvm::Type::getInt32Ty(context());
  }

  // 根据类型节点类型返回对应的LLVM类型
  if (auto *primitiveType = dynamic_cast<ast::PrimitiveType *>(type)) {
    switch (primitiveType->kind) {
    case ast::PrimitiveType::Kind::Int:
      return llvm::Type::getInt32Ty(context());
    case ast::PrimitiveType::Kind::Double:
      return llvm::Type::getDoubleTy(context());
    case ast::PrimitiveType::Kind::Float:
      return llvm::Type::getFloatTy(context());
    case ast::PrimitiveType::Kind::Bool:
      return llvm::Type::getInt1Ty(context());
    case ast::PrimitiveType::Kind::Byte:
      return llvm::Type::getInt8Ty(context());
    case ast::PrimitiveType::Kind::Char:
      return llvm::Type::getInt32Ty(context());
    case ast::PrimitiveType::Kind::Void:
      return llvm::Type::getVoidTy(context());
    default:
      return llvm::Type::getInt32Ty(context());
    }
  } else if (auto *namedType = dynamic_cast<ast::NamedType *>(type)) {
    // 根据类型名称返回对应的LLVM类型
    const std::string &typeName = namedType->name;
    if (typeName == "int") {
      return llvm::Type::getInt32Ty(context());
    } else if (typeName == "double") {
      return llvm::Type::getDoubleTy(context());
    } else if (typeName == "float") {
      return llvm::Type::getFloatTy(context());
    } else if (typeName == "bool") {
      return llvm::Type::getInt1Ty(context());
    } else if (typeName == "byte") {
      return llvm::Type::getInt8Ty(context());
    } else if (typeName == "char") {
      return llvm::Type::getInt32Ty(context());
    } else if (typeName == "void") {
      return llvm::Type::getVoidTy(context());
    }
  } else if (auto *sliceType = dynamic_cast<ast::SliceType *>(type)) {
    // 生成切片类型，类似于 LiteralView，包含 ptr 和 len 两个字段
    std::string sliceTypeName = "Slice_" + sliceType->baseType->toString();

    // 检查是否已经存在该切片类型
    auto it = sliceTypes_.find(sliceTypeName);
    if (it != sliceTypes_.end()) {
      return it->second;
    }

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

    return sliceStructType;
  }

  // 默认返回i32类型
  return llvm::Type::getInt32Ty(context());
}

llvm::Type *LLVMCodeGenerator::getLiteralViewType() {
  // Create the LiteralView struct type if it doesn't exist
  if (!literalViewType_) {
    std::vector<llvm::Type *> fields;
    fields.push_back(
        llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0)); // ptr
    fields.push_back(llvm::Type::getInt32Ty(context()));              // len
    literalViewType_ =
        llvm::StructType::create(context(), fields, "LiteralView");
  }
  return literalViewType_;
}

// Additional missing function implementations
llvm::Value *LLVMCodeGenerator::generateMemberExpr(
    std::unique_ptr<ast::MemberExpr> memberExpr) {
  // 生成对象表达式
  llvm::Value *object = generateExpression(std::move(memberExpr->object));
  if (!object) {
    return nullptr;
  }

  std::string memberName = memberExpr->member;

  // 检查是否是切片类型的属性访问
  if (memberName == "len" || memberName == "ptr") {
    // 生成切片类型名称，与 generateArrayInitExpr 函数保持一致
    std::string sliceTypeName =
        "Slice_" + getLLVMTypeName(llvm::Type::getInt32Ty(context()));

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

    if (memberName == "len") {
      // 使用 CreateStructGEP 访问 len 字段（索引为 1）
      llvm::Value *lenFieldPtr =
          builder()->CreateStructGEP(sliceStructType, object, 1, "slice.len");
      // 加载 len 字段的值
      return builder()->CreateLoad(builder()->getInt32Ty(), lenFieldPtr,
                                   "slice.len");
    } else if (memberName == "ptr") {
      // 使用 CreateStructGEP 访问 ptr 字段（索引为 0）
      llvm::Value *ptrFieldPtr =
          builder()->CreateStructGEP(sliceStructType, object, 0, "slice.ptr");
      // 加载 ptr 字段的值
      // 使用与 generateArrayInitExpr 函数相同的指针类型
      llvm::Type *ptrType =
          llvm::PointerType::get(llvm::Type::getInt32Ty(context()), 0);
      return builder()->CreateLoad(ptrType, ptrFieldPtr, "slice.ptr");
    }
  }

  // 处理其他类型的成员访问
  if (object->getType()->isStructTy()) {
    llvm::StructType *structType =
        dyn_cast<llvm::StructType>(object->getType());
    if (structType) {
      std::string structName = structType->getName().str();

      // 检查是否是切片类型
      if (structName.find("Slice_") == 0) {
        // 切片类型的属性访问
        if (memberName == "len") {
          // 返回 len 字段（索引 1）
          return builder()->CreateExtractValue(object, 1, "slice.len");
        } else if (memberName == "ptr") {
          // 返回 ptr 字段（索引 0）
          return builder()->CreateExtractValue(object, 0, "slice.ptr");
        }
      }
    }
  }

  // 检查 object 是否为指针类型
  if (object->getType()->isPointerTy()) {
    // 指针类型，使用 -> 操作符
    // 对于成员变量，需要解引用指针
    // 对于成员函数，直接传递指针作为 this 参数

    // 这里简化处理，假设是成员函数调用
    // 实际应该根据成员类型判断

    // 构建函数名，格式为 "ClassName_MemberName"
    std::string className = "Object";
    std::string funcName = className + "_" + memberName;

    // 查找函数
    auto it = functions_.find(funcName);
    if (it != functions_.end()) {
      llvm::Function *func = it->second;
      // 调用成员函数，传入 this 指针
      return builder()->CreateCall(func, {object});
    }
  } else {
    // 值类型，使用 . 操作符
    // 对于成员变量，直接访问
    // 对于成员函数，需要取地址

    // 这里简化处理，假设是成员函数调用
    // 实际应该根据成员类型判断

    // 构建函数名，格式为 "ClassName_MemberName"
    std::string className = "Object";
    std::string funcName = className + "_" + memberName;

    // 查找函数
    auto it = functions_.find(funcName);
    if (it != functions_.end()) {
      llvm::Function *func = it->second;
      // 取对象地址作为 this 指针
      llvm::Value *objectPtr = builder()->CreatePointerCast(
          object, llvm::PointerType::get(object->getType(), 0));
      // 调用成员函数
      return builder()->CreateCall(func, {objectPtr});
    }
  }

  return nullptr;
}

llvm::Value *LLVMCodeGenerator::generateSubscriptExpr(
    std::unique_ptr<ast::SubscriptExpr> subscriptExpr, bool isLValue) {
  // 生成数组/切片表达式
  llvm::Value *array = generateExpression(std::move(subscriptExpr->object));
  if (!array) {
    return nullptr;
  }

  // 生成索引表达式
  llvm::Value *index = generateExpression(std::move(subscriptExpr->index));
  if (!index) {
    return nullptr;
  }

  // 确保索引类型是整数
  if (!index->getType()->isIntegerTy()) {
    if (index->getType()->isFloatingPointTy()) {
      // 浮点数转换为整数
      index = builder()->CreateFPToSI(index, llvm::Type::getInt32Ty(context()),
                                      "index.cast");
    } else {
      // 其他类型转换为整数
      index = builder()->CreatePointerCast(
          index, llvm::Type::getInt32Ty(context()), "index.cast");
    }
  }

  // 检查是否是切片类型
  // 对于切片类型，我们直接处理，因为我们知道它是一个结构体
  // 假设 object 是指向切片结构体的指针
  // 生成切片类型名称
  std::string sliceTypeName =
      "Slice_" + getLLVMTypeName(llvm::Type::getInt32Ty(context()));

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

  // 获取切片的 ptr 字段
  llvm::Value *ptrFieldPtr =
      builder()->CreateStructGEP(sliceStructType, array, 0, "slice.ptr");
  // 加载 ptr 字段的值
  // 使用与 generateArrayInitExpr 函数相同的指针类型
  llvm::Type *ptrType =
      llvm::PointerType::get(llvm::Type::getInt32Ty(context()), 0);
  llvm::Value *ptr = builder()->CreateLoad(ptrType, ptrFieldPtr, "slice.ptr");

  // 获取切片的 len 字段
  llvm::Value *lenFieldPtr =
      builder()->CreateStructGEP(sliceStructType, array, 1, "slice.len");
  llvm::Value *len =
      builder()->CreateLoad(builder()->getInt32Ty(), lenFieldPtr, "slice.len");

  // 假设元素类型是 int32
  llvm::Type *elementType = builder()->getInt32Ty();

  // 计算元素地址
  // 注意：ptr 已经是指向元素的指针，所以我们直接使用它
  llvm::Value *elementPtr =
      builder()->CreateGEP(elementType, ptr, index, "element.ptr");

  if (isLValue) {
    // 返回元素地址作为左值
    return elementPtr;
  } else {
    // 加载元素值作为右值
    return builder()->CreateLoad(elementType, elementPtr, "element.value");
  }
}

llvm::Value *
LLVMCodeGenerator::generateSelfExpr(std::unique_ptr<ast::SelfExpr> selfExpr) {
  // 获取当前函数
  if (!currentFunction_) {
    return nullptr;
  }

  // 检查当前函数是否是成员函数（是否有 self 指针参数）
  if (currentFunction_->arg_size() == 0) {
    // 不是成员函数，没有 self 指针
    return nullptr;
  }

  // 获取第一个参数（假设是 self 指针）
  auto argIt = currentFunction_->arg_begin();
  llvm::Value *selfPtr = argIt;

  return selfPtr;
}

llvm::Value *
LLVMCodeGenerator::generateNewExpr(std::unique_ptr<ast::NewExpr> newExpr) {
  // 获取类型
  ast::Type *type = dynamic_cast<ast::Type *>(newExpr->type.get());
  if (!type) {
    // 类型解析失败
    return nullptr;
  }

  // 生成 LLVM 类型
  llvm::Type *llvmType = generateType(type);
  if (!llvmType) {
    return nullptr;
  }

  // 计算所需内存大小
  llvm::Value *size = llvm::ConstantExpr::getSizeOf(llvmType);

  // 获取 malloc 函数
  llvm::FunctionType *mallocType = llvm::FunctionType::get(
      llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0),
      {llvm::Type::getInt64Ty(context())}, false);
  llvm::FunctionCallee mallocFunc =
      module()->getOrInsertFunction("malloc", mallocType);

  // 调用 malloc
  llvm::Value *ptr = builder()->CreateCall(mallocFunc, {size}, "malloc.result");

  // 类型转换
  llvm::Value *typedPtr = builder()->CreatePointerCast(
      ptr, llvm::PointerType::get(llvmType, 0), "typed.ptr");

  // 处理构造函数调用（如果有参数）
  if (!newExpr->args.empty()) {
    // 这里需要实现构造函数调用逻辑
    // 暂时跳过，因为还没有实现构造函数代码生成
  }

  return typedPtr;
}

llvm::Value *LLVMCodeGenerator::generateDeleteExpr(
    std::unique_ptr<ast::DeleteExpr> deleteExpr) {
  // 生成表达式，获取要释放的指针
  llvm::Value *ptr = generateExpression(std::move(deleteExpr->expr));
  if (!ptr) {
    return nullptr;
  }

  // 检查是否为指针类型
  if (!ptr->getType()->isPointerTy()) {
    // 不是指针类型，直接返回
    return nullptr;
  }

  // 检查是否为类类型指针，需要调用析构函数
  // 这里简化处理，假设所有指针都是类类型
  // 实际应该根据类型信息判断

  // 查找析构函数
  // 假设析构函数命名为 `ClassName_~ClassName`
  // 这里简化处理，使用默认的析构函数名
  std::string destructorName = "~default";

  // 尝试查找析构函数
  auto it = functions_.find(destructorName);
  if (it != functions_.end()) {
    llvm::Function *destructor = it->second;
    // 调用析构函数，传入 this 指针
    builder()->CreateCall(destructor, {ptr});
  }

  // 获取 free 函数
  llvm::FunctionType *freeType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(context()),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0)}, false);
  llvm::FunctionCallee freeFunc =
      module()->getOrInsertFunction("free", freeType);

  // 类型转换为 void*
  llvm::Value *voidPtr = builder()->CreatePointerCast(
      ptr, llvm::PointerType::get(llvm::Type::getInt8Ty(context()), 0),
      "void.ptr");

  // 调用 free
  builder()->CreateCall(freeFunc, {voidPtr});

  return nullptr;
}

llvm::Value *
LLVMCodeGenerator::generateFoldExpr(std::unique_ptr<ast::FoldExpr> foldExpr) {
  // 生成要折叠的表达式
  llvm::Value *exprValue = generateExpression(std::move(foldExpr->expr));
  if (!exprValue) {
    return nullptr;
  }

  // 生成右侧表达式（如果有）
  llvm::Value *rightValue = nullptr;
  if (foldExpr->right) {
    rightValue = generateExpression(std::move(foldExpr->right));
    if (!rightValue) {
      return nullptr;
    }
  }

  // 折叠表达式的实现比较复杂，需要根据折叠类型和操作符进行处理
  // 暂时返回 nullptr，实际实现需要根据具体语义进行处理
  return nullptr;
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

} // namespace llvm_codegen
} // namespace c_hat
