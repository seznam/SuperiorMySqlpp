/*
 *  Author: Tomas Nozicka
 */

#include <sstream>
#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>

#include "settings.hpp"

using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;


int getRowsCount(Connection& connection)
{
    auto&& query = connection.makeQuery("SELECT * FROM test_superior_sqlpp.xuser6");
    query.execute();
    auto&& result = query.store();
    return result.getRowsCount();
}



go_bandit([](){
    describe("Test transactions", [&](){
        auto& s = getSettingsRef();
        Connection connection{s.database, s.user, s.password, s.host, s.port};

        it("does rollback if exception has been thrown", [&](){
            try
            {
                Transaction transaction{connection};
                connection.makeQuery("INSERT INTO test_superior_sqlpp.xuser6 (`id`, `name`) VALUE (11, 'olda');").execute();
                throw 6;
                connection.makeQuery("INSERT INTO test_superior_sqlpp.xuser6 (`id`, `name`) VALUE (12, 'olda');").execute();
            }
            catch (int&)
            {
                AssertThat(getRowsCount(connection), Equals(0));
            }
        });

        it("does commit if no exception has been thrown", [&](){
            {
                Transaction transaction{connection};
                connection.makeQuery("INSERT INTO test_superior_sqlpp.xuser6 (`id`, `name`) VALUE (13, 'olda');").execute();
                connection.makeQuery("INSERT INTO test_superior_sqlpp.xuser6 (`id`, `name`) VALUE (14, 'olda');").execute();
            }

            AssertThat(getRowsCount(connection), Equals(2));
        });

        it("dispatches valid query for different parameters", [&](){
            { Transaction transaction{connection, TransactionCharacteristics::WithConsistentSnapshot}; }
            { Transaction transaction{connection, TransactionCharacteristics::ReadOnly}; }
            { Transaction transaction{connection, TransactionCharacteristics::ReadWrite}; }
            { Transaction transaction{connection, IsolationLevel::ReadUncommitted}; }
            { Transaction transaction{connection, IsolationLevel::ReadCommitted}; }
            { Transaction transaction{connection, IsolationLevel::RepeatableRead}; }
            { Transaction transaction{connection, IsolationLevel::Serializable}; }
        });
    });
});


