#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include "payload_il2cpp_procs.h"

#define FIELD_ATTRIBUTE_STATIC 0x0010

/*
    Convert the UTF-16 IL2CPP string to a UTF-8 std::string.
*/
static std::string dbgIl2CppStringToUtf8(void* string) {
    const uint16_t* chars = il2cpp_string_chars(string);
    int32_t length        = il2cpp_string_length(string);

    if (length == 0)
        return "";

    int utf8Length = WideCharToMultiByte(
        CP_UTF8,
        0,
        (const wchar_t*)chars,
        length,
        nullptr,
        0,
        nullptr,
        nullptr);

    if (utf8Length == 0)
        return "<UTF-16 conversion failed>";

    std::string result((size_t)utf8Length, '\0');

    int convertedLength = WideCharToMultiByte(
        CP_UTF8,
        0,
        (const wchar_t*)chars,
        length,
        result.data(),
        utf8Length,
        nullptr,
        nullptr);

    if (convertedLength == 0)
        return "<UTF-16 conversion failed>";

    return result;
}

/*
    Dump objects
*/

/*
    Call ToString() on an IL2CPP object.

    For boxed value types whose ToString() method is declared by a value
    type, il2cpp_runtime_invoke() expects a pointer to the unboxed data.
*/
static std::string dbgIl2CppObjectToString(void* object) {
    if (object == nullptr)
        return "null";

    void* klass = il2cpp_object_get_class(object);

    void* toStringMethod = il2cpp_class_get_method_from_name(
        klass,
        "ToString",
        0);

    if (toStringMethod == nullptr)
        return "<ToString unavailable>";

    void* methodClass = il2cpp_method_get_class(toStringMethod);

    void* invokeTarget =
        il2cpp_class_is_valuetype(methodClass)
            ? il2cpp_object_unbox(object)
            : object;

    void* exception{};
    void* result = il2cpp_runtime_invoke(
        toStringMethod,
        invokeTarget,
        nullptr,
        &exception);

    if (exception != nullptr) {
        char exceptionText[1024]{};

        il2cpp_format_exception(
            exception,
            exceptionText,
            sizeof(exceptionText));

        exceptionText[sizeof(exceptionText) - 1] = '\0';

        return std::string{ "<ToString threw: " } + exceptionText + '>';
    }

    if (result == nullptr)
        return "null";

    return dbgIl2CppStringToUtf8(result);
}

static void dbgDumpIl2CppClassFieldsImpl(
        std::ostringstream& output,
        void* klass) {
    void* parent = il2cpp_class_get_parent(klass);

    if (parent != nullptr)
        dbgDumpIl2CppClassFieldsImpl(output, parent);

    void* iter{};
    void* field{};

    while ((field = il2cpp_class_get_fields(
                klass,
                &iter)) != nullptr) {
        const char* fieldName = il2cpp_field_get_name(field);
        void* fieldType       = il2cpp_field_get_type(field);
        bool fieldIsStatic    = il2cpp_field_get_flags(field) & FIELD_ATTRIBUTE_STATIC;
        char* typeName        = il2cpp_type_get_name(fieldType);

        output
            << (fieldIsStatic        ? "static " : "")
            << (typeName  != nullptr ? typeName  : "<unknown type>") << ' '
            << (fieldName != nullptr ? fieldName : "<unnamed field>")
            << '\n';

        if (typeName != nullptr)
            il2cpp_free(typeName);
    }
}

static std::string dbgDumpIl2CppClassFields(
        void* klass) {
    std::ostringstream output;

    dbgDumpIl2CppClassFieldsImpl(
        output,
        klass);

    return output.str();
}

static void dbgDumpIl2CppObjectFieldsImpl(
        std::ostringstream& output,
        void* object,
        void* klass) {
    void* parent = il2cpp_class_get_parent(klass);

    if (parent != nullptr) {
        dbgDumpIl2CppObjectFieldsImpl(
            output,
            object,
            parent);
    }

    void* iter{};
    void* field{};

    while ((field = il2cpp_class_get_fields(
                klass,
                &iter)) != nullptr) {
        const char* fieldName = il2cpp_field_get_name(field);
        void* fieldType       = il2cpp_field_get_type(field);
        char* typeName        = il2cpp_type_get_name(fieldType);

        output
            << (typeName  != nullptr ? typeName  : "<unknown type>") << ' '
            << (fieldName != nullptr ? fieldName : "<unnamed field>")
            << " = ";

        void* valueObject =
            il2cpp_field_get_value_object(
                field,
                object);

        output << dbgIl2CppObjectToString(valueObject)
               << '\n';

        if (typeName != nullptr)
            il2cpp_free(typeName);
    }
}

static std::string dbgDumpIl2CppObjectFields(
        void* object) {
    std::ostringstream output;

    dbgDumpIl2CppObjectFieldsImpl(
        output,
        object,
        il2cpp_object_get_class(object));

    return output.str();
}

/*
    Dump methods
*/

static void dbgDumpIl2CppClassMethodsImpl(
        std::ostringstream& output,
        void* klass) {
    void* parent = il2cpp_class_get_parent(klass);

    if (parent != nullptr)
        dbgDumpIl2CppClassMethodsImpl(output, parent);

    void* iter{};
    void* method{};

    while ((method = il2cpp_class_get_methods(
                klass,
                &iter)) != nullptr) {
        const char* methodName = il2cpp_method_get_name(method);

        void* returnType = il2cpp_method_get_return_type(method);
        char* returnTypeName = il2cpp_type_get_name(returnType);

        output
            << (returnTypeName != nullptr
                    ? returnTypeName
                    : "<unknown type>")
            << ' '
            << (methodName != nullptr
                    ? methodName
                    : "<unnamed method>")
            << '(';

        int32_t parameterCount =
            il2cpp_method_get_param_count(method);

        for (int32_t i{}; i < parameterCount; ++i) {
            void* parameterType =
                il2cpp_method_get_param(
                    method,
                    i);

            char* parameterTypeName =
                il2cpp_type_get_name(parameterType);

            if (i != 0)
                output << ", ";

            output << (parameterTypeName != nullptr
                    ? parameterTypeName
                    : "<unknown type>");

            if (parameterTypeName != nullptr)
                il2cpp_free(parameterTypeName);
        }

        output << ')' << '\n';

        if (returnTypeName != nullptr)
            il2cpp_free(returnTypeName);
    }
}

static std::string dbgDumpIl2CppClassMethods(
        void* klass) {
    std::ostringstream output;

    dbgDumpIl2CppClassMethodsImpl(
        output,
        klass);

    return output.str();
}

static std::string dbgDumpIl2CppObjectMethods(
        void* object) {
    return dbgDumpIl2CppClassMethods(
        il2cpp_object_get_class(object));
}