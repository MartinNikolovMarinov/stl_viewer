#include "t-index.h"

template <typename TVal, typename TAllocator>
using I32HashMap = core::HashMap<i32, TVal, TAllocator>;

template <typename TVal, typename TAllocator>
using SVHashMap = core::HashMap<core::StrView, TVal, TAllocator>;

template <typename TAllocator>
using I32HashSet = core::HashSet<i32, TAllocator>;

template <typename TAllocator>
using SVHashSet = core::HashSet<core::StrView, TAllocator>;

template <typename K, typename V>
struct __TestKv { const K& first; const V& second; };

template <typename M, typename K, typename V>
void __test_verifyKeyVal(const M& m, const __TestKv<K, V>& kv) {
    auto& key = kv.first;
    auto& val = kv.second;

    auto a = m.get(key);
    Assert(a != nullptr, "Failed to get data for key");
    Assert(*a == val, "Value at key is incorrect");

    i32 found = 0;
    m.keys([&](const auto& k) -> bool {
        if (core::eq(k, key)) { found++; }
        return true;
    });
    Assert(found == 1, "Key should be found exactly once");

    found = 0;
    m.values([&](const auto& v) -> bool {
        if (v == val) { found++; }
        return true;
    });
    Assert(found > 0, "Value should be found at least once");
}

// test verify many key values with variadic
template <typename M, typename... KV>
void __test_verifyKeyVals(const M& m, const KV&... kv) {
    (__test_verifyKeyVal(m, kv), ...);
}

template <typename M, typename K>
void __test_verifyKey(const M& m, const K& key) {
    auto a = m.get(key);
    Assert(a != nullptr, "Failed to get data for key");

    i32 found = 0;
    m.keys([&](const auto& k) -> bool {
        if (core::eq(k, key)) { found++; }
        return true;
    });
    Assert(found == 1, "Key should be found exactly once");
}

template <typename M, typename... K>
void __test_verifyKeys(const M& m, const K&... keys) {
    (__test_verifyKey(m, keys), ...);
}

template <typename M>
void __test_verifyNoKeys(const M& m) {
    bool noKeys = true;
    m.keys([&](const auto&) -> bool { noKeys = false; return true; });
    Assert(noKeys, "Hash map should not have keys");
}

template <typename M>
void __test_verifyNoValues(const M& m) {
    bool noValues = true;
    m.values([&](const auto&) -> bool { noValues = false; return true; });
    Assert(noValues, "Hash map should not have values");
}

