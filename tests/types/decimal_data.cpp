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
    describe("Test DecimalData", [&](){
        it("has required properties", [&](){
            /*
             * Ensure that most of the functionality like begin, end, ... is tested in class ArrayBase.
             * (This doesn't exclude the possibility of hiding these methods, but it's simple and better than nothing.)
             */
            AssertThat((std::is_base_of<ArrayBase<42>, DecimalDataBase<42>>::value), IsTrue());

            AssertThat((std::is_convertible<DecimalData, std::string>::value), IsTrue());
            AssertThat((std::is_convertible<const DecimalData&, std::string>::value), IsTrue());
        });

        it("can do basic operations", [&](){
            {
                ArrayBase<5> a{};
                DecimalData decimalData{};
            }

            {
                DecimalData decimalData{"al\0fa"};
                AssertThat(decimalData.size(), Equals(5u));
                std::string s{decimalData};
                AssertThat(s.size(), Equals(5u));
                AssertThat(s, Equals("al\0fa"s));

                std::string sv{decimalData};
                AssertThat(sv.size(), Equals(5u));
                AssertThat(sv, Equals("al\0fa"s));
            }

            std::string string{"aaa\0bbb"};
            DecimalData decimalData{string};
            AssertThat(decimalData, Equals(string));

            decimalData = "c\0d"s;
            AssertThat(decimalData, Equals("c\0d"s));

            decimalData = "aaghhh";
            AssertThat(decimalData, Equals("aaghhh"));

            std::string s = decimalData;
            AssertThat(s, Equals("aaghhh"));

            StringView sv = decimalData;
            AssertThat(sv, Equals("aaghhh"));

            sv = "fdgsg";
            decimalData = sv;
            AssertThat(decimalData.getStringView(), Equals(sv));
        });

        it("can do conversions", [&](){
            int i = -159417;
            DecimalData sdi{i};
            AssertThat(sdi.getStringView(), Equals("-159417"));
            AssertThat(static_cast<int>(sdi), Equals(i));
            AssertThat(sdi.to<int>(), Equals(i));

            /*
             * This is depending a lot on locale and other factors, so it might be removed if any problems
             * shall occur. Additionally, in C++ double->string->double might generate different numbers!
             */
            double d = -159417.8448;
            DecimalData sdd{d};
            AssertThat(sdd.getStringView(), Equals("-159417.844800"));
            AssertThat(static_cast<double>(sdd), Equals(d));
            AssertThat(sdd.to<double>(), Equals(d));

            int64_t i64 = 9223372036854775807L;
            DecimalData sdi64{i64};
            AssertThat(sdi64.getStringView(), Equals("9223372036854775807"));
            AssertThat(static_cast<int64_t>(sdi64), Equals(i64));
            AssertThat(sdi64.to<int64_t>(), Equals(i64));

            uint64_t u64 = 18446744073709551615UL;
            DecimalData sdu64{u64};
            AssertThat(sdu64.getStringView(), Equals("18446744073709551615"));
            AssertThat(static_cast<uint64_t>(sdu64), Equals(u64));
            AssertThat(sdu64.to<uint64_t>(), Equals(u64));
        });
    });
});
