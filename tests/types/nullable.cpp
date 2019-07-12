/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>


using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;
using namespace std::string_literals;


go_bandit([](){
    describe("Test nullable", [&](){
        it("can do basic operations", [&](){
            Nullable<std::string> emptyNullable{};
            AssertThat(static_cast<bool>(emptyNullable), IsFalse());


            Nullable<std::string> nullable{"125"};
            AssertThat(static_cast<bool>(nullable), IsTrue());
            AssertThat(*nullable, Equals("125"));

            nullable = "42"s;
            AssertThat(static_cast<bool>(nullable), IsTrue());
            AssertThat(*nullable, Equals("42"));

            nullable = disengagedOption;
            AssertThat(static_cast<bool>(nullable), IsFalse());

            AssertThrows(BadNullableAccess, nullable.value());

            AssertThat(nullable.valueOr("11"), Equals("11"));

            AssertThat(Nullable<std::string>{"31"}, Equals(Nullable<std::string>{"31"}));
            AssertThat(Nullable<int>{21} < Nullable<int>{22}, IsTrue());


            Nullable<int> null{};
            {
                Nullable<int> nullable{5};
                nullable = null;
                AssertThat(nullable.isValid(), IsFalse());
                AssertThat(nullable.detail_getNullRef(), IsTrue());
            }

            {
                Nullable<int> nullable{5};
                nullable = Nullable<int>{};
                AssertThat(nullable.isValid(), IsFalse());
                AssertThat(nullable.detail_getNullRef(), IsTrue());
            }

            {
                Nullable<int> nullable{5};
                nullable = disengagedOption;
                AssertThat(nullable.isValid(), IsFalse());
                AssertThat(nullable.detail_getNullRef(), IsTrue());

                AssertThat((std::is_same<decltype(::SuperiorMySqlpp::disengagedOption), decltype(::SuperiorMySqlpp::null)>::value), IsTrue());
            }
        });

        it("can be messed around through mysql bindings", [&](){
            Nullable<int> nullable{5};
            AssertThat(*nullable, Equals(5));
            *&nullable.detail_getPayloadRef() = 42;
            AssertThat(*nullable, Equals(42));
            AssertThat(static_cast<bool>(nullable), IsTrue());

            *&nullable.detail_getNullRef() = true;
            AssertThat(static_cast<bool>(nullable), IsFalse());

            *&nullable.detail_getNullRef() = false;
            AssertThat(static_cast<bool>(nullable), IsTrue());


            Nullable<int> emptyNullable{};
            AssertThat(emptyNullable.isValid(), IsFalse());
            AssertThat(emptyNullable.detail_getNullRef(), IsTrue());
        });

        it("can wrap advanced types", [&](){
            Nullable<std::string> nullable1{};
            Nullable<std::string> nullable2{"aaa"};

            Nullable<StringData> nullable3{};
            Nullable<StringData> nullable4{inPlace, "aaa"};
        });
    });
});
