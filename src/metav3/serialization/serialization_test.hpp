#pragma once

#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
#include "metav3/metav3.hpp"
#include "metav3/metav3_stl.hpp"

template<typename Serializer, typename T>
void parse_test(const T & original)
{
    Serializer serializer;
    auto serialized = serializer.serialize(original);
    T compare;
    ASSERT_TRUE(serializer.deserialize(compare, serialized));
    ASSERT_EQ(original, compare);
}

template<typename T>
struct SerializationTest : ::testing::Test
{
};

TYPED_TEST_CASE_P(SerializationTest);

TYPED_TEST_P(SerializationTest, simple)
{
#define SIMPLE_TEST(type, value)\
    parse_test<TypeParam>(type(value));
    SIMPLE_TEST(uint8_t, 0);
    SIMPLE_TEST(int, 15);
    SIMPLE_TEST(int, -10001);
    SIMPLE_TEST(float, -1.5);
    SIMPLE_TEST(double, 700.2);
    SIMPLE_TEST(bool, true);
    SIMPLE_TEST(bool, false);
    SIMPLE_TEST(char, 'a');
    SIMPLE_TEST(std::string, "foo");
    SIMPLE_TEST(std::string, "");
#undef SIMPLE_TEST
}

TYPED_TEST_P(SerializationTest, all_the_int_types)
{
    parse_test<TypeParam>(static_cast<signed char>(1));
    parse_test<TypeParam>(static_cast<unsigned char>(2));

    parse_test<TypeParam>(static_cast<signed short>(3));
    parse_test<TypeParam>(static_cast<short>(4));
    parse_test<TypeParam>(static_cast<unsigned short>(5));

    parse_test<TypeParam>(static_cast<signed>(6));
    parse_test<TypeParam>(static_cast<int>(7));
    parse_test<TypeParam>(static_cast<unsigned>(8));

    parse_test<TypeParam>(static_cast<signed long>(9));
    parse_test<TypeParam>(static_cast<long>(10));
    parse_test<TypeParam>(static_cast<unsigned long>(11));

    parse_test<TypeParam>(static_cast<signed long long>(12));
    parse_test<TypeParam>(static_cast<long long>(13));
    parse_test<TypeParam>(static_cast<unsigned long long>(14));
}

TYPED_TEST_P(SerializationTest, list)
{
    parse_test<TypeParam>(std::vector<int>());
    parse_test<TypeParam>(std::vector<int>{ 1, 2, 3, 4, 5 });
    parse_test<TypeParam>(std::set<int>{ 1, 2, 3, 4, 5 });
    parse_test<TypeParam>(std::array<int, 5>{ { 1, 2, 3, 4, 5 } });
    parse_test<TypeParam>(std::array<std::string, 3>{ { "foo", "bar", "baz" } });
    parse_test<TypeParam>(std::vector<std::vector<int> >{ { 1, 2, 3, 4, 5 }, { 6, 7, 9 } });
}

TYPED_TEST_P(SerializationTest, map)
{
    parse_test<TypeParam>(std::map<int, int>{});
    parse_test<TypeParam>(std::map<int, int>{ { 1, 2 }, { 3, 4 }, { 5, 6 }});
}

TYPED_TEST_P(SerializationTest, string)
{
    parse_test<TypeParam>(std::string("Hello,\nWorld!"));
    parse_test<TypeParam>(std::string("π𝄞 "));
    parse_test<TypeParam>(StringView<const char>("range test"));
    parse_test<TypeParam>('\'');
    parse_test<TypeParam>('\n');
    parse_test<TypeParam>('c');
}

namespace srlztest
{
enum an_enum
{
    EnumValueOne,
    EnumValueTwo
};
}

TYPED_TEST_P(SerializationTest, enum_test)
{
    parse_test<TypeParam>(srlztest::EnumValueOne);
    parse_test<TypeParam>(srlztest::EnumValueTwo);
}


namespace srlztest
{
struct base_struct_a
{
    base_struct_a(int a = 0)
        : a(a)
    {
    }
    int a;

