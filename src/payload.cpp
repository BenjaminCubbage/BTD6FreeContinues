#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include "MinHook.h"

/*
    Forward declarations
*/

static bool Patched_il2cpp_init(const char* domain_name);
static void* Patched_GetContinueCost(void* __this, void* method);

/*
    Global variables
*/

bool (*Original_il2cpp_init)(const char* domain_name);
void* (*Original_GetContinueCost)(void* __this, void* method);

/*
    Il2cpp procedures
*/

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
bool (*il2cpp_class_is_valuetype)(void* klass);

/* Methods */
void* (*il2cpp_class_get_method_from_name)(void* klass, const char* name, uint32_t argsCount);
void* (*il2cpp_class_get_methods)(void* klass, void** iter);
char* (*il2cpp_method_get_name)(void* method);
int32_t(*il2cpp_method_get_param_count)(void* method);
void* (*il2cpp_method_get_param)(void* method, int32_t index);
void* (*il2cpp_method_get_return_type)(void* method);
uint32_t(*il2cpp_method_get_flags)(void* method, uint32_t* iflags);

/* Types */
char* (*il2cpp_type_get_name)(void* type);

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
size_t(*il2cpp_field_get_offset)(void* field);

/* Special runtime methods */
void* (*il2cpp_runtime_invoke)(void* method, void* object, void** params, void** exc);
void (*il2cpp_free)(void* ptr);

