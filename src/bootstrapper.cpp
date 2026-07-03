#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <unordered_map>

#ifndef BLOONSTD6_ROOT
#error "BLOONSTD6_ROOT was not defined!"
#endif

#ifndef PAYLOAD_DLL
#error "PAYLOAD_DLL was not defined!"
#endif

#ifndef KERNEL32_DLL
#error "KERNEL32_DLL was not defined!"
#endif

/*
    Normalize a file path.
*/
static std::string canonicalizePath(std::string path) {
    return std::filesystem::canonical(path).string();
}

/*
    Get the canonicalized path of a file from its handle.
*/
static std::string getFullPathFromHFile(HANDLE hFile) {
    thread_local char rawPath[MAX_PATH]{};
    const int written = GetFinalPathNameByHandleA(
        hFile,
        rawPath,
        std::size(rawPath),
        FILE_NAME_NORMALIZED);

    return written == 0 || written > std::size(rawPath)
        ? ""
        : canonicalizePath(rawPath);
}

/*
    Get RVA of the procedure relative to the dll image base.
*/
static uintptr_t getProcRVA(const char* dll, const char* procName) {
    HMODULE hModule = LoadLibraryA(dll);
    if (hModule == nullptr)
        return 0;

    uintptr_t procAddr = (uintptr_t)GetProcAddress(hModule, procName);
    FreeLibrary(hModule);

    return procAddr == 0
        ? 0
        : procAddr - (uintptr_t)hModule;
}

