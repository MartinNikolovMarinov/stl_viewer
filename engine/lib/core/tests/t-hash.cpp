#include "t-index.h"

template <typename THash>
i32 hashCorrectnessTest(THash calcHash) {
    {
        constexpr const char* msg = "Hello, world!";
        u32 first = calcHash(msg, core::cptrLen(msg), 0);
        u32 second = calcHash(msg, core::cptrLen(msg), 0);
        Assert(first == second);
    }
    {
        constexpr const char* msg = "";
        u32 first = calcHash(msg, core::cptrLen(msg), 0);
        u32 second = calcHash(msg, core::cptrLen(msg), 0);
        Assert(first == second);
    }
    {
        constexpr const char* msg = nullptr;
        u32 first = calcHash(msg, core::cptrLen(msg), 0);
        u32 second = calcHash(msg, core::cptrLen(msg), 0);
        Assert(first == second);
    }
    {
        constexpr const char* msg = nullptr;
        u32 first = calcHash(msg, core::cptrLen(msg), 12);
        u32 second = calcHash(msg, core::cptrLen(msg), 12);
        Assert(first == second);
    }
    {
        constexpr const char* msg = nullptr;
        u32 first = calcHash(msg, core::cptrLen(msg), 0);
        u32 second = calcHash(msg, core::cptrLen(msg), 1234);
        Assert(first != second);
    }
    {
        constexpr const char* msg = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor"
                                    "incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud"
                                    "exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute"
                                    "irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla"
                                    "pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia"
                                    "deserunt mollit anim id est laborum. Fugiat aute irure anim ipsum mollit nisi amet"
                                    "laborum quis amet eiusmod. Aliquip officia dolore reprehenderit eiusmod aliqua aute"
                                    "nisi. Culpa esse velit Lorem ad nostrud. Quis amet laborum enim aliqua esse in ipsum"
                                    "incididunt aliquip ea cupidatat sit fugiat dolore. In velit ad excepteur esse esse"
                                    "in ut quis elit consectetur dolore. Id exercitation fugiat id consectetur excepteur"
                                    "aute nulla deserunt ipsum. Esse excepteur id do nulla dolore tempor mollit commodo"
                                    "ipsum velit nostrud exercitation. Sint occaecat consequat qui non ullamco occaecat"
                                    "voluptate. Reprehenderit proident do consectetur consectetur duis aute adipisicing"
                                    "id officia aliquip consequat deserunt. Sint non magna ipsum ut sint incididunt amet"
                                    "et. Lorem aute pariatur irure id culpa laboris nisi quis Lorem occaecat. Est commodo"
                                    "velit sint veniam elit officia laboris nostrud veniam. Mollit ut qui non"
                                    "consectetur. Proident fugiat et adipisicing qui ea veniam qui adipisicing ut ea. Qui"
                                    "officia in cillum magna commodo consectetur aute cillum ex et. Id tempor eiusmod"
                                    "commodo dolor enim sint eiusmod exercitation incididunt esse nulla.";
        u32 first = calcHash(msg, core::cptrLen(msg), 0);
        u32 second = calcHash(msg, core::cptrLen(msg), 0);
        Assert(first == second);
    }
    return 0;
}

i32 runHashTestsSuite() {
    RunTest(hashCorrectnessTest, &core::simpleHash_32);
    RunTest(hashCorrectnessTest, &core::djb2_32);

    return 0;
}
