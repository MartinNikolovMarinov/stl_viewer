#include <core_intrinsics.h>

#if COMPILER_MSVC
#include <intrin.h>
#elif defined(CPU_ARCH_X86_64)
#include <x86intrin.h>
#elif defined(CPU_ARCH_ARM64) && defined(OS_MAC) && OS_MAC == 1
#include <mach/mach_time.h>
#endif

namespace core {

u64 intrin_getCpuTicks() {
#if defined(CPU_ARCH_X86_64)
    return __rdtsc();
#elif CPU_ARCH_ARM64 && defined(OS_MAC) && OS_MAC == 1
    return mach_absolute_time();
#endif
}

} // namespace core