int main() {
    const std::string exePath = BLOONSTD6_ROOT "\\BloonsTD6.exe";

    /*
        Start BTD6 as a debugged process

        TODO: Check success
    */
    STARTUPINFOA startupInfo{
        .cb = sizeof(STARTUPINFOA)
    };
    PROCESS_INFORMATION pi{};
    CreateProcessA(
        exePath.c_str(),
        nullptr,
        nullptr,
        nullptr,
        false,
        DEBUG_ONLY_THIS_PROCESS,
        nullptr,
        BLOONSTD6_ROOT,
        &startupInfo,
        &pi);
    DebugSetProcessKillOnExit(true);

    std::cout
        << "BTD6Patcher: Waiting for child process to load kernel32.dll..."
        << std::endl;

    /*
        Keep track of the spawned threads so we can
        suspend them later.
    */
    std::unordered_map<int, HANDLE> spawnedThreads = {
        { pi.dwThreadId, pi.hThread }
    };

    /*
        Save dll bases when they are loaded.
    */
    uintptr_t kernel32Dllbase{};
    uintptr_t payloadDllBase{};

    /*
        Loop through debug events until game assembly is
        loaded or process dies.
    */
    for (DEBUG_EVENT event;;) {
        if (WaitForDebugEvent(&event, 1000) == 0) {
            std::cout
                << "\x1b[91m"
                   "BTD6Patcher: Timed out while waiting for kernel32.dll to load!"
                << "\x1b[0m"
                << std::endl;

            return 1;
        }

        if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            /* Loaded DLL, check if it's kernel32.dll */
            const HANDLE hFile        = event.u.LoadDll.hFile;
            const std::string dllPath = getFullPathFromHFile(hFile);
            CloseHandle(hFile);

            std::cout
                << "\x1b[90m"
                   "BTD6Patcher: Game loaded library "
                << dllPath
                << "\x1b[0m"
                << std::endl;

            if (dllPath == canonicalizePath(KERNEL32_DLL)) {
                /* Found it! */
                kernel32Dllbase = reinterpret_cast<uintptr_t>(
                    event.u.LoadDll.lpBaseOfDll);

                ContinueDebugEvent(
                    event.dwProcessId,
                    event.dwThreadId,
                    DBG_CONTINUE);
                break;
            }
        }

        /* Track spawned threads */
        if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT)
            spawnedThreads.insert_or_assign(
                event.dwThreadId, 
                event.u.CreateThread.hThread);
        else if (event.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
            spawnedThreads.erase(event.dwThreadId);

        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            /* Don't care */
            CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            /* Process died :( */
            std::cout
                << "\x1b[91m"
                   "BTD6Patcher: Process exited before kernel32.dll was loaded! "
                   "Does it exist? Expected at: " KERNEL32_DLL
                << "\x1b[0m"
                << std::endl;
            return 1;
        }

        ContinueDebugEvent(
            event.dwProcessId,
            event.dwThreadId,
            DBG_CONTINUE);
    }

    std::cout
        << "BTD6Patcher: kernel32.dll was loaded! Injecting payload..."
        << std::endl;

    /*
        Allocate payload dll path string
    */
    void* stringPtr = VirtualAllocEx(
        pi.hProcess,
        nullptr,
        sizeof(PAYLOAD_DLL),
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);

    WriteProcessMemory(
        pi.hProcess,
        stringPtr,
        PAYLOAD_DLL,
        sizeof(PAYLOAD_DLL),
        nullptr);

    uintptr_t loadLibraryAddr = kernel32Dllbase + getProcRVA(KERNEL32_DLL, "LoadLibraryA");

    /*
        Inject payload library
    */
    SECURITY_ATTRIBUTES secAttr{
        .nLength = sizeof(SECURITY_ATTRIBUTES)
    };
    DWORD remoteThreadId;
    CreateRemoteThread(
        pi.hProcess, 
        &secAttr, 
        0, 
        (LPTHREAD_START_ROUTINE)loadLibraryAddr,
        stringPtr,
        0, 
        &remoteThreadId);

    /*
        Wait for remote thread to complete.
    */
    for (DEBUG_EVENT event;;) {
        if (WaitForDebugEvent(&event, 1000) == 0) {
            std::cout
                << "\x1b[91m"
                   "BTD6Patcher: Timed out while waiting for library to load!"
                   "\x1b[0m"
                << std::endl;
            return 1;
        }

        if (event.dwThreadId == remoteThreadId) {
            if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
                const HANDLE hFile        = event.u.LoadDll.hFile;
                const std::string dllPath = getFullPathFromHFile(hFile);
                CloseHandle(hFile);

                if (canonicalizePath(PAYLOAD_DLL) == dllPath) {
                    /* Our payload was loaded! */
                    std::cout
                        << "BTD6Patcher: Payload library was injected! Waiting for "
                           "remote thread to exit..."
                        << std::endl;

                    payloadDllBase = (uintptr_t)event.u.LoadDll.lpBaseOfDll;
                }
            } else if (event.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT) {
                if (event.u.ExitThread.dwExitCode == 0) {
                    std::cout
                        << "\x1b[91m"
                           "BTD6Patcher: Remote thread LoadLibrary failed!"
                           "\x1b[0m"
                        << std::endl;
                    return 1;
                }

                if (payloadDllBase == 0) {
                    std::cout
                        << "\x1b[91m"
                           "BTD6Patcher: Remote thread exited without injecting payload!"
                           "\x1b[0m"
                        << std::endl;
                    return 1;
                }

                ContinueDebugEvent(
                    pi.dwProcessId,
                    remoteThreadId,
                    DBG_CONTINUE);
                break;
            } else if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
                CloseHandle(event.u.CreateProcessInfo.hFile);
        } else {
            /* Continue tracking spawned threads */
            if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT)
                spawnedThreads.insert_or_assign(
                    event.dwThreadId,
                    event.u.CreateThread.hThread);
            else if (event.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
                spawnedThreads.erase(event.dwThreadId);

            if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT)
                CloseHandle(event.u.LoadDll.hFile);
            else if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
                CloseHandle(event.u.CreateProcessInfo.hFile);
        }

        ContinueDebugEvent(
            event.dwProcessId,
            event.dwThreadId,
            DBG_CONTINUE);
    }

    std::cout 
        << "BTD6Patcher: Remote thread exited! Suspending game threads..." 
        << std::endl;

    /*
        Suspend all of the game's spawned threads.
    */
    for (const auto& [id, handle] : spawnedThreads) {
        if (SuspendThread(handle) != -1)
            std::cout
            << "\x1b[90m"
               "BTD6Patcher: Suspended thread "
            << id
            << "\x1b[0m"
            << std::endl;
        else
            std::cout
            << "\x1b[33m"
               "BTD6Patcher: Failed to suspend a thread! This is probably a "
               "bug in the bootstrapper. Continuing anyways..."
               "\x1b[0m"
            << std::endl;
    }

    std::cout
        << "BTD6Patcher: Executing payload..."
        << std::endl;

    /*
        Execute payload's Run export in a new remote thread.
    */

    const uintptr_t payloadRunRVA = getProcRVA(PAYLOAD_DLL, "Run");

    if (payloadRunRVA == 0) {
        std::cout
            << "\x1b[33m"
               "BTD6Patcher: Failed to query RVA for procedure 'Run' "
               "from payload dll. Is it an export?"
               "\x1b[0m"
            << std::endl;
        return 1;
    }

    const uintptr_t payloadRunAddr = payloadDllBase + payloadRunRVA;

    CreateRemoteThread(
        pi.hProcess,
        &secAttr,
        0,
        (LPTHREAD_START_ROUTINE)payloadRunAddr,
        0,
        0,
        &remoteThreadId);

    /*
        Wait for remote thread to exit.
    */
    for (DEBUG_EVENT event;;) {
        if (WaitForDebugEvent(&event, 1000) == 0) {
            std::cout
                << "\x1b[91m"
                   "BTD6Patcher: Timed out while waiting for remote thread to exit! "
                   "We may have gotten unlucky with the loader lock. Maybe try "
                   "again?"
                   "\x1b[0m"
                << std::endl;
            return 1;
        }

        if (event.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT) {
            /* Remote thread running payload has exited */
            if (event.u.ExitThread.dwExitCode != 0) {
                std::cout
                    << "\x1b[91m"
                       "BTD6Patcher: Payload thread executed with non-zero status "
                       "code! Code: "
                    << event.u.ExitThread.dwExitCode
                    << "\x1b[0m"
                    << std::endl;
                return 1;
            }

            break;
        }

        if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT)
            CloseHandle(event.u.LoadDll.hFile);
        else if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
            CloseHandle(event.u.CreateProcessInfo.hFile);

        ContinueDebugEvent(
            event.dwProcessId, 
            event.dwThreadId,
            DBG_CONTINUE);
    }

    std::cout
        << "\x1b[92m"
           "BTD6Patcher: Executed patch! Resuming game threads and detaching..."
           "\x1b[0m"
        << std::endl;

    /* We don't need to close handles for threads. */
    for (const auto& [_, handle] : spawnedThreads)
        ResumeThread(handle);
    spawnedThreads.clear();

    /* Detach */

    DebugActiveProcessStop(pi.dwProcessId);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    std::cout
        << "Press any key to close this terminal."
        << std::endl;
    std::cin.get();
}