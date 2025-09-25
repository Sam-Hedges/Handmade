#include <cstdint>
#include <iostream>
#include <windows.h>

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {

    OutputDebugStringA("Hello World!\n");
    return 0;
}

// int main() {
//
//     std::cout << "Hello World";
//
//     std::string ligma;
//     std::cin >> ligma;
//
//     return 0;
// }
