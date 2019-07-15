/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>

#include "settings.hpp"


using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;
using namespace std::string_literals;
using namespace std::chrono_literals;


template<typename C>
void testConnectionBySelect(C&& connection)
{
    auto query = connection.makeQuery(
        "SELECT * FROM test_superior_sqlpp.xuser_empty ORDER BY `id` ASC"
    );
    query.execute();

    StoreQueryResult result{query.store()};
    auto rowsCount = result.getRowsCount();
    AssertThat(rowsCount, Equals(0u));
}


template<typename T>
int count(const T& collection)
{
    using std::begin;
    using std::end;
    return std::distance(begin(collection), end(collection));
}


auto MySqlFactoryLambda = [](){
    return std::async(std::launch::async, [&](){
        return std::make_shared<SuperiorMySqlpp::Connection>(
        "databaseName",
        "user",
        "password",
        "host",
        42);
    });
};

class MySqlFactory
{
public:
    template<typename... Args>
    MySqlFactory(Args&&...)
    {}

    MySqlFactory(const MySqlFactory&) = default;
    MySqlFactory(MySqlFactory&&) = default;

    auto operator()() const
    {
        return MySqlFactoryLambda();
    }
};


go_bandit([](){
    describe("Test ms connection pools", [&](){
        auto& s = getSettingsRef();

        it("will instantiate", [&](){
            MasterSlaveConnectionPools<MySqlFactory, int>{std::forward_as_tuple(), std::forward_as_tuple(), std::forward_as_tuple()};
            MasterSlaveConnectionPools<MySqlFactory, std::string>{std::forward_as_tuple(), std::forward_as_tuple(), std::forward_as_tuple()};
            MasterSlaveSharedPtrPools<ConnectionPool<MySqlFactory>, int>{std::forward_as_tuple(), std::forward_as_tuple(), std::forward_as_tuple()};

            MasterSlaveConnectionPools<std::decay_t<decltype(MySqlFactoryLambda)>, int>{std::forward_as_tuple(MySqlFactoryLambda), std::forward_as_tuple(), std::forward_as_tuple()};
            MasterSlaveConnectionPools<std::decay_t<decltype(MySqlFactoryLambda)>, int>{std::forward_as_tuple(std::move(MySqlFactoryLambda)), std::forward_as_tuple(), std::forward_as_tuple()};
        });

        it("works only with master", [&](){
            auto masterFactory = [&](){
                return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
            };
            auto msConnectionPools = makeMasterSlaveConnectionPools(std::move(masterFactory));

            auto masterConnectionPtr = msConnectionPools.getMasterConnection();
            testConnectionBySelect(*masterConnectionPtr);

            // when there is no slave, connection to master shall be returned
            auto slaveConnectionPtr = msConnectionPools.getSlaveConnection();
            testConnectionBySelect(*slaveConnectionPtr);

            // there can be no slave -> LogicError
            AssertThrows(LogicError, msConnectionPools.getSlaveConnection(5));

            AssertThat(msConnectionPools.size(), Equals(1u));
            AssertThat(count(msConnectionPools), Equals(1));
        });

        it("works only with one slave", [&](){
            auto factory = [&](){
                return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
            };
            auto msConnectionPools = makeMasterSlaveConnectionPools(factory);


            msConnectionPools.emplaceSlavePool(42, makeConnectionPool(factory));

            auto slaveConnectionPtr = msConnectionPools.getSlaveConnection();
            testConnectionBySelect(*slaveConnectionPtr);

            slaveConnectionPtr = msConnectionPools.getSlaveConnection(42);
            testConnectionBySelect(*slaveConnectionPtr);

            // there is no slave with this id -> LogicError
            AssertThrows(LogicError, msConnectionPools.getSlaveConnection(5));

            AssertThat(msConnectionPools.size(), Equals(2u));
            AssertThat(count(msConnectionPools), Equals(2));
        });

        it("works with master and multiple slaves", [&](){
            auto factory = [&](){
                return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
            };
            auto msConnectionPools = makeMasterSlaveConnectionPools(factory);

            auto& masterPool = msConnectionPools.getMasterPoolRef();
            masterPool.startResourceCountKeeper();

            msConnectionPools.emplaceSlavePool(42, makeConnectionPool(factory)).startResourceCountKeeper();
            testConnectionBySelect(*msConnectionPools.getSlaveConnection(42));

            // there is already a slave with this id -> LogicError
            AssertThrows(LogicError, msConnectionPools.emplaceSlavePool(42, makeConnectionPool(factory)));

            testConnectionBySelect(*msConnectionPools.getSlaveConnection(42));


            msConnectionPools.addSlavePool(43,  makeConnectionPool(factory)).startResourceCountKeeper();
            testConnectionBySelect(*msConnectionPools.getSlaveConnection(43));

            // there is already a slave with this id -> LogicError
            AssertThrows(LogicError, msConnectionPools.addSlavePool(43, makeConnectionPool(factory)));

            testConnectionBySelect(*msConnectionPools.getSlaveConnection(43));


            msConnectionPools.eraseSlavePool(43);
            auto& pool = msConnectionPools.addSlavePool(43, makeConnectionPool(factory));
            testConnectionBySelect(*msConnectionPools.getSlaveConnection(43));
            pool.startResourceCountKeeper();
            testConnectionBySelect(*msConnectionPools.getSlaveConnection(43));
            pool.stopResourceCountKeeper();
            testConnectionBySelect(*msConnectionPools.getSlaveConnection(43));
            pool.startResourceCountKeeper();
            testConnectionBySelect(*msConnectionPools.getSlaveConnection(43));


            msConnectionPools.eraseSlavePool(43);
            AssertThrows(LogicError, msConnectionPools.getSlaveConnection(43));

            AssertThat(msConnectionPools.size(), Equals(2u));
            AssertThat(count(msConnectionPools), Equals(2));
        });

        it("is iterable", [&](){
            auto factory = [&](){
                return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
            };
            auto msConnectionPools = makeMasterSlaveConnectionPools(factory);

            auto& masterPool = msConnectionPools.getMasterPoolRef();
            masterPool.startResourceCountKeeper();

            msConnectionPools.emplaceSlavePool(42, makeConnectionPool(factory)).startResourceCountKeeper();
            msConnectionPools.emplaceSlavePool(43, makeConnectionPool(factory)).startResourceCountKeeper();

            int s = 0;
            for (auto&& pool: msConnectionPools)
            {
                ++s;
                pool.get()->ping();
            }
            AssertThat(s, Equals(3));

            s = 0;
            for (auto&& slavePool: msConnectionPools.slavesCollection)
            {
                ++s;
                slavePool.get()->ping();
            }
            AssertThat(s, Equals(2));
        });
    });
});


