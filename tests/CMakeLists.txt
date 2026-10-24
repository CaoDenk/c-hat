# Catch2 is located by the top-level CMakeLists.txt

add_subdirectory(parser)
add_subdirectory(semantic)
add_subdirectory(class_system)
add_subdirectory(exception)
add_subdirectory(array)
add_subdirectory(defer)
add_subdirectory(new_delete)
add_subdirectory(tuple)
add_subdirectory(types)
add_subdirectory(lambda)
add_subdirectory(variadic)
add_subdirectory(property)
add_subdirectory(pointer)
add_subdirectory(late)
add_subdirectory(match)
add_subdirectory(immutable_method)
add_subdirectory(result_type)
add_subdirectory(overload)
add_subdirectory(access_control)
add_subdirectory(foreach)
add_subdirectory(generics)
add_subdirectory(module)
add_subdirectory(nullable)
add_subdirectory(reference)
add_subdirectory(static)
add_subdirectory(string_literal)
add_subdirectory(extension)
add_subdirectory(goto)
add_subdirectory(ffi)
add_subdirectory(coroutine)
add_subdirectory(attribute)
add_subdirectory(builtin_vars)
add_subdirectory(reflection)
add_subdirectory(extra_expressions)
add_subdirectory(integration)

# Register every Catch2 test executable with CTest so `ctest` works on all
# platforms (the test binaries themselves are still runnable directly).
set(C_HAT_TEST_TARGETS
    array_catch2_test
    attribute_catch2_test
    builtin_vars_catch2_test
    class_catch2_test
    coroutine_catch2_test
    defer_catch2_test
    exception_catch2_test
    extension_catch2_test
    extra_expr_catch2_test
    ffi_catch2_test
    foreach_catch2_test
    generics_catch2_test
    goto_catch2_test
    immutable_method_catch2_test
    integration_catch2_test
    lambda_catch2_test
    late_catch2_test
    match_catch2_test
    module_catch2_test
    new_delete_catch2_test
    nullable_catch2_test
    overload_catch2_test
    parser_catch2_test
    pointer_catch2_test
    property_catch2_test
    reference_catch2_test
    reflection_catch2_test
    result_type_catch2_test
    semantic_catch2_test
    static_catch2_test
    string_literal_catch2_test
    tuple_catch2_test
    variadic_catch2_test
)

foreach(test_target IN LISTS C_HAT_TEST_TARGETS)
    if(TARGET ${test_target})
        add_test(NAME ${test_target} COMMAND ${test_target})
    endif()
endforeach()
