
#include "First.h"
#include "FirstConfig.h" // <-- This is a cmake generated file

#include "Atom.h"

int main(int argc, char* argv[])
{
    Vector3D<float> vec(1.0f, 2.0f, 3.0f);
    std::cout << "Vector: " << vec.X << ", " << vec.Y << ", " << vec.Z << std::endl;

    Systems::Atom atom(1, 1);
    std::cout << "Atom: " << atom.ID << " | " << atom.Z << " | " << atom.electronsNumber << std::endl;

    // hi there

    return 0;
}
