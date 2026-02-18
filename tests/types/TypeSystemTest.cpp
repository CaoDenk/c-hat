#include "../src/types/Types.h"
#include <catch2/catch_test_macros.hpp>

using namespace c_hat::types;

TEST_CASE("Primitive types", "[types]") {
  SECTION("Type creation") {
    auto voidType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Void);
    auto boolType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Bool);
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto doubleType =
        TypeFactory::getPrimitiveType(PrimitiveType::Kind::Double);
    auto charType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Char);
    auto fp16Type = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Fp16);
    auto bf16Type = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Bf16);

    REQUIRE(voidType != nullptr);
    REQUIRE(boolType != nullptr);
    REQUIRE(intType != nullptr);
    REQUIRE(doubleType != nullptr);
    REQUIRE(charType != nullptr);
    REQUIRE(fp16Type != nullptr);
    REQUIRE(bf16Type != nullptr);
  }

  SECTION("Type compatibility") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto doubleType =
        TypeFactory::getPrimitiveType(PrimitiveType::Kind::Double);

    REQUIRE(intType->isCompatibleWith(*intType) == true);
    REQUIRE(intType->isCompatibleWith(*doubleType) == false);
  }

  SECTION("Type hierarchy") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto doubleType =
        TypeFactory::getPrimitiveType(PrimitiveType::Kind::Double);

    REQUIRE(intType->isSubtypeOf(*intType) == true);
    REQUIRE(intType->isSubtypeOf(*doubleType) == false);
  }
}

TEST_CASE("Array types", "[types]") {
  SECTION("Array type creation") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto arrayType = TypeFactory::getArrayType(intType, 10);

    REQUIRE(arrayType != nullptr);
    REQUIRE(arrayType->toString() == "int[10]");
  }

  SECTION("Array type compatibility") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto arrayType = TypeFactory::getArrayType(intType, 10);
    auto sameArrayType = TypeFactory::getArrayType(intType, 10);
    auto differentSizeArrayType = TypeFactory::getArrayType(intType, 20);

    REQUIRE(arrayType->isCompatibleWith(*sameArrayType) == true);
    REQUIRE(arrayType->isCompatibleWith(*differentSizeArrayType) == false);
  }

  SECTION("Array to slice relationship") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto arrayType = TypeFactory::getArrayType(intType, 10);
    auto sliceType = TypeFactory::getSliceType(intType);

    REQUIRE(arrayType->isSubtypeOf(*sliceType) == true);
  }
}

TEST_CASE("Slice types", "[types]") {
  SECTION("Slice type creation") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto sliceType = TypeFactory::getSliceType(intType);

    REQUIRE(sliceType != nullptr);
    REQUIRE(sliceType->toString() == "int[]");
  }

  SECTION("Slice type compatibility") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto sliceType = TypeFactory::getSliceType(intType);
    auto sameSliceType = TypeFactory::getSliceType(intType);
    auto differentElementTypeSliceType = TypeFactory::getSliceType(
        TypeFactory::getPrimitiveType(PrimitiveType::Kind::Double));

    REQUIRE(sliceType->isCompatibleWith(*sameSliceType) == true);
    REQUIRE(sliceType->isCompatibleWith(*differentElementTypeSliceType) ==
            false);
  }
}

TEST_CASE("Pointer types", "[types]") {
  SECTION("Pointer type creation") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto pointerType = TypeFactory::getPointerType(intType);

    REQUIRE(pointerType != nullptr);
    REQUIRE(pointerType->toString() == "int^");
  }

  SECTION("Pointer type compatibility") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto pointerType = TypeFactory::getPointerType(intType);
    auto samePointerType = TypeFactory::getPointerType(intType);

    REQUIRE(pointerType->isCompatibleWith(*samePointerType) == true);
  }

  SECTION("Pointer type hierarchy") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto pointerType = TypeFactory::getPointerType(intType);

    REQUIRE(pointerType->isSubtypeOf(*pointerType) == true);
  }
}

TEST_CASE("Function types", "[types]") {
  SECTION("Function type creation") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto doubleType =
        TypeFactory::getPrimitiveType(PrimitiveType::Kind::Double);
    std::vector<std::shared_ptr<Type>> paramTypes = {intType, doubleType};
    auto funcType = TypeFactory::getFunctionType(intType, paramTypes);

    REQUIRE(funcType != nullptr);
    REQUIRE(funcType->toString() == "func(int, double) -> int");
  }

  SECTION("Function type compatibility") {
    auto intType = TypeFactory::getPrimitiveType(PrimitiveType::Kind::Int);
    auto doubleType =
        TypeFactory::getPrimitiveType(PrimitiveType::Kind::Double);
    std::vector<std::shared_ptr<Type>> paramTypes = {intType, doubleType};
    auto funcType = TypeFactory::getFunctionType(intType, paramTypes);
    auto sameFuncType = TypeFactory::getFunctionType(intType, paramTypes);
    auto differentReturnTypeFuncType =
        TypeFactory::getFunctionType(doubleType, paramTypes);

    REQUIRE(funcType->isCompatibleWith(*sameFuncType) == true);
    REQUIRE(funcType->isCompatibleWith(*differentReturnTypeFuncType) == false);
  }
}

TEST_CASE("Class types", "[types]") {
  SECTION("Class type creation") {
    auto classType = TypeFactory::getClassType("MyClass");

    REQUIRE(classType != nullptr);
    REQUIRE(classType->toString() == "MyClass");
  }

  SECTION("Class type compatibility") {
    auto classType = TypeFactory::getClassType("MyClass");
    auto sameClassType = TypeFactory::getClassType("MyClass");
    auto differentClassType = TypeFactory::getClassType("OtherClass");

    REQUIRE(classType->isCompatibleWith(*sameClassType) == true);
    REQUIRE(classType->isCompatibleWith(*differentClassType) == false);
  }

  SECTION("Class type hierarchy") {
    auto baseClassType = TypeFactory::getClassType("BaseClass");
    auto derivedClassType = std::static_pointer_cast<ClassType>(
        TypeFactory::getClassType("DerivedClass"));
    derivedClassType->addBaseClass(
        std::static_pointer_cast<ClassType>(baseClassType));

    REQUIRE(derivedClassType->isSubtypeOf(*baseClassType) == true);
    REQUIRE(baseClassType->isSubtypeOf(*derivedClassType) == false);
  }
}
