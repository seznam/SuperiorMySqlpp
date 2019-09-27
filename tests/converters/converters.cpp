/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <bandit/bandit.h>
#include <superior_mysqlpp/converters.hpp>

struct Foo {
    int intValue;
    Foo(int intValue) : intValue(intValue) {}

    bool operator==(const Foo &other) const {
        return intValue == other.intValue;
    }
};

struct Bar {
    int intValue;
    Bar(int intValue) : intValue(intValue) {}

    bool operator==(const Bar &other) const {
        return intValue == other.intValue;
    }
};



namespace SuperiorMySqlpp { namespace Converters {
    template<typename T>
    inline std::enable_if_t<std::is_same<T, Foo>::value, T> to(
        const char* str,
        unsigned int length
    ) {
        return to<int>(str, length);
    }

}}

template<typename T>
T decode(const char* str, unsigned int length) {
    return SuperiorMySqlpp::Converters::to<T>(str, length);
}

namespace SuperiorMySqlpp { namespace Converters {

    template<typename T>
    struct To<T, std::enable_if_t<std::is_same<T, Bar>::value>> {
        T operator()(const char* str, unsigned int length) {
            return To<int>{}(str, length);
        }
    };

}}

using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;


go_bandit([](){
    describe("Test converters", [&](){
        it("can convert string to int", [&](){
            std::string s{"125"};
            auto i = Converters::to<int>(s.data(), s.length());
            AssertThat(i, Equals(125));
        });
        it("can custom-convert old-style", [&](){
            std::string s{"125"};
            auto i = Converters::to<Foo>(s.data(), s.length());
            AssertThat(i, Equals(Foo{125}));
        });
        it("can custom-convert new-style", [&](){
            std::string s{"125"};
            auto i = Converters::to<Bar>(s.data(), s.length());
            AssertThat(i, Equals(Bar{125}));
        });
    });
});


