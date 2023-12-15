#include <core_mem.h>

// TODO2: [PERFORMANCE] Everything in this file can be much faster.

namespace core {

void* memcopy(void* dest, const void* src, addr_size len) {
    // TODO2: [PERFORMANCE] William Chan has a good implementation of a fast memcpy.
    char* ddest = reinterpret_cast<char*>(dest);
    const char* ssrc = reinterpret_cast<const char*>(src);
    addr_size remain = len % 4;
    for (addr_size i = 0; i < (len - remain); i+=4) {
        ddest[i] = ssrc[i];
        ddest[i+1] = ssrc[i+1];
        ddest[i+2] = ssrc[i+2];
        ddest[i+3] = ssrc[i+3];
    }

    switch (remain) {
    case 1:
        ddest[len-1] = ssrc[len-1];
        break;
    case 2:
        ddest[len-1] = ssrc[len-1];
        ddest[len-2] = ssrc[len-2];
        break;
    case 3:
        ddest[len-1] = ssrc[len-1];
        ddest[len-2] = ssrc[len-2];
        ddest[len-3] = ssrc[len-3];
        break;
    }

    return dest;
}

void* memset(void* dest, u8 c, addr_size n) {
    u8* ddest = reinterpret_cast<u8*>(dest);
    addr_size remain = n % 4;
    for (addr_size i = 0; i < (n - remain); i+=4) {
        ddest[i] = c;
        ddest[i+1] = c;
        ddest[i+2] = c;
        ddest[i+3] = c;
    }

    switch (remain) {
    case 1:
        ddest[n-1] = c;
        break;
    case 2:
        ddest[n-1] = c;
        ddest[n-2] = c;
        break;
    case 3:
        ddest[n-1] = c;
        ddest[n-2] = c;
        ddest[n-3] = c;
        break;
    }

    return dest;
}

i32 memcmp(const void* s1, const void* s2, addr_size n) {
    const u8* p1 = reinterpret_cast<const u8*>(s1);
    const u8* p2 = reinterpret_cast<const u8*>(s2);
    addr_size remain = n % 4;
    for (addr_size i = 0; i < (n - remain); i+=4) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
        if (p1[i+1] != p2[i+1]) return p1[i+1] - p2[i+1];
        if (p1[i+2] != p2[i+2]) return p1[i+2] - p2[i+2];
        if (p1[i+3] != p2[i+3]) return p1[i+3] - p2[i+3];
    }

    switch (remain) {
    case 1:
        if (p1[n-1] != p2[n-1]) return p1[n-1] - p2[n-1];
        break;
    case 2:
        if (p1[n-1] != p2[n-1]) return p1[n-1] - p2[n-1];
        if (p1[n-2] != p2[n-2]) return p1[n-2] - p2[n-2];
        break;
    case 3:
        if (p1[n-1] != p2[n-1]) return p1[n-1] - p2[n-1];
        if (p1[n-2] != p2[n-2]) return p1[n-2] - p2[n-2];
        if (p1[n-3] != p2[n-3]) return p1[n-3] - p2[n-3];
        break;
    }

    return 0;
}

void swapBytes(void* a, void* b, addr_size size) {
    u8* p1 = reinterpret_cast<u8*>(a);
    u8* p2 = reinterpret_cast<u8*>(b);
    addr_size remain = size % 4;

    for (addr_size i = 0; i < (size - remain); i+=4) {
        swap(p1[i], p2[i]);
        swap(p1[i+1], p2[i+1]);
        swap(p1[i+2], p2[i+2]);
        swap(p1[i+3], p2[i+3]);
    }

    switch (remain) {
    case 1:
        swap(p1[size-1], p2[size-1]);
        break;
    case 2:
        swap(p1[size-1], p2[size-1]);
        swap(p1[size-2], p2[size-2]);
        break;
    case 3:
        swap(p1[size-1], p2[size-1]);
        swap(p1[size-2], p2[size-2]);
        swap(p1[size-3], p2[size-3]);
        break;
    }
}

}
