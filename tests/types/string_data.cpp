/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <tuple>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>


using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;
using namespace std::string_literals;


go_bandit([](){
    describe("Test StringData", [&](){
        it("has required properties", [&](){
            /*
             * Ensure that most of the functionality like begin, end, ... is tested in class ArrayBase.
             * (This doesn't exclude the possibility of hiding these methods, but it's simple and better than nothing.)
             */
            AssertThat((std::is_base_of<ArrayBase<42>, StringDataBase<42>>::value), IsTrue());

            AssertThat((std::is_convertible<StringData, std::string>::value), IsTrue());
            AssertThat((std::is_convertible<const StringData&, std::string>::value), IsTrue());
        });

        it("can do basic operations", [&](){
            {
                ArrayBase<5> a{};
                StringData stringData{};
            }

            {
                StringData stringData{"al\0fa"};
                AssertThat(stringData.size(), Equals(5u));
                std::string s{stringData};
                AssertThat(s.size(), Equals(5u));
                AssertThat(s, Equals("al\0fa"s));

                std::string sv{stringData};
                AssertThat(sv.size(), Equals(5u));
                AssertThat(sv, Equals("al\0fa"s));
            }

            std::string string{"aaa\0bbb"};
            StringData stringData{string};
            AssertThat(stringData, Equals(string));

            stringData = "c\0d"s;
            AssertThat(stringData, Equals("c\0d"s));

            stringData = "aaghhh";
            AssertThat(stringData, Equals("aaghhh"));

            std::string s = stringData;
            AssertThat(s, Equals("aaghhh"));

            StringView sv = stringData;
            AssertThat(sv, Equals("aaghhh"));

            sv = "fdgsg";
            stringData = sv;
            AssertThat(stringData.getStringView(), Equals(sv));
        });

        it("can do conversions", [&](){
            int i = -159417;
            StringData sdi{i};
            AssertThat(sdi.getStringView(), Equals("-159417"));
            AssertThat(static_cast<int>(sdi), Equals(i));
            AssertThat(sdi.to<int>(), Equals(i));

            /*
             * This is depending a lot on locale and other factors, so it might be removed if any problems
             * shall occur. Additionally, in C++ double->string->double might generate different numbers!
             */
            double d = -159417.8448;
            StringData sdd{d};
            AssertThat(sdd.getStringView(), Equals("-159417.844800"));
            AssertThat(static_cast<double>(sdd), Equals(d));
            AssertThat(sdd.to<double>(), Equals(d));
        });

        it("can do basic operators", [&](){
            AssertThat(StringData{}==StringData{}, IsTrue());
            AssertThat(StringData{}==StringData{""}, IsTrue());
            AssertThat(StringData{}==StringData{"b"}, IsFalse());
            AssertThat(StringData{"a"}==StringData{"b"}, IsFalse());

            AssertThat(StringData{}!=StringData{}, IsFalse());
            AssertThat(StringData{}!=StringData{""}, IsFalse());
            AssertThat(StringData{}!=StringData{"b"}, IsTrue());
            AssertThat(StringData{"a"}!=StringData{"b"}, IsTrue());
        });
    });
});
