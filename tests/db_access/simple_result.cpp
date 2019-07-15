/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>

#include "settings.hpp"

using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;


go_bandit([](){
    describe("Test simple result", [&](){
        auto& s = getSettingsRef();
        Connection connection{s.database, s.user, s.password, s.host, s.port};

        it("can do simple query", [&](){
            auto query = connection.makeQuery("INSERT INTO test_superior_sqlpp.xuser2 (`id`, `name`) VALUE (77, 'olda');");
            query.execute();
        });

        it("can do simple multi-statement query", [&](){
            auto query = connection.makeQuery(
                "INSERT INTO test_superior_sqlpp.xuser2 (`id`, `name`) VALUE (80, 'olda0');"
                "INSERT INTO test_superior_sqlpp.xuser2 (`id`, `name`) VALUE (81, 'olda1');"
            );
            query.execute();
            do
            {
                std::stringstream{} << "affected rows: " << query.affectedRows() << std::endl;
            } while (query.nextResult());
        });
    });
});


