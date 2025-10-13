#include <iostream>
#include <windows.h>

struct projectile {
    int Damage;
    short Size;
    char Speed;
    char Distance;
};

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {

    OutputDebugStringA("Hello World!\n");

    /*
    char SmallS;
    char unsigned SmallU;

    short MediumS;
    short unsigned MediumU;

    int LargeS;
    int unsigned LargeU;
    */

    projectile Test;

    Test.Damage = 1;
    Test.Distance = 2;
    Test.Size = 3;
    Test.Speed = 4;

    projectile Projectiles[40];

    projectile *ProjectilePointer = Projectiles;

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
