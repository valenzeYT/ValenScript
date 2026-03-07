#include <windows.h>
#include <iostream>
int main(){
    HMODULE h = LoadLibraryA("lib\\cpu.dll");
    if(!h){
        std::cout << "LoadLibrary failed: " << GetLastError() << "\n";
        return 1;
    }
    auto fn = reinterpret_cast<void(*)()>(GetProcAddress(h, "register_module"));
    if(!fn){
        std::cout << "GetProcAddress failed: " << GetLastError() << "\n";
        return 2;
    }
    std::cout << "Loaded ok\n";
    return 0;
}
