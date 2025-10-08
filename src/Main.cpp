#include <cstdint>
#include <iostream>
#include <windows.h>

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {

    OutputDebugStringA("Hello World!\n");

    char SmallS;
    char unsigned SmallU;

    short MediumS;
    short unsigned MediumU;

    int LargeS;
    int unsigned LargeU;

    char unsigned Test;

    Test = 255;

    Test += 1;

    Test -= 1;

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
