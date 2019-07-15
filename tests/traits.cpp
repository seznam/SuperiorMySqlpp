/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>


using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;
using namespace SuperiorMySqlpp::Traits;

using std::is_same;
using std::decay;

template<typename...>
struct T;


go_bandit([](){
    describe("Test type traits", [&](){
        it("can do ReplaceType", [&](){
            AssertThat((is_same<
                ReplaceType_t<T, int, long, float, int, long>,
                T<float, long, long>
            >::value), IsTrue());

            AssertThat((is_same<
                ReplaceType_t<T, int, long>,
                T<>
            >::value), IsTrue());

            AssertThat((is_same<
                ReplaceType_t<T, int, long, int>,
                T<long>
            >::value), IsTrue());

            AssertThat((is_same<
                ReplaceType_t<T, int, long, int, int>,
                T<long, long>
            >::value), IsTrue());
        });

        it("can do IsContained trait", [&](){
            AssertThat((IsContained<int>::value), IsFalse());
            AssertThat((IsContained<int, int>::value), IsTrue());
            AssertThat((IsContained<int, long>::value), IsFalse());
            AssertThat((IsContained<int, int, long>::value), IsTrue());
            AssertThat((IsContained<int, long, int>::value), IsTrue());
            AssertThat((IsContained<int, char, long>::value), IsFalse());
            AssertThat((IsContained<int, long, short>::value), IsFalse());
        });
    });
});
