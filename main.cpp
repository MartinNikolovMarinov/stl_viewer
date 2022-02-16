#include "lib/core/src/core.h"

i32 main(i32 argc, constptr char *argv[])
{
    core::PrintF("Hello World is %d symbols long\n", core::CharPtrLen("Hello World"));
    return 0;
}
