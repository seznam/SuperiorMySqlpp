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
    describe("Test ArrayBase", [&](){
        it("can do basic operations", [&](){
            {
                constexpr unsigned long arraySize = 0;
                ArrayBase<arraySize> array{};

                AssertThat(array.begin()==array.cbegin(), IsTrue());
                AssertThat(array.end()==array.cend(), IsTrue());
                AssertThat(array.begin()==array.end(), IsTrue());
                AssertThat(array.cbegin()==array.cend(), IsTrue());
                AssertThat(array.end()==array.endOfStorage(), IsTrue());
                AssertThat(array.cend()==array.cendOfStorage(), IsTrue());

                AssertThat(array.empty(), IsTrue());
                AssertThat(array.size(), Equals(0u));
                AssertThat(array.maxSize(), Equals(arraySize));
            }

            {
                constexpr unsigned long arraySize = 1;
                ArrayBase<arraySize> array{};

                AssertThat(array.begin()==array.cbegin(), IsTrue());
                AssertThat(array.end()==array.cend(), IsTrue());
                AssertThat(array.begin()==array.end(), IsTrue());
                AssertThat(array.cbegin()==array.cend(), IsTrue());
                AssertThat(array.end()+arraySize==array.endOfStorage(), IsTrue());
                AssertThat(array.cend()+arraySize==array.cendOfStorage(), IsTrue());

                AssertThat(array.front(), Equals(*array.begin()));
                AssertThat(array.back(), Equals(*(array.end())));

                AssertThat(array.empty(), IsTrue());
                AssertThat(array.size(), Equals(0u));
                AssertThat(array.maxSize(), Equals(arraySize));


                array.counterRef() = 1;
                array[0] = 'a';


                AssertThat(array.begin()==array.cbegin(), IsTrue());
                AssertThat(array.end()==array.cend(), IsTrue());
                AssertThat(array.begin()+1==array.end(), IsTrue());
                AssertThat(array.cbegin()+1==array.cend(), IsTrue());
                AssertThat(array.end()+arraySize-1==array.endOfStorage(), IsTrue());
                AssertThat(array.cend()+arraySize-1==array.cendOfStorage(), IsTrue());

                AssertThat(array.front(), Equals(*array.begin()));
                AssertThat(array.back(), Equals(*(array.end()-1)));

                AssertThat(array.empty(), IsFalse());
                AssertThat(array.size(), Equals(1u));
                AssertThat(array.maxSize(), Equals(arraySize));

                AssertThat(array[0], Equals('a'));
            }
        });
    });
});
