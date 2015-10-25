#include "metafast/metafast.hpp"


#ifndef DISABLE_TESTS
#include <gtest/gtest.h>
#include <vector>

struct HasMembers
{
    int i = 0;
    float f = 0.0f;
    bool b = false;
};

REFLECT_CLASS_START(HasMembers, 0)
    REFLECT_MEMBER(i)
    REFLECT_MEMBER(f)
    REFLECT_MEMBER(b)
REFLECT_CLASS_END()

template<typename T>
Range<const unsigned char> as_bytes(const T & object)
{
    auto begin = reinterpret_cast<const unsigned char *>(std::addressof(object));
    return { begin, begin + sizeof(T) };
}

metaf::BinaryInput StringToBinaryInput(const std::string & input)
{
    auto begin = reinterpret_cast<const unsigned char *>(input.data());
    return { { begin, begin + input.size() } };
}

struct InMemoryBinaryInput
{
    InMemoryBinaryInput(std::string data)
        : data(std::move(data))
        , input(StringToBinaryInput(this->data))
    {
    }

    operator metaf::BinaryInput &()
    {
        return input;
    }

    std::string data;
    metaf::BinaryInput input;
};

template<typename T>
InMemoryBinaryInput serialize_to_buffer(const T & value)
{
    std::stringstream buffer;
    metaf::detail::reference(buffer, value);
    return { buffer.str() };
}

template<typename T>
T roundtrip(const T & value)
{
    auto as_input = serialize_to_buffer(value);
    T result;
    metaf::detail::reference(as_input, result);
    return result;
}

#ifdef STORE_AS_FLOAT8_IF_POSSIBLE
metaf::detail::FloatComponents compression_roundtrip(metaf::detail::FloatComponents input)
{
    return metaf::detail::FloatComponents(metaf::detail::FloatComponentsCompressed(input));
}

::testing::AssertionResult TestCompressedFloat(float f)
{
    metaf::detail::FloatComponents a(f);
    if (!a.can_be_compressed())
        return ::testing::AssertionFailure();
    if (a.f != compression_roundtrip(a).f)
        return ::testing::AssertionFailure();
    return testing::AssertionSuccess();
}

TEST(metafast, float_compressed)
{
    ASSERT_TRUE(TestCompressedFloat(0.0f));
    ASSERT_TRUE(TestCompressedFloat(1.0f));
    ASSERT_TRUE(TestCompressedFloat(2.0f));
    ASSERT_TRUE(TestCompressedFloat(3.0f));
    ASSERT_TRUE(TestCompressedFloat(4.0f));
    ASSERT_TRUE(TestCompressedFloat(0.125f));
    ASSERT_TRUE(TestCompressedFloat(0.25f));
    ASSERT_TRUE(TestCompressedFloat(0.5f));
    ASSERT_TRUE(TestCompressedFloat(0.75f));
    ASSERT_TRUE(TestCompressedFloat(-0.0f));
    ASSERT_TRUE(TestCompressedFloat(-1.0f));
    ASSERT_TRUE(TestCompressedFloat(-2.0f));
    ASSERT_TRUE(TestCompressedFloat(-3.0f));
    ASSERT_TRUE(TestCompressedFloat(-4.0f));
    ASSERT_TRUE(TestCompressedFloat(-0.125f));
    ASSERT_TRUE(TestCompressedFloat(-0.25f));
    ASSERT_TRUE(TestCompressedFloat(-0.5f));
    ASSERT_TRUE(TestCompressedFloat(-0.75f));
}
#ifdef FLOAT8_SUPPORTS_NAN_AND_INFINITY
TEST(metafast, float_compressed_special_values)
{
    metaf::detail::FloatComponents qnan(std::numeric_limits<float>::quiet_NaN());
    ASSERT_TRUE(qnan.is_nan_or_infinity());
    ASSERT_FALSE(qnan.is_infinity());
    ASSERT_TRUE(qnan.quiet_nan_bit);
    metaf::detail::FloatComponents qnan_roundtrip = compression_roundtrip(qnan);
    ASSERT_TRUE(qnan_roundtrip.is_nan_or_infinity());
    ASSERT_FALSE(qnan_roundtrip.is_infinity());
    ASSERT_TRUE(qnan_roundtrip.quiet_nan_bit);
    metaf::detail::FloatComponents snan(std::numeric_limits<float>::signaling_NaN());
    ASSERT_TRUE(snan.is_nan_or_infinity());
    ASSERT_FALSE(snan.is_infinity());
    ASSERT_FALSE(snan.quiet_nan_bit);
    metaf::detail::FloatComponents snan_roundtrip = compression_roundtrip(snan);
    ASSERT_TRUE(snan_roundtrip.is_nan_or_infinity());
    ASSERT_FALSE(snan_roundtrip.is_infinity());
    ASSERT_FALSE(snan_roundtrip.quiet_nan_bit);
    metaf::detail::FloatComponents infinity(std::numeric_limits<float>::infinity());
    ASSERT_TRUE(infinity.is_nan_or_infinity());
    ASSERT_TRUE(TestCompressedFloat(std::numeric_limits<float>::infinity()));
    metaf::detail::FloatComponents negative_infinity(-std::numeric_limits<float>::infinity());
    ASSERT_TRUE(negative_infinity.is_nan_or_infinity());
    ASSERT_TRUE(TestCompressedFloat(-std::numeric_limits<float>::infinity()));
}
#endif
/*TEST(metafast, all_compressed_floats)
{
    metaf::detail::FloatComponentsCompressed compressed(metaf::detail::FloatComponents(0.0f));
    std::cout << metaf::detail::FloatComponents(compressed).f << '\n';
    for (++compressed.exponent; compressed.exponent; ++compressed.exponent)
    {
        do
        {
            std::cout << metaf::detail::FloatComponents(compressed).f << '\n';
            ++compressed.mantissa;
        }
        while(compressed.mantissa);
    }
}*/

