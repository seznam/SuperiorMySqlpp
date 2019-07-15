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
    describe("Test Datetime", [&](){
        it("can do basic operations", [&](){
            {
                Datetime datetime{9999, 12, 31, 23, 59, 57, 111};
                AssertThat(datetime.getYear(), Equals(9999u));
                AssertThat(datetime.getMonth(), Equals(12u));
                AssertThat(datetime.getDay(), Equals(31u));
                AssertThat(datetime.getHour(), Equals(23u));
                AssertThat(datetime.getMinute(), Equals(59u));
                AssertThat(datetime.getSecond(), Equals(57u));
                AssertThat(datetime.getSecondFraction(), Equals(111u));
            }
        });

        it("can do basic operators", [&](){
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 42}==Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2011, 2, 5, 12, 30, 50, 42}==Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 3, 5, 12, 30, 50, 42}==Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 6, 12, 30, 50, 42}==Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 13, 30, 50, 42}==Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 31, 50, 42}==Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 51, 42}==Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 43}==Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());

            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 42}!=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2011, 2, 5, 12, 30, 50, 42}!=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 3, 5, 12, 30, 50, 42}!=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 6, 12, 30, 50, 42}!=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 13, 30, 50, 42}!=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 31, 50, 42}!=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 51, 42}!=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 43}!=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());

            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2009, 2, 5, 12, 30, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 1, 5, 12, 30, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 4, 12, 30, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 11, 30, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 29, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 49, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 41}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2011, 2, 5, 12, 30, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 3, 5, 12, 30, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 6, 12, 30, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 13, 30, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 31, 50, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 51, 42}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 43}<Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());

            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2009, 2, 5, 12, 30, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 1, 5, 12, 30, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 4, 12, 30, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 11, 30, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 29, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 49, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 41}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2011, 2, 5, 12, 30, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 3, 5, 12, 30, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 6, 12, 30, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 13, 30, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 31, 50, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 51, 42}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 43}>Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());

            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2011, 2, 5, 12, 30, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 3, 5, 12, 30, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 6, 12, 30, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 13, 30, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 31, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 51, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 43}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2009, 2, 5, 12, 30, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 1, 5, 12, 30, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 4, 12, 30, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 11, 30, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 29, 50, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 49, 42}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 41}>=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());

            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2011, 2, 5, 12, 30, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 3, 5, 12, 30, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 6, 12, 30, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 13, 30, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 31, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 51, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 43}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsFalse());
            AssertThat((Datetime{2009, 2, 5, 12, 30, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 1, 5, 12, 30, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 4, 12, 30, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 11, 30, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 29, 50, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 49, 42}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
            AssertThat((Datetime{2010, 2, 5, 12, 30, 50, 41}<=Datetime{2010, 2, 5, 12, 30, 50, 42}), IsTrue());
        });
    });
});