template <typename TAllocator>
i32 initializeHashMapTest() {
    {
        I32HashMap<i32, TAllocator> m;
        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
        __test_verifyNoValues(m);
    }
    {
        SVHashMap<i32, TAllocator> m;
        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
        __test_verifyNoValues(m);
    }
    {
        I32HashMap<i32, TAllocator> m(7);
        Assert(m.len() == 0);
        Assert(m.cap() > 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
        __test_verifyNoValues(m);
    }
    {
        SVHashMap<i32, TAllocator> m(7);
        Assert(m.len() == 0);
        Assert(m.cap() > 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
        __test_verifyNoValues(m);
    }

    return 0;
}

template <typename TAllocator>
i32 initializeHashSetTest() {
    {
        I32HashSet<TAllocator> m;
        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
    }
    {
        SVHashSet<TAllocator> m;
        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
    }
    {
        I32HashSet<TAllocator> m(7);
        Assert(m.len() == 0);
        Assert(m.cap() > 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
    }
    {
        SVHashSet<TAllocator> m(7);
        Assert(m.len() == 0);
        Assert(m.cap() > 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
    }

    return 0;
}

template <typename TAllocator>
i32 putMoveCopyHashMapTest() {
    using core::sv;

    {
        I32HashMap<i32, TAllocator> m;
        m.put(1, 1);
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1});
        Assert(m.len() == 1);
        Assert(!m.empty());
        m.put(2, 1);
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1}, __TestKv<i32, i32>{2, 1});
        Assert(m.len() == 2);
        Assert(!m.empty());
        m.put(2, 9);
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1}, __TestKv<i32, i32>{2, 9});
        Assert(m.len() == 2);
        Assert(!m.empty());

        // Move m to m2

        I32HashMap<i32, TAllocator> m2(core::move(m));
        Assert(m2.len() == 2);
        __test_verifyKeyVals(m2, __TestKv<i32, i32>{1, 1}, __TestKv<i32, i32>{2, 9});
        Assert(!m2.empty());

        // Verify m is empty

        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);

        // Copy back to m

        m = m2.copy();
        Assert(m.len() == 2);
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1}, __TestKv<i32, i32>{2, 9});
        Assert(!m.empty());

        Assert(m2.len() == 2);
        __test_verifyKeyVals(m2, __TestKv<i32, i32>{1, 1}, __TestKv<i32, i32>{2, 9});
        Assert(!m2.empty());
    }

    {
        I32HashMap<i32, TAllocator> m(2);
        m.put(1, 1);
        Assert(m.len() == 1);
        Assert(!m.empty());
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1});
        m.put(2, 1);
        Assert(m.len() == 2);
        Assert(!m.empty());
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1}, __TestKv<i32, i32>{2, 1});
        m.put(2, 9);
        Assert(m.len() == 2);
        Assert(!m.empty());
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1}, __TestKv<i32, i32>{2, 9});
        m.put(3, 5);
        Assert(m.len() == 3);
        Assert(!m.empty());
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1}, __TestKv<i32, i32>{2, 9}, __TestKv<i32, i32>{3, 5});
        m.put(4, 5);
        Assert(m.len() == 4);
        Assert(!m.empty());
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1},
                                __TestKv<i32, i32>{2, 9},
                                __TestKv<i32, i32>{3, 5},
                                __TestKv<i32, i32>{4, 5});

        // Move m to m2

        auto m2 = core::move(m);
        Assert(m2.len() == 4);
        Assert(!m2.empty());
        __test_verifyKeyVals(m2, __TestKv<i32, i32>{1, 1},
                                 __TestKv<i32, i32>{2, 9},
                                 __TestKv<i32, i32>{3, 5},
                                 __TestKv<i32, i32>{4, 5});

        // Verify m is empty

        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);

        // Copy back to m

        m = m2.copy();

        Assert(m.len() == 4);
        Assert(!m.empty());
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1},
                                __TestKv<i32, i32>{2, 9},
                                __TestKv<i32, i32>{3, 5},
                                __TestKv<i32, i32>{4, 5});
    }

    {
        SVHashMap<i32, TAllocator> m;
        m.put(sv("1"), 1);
        Assert(m.len() == 1);
        __test_verifyKeyVals(m, __TestKv<core::StrView, i32>{sv("1"), 1});
        m.put(sv("2"), 1);
        Assert(m.len() == 2);
        __test_verifyKeyVals(m, __TestKv<core::StrView, i32>{sv("1"), 1}, __TestKv<core::StrView, i32>{sv("2"), 1});
        m.put(sv("2"), 9);
        Assert(m.len() == 2);
        __test_verifyKeyVals(m, __TestKv<core::StrView, i32>{sv("1"), 1}, __TestKv<core::StrView, i32>{sv("2"), 9});

        // Move m to m2

        auto m2 = core::move(m);
        Assert(m2.len() == 2);
        __test_verifyKeyVals(m2, __TestKv<core::StrView, i32>{sv("1"), 1}, __TestKv<core::StrView, i32>{sv("2"), 9});

        // Verify m is empty

        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);

        // Copy back to m

        m = m2.copy();
        Assert(m.len() == 2);
        __test_verifyKeyVals(m, __TestKv<core::StrView, i32>{sv("1"), 1}, __TestKv<core::StrView, i32>{sv("2"), 9});

        Assert(m2.len() == 2);
        __test_verifyKeyVals(m2, __TestKv<core::StrView, i32>{sv("1"), 1}, __TestKv<core::StrView, i32>{sv("2"), 9});
    }

    {
        SVHashMap<i32, TAllocator> m(2);
        m.put(sv("1"), 1);
        Assert(m.len() == 1);
        __test_verifyKeyVals(m, __TestKv<core::StrView, i32>{sv("1"), 1});
        m.put(sv("2"), 1);
        Assert(m.len() == 2);
        __test_verifyKeyVals(m, __TestKv<core::StrView, i32>{sv("1"), 1}, __TestKv<core::StrView, i32>{sv("2"), 1});
        m.put(sv("2"), 9);
        Assert(m.len() == 2);
        __test_verifyKeyVals(m, __TestKv<core::StrView, i32>{sv("1"), 1}, __TestKv<core::StrView, i32>{sv("2"), 9});
        m.put(sv("3"), 5);
        Assert(m.len() == 3);
        __test_verifyKeyVals(m, __TestKv<core::StrView, i32>{sv("1"), 1},
                                __TestKv<core::StrView, i32>{sv("2"), 9},
                                __TestKv<core::StrView, i32>{sv("3"), 5});
        m.put(sv("4"), 5);
        Assert(m.len() == 4);
        __test_verifyKeyVals(m, __TestKv<core::StrView, i32>{sv("1"), 1},
                                __TestKv<core::StrView, i32>{sv("2"), 9},
                                __TestKv<core::StrView, i32>{sv("3"), 5},
                                __TestKv<core::StrView, i32>{sv("4"), 5});

        // Move m to m2

        auto m2 = core::move(m);
        Assert(m2.len() == 4);
        __test_verifyKeyVals(m2, __TestKv<core::StrView, i32>{sv("1"), 1},
                                 __TestKv<core::StrView, i32>{sv("2"), 9},
                                 __TestKv<core::StrView, i32>{sv("3"), 5},
                                 __TestKv<core::StrView, i32>{sv("4"), 5});

        // Verify m is empty

        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);

        // Copy back to m

        m = m2.copy();
        Assert(m.len() == 4);
        __test_verifyKeyVals(m, __TestKv<core::StrView, i32>{sv("1"), 1},
                                __TestKv<core::StrView, i32>{sv("2"), 9},
                                __TestKv<core::StrView, i32>{sv("3"), 5},
                                __TestKv<core::StrView, i32>{sv("4"), 5});


        Assert(m2.len() == 4);
        __test_verifyKeyVals(m2, __TestKv<core::StrView, i32>{sv("1"), 1},
                                 __TestKv<core::StrView, i32>{sv("2"), 9},
                                 __TestKv<core::StrView, i32>{sv("3"), 5},
                                 __TestKv<core::StrView, i32>{sv("4"), 5});
    }

    return 0;
}

