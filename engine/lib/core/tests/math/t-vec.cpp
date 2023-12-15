#include "../t-index.h"

constexpr i32 vectorEqualsTest() {
    {
        core::vec2<i32> a = core::v(1, 2);
        core::vec2<i32> b = core::v(1, 2);
        Assert(a.equals(b));
        core::vec2<i32> c = core::v(2, 1);
        Assert(!a.equals(c));
    }
    {
        core::vec3<i32> a = core::v(1, 2, 3);
        core::vec3<i32> b = core::v(1, 2, 3);
        Assert(a.equals(b));
        core::vec3<i32> c = core::v(2, 1, 3);
        Assert(!a.equals(c));
    }
    {
        core::vec4<i32> a = core::v(1, 2, 3, 4);
        core::vec4<i32> b = core::v(1, 2, 3, 4);
        Assert(a.equals(b));
        core::vec4<i32> c = core::v(2, 1, 3, 4);
        Assert(!a.equals(c));
    }
    {
        core::vec2<f32> a = core::v(1.0f, 2.0f);
        core::vec2<f32> b = core::v(1.0f, 2.1f);
        Assert(!a.equals(b));
        Assert(!a.equals(b, 0.09f));
        Assert(!a.equals(b, 0.09999f));
        Assert(a.equals(b, 0.10001f));
    }

    return 0;
}

constexpr i32 vectorAddSubMulDivTest() {
    {
        core::vec2<i32> a = core::v(1, 2);
        core::vec2<i32> b = core::v(3, 4);
        a.add(b); Assert(a.equals(core::v(4, 6)));
        a.sub(b); Assert(a.equals(core::v(1, 2)));
        a.mul(b); Assert(a.equals(core::v(3, 8)));
        a.div(b); Assert(a.equals(core::v(1, 2)));
    }
    {
        auto a = core::v(1, 2);
        auto b = core::v(3, 4);
        Assert(a + b == core::v(4, 6));
        Assert(a + b - b == core::v(1, 2));
        Assert(a * b == core::v(3, 8));
        Assert(a * b / b == core::v(1, 2));
    }
    {
        core::vec3<i32> a = core::v(1, 2, 3);
        core::vec3<i32> b = core::v(3, 4, 5);
        a.add(b); Assert(a.equals(core::v(4, 6, 8)));
        a.sub(b); Assert(a.equals(core::v(1, 2, 3)));
        a.mul(b); Assert(a.equals(core::v(3, 8, 15)));
        a.div(b); Assert(a.equals(core::v(1, 2, 3)));
    }
    {
        auto a = core::v(1, 2, 3);
        auto b = core::v(3, 4, 5);
        Assert(a + b == core::v(4, 6, 8));
        Assert(a + b - b == core::v(1, 2, 3));
        Assert(a * b == core::v(3, 8, 15));
        Assert(a * b / b == core::v(1, 2, 3));
    }
    {
        auto a = core::v(1, 2, 3, 4);
        auto b = core::v(3, 4, 5, 6);
        a.add(b); Assert(a.equals(core::v(4, 6, 8, 10)));
        a.sub(b); Assert(a.equals(core::v(1, 2, 3, 4)));
        a.mul(b); Assert(a.equals(core::v(3, 8, 15, 24)));
        a.div(b); Assert(a.equals(core::v(1, 2, 3, 4)));
    }
    {
        auto a = core::v(1, 2, 3, 4);
        auto b = core::v(3, 4, 5, 6);
        Assert(a + b == core::v(4, 6, 8, 10));
        Assert(a + b - b == core::v(1, 2, 3, 4));
        Assert(a * b == core::v(3, 8, 15, 24));
        Assert(a * b / b == core::v(1, 2, 3, 4));
    }
    {
        core::vec2<f32> a = core::v(1.0f, 2.0f);
        core::vec2<i32> b = core::v(0, 1);
        a.add(b); Assert(a.equals(core::v(1.0f, 3.0f)));
        b.add(a); Assert(b.equals(core::v(1, 4)));
        a.sub(b); Assert(a.equals(core::v(0.0f, -1.0f)));
        b.sub(a); Assert(b.equals(core::v(1, 5)));
        a.mul(b); Assert(a.equals(core::v(0.0f, -5.0f)));
        b.mul(a); Assert(b.equals(core::v(0, -25)));
        // These will fail due to division by zero:
        // a.div(b); Assert(a.equals(v(0.0f, 0.2f)));
        // b.div(a); Assert(b.equals(v(0, -5)));
        a++;
        b++;
        a.div(b); Assert(a.equals(core::v(1.0f, 0.166666667f), 0.0000001f));
        a.inc();
        b.div(a); Assert(b.equals(core::v(0, -24)));
    }
    {
        core::vec2<f32> a = core::v(1.0f, 2.0f);
        core::vec2<f32> b = -a;
        Assert(b.equals(core::v(-1.0f, -2.0f)));
        Assert(b.equals(-a));
        Assert(a.equals(-b));
        b = -b;
        Assert(b.equals(a));
    }
    {
        auto a = core::v(1, 2);
        Assert(a + 1 == core::v(2, 3));
        Assert(a - 1 == core::v(0, 1));
        Assert(a * 2 == core::v(2, 4));
        Assert(a / 2 == core::v(0, 1));
        Assert(1 + a == core::v(2, 3));
        Assert(1 - a == core::v(0, -1));
        Assert(2 * a == core::v(2, 4));
        Assert(2 / (a + core::v(1, 0)) == core::v(1, 1));
        Assert(4 / a == core::v(4, 2));
        Assert(8 / a == core::v(8, 4));
        Assert(8 / a / a == core::v(8, 2));
    }

    return 0;
}

