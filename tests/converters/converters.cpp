/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp/converters.hpp>


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
    });
});


