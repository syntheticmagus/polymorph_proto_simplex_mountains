#include "morph_cute_png.h"

#include <iostream>

int main()
{
    std::cout << "Hello, world!" << std::endl;

    if (is_morph_cute_png_available())
    {
        std::cout << "morph_cute_png is available!" << std::endl;
    }

    return 0;
}