template <typename TAllocator>
i32 putMoveCopyHashSetTest() {
    using core::sv;

    {
        I32HashSet<TAllocator> m;
        m.put(1);
        Assert(m.len() == 1);
        __test_verifyKeys(m, 1);
        m.put(2);
        Assert(m.len() == 2);
        __test_verifyKeys(m, 1, 2);
        m.put(2);
        Assert(m.len() == 2);
        __test_verifyKeys(m, 1, 2);

        // Move m to m2

        auto m2 = core::move(m);
        Assert(m2.len() == 2);
        __test_verifyKeys(m2, 1, 2);

        // Verify m is empty

        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyKeys(m);

        // Copy back to m

        m = m2.copy();
        Assert(m.len() == 2);
        __test_verifyKeys(m, 1, 2);

        Assert(m2.len() == 2);
        __test_verifyKeys(m2, 1, 2);
    }

    {
        I32HashSet<TAllocator> m(2);
        m.put(1);
        Assert(m.len() == 1);
        __test_verifyKeys(m, 1);
        m.put(2);
        Assert(m.len() == 2);
        __test_verifyKeys(m, 1, 2);
        m.put(3);
        Assert(m.len() == 3);
        __test_verifyKeys(m, 1, 2, 3);
        m.put(4);
        Assert(m.len() == 4);
        __test_verifyKeys(m, 1, 2, 3, 4);

        // Move m to m2

        auto m2 = core::move(m);
        Assert(m2.len() == 4);
        __test_verifyKeys(m2, 1, 2, 3, 4);

        // Verify m is empty

        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyKeys(m);

        // Copy back to m

        m = m2.copy();
        Assert(m.len() == 4);
        __test_verifyKeys(m, 1, 2, 3, 4);

        Assert(m2.len() == 4);
        __test_verifyKeys(m2, 1, 2, 3, 4);
    }

    {
        SVHashSet<TAllocator> m;
        m.put(sv("1"));
        Assert(m.len() == 1);
        __test_verifyKeys(m, sv("1"));
        m.put(sv("2"));
        Assert(m.len() == 2);
        __test_verifyKeys(m, sv("1"), sv("2"));
        m.put(sv("2"));
        Assert(m.len() == 2);
        __test_verifyKeys(m, sv("1"), sv("2"));

        // Move m to m2

        auto m2 = core::move(m);
        Assert(m2.len() == 2);
        __test_verifyKeys(m2, sv("1"), sv("2"));

        // Verify m is empty

        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyKeys(m);

        // Copy back to m

        m = m2.copy();
        Assert(m.len() == 2);
        __test_verifyKeys(m, sv("1"), sv("2"));

        Assert(m2.len() == 2);
        __test_verifyKeys(m2, sv("1"), sv("2"));
    }

    {
        SVHashSet<TAllocator> m(2);
        m.put(sv("1"));
        Assert(m.len() == 1);
        __test_verifyKeys(m, sv("1"));
        m.put(sv("2"));
        Assert(m.len() == 2);
        __test_verifyKeys(m, sv("1"), sv("2"));
        m.put(sv("3"));
        Assert(m.len() == 3);
        __test_verifyKeys(m, sv("1"), sv("2"), sv("3"));
        m.put(sv("4"));
        Assert(m.len() == 4);
        __test_verifyKeys(m, sv("1"), sv("2"), sv("3"), sv("4"));

        // Move m to m2

        auto m2 = core::move(m);
        Assert(m2.len() == 4);
        __test_verifyKeys(m2, sv("1"), sv("2"), sv("3"), sv("4"));

        // Verify m is empty

        Assert(m.len() == 0);
        Assert(m.cap() == 0);
        Assert(m.empty());
        __test_verifyKeys(m);

        // Copy back to m

        m = m2.copy();
        Assert(m.len() == 4);
        __test_verifyKeys(m, sv("1"), sv("2"), sv("3"), sv("4"));

        Assert(m2.len() == 4);
        __test_verifyKeys(m2, sv("1"), sv("2"), sv("3"), sv("4"));
    }

    return 0;
}