    bool operator==(const base_struct_a & other) const
    {
        return a == other.a;
    }
};
struct base_struct_b
{
    base_struct_b(int b = 0)
        : b(b)
    {
    }
    int b;

    bool operator==(const base_struct_b & other) const
    {
        return b == other.b;
    }
};
struct derived_struct : base_struct_a, base_struct_b
{
    derived_struct(int a = 0, int b = 0, int c = 0)
        : base_struct_a(a), base_struct_b(b), c(c)
    {
    }
    int c;

    bool operator==(const derived_struct & other) const
    {
        return base_struct_a::operator==(other) && base_struct_b::operator==(other) && c == other.c;
    }
};

struct base_struct_d
{
    base_struct_d(int d)
        : d(d)
    {
    }

    int d;

    bool operator==(const base_struct_d & other) const
    {
        return d == other.d;
    }
};
struct derived_derived : base_struct_d, derived_struct
{
    derived_derived(int a = 0, int b = 0, int c = 0, int d = 0, int e = 0)
        : base_struct_d(d), derived_struct(a, b, c), e(e)
    {
    }
    base_struct_a e;

    bool operator==(const derived_derived & other) const
    {
        return base_struct_d::operator==(other) && derived_struct::operator==(other) && e == other.e;
    }
};
}

TYPED_TEST_P(SerializationTest, derived_two_levels)
{
    parse_test<TypeParam>(srlztest::derived_derived(5, 6, 7, 8, 9));
}

namespace srlztest
{
struct pointer_to_struct_base
{
    pointer_to_struct_base(int a)
        : a(a)
    {
    }
    pointer_to_struct_base(const pointer_to_struct_base &) = default;
    pointer_to_struct_base & operator=(const pointer_to_struct_base &) = default;
    virtual ~pointer_to_struct_base() = default;

    virtual bool operator==(const pointer_to_struct_base & other) const
    {
        return a == other.a;
    }

    int a = 0;
};
struct pointer_to_struct_offsetting_base
{
    pointer_to_struct_offsetting_base(int c)
        : c(c)
    {
    }
    int c;

    bool operator==(const pointer_to_struct_offsetting_base & other) const
    {
        return c == other.c;
    }
};

struct pointer_to_struct_derived : pointer_to_struct_offsetting_base, pointer_to_struct_base
{
    pointer_to_struct_derived(int a = 0, int b = 0, int c = 0)
        : pointer_to_struct_offsetting_base(c), pointer_to_struct_base(a), b(b)
    {
    }

    bool operator==(const pointer_to_struct_derived & other) const
    {
        return pointer_to_struct_offsetting_base::operator==(other) && pointer_to_struct_base::operator==(other) && b == other.b && a == other.a;
    }
    bool operator==(const pointer_to_struct_base & other) const override
    {
        const pointer_to_struct_derived * as_derived = dynamic_cast<const pointer_to_struct_derived *>(&other);
        if (as_derived) return *this == *as_derived;
        else return pointer_to_struct_base::operator==(other);
    }

    int b;
};
}


template<typename Serializer, typename T>
void ptr_parse_test(const T & original)
{
    Serializer serializer;
    auto serialized = serializer.serialize(original);
    T compare;
    ASSERT_TRUE(serializer.deserialize(compare, serialized));
    ASSERT_TRUE(original == compare || *original == *compare);
}

TYPED_TEST_P(SerializationTest, pointer_to_struct)
{
    ptr_parse_test<TypeParam>(std::unique_ptr<srlztest::pointer_to_struct_derived>());
    ptr_parse_test<TypeParam>(std::unique_ptr<srlztest::pointer_to_struct_derived>(new srlztest::pointer_to_struct_derived(5, 6, 7)));
    ptr_parse_test<TypeParam>(std::unique_ptr<srlztest::pointer_to_struct_base>(new srlztest::pointer_to_struct_derived(6, 7, 8)));
}

