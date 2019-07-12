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
    describe("Test Date", [&](){
        it("can do basic operations", [&](){
            {
                Date date{2015, 12, 31};
                AssertThat(date.getYear(), Equals(2015u));
                AssertThat(date.getMonth(), Equals(12u));
                AssertThat(date.getDay(), Equals(31u));
            }
        });

        it("can do basic operators", [&](){
            AssertThat((Date{2010, 20, 12}==Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2011, 20, 12}==Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2010, 22, 12}==Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2010, 20, 14}==Date{2010, 20, 12}), IsFalse());

            AssertThat((Date{2010, 20, 12}!=Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2011, 20, 12}!=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2010, 22, 12}!=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2010, 20, 14}!=Date{2010, 20, 12}), IsTrue());

            AssertThat((Date{2010, 20, 12}<Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2009, 20, 12}<Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2010, 19, 12}<Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2010, 20, 11}<Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2009, 40, 12}<Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2009, 20, 22}<Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2011, 20, 12}<Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2010, 25, 12}<Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2010, 20, 15}<Date{2010, 20, 12}), IsFalse());

            AssertThat((Date{2010, 20, 12}<=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2009, 20, 12}<=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2010, 19, 12}<=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2010, 20, 11}<=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2009, 25, 11}<=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2009, 20, 15}<=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2011, 20, 12}<=Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2010, 21, 12}<=Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2010, 20, 13}<=Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2011, 20, 13}<=Date{2010, 20, 12}), IsFalse());

            AssertThat((Date{2010, 20, 12}>Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2009, 20, 12}>Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2010, 19, 12}>Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2010, 20, 11}>Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2009, 40, 12}>Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2009, 20, 22}>Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2011, 20, 12}>Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2010, 25, 12}>Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2010, 20, 15}>Date{2010, 20, 12}), IsTrue());

            AssertThat((Date{2010, 20, 12}>=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2009, 20, 12}>=Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2010, 19, 12}>=Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2010, 20, 11}>=Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2009, 25, 11}>=Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2009, 20, 15}>=Date{2010, 20, 12}), IsFalse());
            AssertThat((Date{2011, 20, 12}>=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2010, 21, 12}>=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2010, 20, 12}>=Date{2010, 20, 12}), IsTrue());
            AssertThat((Date{2011, 20, 13}>=Date{2010, 20, 12}), IsTrue());
        });
    });
});
