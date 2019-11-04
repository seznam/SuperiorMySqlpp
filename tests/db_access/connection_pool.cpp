/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <future>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>

#include "settings.hpp"
#include "test_utils.hpp"


using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace {
    auto makeSharedPtrConnection()
    {
        auto& s = getSettingsRef();
        return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
    }

    template <typename T>
    auto assertPoolState(const T& connectionPool, size_t size, size_t available)
    {
        auto&& poolState = connectionPool.poolState();
        AssertThat(poolState.size, Equals(size));
        AssertThat(poolState.available, Equals(available));
    }

    template <typename T>
    auto waitForPoolState(const T& connectionPool, std::chrono::milliseconds max, size_t size, size_t available)
    {
        backoffSleep(max, [&]() {
            auto&& poolState = connectionPool.poolState();
            return poolState.size == size && poolState.available == available;
        });
        assertPoolState(connectionPool, size, available);
    }
}

go_bandit([](){
    using namespace std::chrono_literals;

    describe("Test connection pool", [&](){
        auto& s = getSettingsRef();


        it("can clear pool properly", [&](){
            auto&& connectionPool = makeConnectionPool<true, true, true, true, true>([&](){
                return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
            });
            connectionPool.startHealthCareJob();
            assertPoolState(connectionPool, 0u, 0u);

            connectionPool.clearPool();
            assertPoolState(connectionPool, 0u, 0u);

            {
                auto&& item = connectionPool.get();
                static_cast<void>(item);
                assertPoolState(connectionPool, 1u, 0u);
            }

            waitForPoolState(connectionPool, 10s, 1u, 1u);

            connectionPool.clearPool();
            assertPoolState(connectionPool, 0u, 0u);
            connectionPool.clearPool();
            assertPoolState(connectionPool, 0u, 0u);

            {
                auto&& item1 = connectionPool.get();
                auto&& item2 = connectionPool.get();
                static_cast<void>(item1);
                static_cast<void>(item2);
                assertPoolState(connectionPool, 2u, 0u);

                connectionPool.clearPool();
                assertPoolState(connectionPool, 0u, 0u);

                connectionPool.get();
                waitForPoolState(connectionPool, 2s, 1u, 1u);
            }

            connectionPool.clearPool();

            connectionPool.setMinSpare(5);
            connectionPool.setMaxSpare(100);
            connectionPool.setHealthCareJobSleepTime(500ms);
            connectionPool.startResourceCountKeeper();
            connectionPool.startHealthCareJob();

            waitForPoolState(connectionPool, 10s, 5u, 5u);

            connectionPool.stopResourceCountKeeper();
            while (connectionPool.isResourceCountKeeperThreadRunning()){
                ;
            }
            connectionPool.clearPool();
            connectionPool.startResourceCountKeeper();

            waitForPoolState(connectionPool, 10s, 5u, 5u);

            {
                auto connection = connectionPool.get();
            }
            // We expect size 6, but it can be more.
            // When HealthCareJob check connections, it get connections for try_ping, so available connections in pool could be 0 at this time.
            // If this happen whilst ResourceCountKeeper job is checking pool state, new connections are spawned.
            backoffSleep(10s, [&](){
                auto&& poolState = connectionPool.poolState();
                return poolState.size>=6u && poolState.available>=6u;
            });
            auto&& poolState = connectionPool.poolState();
            AssertThat(poolState.size, IsGreaterThanOrEqualTo(6u));
            AssertThat(poolState.available, IsGreaterThanOrEqualTo(6u));

            connectionPool.clearPool();
            assertPoolState(connectionPool, 0u, 0u);
        });

        it("can spin default connections", [&](){
            auto connectionPool = makeConnectionPool([&](){
                return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
            });
            connectionPool.setMinSpare(10);
            connectionPool.setMaxSpare(20);
            assertPoolState(connectionPool, 0u, 0u);

            connectionPool.startResourceCountKeeper();

            waitForPoolState(connectionPool, 1s, 10u, 10u);

            connectionPool.clearPool();
            waitForPoolState(connectionPool, 1s, 10u, 10u);

            connectionPool.stopResourceCountKeeper();
            connectionPool.clearPool();
            assertPoolState(connectionPool, 0u, 0u);
        });

        it("can work without keeper job", [&](){
            auto connectionPool = makeConnectionPool(makeSharedPtrConnection);
            assertPoolState(connectionPool, 0u, 0u);

            {
                auto connection1 = connectionPool.get();
                assertPoolState(connectionPool, 1u, 0u);
            }
            assertPoolState(connectionPool, 1u, 1u);

            {
                auto connection1 = connectionPool.get();
                assertPoolState(connectionPool, 1u, 0u);
                {
                    auto connection2 = connectionPool.get();
                    assertPoolState(connectionPool, 2u, 0u);
                    auto connection3 = connectionPool.get();
                    assertPoolState(connectionPool, 3u, 0u);
                }
                assertPoolState(connectionPool, 3u, 2u);
            }
            assertPoolState(connectionPool, 3u, 3u);
        });

        it("has working keeper job", [&](){
            auto connectionPool = makeConnectionPool(makeSharedPtrConnection);

            connectionPool.setMinSpare(1);
            connectionPool.setMaxSpare(2);
            assertPoolState(connectionPool, 0u, 0u);

            connectionPool.startResourceCountKeeper();

            waitForPoolState(connectionPool, 1s, 1u, 1u);


            {
                auto connection1 = connectionPool.get();
                waitForPoolState(connectionPool, 1s, 2u, 1u);

                {
                    auto connection2 = connectionPool.get();
                    waitForPoolState(connectionPool, 1s, 3u, 1u);

                    auto connection3 = connectionPool.get();
                    waitForPoolState(connectionPool, 1s, 4u, 1u);

                    auto connection4 = connectionPool.get();
                    waitForPoolState(connectionPool, 1s, 5u, 1u);
                }

                waitForPoolState(connectionPool, 1s, 3u, 2u);

                {
                    auto connection2 = connectionPool.get();
                    waitForPoolState(connectionPool, 1s, 3u, 1u);

                    auto connection3 = connectionPool.get();
                    waitForPoolState(connectionPool, 1s, 4u, 1u);

                    auto connection4 = connectionPool.get();
                    waitForPoolState(connectionPool, 1s, 5u, 1u);
                }

                waitForPoolState(connectionPool, 1s, 3u, 2u);
            }

            waitForPoolState(connectionPool, 1s, 2u, 2u);

            {
                auto connection2 = connectionPool.get();
                waitForPoolState(connectionPool, 1s, 2u, 1u);
            }

            waitForPoolState(connectionPool, 1s, 2u, 2u);
        });

        it("can count unhealthy resources", [&](){
            unsigned int timeout = 1;
            auto&& connectionPool = makeConnectionPool([&](){
                return std::async(std::launch::async, [&](){
                    return std::make_shared<Connection>(
                        s.database, s.user, s.password, s.host, s.port,
                        std::make_tuple(
                                    std::make_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &timeout),
                                    std::make_tuple(SuperiorMySqlpp::ConnectionOptions::readTimeout, &timeout),
                                    std::make_tuple(SuperiorMySqlpp::ConnectionOptions::writeTimeout, &timeout)
                        ),
                        DefaultLogger::getLoggerPtr()
                    );
                });
            });

            connectionPool.get();
            AssertThat(connectionPool.poolState().size, Equals(1u));
            AssertThat(connectionPool.poolFullState().unhealthy, Equals(0u));

            stopMySql();

            AssertThat(connectionPool.poolFullState().unhealthy, Equals(1u));

            startMySql();
            waitForMySql();

            connectionPool.setHealthCareJobSleepTime(50ms);
            connectionPool.startHealthCareJob();

            backoffSleep(1000ms, [&](){
                return connectionPool.poolFullState().unhealthy == 0;
            });

            AssertThat(connectionPool.poolFullState().unhealthy, Equals(0u));
        });

        it("can remove faulty connections", [&](){
            auto&& connectionPool = makeConnectionPool([&](){
                return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
            });
            connectionPool.setHealthCareJobSleepTime(50ms);
            connectionPool.startHealthCareJob();

            connectionPool.get();

            auto breakConnection = [&](Connection& connection){
                // we must simulate breaking the connection
                auto&& driver = connection.detail_getDriver();
                auto&& mysqlPtr = driver.detail_getMysqlPtr();
                mysql_close(mysqlPtr);
                mysql_init(mysqlPtr);
                mysql_real_connect(mysqlPtr, s.host.c_str(), "no-nexisting", "", "", s.port, nullptr, 0);
            };

            breakConnection(*connectionPool.get());
            backoffSleep(1000ms, [&](){
                return connectionPool.get()->tryPing() == true;
            });

            auto&& goodConnection = *connectionPool.get();
            AssertThat(goodConnection.tryPing(), IsTrue());

            connectionPool.stopHealthCareJob();

            breakConnection(*connectionPool.get());
            std::this_thread::sleep_for(100ms);

            auto&& badConnection = *connectionPool.get();
            AssertThat(badConnection.tryPing(), IsFalse());
        });

        it("can recover from restarting MySQL", [&](){
            auto&& standardLogger = DefaultLogger::getLoggerPtr();
            auto&& silentLogger = std::make_shared<Loggers::Base>();

            auto restartServer = [&] {
                DefaultLogger::setLoggerPtr(silentLogger);
                restartMySql();
                waitForMySql();
                DefaultLogger::setLoggerPtr(standardLogger);
            };

            auto&& connectionPool = makeConnectionPool([&](){
                return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
            });
            connectionPool.setHealthCareJobSleepTime(50ms);

            AssertThat(connectionPool.poolState().size, Equals(0u));
            backoffSleep(1000ms, [&](){
                return connectionPool.get()->tryPing() == true;
            });
            AssertThat(connectionPool.poolState().size, Equals(1u));
            AssertThat(connectionPool.get()->tryPing(), IsTrue());
            AssertThat(connectionPool.poolState().size, Equals(1u));

            restartServer();

            AssertThat(connectionPool.poolState().size, Equals(1u));
            backoffSleep(1000ms, 150ms, [&](){
                return connectionPool.get()->tryPing() == false;
            });
            AssertThat(connectionPool.poolState().size, Equals(1u));
            AssertThat(connectionPool.get()->tryPing(), IsFalse());
            AssertThat(connectionPool.poolState().size, Equals(1u));

            connectionPool.startHealthCareJob();

            AssertThat(connectionPool.poolState().size, Equals(1u));
            backoffSleep(1000ms, [&](){
                return connectionPool.get()->tryPing() == true;
            });
            AssertThat(connectionPool.poolState().size, Equals(1u));
            AssertThat(connectionPool.get()->tryPing(), IsTrue());
            AssertThat(connectionPool.poolState().size, Equals(1u));

            connectionPool.stopHealthCareJob();

            AssertThat(connectionPool.poolState().size, Equals(1u));
            backoffSleep(1000ms, [&](){
                return connectionPool.get()->tryPing() == true;
            });
            AssertThat(connectionPool.poolState().size, Equals(1u));
            AssertThat(connectionPool.get()->tryPing(), IsTrue());
            AssertThat(connectionPool.poolState().size, Equals(1u));

            restartServer();

            AssertThat(connectionPool.poolState().size, Equals(1u));
            backoffSleep(1000ms, [&](){
                return connectionPool.get()->tryPing() == false;
            });
            AssertThat(connectionPool.poolState().size, Equals(1u));
            AssertThat(connectionPool.get()->tryPing(), IsFalse());
            AssertThat(connectionPool.poolState().size, Equals(1u));
        });

        it("can start when MySQL is stopped and recover", [&](){
            stopMySql();
            auto&& connectionPool = makeConnectionPool([&](){
                return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
            });
            connectionPool.setHealthCareJobSleepTime(500ms);
            connectionPool.startResourceCountKeeper();
            connectionPool.startHealthCareJob();

            startMySql();
            std::this_thread::sleep_for(2000ms);
            restartMySql();

            backoffSleep(10s, [&](){
                return connectionPool.poolState().size>0 && connectionPool.poolFullState().unhealthy==0;
            });
            AssertThat(connectionPool.poolState().size>0 && connectionPool.poolFullState().unhealthy==0, IsTrue());
        });
    });
});