template <typename TAllocator>
i32 getWhenHashMapIsFilledToCapacityTest() {
    core::HashMap<i32, i32, TAllocator> m(2);

    m.put(1, 1);
    m.put(2, 1);

    Assert(m.len() == 2);
    Assert(m.cap() == 2); // I don't usually assume the cap but this is a special case.
    Assert(!m.empty());

    Assert(m.get(1) != nullptr);
    Assert(m.get(2) != nullptr);
    Assert(m.get(3) == nullptr); // Make sure these don't loop forever.
    Assert(m.get(55) == nullptr);

    return 0;
}

template <typename TAllocator>
i32 getWhenHashSetIsFilledToCapacityTest() {
    core::HashSet<i32, TAllocator> m(2);

    m.put(1);
    m.put(2);

    Assert(m.len() == 2);
    Assert(m.cap() == 2); // I don't usually assume the cap but this is a special case.
    Assert(!m.empty());

    Assert(m.get(1) != nullptr);
    Assert(m.get(2) != nullptr);
    Assert(m.get(3) == nullptr); // Make sure these don't loop forever.
    Assert(m.get(55) == nullptr);

    return 0;
}

template <typename TAllocator>
i32 removeFromHashMapTest() {
    {
        I32HashMap<i32, TAllocator> m(2);
        m.put(1, 1);
        m.remove(1);

        Assert(m.len() == 0);
        Assert(m.cap() > 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
        __test_verifyNoValues(m);
    }

    {
        I32HashMap<i32, TAllocator> m;
        m.put(1, 1);
        m.put(2, 1);
        m.put(3, 1);

        m.remove(2);
        Assert(m.len() == 2);
        Assert(!m.empty());
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 1}, __TestKv<i32, i32>{3, 1});

        m.remove(1);
        Assert(m.len() == 1);
        Assert(!m.empty());
        __test_verifyKeyVals(m, __TestKv<i32, i32>{3, 1});

        m.remove(3);
        Assert(m.len() == 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
        __test_verifyNoValues(m);
    }

    {
        I32HashMap<i32, TAllocator> m(2);

        m.put(1, 1);
        m.put(2, 1);

        m.remove(1);
        m.remove(2);

        m.put(1, 7);
        m.put(2, 8);
        Assert(m.len() == 2);
        Assert(!m.empty());
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 7}, __TestKv<i32, i32>{2, 8});

        m.put(3, 9);
        Assert(m.len() == 3);
        Assert(!m.empty());
        __test_verifyKeyVals(m, __TestKv<i32, i32>{1, 7}, __TestKv<i32, i32>{2, 8}, __TestKv<i32, i32>{3, 9});
    }

    return 0;
}