#endif

TEST(metafast, simple)
{
    HasMembers foo;
    foo.i = 5;
    foo.f = 10.0f;
    foo.b = true;
    HasMembers copy = roundtrip(foo);
    ASSERT_EQ(foo.i, copy.i);
    ASSERT_EQ(foo.f, copy.f);
    ASSERT_EQ(foo.b, copy.b);
}

struct StructWithVector
{
    std::vector<float> vec;
};
REFLECT_CLASS_START(StructWithVector, 0)
    REFLECT_MEMBER(vec)
REFLECT_CLASS_END()

TEST(metafast, vector)
{
    StructWithVector to_deserialize = { { 1.0f, 2.0f, 3.0f } };
    StructWithVector copy = roundtrip(to_deserialize);
    ASSERT_EQ(to_deserialize.vec, copy.vec);
}

struct Base
{
    Base()
    {
    }

    Base(int foo)
        : foo(foo)
    {
    }

    FRIEND_REFLECT_CLASSES(Base);

    int GetFoo() const
    {
        return foo;
    }
private:
    int foo = 0;
};
struct Derived : Base
{
    Derived()
    {
    }

    Derived(int foo, int bar)
        : Base(foo), bar(bar)
    {
    }
    FRIEND_REFLECT_CLASSES(Derived);

    int GetBar() const
    {
        return bar;
    }
private:
    int bar = 0;
};

REFLECT_CLASS_START(Base, 0)
    REFLECT_MEMBER(foo)
REFLECT_CLASS_END()
REFLECT_CLASS_START(Derived, 0)
    REFLECT_BASE(Base)
    REFLECT_MEMBER(bar)
REFLECT_CLASS_END()

TEST(metafast, base)
{
    Derived a = { 5, 6 };
    Derived b = roundtrip(a);
    ASSERT_EQ(5, b.GetFoo());
    ASSERT_EQ(6, b.GetBar());
}
TEST(metafast, roundtrip_float)
{
    float a = 65791;
    float b = roundtrip(a);
    ASSERT_EQ(a, b);

    a = 508;
    b = roundtrip(a);
    ASSERT_EQ(a, b);
}

struct StructWithDefaults
{
    int a = 5;
    std::vector<float> b = { 1.0f, 2.0f, 3.0f };
    int c[3] = { 4, 5, 6 };

    bool operator==(const StructWithDefaults & other) const
    {
        return a == other.a && b == other.b && std::equal(c, c + 3, other.c);
    }
    bool operator!=(const StructWithDefaults & other) const
    {
        return !(*this == other);
    }
};

REFLECT_CLASS_START(StructWithDefaults, 0)
    REFLECT_MEMBER(a)
    REFLECT_MEMBER(b)
    REFLECT_MEMBER(c)
REFLECT_CLASS_END()

TEST(metafast, defaults)
{
    StructWithDefaults a;
    StructWithDefaults b = roundtrip(a);
    ASSERT_EQ(a, b);

    ++a.a;
    b = roundtrip(a);
    ASSERT_EQ(a, b);
    --a.a;

    a.b.push_back(5.0f);
    b = roundtrip(a);
    ASSERT_EQ(a, b);
    a.b.pop_back();

    --a.c[2];
    b = roundtrip(a);
    ASSERT_EQ(a, b);

    a.b.push_back(6.0f);
    b = roundtrip(a);
    ASSERT_EQ(a, b);
}

struct DerivedWithDefaults : StructWithDefaults
{
    bool d = true;

    bool operator==(const DerivedWithDefaults & other) const
    {
        return d == other.d && StructWithDefaults::operator==(other);
    }
    bool operator!=(const DerivedWithDefaults & other) const
    {
        return !(*this == other);
    }
};

REFLECT_CLASS_START(DerivedWithDefaults, 0)
    REFLECT_BASE(StructWithDefaults)
    REFLECT_MEMBER(d)
REFLECT_CLASS_END()

TEST(metafast, default_base)
{
    DerivedWithDefaults a;
    DerivedWithDefaults b = roundtrip(a);
    ASSERT_EQ(a, b);

    a.d = false;
    b = roundtrip(a);
    ASSERT_EQ(a, b);

    a.d = true;
    a.b.push_back(5.0f);
    b = roundtrip(a);
    ASSERT_EQ(a, b);

    a.d = false;
    b = roundtrip(a);
    ASSERT_EQ(a, b);
}

TEST(metafast, two_defaults)
{
    std::vector<DerivedWithDefaults> a(2);
    a[1].b.push_back(6.0f);

    std::vector<DerivedWithDefaults> b = roundtrip(a);
    ASSERT_EQ(a, b);
}

#endif
