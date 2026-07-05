#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "MinHook.h"
#include "payload_il2cpp_dbg.h"
#include "payload_il2cpp_procs.h"
#include "payload_il2cpp_utils.h"

/*
    Patch declarations
*/

bool (*Original_il2cpp_init)(const char* domain_name);
static bool Patched_il2cpp_init(const char* domain_name);

void (*Original_OpenDefeatScreen)(void* __this, void* menuData, void* methodInfo);
static void Patched_OpenDefeatScreen(void* __this, void* menuData, void* methodInfo);

void* (*Original_GetContinueCost)(void* __this, void* methodInfo);
static void* Patched_GetContinueCost(void* __this, void* methodInfo);

/*
    Utilities
*/

static const char* workDir() {
    thread_local char pwd[MAX_PATH]{};
    if (pwd[0] == '\0')
        GetCurrentDirectoryA(MAX_PATH, pwd);
    return pwd;
}

/*
    Exports
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

    il2CppProcsInit(hModule);

    /* Install il2cpp_init hook */
    return
        MH_CreateHook(
            il2cpp_init,
            Patched_il2cpp_init,
            (void**)&Original_il2cpp_init)
        != MH_STATUS::MH_OK ||
        MH_EnableHook(MH_ALL_HOOKS)
        != MH_STATUS::MH_OK
            ? 2
            : 0;
}

/*
    Patches
*/

static bool Patched_il2cpp_init(const char* domain_name) {
    if (!Original_il2cpp_init(domain_name))
        return false;

    void* defeatScreenClass = getIl2CppClass(
        "Assembly-CSharp.dll",
        "Assets.Scripts.Unity.UI_New.GameOver",
        "DefeatScreen");

    if (defeatScreenClass == nullptr)
        return false;

    void* defeatScreenOpenMethod = getIl2CppClassMethod(
        defeatScreenClass,
        "Open",
        { "System.Object" });

    if (defeatScreenOpenMethod == nullptr)
        return false;

    void* inGameClass = getIl2CppClass(
        "Assembly-CSharp.dll",
        "Assets.Scripts.Unity.UI_New.InGame",
        "InGame");

    if (inGameClass == nullptr)
        return false;

    void* getContinueCostMethod = getIl2CppClassMethod(
        inGameClass,
        "GetContinueCost",
        {});

    if (getContinueCostMethod == nullptr)
        return false;

    return
        MH_CreateHook(
            *(void**)defeatScreenOpenMethod,
            Patched_OpenDefeatScreen,
            (void **)&Original_OpenDefeatScreen)
        == MH_STATUS::MH_OK &&
        MH_CreateHook(
            *(void**)getContinueCostMethod,
            Patched_GetContinueCost,
            (void**)&Original_GetContinueCost)
        == MH_STATUS::MH_OK &&
        MH_EnableHook(MH_ALL_HOOKS)
        == MH_STATUS::MH_OK;
}

static void* Patched_GetContinueCost(void* __this, void* methodInfo) {
    invokeIl2CppObjectMethod(__this, "set_ContinuesUsed",
        { "System.Int32" },
        { (void*)-1 });

    return createKonFuzeInteger(0);
}

static void Patched_OpenDefeatScreen(void* __this, void* menuData, void* methodInfo) {
    *((void**)getIl2CppObjectMemberAddr(
        menuData,
        "continuePrice")) = createKonFuzeInteger(0);

    Original_OpenDefeatScreen(__this, menuData, methodInfo);
}