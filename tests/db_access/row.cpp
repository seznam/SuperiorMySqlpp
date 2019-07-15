/*
 * Author: Radek Smejdir
 */

#include <bandit/bandit.h>
#include <sstream>

#include <superior_mysqlpp.hpp>

#include "settings.hpp"

using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;

go_bandit([](){
    describe("Test Row", [&](){
        auto& s = getSettingsRef();
        auto connection = std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port);

        it("can print nonnull data into stream", [&](){
            auto query = connection->makeQuery("SELECT * FROM `test_superior_sqlpp`.`row` WHERE `id`=1");
            query.execute();
            auto result = query.store();

            std::ostringstream out;
            out << result.fetchRow();

            AssertThat(result.getRowsCount(), Equals(1u));
            AssertThat(out.str(), Equals("[1, value]"));
        });

        it("can print data containing null value into stream", [&](){
            auto query = connection->makeQuery("SELECT * FROM `test_superior_sqlpp`.`row` WHERE `id`=2");
            query.execute();
            auto result = query.store();

            std::ostringstream out;
            out << result.fetchRow();

            AssertThat(result.getRowsCount(), Equals(1u));
            AssertThat(out.str(), Equals("[2, ]"));
        });
    });
});
