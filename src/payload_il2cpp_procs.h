#pragma once

/* Domains / assemblies / images */
void* (*il2cpp_domain_get)();
void* (*il2cpp_domain_assembly_open)(void* domain, const char* assembly);
void* (*il2cpp_assembly_get_image)(void* assembly);

/* Classes */
void* (*il2cpp_class_from_name)(void* image, const char* namespaze, const char* name);
void* (*il2cpp_class_from_type)(void* type);
char* (*il2cpp_class_get_name)(void* klass);
char* (*il2cpp_class_get_namespace)(void* klass);
char* (*il2cpp_class_get_assemblyname)(void* klass);
void* (*il2cpp_class_get_parent)(void* klass);
bool (*il2cpp_class_is_valuetype)(void* klass);
void* (*il2cpp_class_get_field_from_name)(void* klass, const char* name);

/* Methods */
void* (*il2cpp_class_get_method_from_name)(void* klass, const char* name, uint32_t argsCount);
void* (*il2cpp_class_get_methods)(void* klass, void** iter);
void* (*il2cpp_method_get_class)(void* method);
char* (*il2cpp_method_get_name)(void* method);
int32_t (*il2cpp_method_get_param_count)(void* method);
void* (*il2cpp_method_get_param)(void* method, int32_t index);
void* (*il2cpp_method_get_return_type)(void* method);
uint32_t (*il2cpp_method_get_flags)(void* method, uint32_t* iflags);

/* Types */
char* (*il2cpp_type_get_name)(void* type);
int (*il2cpp_type_get_type)(void* type);

/* Strings */
int32_t (*il2cpp_string_length)(void* string);
uint16_t* (*il2cpp_string_chars)(void* string);

/* Objects / boxing */
void* (*il2cpp_object_get_class)(void* object);
void* (*il2cpp_object_new)(void* klass);
void* (*il2cpp_object_unbox)(void* object);
void* (*il2cpp_value_box)(void* klass, void* data);

/* Fields */
void* (*il2cpp_class_get_fields)(void* klass, void** iter);
const char* (*il2cpp_field_get_name)(void* field);
void* (*il2cpp_field_get_type)(void* field);
void (*il2cpp_field_get_value)(void* object, void* field, void* value);
void* (*il2cpp_field_get_value_object)(void* field, void* object);
size_t (*il2cpp_field_get_offset)(void* field);
uint32_t (*il2cpp_field_get_flags)(void* field);

/* Exceptions */
void (*il2cpp_format_exception)(
    void* exception,
    char* message,
    int messageSize);

/* Special runtime methods */
void (*il2cpp_init)(const char* domainName);
void* (*il2cpp_runtime_invoke)(
    void* method,
    void* object,
    void** params,
    void** exc);
void (*il2cpp_free)(void* ptr);

#define LOAD_IL2CPP_PROC(name) \
    do { name = (decltype(name))GetProcAddress(hModule, #name); } while (0)

/*
    Pass in the loaded il2cpp library's hModule.
*/
void il2CppProcsInit(HMODULE hModule) {
    LOAD_IL2CPP_PROC(il2cpp_domain_get);
    LOAD_IL2CPP_PROC(il2cpp_domain_assembly_open);
    LOAD_IL2CPP_PROC(il2cpp_assembly_get_image);

    LOAD_IL2CPP_PROC(il2cpp_class_from_name);
    LOAD_IL2CPP_PROC(il2cpp_class_from_type);
    LOAD_IL2CPP_PROC(il2cpp_class_get_name);
    LOAD_IL2CPP_PROC(il2cpp_class_get_namespace);
    LOAD_IL2CPP_PROC(il2cpp_class_get_assemblyname);
    LOAD_IL2CPP_PROC(il2cpp_class_get_parent);
    LOAD_IL2CPP_PROC(il2cpp_class_is_valuetype);
    LOAD_IL2CPP_PROC(il2cpp_class_get_field_from_name);

    LOAD_IL2CPP_PROC(il2cpp_class_get_method_from_name);
    LOAD_IL2CPP_PROC(il2cpp_class_get_methods);
    LOAD_IL2CPP_PROC(il2cpp_method_get_class);
    LOAD_IL2CPP_PROC(il2cpp_method_get_name);
    LOAD_IL2CPP_PROC(il2cpp_method_get_param_count);
    LOAD_IL2CPP_PROC(il2cpp_method_get_param);
    LOAD_IL2CPP_PROC(il2cpp_method_get_return_type);
    LOAD_IL2CPP_PROC(il2cpp_method_get_flags);

    LOAD_IL2CPP_PROC(il2cpp_type_get_name);
    LOAD_IL2CPP_PROC(il2cpp_type_get_type);

    LOAD_IL2CPP_PROC(il2cpp_string_length);
    LOAD_IL2CPP_PROC(il2cpp_string_chars);

    LOAD_IL2CPP_PROC(il2cpp_object_get_class);
    LOAD_IL2CPP_PROC(il2cpp_object_new);
    LOAD_IL2CPP_PROC(il2cpp_object_unbox);
    LOAD_IL2CPP_PROC(il2cpp_value_box);

    LOAD_IL2CPP_PROC(il2cpp_class_get_fields);
    LOAD_IL2CPP_PROC(il2cpp_field_get_name);
    LOAD_IL2CPP_PROC(il2cpp_field_get_type);
    LOAD_IL2CPP_PROC(il2cpp_field_get_value);
    LOAD_IL2CPP_PROC(il2cpp_field_get_value_object);
    LOAD_IL2CPP_PROC(il2cpp_field_get_offset);
    LOAD_IL2CPP_PROC(il2cpp_field_get_flags);

    LOAD_IL2CPP_PROC(il2cpp_format_exception);

    LOAD_IL2CPP_PROC(il2cpp_init);
    LOAD_IL2CPP_PROC(il2cpp_runtime_invoke);
    LOAD_IL2CPP_PROC(il2cpp_free);
}

#undef LOAD_IL2CPP_PROC