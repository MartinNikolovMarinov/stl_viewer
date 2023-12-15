#include "../t-index.h"

constexpr i32 translateTest() {
    core::vec2f v = core::v(1.0f, 2.0f);
    core::vec2f t = core::v(3.0f, 4.0f);
    v = core::translate(v, t);
    Assert(v == core::v(4.0f, 6.0f));

    v = core::v(1.0f, 2.0f);
    t = core::v(-3.0f, -4.0f);
    v = core::translate(v, t);
    Assert(v == core::v(-2.0f, -2.0f));

    return 0;
}

constexpr i32 scaleTest() {
    core::vec2f v = core::v(1.0f, 2.0f);
    core::vec2f s = core::v(3.0f, 4.0f);
    v = core::scale(v, s);
    Assert(v == core::v(3.0f, 8.0f));

    v = core::v(1.0f, 2.0f);
    s = core::v(-3.0f, -4.0f);
    v = core::scale(v, s);
    Assert(v == core::v(-3.0f, -8.0f));

    v = core::v(1.0f, 2.0f);
    s = core::v(0.0f, 0.0f);
    v = core::scale(v, s);
    Assert(v == core::v(0.0f, 0.0f));

    return 0;
}

i32 rotate2dTest() {
    constexpr core::vec2f origin = core::v(0.0f, 0.0f);

    core::vec2f v = core::v(1.0f, 2.0f);
    v = core::rotate(v, origin, core::degToRad(0.0f));
    Assert(v.equals(core::v(1.0f, 2.0f), 0.0001f));

    v = core::v(1.0f, 2.0f);
    v = core::rotate(v, origin, core::degToRad(90.0f));
    Assert(v.equals(core::v(-2.0f, 1.0f), 0.0001f));

    v = core::v(1.0f, 2.0f);
    v = core::rotate(v, origin, core::degToRad(180.0f));
    Assert(v.equals(core::v(-1.0f, -2.0f), 0.0001f));

    v = core::v(1.0f, 2.0f);
    v = core::rotate(v, origin, core::degToRad(270.0f));
    Assert(v.equals(core::v(2.0f, -1.0f), 0.0001f));

    v = core::v(1.0f, 2.0f);
    v = core::rotate(v, origin, core::degToRad(360.0f));
    Assert(v.equals(core::v(1.0f, 2.0f), 0.0001f));

    v = core::v(1.0f, 2.0f);
    v = core::rotateRight(v, origin, core::degToRad(0.0f));
    Assert(v.equals(core::v(1.0f, 2.0f), 0.0001f));

    v = core::v(1.0f, 2.0f);
    v = core::rotateRight(v, origin, core::degToRad(90.0f));
    Assert(v.equals(core::v(2.0f, -1.0f), 0.0001f));

    v = core::v(1.0f, 2.0f);
    v = core::rotateRight(v, origin, core::degToRad(180.0f));
    Assert(v.equals(core::v(-1.0f, -2.0f), 0.0001f));

    v = core::v(1.0f, 2.0f);
    v = core::rotateRight(v, origin, core::degToRad(270.0f));
    Assert(v.equals(core::v(-2.0f, 1.0f), 0.0001f));

    v = core::v(1.0f, 2.0f);
    v = core::rotateRight(v, origin, core::degToRad(360.0f));
    Assert(v.equals(core::v(1.0f, 2.0f), 0.0001f));

    constexpr core::vec2f origin2 = core::v(1.0f, 2.0f);

    v = core::v(1.0f, 2.0f);
    v = core::rotate(v, origin2, core::degToRad(0.0f));
    Assert(v.equals(core::v(1.0f, 2.0f), 0.0001f));

    return 0;
}

i32 runTransformsTestsSuite() {
    RunTest(translateTest);
    RunTest(scaleTest);
    RunTest(rotate2dTest);

    return 0;
}

i32 runCompiletimeTransformsTestsSuite() {
    RunTestCompileTime(translateTest);
    RunTestCompileTime(scaleTest);

    return 0;
}