#define LOAD_IL2CPP_PROC(name)                                 \
    do {                                                       \
        name = (decltype(name))GetProcAddress(hModule, #name); \
    } while (0)

/*
    Utilities
*/

static const char* workDir() {
    thread_local char pwd[MAX_PATH]{};
    if (pwd[0] == '\0')
        GetCurrentDirectoryA(MAX_PATH, pwd);
    return pwd;
}

static void* getIl2CppClass(
        const std::string& assemblyName, 
        const std::string& namespaceName, 
        const std::string& className) {
    void* domain = il2cpp_domain_get();
    void* assembly = il2cpp_domain_assembly_open(domain, assemblyName.c_str());
    void* image = il2cpp_assembly_get_image(assembly);
    return il2cpp_class_from_name(
        image,
        namespaceName.c_str(),
        className.c_str());
}

static void* getIl2CppClassMethod(
        void* klass,
        const std::string& methodName,
        const std::vector<std::string>& argumentTypeNames) {
    void* iter{};
    void* method{};

    while ((method = il2cpp_class_get_methods(klass, &iter)) != nullptr) {
        if (il2cpp_method_get_name(method) == methodName) {
            if (argumentTypeNames.size() != il2cpp_method_get_param_count(method))
                continue;

            bool isMatch{ true };
            for (size_t i{}; i < argumentTypeNames.size(); ++i) {
                void* type = il2cpp_method_get_param(method, (int32_t)i);
                char* typeName = il2cpp_type_get_name(type);

                if (typeName != argumentTypeNames[i]) {
                    isMatch = false;
                    break;
                }
            }
            
            if (isMatch)
                return method;
        }
    }

    return nullptr;
}

static void* createKonFuzeInteger(int32_t value) {
    thread_local void* klass{};
    thread_local void* ctor{};

    if (klass == nullptr) {
        klass = getIl2CppClass(
            "Assembly-CSharp.dll",
            "Assets.Scripts.Utils",
            "KonFuze");
        ctor = getIl2CppClassMethod(
            klass, 
            "op_Implicit", 
            { "System.Int32" });
    }

    if (klass == nullptr || ctor == nullptr)
        return nullptr;

    void* exc = nullptr;
    void* arg = &value;
    return il2cpp_runtime_invoke(ctor, nullptr, &arg, &exc);
}

static bool setContinueCount(void* inGameObj, int value) {
    void* method = getIl2CppClassMethod(
        il2cpp_object_get_class(inGameObj), 
        "set_ContinuesUsed",
        { "System.Int32" });

    void* exc = nullptr;
    void* arg = &value;
    il2cpp_runtime_invoke(method, inGameObj, &arg, &exc);

    return exc == nullptr;
}

/*
    Entrypoints
*/

extern "C" __declspec(dllexport)
bool DllMain(HINSTANCE, DWORD, LPVOID) {
    return MH_Initialize() == MH_STATUS::MH_OK;
}

extern "C" __declspec(dllexport)
int Run(void*) {
    const HMODULE hModule = LoadLibraryA(
        (workDir() + std::string{ "\\GameAssembly.dll" }).c_str());

    if (hModule == nullptr)
        return 1;

    /* Install il2cpp_init hook */
    const FARPROC initProc = GetProcAddress(hModule, "il2cpp_init");

    if (MH_CreateHook(
            initProc,
            Patched_il2cpp_init,
            (void**)&Original_il2cpp_init) != MH_STATUS::MH_OK ||
        MH_EnableHook(MH_ALL_HOOKS) != MH_STATUS::MH_OK) {
        return 2;
    }

    /* Get proc addresses */
    LOAD_IL2CPP_PROC(il2cpp_domain_get);
    LOAD_IL2CPP_PROC(il2cpp_domain_assembly_open);
    LOAD_IL2CPP_PROC(il2cpp_assembly_get_image);

    LOAD_IL2CPP_PROC(il2cpp_class_from_name);
    LOAD_IL2CPP_PROC(il2cpp_class_from_type);
    LOAD_IL2CPP_PROC(il2cpp_class_get_name);
    LOAD_IL2CPP_PROC(il2cpp_class_get_namespace);
    LOAD_IL2CPP_PROC(il2cpp_class_get_assemblyname);
    LOAD_IL2CPP_PROC(il2cpp_class_is_valuetype);

    LOAD_IL2CPP_PROC(il2cpp_class_get_method_from_name);
    LOAD_IL2CPP_PROC(il2cpp_class_get_methods);
    LOAD_IL2CPP_PROC(il2cpp_method_get_name);
    LOAD_IL2CPP_PROC(il2cpp_method_get_param_count);
    LOAD_IL2CPP_PROC(il2cpp_method_get_param);
    LOAD_IL2CPP_PROC(il2cpp_method_get_return_type);
    LOAD_IL2CPP_PROC(il2cpp_method_get_flags);

    LOAD_IL2CPP_PROC(il2cpp_type_get_name);

    LOAD_IL2CPP_PROC(il2cpp_object_get_class);
    LOAD_IL2CPP_PROC(il2cpp_object_new);
    LOAD_IL2CPP_PROC(il2cpp_object_unbox);
    LOAD_IL2CPP_PROC(il2cpp_value_box);

    LOAD_IL2CPP_PROC(il2cpp_class_get_fields);
    LOAD_IL2CPP_PROC(il2cpp_field_get_name);
    LOAD_IL2CPP_PROC(il2cpp_field_get_type);
    LOAD_IL2CPP_PROC(il2cpp_field_get_value);
    LOAD_IL2CPP_PROC(il2cpp_field_get_offset);

    LOAD_IL2CPP_PROC(il2cpp_runtime_invoke);
    LOAD_IL2CPP_PROC(il2cpp_free);
    return 0;
}

/*
    Patches
*/

static bool Patched_il2cpp_init(const char* domain_name) {
    Original_il2cpp_init(domain_name);
    
    void* klass = getIl2CppClass(
        "Assembly-CSharp.dll",
        "Assets.Scripts.Unity.UI_New.InGame",
        "InGame");

    void* mmCostMethod = getIl2CppClassMethod(klass, "GetContinueCost", {});

    if (!mmCostMethod)
        return false;

    return MH_CreateHook(
            *(void**)(mmCostMethod),
            Patched_GetContinueCost,
            (void **)&Original_GetContinueCost) == MH_STATUS::MH_OK &&
        MH_EnableHook(MH_ALL_HOOKS) == MH_STATUS::MH_OK;
}

static void* Patched_GetContinueCost(void* __this, void* methodInfo) {
    void* oldCost = Original_GetContinueCost(__this, methodInfo);
    void* newCost = createKonFuzeInteger(0);

    if (newCost == nullptr)
        return nullptr;

    return setContinueCount(__this, -1)
        ? newCost
        : oldCost;
}