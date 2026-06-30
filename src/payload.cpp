#include <iostream>

extern "C" {

__declspec(dllexport)
bool DllMain(HINSTANCE, DWORD, LPVOID) {
    return true;
}

__declspec(dllexport)
int Run(void*) {
    return 0;
}

} // extern "C"