template <typename TAllocator>
i32 removeFromHashSetTest() {
    {
        I32HashSet<TAllocator> m(2);
        m.put(1);
        m.remove(1);

        Assert(m.len() == 0);
        Assert(m.cap() > 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
    }

    {
        I32HashSet<TAllocator> m;
        m.put(1);
        m.put(2);
        m.put(3);

        m.remove(2);
        Assert(m.len() == 2);
        Assert(!m.empty());
        __test_verifyKeys(m, 1, 3);

        m.remove(1);
        Assert(m.len() == 1);
        Assert(!m.empty());
        __test_verifyKeys(m, 3);

        m.remove(3);
        Assert(m.len() == 0);
        Assert(m.empty());
        __test_verifyNoKeys(m);
    }

    {
        I32HashSet<TAllocator> m(2);

        m.put(1);
        m.put(2);

        m.remove(1);
        m.remove(2);

        m.put(1);
        m.put(2);
        Assert(m.len() == 2);
        Assert(!m.empty());
        __test_verifyKeys(m, 1, 2);

        m.put(3);
        Assert(m.len() == 3);
        Assert(!m.empty());
        __test_verifyKeys(m, 1, 2, 3);
    }

    return 0;
}

template <typename TAllocator>
i32 complexTypesInHashMapTest() {
    using core::sv;

    {
        defer { SVCT::nextId = 0; };

        SVHashMap<SVCT, TAllocator> m(2); // should not call the ctor!

        Assert(SVCT::nextId == 0);

        m.put(sv("1"), SVCT{});
        m.put(sv("2"), SVCT{});
        m.put(sv("3"), SVCT{});
        m.put(sv("4"), SVCT{});

        Assert(SVCT::nextId == 4);
        Assert(m.get(sv("1")));
        Assert(m.get(sv("2")));
        Assert(m.get(sv("3")));
        Assert(m.get(sv("4")));

        Assert(m.get(sv("1"))->a == 0);
        Assert(m.get(sv("2"))->a == 1);
        Assert(m.get(sv("3"))->a == 2);
        Assert(m.get(sv("4"))->a == 3);
    }

    {
        defer { CT::resetAll(); };

        SVHashMap<CT, TAllocator> m(2);

        Assert(CT::dtorsCalled() == 0);
        Assert(CT::totalCtorsCalled() == 0);
        Assert(CT::assignmentsTotalCalled() == 0);

        m.put(sv("1"), CT{});

        Assert(CT::defaultCtorCalled() == 1);
        Assert(CT::copyCtorCalled() == 0);
        Assert(CT::moveCtorCalled() == 0);
        Assert(CT::assignmentsCopyCalled() == 0);
        Assert(CT::assignmentsMoveCalled() == 1); // put calls move assignment
        Assert(CT::dtorsCalled() == 1);
        Assert(CT::totalCtorsCalled() == 1);
        Assert(CT::assignmentsTotalCalled() == 1);

        CT c;
        m.put(sv("2"), c);

        Assert(CT::defaultCtorCalled() == 2);
        Assert(CT::copyCtorCalled() == 0);
        Assert(CT::moveCtorCalled() == 0);
        Assert(CT::assignmentsCopyCalled() == 1); // put calls copy assignment
        Assert(CT::assignmentsMoveCalled() == 1);
        Assert(CT::dtorsCalled() == 1);
        Assert(CT::totalCtorsCalled() == 2);
        Assert(CT::assignmentsTotalCalled() == 2);

        // This triggers a resize
        m.put(sv("3"), core::move(c));

        Assert(CT::defaultCtorCalled() == 2);
        Assert(CT::copyCtorCalled() == 0);
        Assert(CT::moveCtorCalled() == 0);
        Assert(CT::assignmentsCopyCalled() == 1);
        Assert(CT::assignmentsMoveCalled() == 4); // old acount (1) += 2 assignments to resize + 1 assignment for put
        Assert(CT::dtorsCalled() == 3); // old dcount (1) += 1 before put is called + 1 for put
        Assert(CT::totalCtorsCalled() == 2);
        Assert(CT::assignmentsTotalCalled() == 5);

        m.put(sv("3"), CT{}); // update

        Assert(CT::defaultCtorCalled() == 3);
        Assert(CT::copyCtorCalled() == 0);
        Assert(CT::moveCtorCalled() == 0);
        Assert(CT::assignmentsCopyCalled() == 1);
        Assert(CT::assignmentsMoveCalled() == 5);
        Assert(CT::dtorsCalled() == 4);
        Assert(CT::totalCtorsCalled() == 3);
        Assert(CT::assignmentsTotalCalled() == 6);

        // Make sure data is appropriately copied and moved in the containder.

        auto v1 = m.get(sv("1")); Assert(v1);
        auto v2 = m.get(sv("2")); Assert(v2);
        auto v3 = m.get(sv("3")); Assert(v3);

        Assert(v1->a == CT::defaultValue);
        Assert(v2->a == CT::defaultValue);
        Assert(v3->a == CT::defaultValue);

        v1->a = 1;
        Assert(v1->a == 1);
        Assert(v2->a == CT::defaultValue);
        Assert(v3->a == CT::defaultValue);

        v2->a = 2;
        Assert(v1->a == 1);
        Assert(v2->a == 2);
        Assert(v3->a == CT::defaultValue);

        v3->a = 3;
        Assert(v1->a == 1);
        Assert(v2->a == 2);
        Assert(v3->a == 3);
    }

    {
        // Make sure that put calls the destructor and memory leaks are not possible.

        struct tmp {
            char* data = nullptr;
            tmp() { data = reinterpret_cast<char*>(TAllocator::alloc(1)); }
            tmp(const tmp& other) {
                data = reinterpret_cast<char*>(TAllocator::alloc(1));
                core::memcopy(data, other.data, 1);
            }
            tmp(tmp&& other) {
                if (this == &other) return;
                data = other.data;
                other.data = nullptr;
            }
            ~tmp() {
                free();
            }

            void free() {
                if (data) {
                    TAllocator::free(data);
                }
                data = nullptr;
            }

            tmp& operator=(const tmp& other) {
                if (this == &other) return *this;
                free();
                data = reinterpret_cast<char*>(TAllocator::alloc(1));
                core::memcopy(data, other.data, 1);
                return *this;
            }
            tmp& operator=(tmp&& other) {
                if (this == &other) return *this;
                free();
                data = other.data;
                other.data = nullptr;
                return *this;
            }
        };

        SVHashMap<tmp, TAllocator> m(2);
        tmp t;
        m.put(sv("1"), t);
        m.put(sv("1"), tmp{});

        // m.get(sv("1"))->data = nullptr; // this will be detected as a memory leak.
    }

    return 0;
}

constexpr i32 hashMapTraitsTest() {
    static_assert(core::is_standard_layout_v<core::HashMap<core::StrView, i32>>, "HashMap must be standard layout");
    static_assert(core::is_standard_layout_v<core::HashSet<core::StrView>>, "HashSet must be standard layout");

    return 0;
}

i32 runHashMapTestsSuite() {
    constexpr addr_size BUFF_SIZE = core::KILOBYTE * 4;
    char buf[BUFF_SIZE];

    core::StdAllocator::init(nullptr);
    core::StdStatsAllocator::init(nullptr);
    core::BumpAllocator::init(nullptr, buf, BUFF_SIZE);

    auto checkLeaks = []() {
        Assert(core::StdAllocator::usedMem() == 0);
        Assert(core::StdStatsAllocator::usedMem() == 0, "Memory leak detected!");
        Assert(core::BumpAllocator::usedMem() == 0);
    };

    {
        RunTest_DisplayMemAllocs(initializeHashMapTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(initializeHashMapTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(initializeHashMapTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(putMoveCopyHashMapTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(putMoveCopyHashMapTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(putMoveCopyHashMapTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(removeFromHashMapTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(removeFromHashMapTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(removeFromHashMapTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(complexTypesInHashMapTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(complexTypesInHashMapTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(complexTypesInHashMapTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(getWhenHashMapIsFilledToCapacityTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(getWhenHashMapIsFilledToCapacityTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(getWhenHashMapIsFilledToCapacityTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }

    {
        RunTest_DisplayMemAllocs(initializeHashSetTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(initializeHashSetTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(initializeHashSetTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(putMoveCopyHashSetTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(putMoveCopyHashSetTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(putMoveCopyHashSetTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(removeFromHashSetTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(removeFromHashSetTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(removeFromHashSetTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }

    return 0;
}

constexpr i32 runCompiletimeHashMapTestsSuite() {
    RunTestCompileTime(hashMapTraitsTest);

    return 0;
}