constexpr i32 vectorLengthTest() {
    constexpr f64 epsillon = 0.000001;
    Assert(core::abs(core::v(1, 2).length() - 2.23606797749979) < epsillon);
    Assert(core::abs(core::v(1, 2, 3).length() - 3.7416573867739413) < epsillon);
    Assert(core::abs(core::v(1, 2, 3, 4).length() - 5.477225575051661) < epsillon);
    Assert(core::abs(core::v(1.0f, 2.0f).length() - 2.23606797749979) < epsillon);

    return 0;
}

constexpr i32 VectorDotProductTest() {
    Assert(core::v(1, 2).dot(core::v(3, 4)) == 11.0);
    Assert(core::v(1, 2, 3).dot(core::v(3, 4, 5)) == 26.0);
    Assert(core::v(1, 2, 3, 4).dot(core::v(3, 4, 5, 6)) == 50.0);
    Assert(core::v(1.0f, 2.0f).dot(core::v(3.0f, 4.0f)) == 11.0);

    // Properties of dot product:
    {
        // A ⋅ B = B ⋅ A
        auto a = core::v(1, 2);
        auto b = core::v(3, 4);
        Assert(a.dot(b) == b.dot(a));
    }
    {
        // (x * A) ⋅ B = x * (A ⋅ B)
        auto a = core::v(1, 2);
        auto b = core::v(3, 4);
        i32 x = 2;
        Assert((x * a).dot(b) == a.dot(b) * x);
    }
    {
        // A ⋅ (B + C) = A ⋅ B + A ⋅ C
        auto a = core::v(1, 2);
        auto b = core::v(3, 4);
        auto c = core::v(5, 6);
        Assert(a.dot(b + c) == a.dot(b) + a.dot(c));
    }
    {
        // A ⋅ A = len(A)^2
        auto a = core::v(1, 2);
        constexpr f64 epsillon = 0.000001;
        Assert(core::abs(a.dot(a) - a.length() * a.length()) < epsillon);
    }
    {
        // A ⋅ B ≤ len(A) * len(B)
        auto a = core::v(1, 2);
        auto b = core::v(1, 2);
        Assert(a.dot(b) <= a.length() * b.length());
    }
    {
        // Adding different types of vectors is ok when using the direct functions and NOT the operators.
        core::vec4<i32> a = core::v(1, 2, 3, 4);
        core::vec4<f32> b = core::v(1.0f, 2.0f, 3.0f, 4.0f);
        a.add(b);
        // a += b; // fails!
        // auto c = a + b; // also fails!
        Assert(a.equals(core::v(2, 4, 6, 8)));
        a.sub(b);
        Assert(a.equals(core::v(1, 2, 3, 4)));
        a.mul(b);
        Assert(a.equals(core::v(1, 4, 9, 16)));
        a.div(b);
        Assert(a.equals(core::v(1, 2, 3, 4)));
    }

    return 0;
}

constexpr i32 VectorCrossProductTest() {
    Assert(core::v(1, 2, 3).cross(core::v(3, 4, 5)).equals(core::v(-2, 4, -2)));
    Assert(core::v(1.0f, 2.0f, 3.0f).cross(core::v(3.0f, 4.0f, 5.0f)).equals(core::v(-2.0f, 4.0f, -2.0f)));

    // Properties of the cross product:
    {
        // A × B = − (B × A)
        auto a = core::v(1, 2, 3);
        auto b = core::v(3, 4, 5);
        Assert(a.cross(b).equals(-b.cross(a)));
    }
    {
        // (x * B) × A = x (B × A)
        auto a = core::v(1, 2, 3);
        auto b = core::v(3, 4, 5);
        i32 x = 2;
        Assert((x * b).cross(a).equals(x * b.cross(a)));
    }
    {
        // B × (A + C) = B × A + B × C
        auto a = core::v(1, 2, 3);
        auto b = core::v(3, 4, 5);
        auto c = core::v(5, 6, 7);
        Assert(b.cross(a + c).equals(b.cross(a) + b.cross(c)));
    }
    {
        // B × B = (0,0,0)
        auto b = core::v(3, 4, 5);
        Assert(b.cross(b).equals(core::vzero<b.dimensions(), decltype(b)::DataType>()));
    }
    {
        // (B × A) ⋅ C = (C × B) ⋅ A = (A × C) ⋅ B
        auto a = core::v(1, 2, 3);
        auto b = core::v(3, 4, 5);
        auto c = core::v(5, 6, 7);
        Assert(b.cross(a).dot(c) == c.cross(b).dot(a));
        Assert(c.cross(b).dot(a) == a.cross(c).dot(b));
    }

    core::vcross(core::v(1, 2, 3), core::v(3, 4, 5));

    return 0;
}

i32 runVecTestsSuite() {
    RunTest(vectorEqualsTest);
    RunTest(vectorAddSubMulDivTest);
    RunTest(vectorLengthTest);
    RunTest(VectorDotProductTest);
    RunTest(VectorCrossProductTest);

    return 0;
}

constexpr i32 runCompiletimeVecTestsSuite() {
    RunTestCompileTime(vectorEqualsTest);
    RunTestCompileTime(vectorAddSubMulDivTest);
    RunTestCompileTime(vectorLengthTest);
    RunTestCompileTime(VectorDotProductTest);
    RunTestCompileTime(VectorCrossProductTest);

    return 0;
}
