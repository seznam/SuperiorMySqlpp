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
    describe("Test Time", [&](){
        it("can do basic operations", [&](){
            {
                Time time0{100, 59, 58};
                Time time1{time0};
                Time time = time1;
                AssertThat(time.getHour(), Equals(100u));
                AssertThat(time.getMinute(), Equals(59u));
                AssertThat(time.getSecond(), Equals(58u));
                AssertThat(time.isNegative(), IsFalse());
            }

            {
                Time time{100, 59, 58, true};
                AssertThat(time.getHour(), Equals(100u));
                AssertThat(time.getMinute(), Equals(59u));
                AssertThat(time.getSecond(), Equals(58u));
                AssertThat(time.isNegative(), IsTrue());
            }
        });

        it("can do basic operators", [&](){
            AssertThat((Time{10, 20, 42}==Time{10, 20, 42}), IsTrue());
            AssertThat((Time{11, 20, 42}==Time{10, 20, 42}), IsFalse());
            AssertThat((Time{10, 22, 42}==Time{10, 20, 42}), IsFalse());
            AssertThat((Time{10, 20, 44}==Time{10, 20, 42}), IsFalse());

            AssertThat((Time{10, 20, 42}!=Time{10, 20, 42}), IsFalse());
            AssertThat((Time{11, 20, 42}!=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{10, 22, 42}!=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{10, 20, 44}!=Time{10, 20, 42}), IsTrue());

            AssertThat((Time{10, 20, 42}<Time{10, 20, 42}), IsFalse());
            AssertThat((Time{ 9, 20, 42}<Time{10, 20, 42}), IsTrue());
            AssertThat((Time{10, 19, 42}<Time{10, 20, 42}), IsTrue());
            AssertThat((Time{10, 20, 41}<Time{10, 20, 42}), IsTrue());
            AssertThat((Time{ 9, 40, 42}<Time{10, 20, 42}), IsTrue());
            AssertThat((Time{ 9, 20, 52}<Time{10, 20, 42}), IsTrue());
            AssertThat((Time{11, 20, 42}<Time{10, 20, 42}), IsFalse());
            AssertThat((Time{10, 25, 42}<Time{10, 20, 42}), IsFalse());
            AssertThat((Time{10, 20, 45}<Time{10, 20, 42}), IsFalse());

            AssertThat((Time{10, 20, 42}<=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{ 9, 20, 42}<=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{10, 19, 42}<=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{10, 20, 41}<=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{ 9, 25, 41}<=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{ 9, 20, 45}<=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{11, 20, 42}<=Time{10, 20, 42}), IsFalse());
            AssertThat((Time{10, 21, 42}<=Time{10, 20, 42}), IsFalse());
            AssertThat((Time{10, 20, 43}<=Time{10, 20, 42}), IsFalse());
            AssertThat((Time{11, 20, 43}<=Time{10, 20, 42}), IsFalse());

            AssertThat((Time{10, 20, 42}>Time{10, 20, 42}), IsFalse());
            AssertThat((Time{ 9, 20, 42}>Time{10, 20, 42}), IsFalse());
            AssertThat((Time{10, 19, 42}>Time{10, 20, 42}), IsFalse());
            AssertThat((Time{10, 20, 41}>Time{10, 20, 42}), IsFalse());
            AssertThat((Time{ 9, 40, 42}>Time{10, 20, 42}), IsFalse());
            AssertThat((Time{ 9, 20, 52}>Time{10, 20, 42}), IsFalse());
            AssertThat((Time{11, 20, 42}>Time{10, 20, 42}), IsTrue());
            AssertThat((Time{10, 25, 42}>Time{10, 20, 42}), IsTrue());
            AssertThat((Time{10, 20, 45}>Time{10, 20, 42}), IsTrue());

            AssertThat((Time{10, 20, 42}>=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{ 9, 20, 42}>=Time{10, 20, 42}), IsFalse());
            AssertThat((Time{10, 19, 42}>=Time{10, 20, 42}), IsFalse());
            AssertThat((Time{10, 20, 41}>=Time{10, 20, 42}), IsFalse());
            AssertThat((Time{ 9, 25, 41}>=Time{10, 20, 42}), IsFalse());
            AssertThat((Time{ 9, 20, 45}>=Time{10, 20, 42}), IsFalse());
            AssertThat((Time{11, 20, 42}>=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{10, 21, 42}>=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{10, 20, 42}>=Time{10, 20, 42}), IsTrue());
            AssertThat((Time{11, 20, 43}>=Time{10, 20, 42}), IsTrue());
        });
    });
});