namespace srlztest
{
struct pointer_member
{
    pointer_member() = default;
    pointer_member(int a, int b, int c)
        : ptr(new pointer_to_struct_derived(a, b, c))
    {
    }
    std::unique_ptr<pointer_to_struct_base> ptr;

    bool operator==(const pointer_member & other) const
    {
        if (ptr) return other.ptr && *ptr == *other.ptr;
        else return !other.ptr;
    }
};
#ifdef META_SUPPORTS_CONDITIONAL_MEMBER
struct conditional_member
{
    conditional_member(bool which = true, int i = 0)
        : which(which)
    {
        if (which) a = i;
        else b = i;
    }
    bool operator==(const conditional_member & other) const
    {
        return which == other.which && a == other.a && b == other.b;
    }

    int a = 0;
    int b = 0;
    bool which;
};
#endif

#ifdef META_SUPPORTS_TYPE_ERASURE
struct type_erasure
{
    type_erasure() = default;
    type_erasure(type_erasure &&) = default;
    template<typename T>
    type_erasure(T && object)
        : object(new derived<typename std::decay<T>::type>(std::forward<T>(object)))
    {
    }
    type_erasure & operator=(type_erasure &&) = default;
    template<typename T, typename decayed = typename std::decay<T>::type>
    decayed & operator=(T && other)
    {
        object = std::unique_ptr<base>(new derived<decayed>(std::forward<T>(other)));
        return static_cast<derived<decayed> &>(*object).object;
    }

    bool operator==(const type_erasure & other) const
    {
        if (object) return other.object && *object == *other.object;
        else return !other.object;
    }

    const std::type_info & target_type() const
    {
        if (object)
            return object->target_type();
        else
            return typeid(void);
    }
    template<typename T>
    T * target()
    {
        if (object && object->target_type() == typeid(T))
            return &static_cast<derived<T> &>(*object).object;
        else
            return nullptr;
    }

private:
    struct base
    {
        virtual const std::type_info & target_type() const = 0;
        virtual bool operator==(const base & other) const = 0;
        virtual ~base() = default;
    };
    template<typename T>
    struct derived : base
    {
        derived(T && object)
            : object(std::move(object))
        {
        }
        derived(const T & object)
            : object(object)
        {
        }
        virtual const std::type_info & target_type() const override
        {
            return typeid(T);
        }
        virtual bool operator==(const base & other) const override
        {
            if (other.target_type() != target_type())
                return false;
            return object == static_cast<const derived &>(other).object;
        }

        T object;
    };

    std::unique_ptr<base> object;
};

struct type_erasure_member
{
    type_erasure_member(int i = 0)
        : a(i)
    {
    }

    bool operator==(const type_erasure_member & other) const
    {
        return a == other.a;
    }
    int a = 0;
};
#endif
}
TYPED_TEST_P(SerializationTest, pointer_member)
{
    parse_test<TypeParam>(srlztest::pointer_member());
    parse_test<TypeParam>(srlztest::pointer_member(5, 6, 7));
}

#ifdef META_SUPPORTS_CONDITIONAL_MEMBER
TYPED_TEST_P(SerializationTest, conditional_member)
{
    parse_test<TypeParam>(srlztest::conditional_member(true, 1));
    parse_test<TypeParam>(srlztest::conditional_member(false, 1));
}
#else
TYPED_TEST_P(SerializationTest, conditional_member)
{
}
#endif

#ifdef META_SUPPORTS_TYPE_ERASURE
TYPED_TEST_P(SerializationTest, type_erasure)
{
    parse_test<TypeParam>(srlztest::type_erasure(srlztest::type_erasure_member(5)));
}
#else
TYPED_TEST_P(SerializationTest, type_erasure)
{
}
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
REGISTER_TYPED_TEST_CASE_P(SerializationTest, simple, all_the_int_types, list, map, string, enum_test, derived_two_levels, pointer_to_struct, pointer_member, conditional_member, type_erasure);
#pragma GCC diagnostic pop

#endif
