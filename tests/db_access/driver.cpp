/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp/low_level/dbdriver.hpp>

#include "settings.hpp"


using namespace bandit;
using namespace SuperiorMySqlpp;
using namespace SuperiorMySqlpp::LowLevel;
using namespace std::string_literals;

go_bandit([](){
    describe("Test driver", [&](){
        auto& s = getSettingsRef();

        it("will initialize", [&](){
            DBDriver driver{};
            driver.connect(s.host.c_str(), s.user.c_str(), s.password.c_str(), nullptr, s.port, nullptr);
        });

        it("can make hex string from string types", [&]() {
            DBDriver driver{};
            std::string expected{"666F6F626172"};

            AssertThat(driver.makeHexString("foobar"), Equals(expected));
            AssertThat(driver.makeHexString("foobar"s), Equals(expected));
        });
    });
});


