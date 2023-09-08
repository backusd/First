#include "First.h"
#include "FirstConfig.h" // <-- This is a cmake generated file

#include "RestrictedHartreeFock.h"


int main(int argc, char* argv[])
{
    int iterations = 3000;
    std::unique_ptr<HartreeFock::RestrictedHartreeFock> algorithm = std::make_unique<HartreeFock::RestrictedHartreeFock>(iterations);

    std::cout << *algorithm.get() << std::endl;

    return 0;
}
