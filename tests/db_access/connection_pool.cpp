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

auto makeSharedPtrConnection()
{
    auto& s = getSettingsRef();
    return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
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

            auto&& poolState = connectionPool.poolState();
            AssertThat(poolState.size, Equals(0U));
            AssertThat(poolState.available, Equals(0U));

            connectionPool.clearPool();
            poolState = connectionPool.poolState();
            AssertThat(poolState.size, Equals(0U));
            AssertThat(poolState.available, Equals(0U));

            {
                auto&& item = connectionPool.get();
                static_cast<void>(item);
                poolState = connectionPool.poolState();
                AssertThat(poolState.size, Equals(1U));
                AssertThat(poolState.available, Equals(0U));
            }

            backoffSleep(10s, [&](){
                auto&& poolState = connectionPool.poolState();
                return poolState.size==1U && poolState.available==1U;
            });

            poolState = connectionPool.poolState();
            AssertThat(poolState.size, Equals(1U));
            AssertThat(poolState.available, Equals(1U));

            connectionPool.clearPool();
            poolState = connectionPool.poolState();
            AssertThat(poolState.size, Equals(0U));
            AssertThat(poolState.available, Equals(0U));
            connectionPool.clearPool();
            poolState = connectionPool.poolState();
            AssertThat(poolState.size, Equals(0U));
            AssertThat(poolState.available, Equals(0U));

            {
                auto&& item1 = connectionPool.get();
                auto&& item2 = connectionPool.get();
                static_cast<void>(item1);
                static_cast<void>(item2);
                poolState = connectionPool.poolState();
                AssertThat(poolState.size, Equals(2U));
                AssertThat(poolState.available, Equals(0U));

                connectionPool.clearPool();
                poolState = connectionPool.poolState();
                AssertThat(poolState.size, Equals(0U));
                AssertThat(poolState.available, Equals(0U));

                connectionPool.get();
                backoffSleep(2s, [&](){
                    auto&& poolState = connectionPool.poolState();
                    return poolState.size==1 && poolState.available==1;
                });
                poolState = connectionPool.poolState();
                AssertThat(poolState.size, Equals(1U));
                AssertThat(poolState.available, Equals(1U));
            }

            connectionPool.clearPool();

            connectionPool.setMinSpare(5);
            connectionPool.setMaxSpare(100);
            connectionPool.setHealthCareJobSleepTime(500ms);
            connectionPool.startResourceCountKeeper();
            connectionPool.startHealthCareJob();

            backoffSleep(10s, [&](){
                auto&& poolState = connectionPool.poolState();
                return poolState.size==5 && poolState.available==5;
            });
            poolState = connectionPool.poolState();
            AssertThat(poolState.size, Equals(5U));
            AssertThat(poolState.available, Equals(5U));

            connectionPool.stopResourceCountKeeper();
            while (connectionPool.isResourceCountKeeperThreadRunning()){
                ;
            }
            connectionPool.clearPool();
            connectionPool.startResourceCountKeeper();

            backoffSleep(10s, [&](){
                auto&& poolState = connectionPool.poolState();
                return poolState.size==5 && poolState.available==5;
            });
            poolState = connectionPool.poolState();
            AssertThat(poolState.size, Equals(5U));
            AssertThat(poolState.available, Equals(poolState.size));

            {
                auto connection = connectionPool.get();
            }
            backoffSleep(10s, [&](){
                auto&& poolState = connectionPool.poolState();
                return poolState.size>=6U && poolState.available>=6U;
            });
            poolState = connectionPool.poolState();
            AssertThat(poolState.size, IsGreaterThanOrEqualTo(6U));
            AssertThat(poolState.available, IsGreaterThanOrEqualTo(6U));

            connectionPool.clearPool();
            poolState = connectionPool.poolState();
            AssertThat(poolState.size, Equals(0U));
            AssertThat(poolState.available, Equals(poolState.size));
        });

        it("can spin default connections", [&](){
            auto connectionPool = makeConnectionPool([&](){
                return std::async(std::launch::async, [&](){ return std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port); });
            });

            connectionPool.setMinSpare(10);
            connectionPool.setMaxSpare(20);
            AssertThat(connectionPool.poolState().size, Equals(0u));
            AssertThat(connectionPool.poolState().available, Equals(0u));

            connectionPool.startResourceCountKeeper();

            backoffSleep(1000ms, [&](){
                return connectionPool.poolState().size==10 && connectionPool.poolState().available==10;
            });
            AssertThat(connectionPool.poolState().available, Equals(10u));
            AssertThat(connectionPool.poolState().size, Equals(10u));

            connectionPool.clearPool();
            backoffSleep(1000ms, [&](){
                return connectionPool.poolState().size==10 && connectionPool.poolState().available==10;
            });
            AssertThat(connectionPool.poolState().size, Equals(10u));
            AssertThat(connectionPool.poolState().available, Equals(10u));

            connectionPool.stopResourceCountKeeper();
            connectionPool.clearPool();
            AssertThat(connectionPool.poolState().size, Equals(0u));
            AssertThat(connectionPool.poolState().available, Equals(0u));
        });

        it("can work without keeper job", [&](){
            auto connectionPool = makeConnectionPool(makeSharedPtrConnection);
            AssertThat(connectionPool.poolState().size, Equals(0u));
            AssertThat(connectionPool.poolState().available, Equals(0u));

            {
                auto connection1 = connectionPool.get();
                AssertThat(connectionPool.poolState().size, Equals(1u));
                AssertThat(connectionPool.poolState().available, Equals(0u));
            }
            AssertThat(connectionPool.poolState().size, Equals(1u));
            AssertThat(connectionPool.poolState().available, Equals(1u));

            {
                auto connection1 = connectionPool.get();
                AssertThat(connectionPool.poolState().size, Equals(1u));
                AssertThat(connectionPool.poolState().available, Equals(0u));
                {
                    auto connection2 = connectionPool.get();
                    AssertThat(connectionPool.poolState().size, Equals(2u));
                    AssertThat(connectionPool.poolState().available, Equals(0u));
                    auto connection3 = connectionPool.get();
                    AssertThat(connectionPool.poolState().size, Equals(3u));
                    AssertThat(connectionPool.poolState().available, Equals(0u));
                }
                AssertThat(connectionPool.poolState().size, Equals(3u));
                AssertThat(connectionPool.poolState().available, Equals(2u));
            }
            AssertThat(connectionPool.poolState().size, Equals(3u));
            AssertThat(connectionPool.poolState().available, Equals(3u));
        });

        it("has working keeper job", [&](){
            auto connectionPool = makeConnectionPool(makeSharedPtrConnection);

            connectionPool.setMinSpare(1);
            connectionPool.setMaxSpare(2);
            AssertThat(connectionPool.poolState().size, Equals(0u));
            AssertThat(connectionPool.poolState().available, Equals(0u));

            connectionPool.startResourceCountKeeper();

            backoffSleep(1000ms, [&](){
                return connectionPool.poolState().size==1 && connectionPool.poolState().available==1;
            });
            AssertThat(connectionPool.poolState().size, Equals(1u));
            AssertThat(connectionPool.poolState().available, Equals(1u));


            {
                auto connection1 = connectionPool.get();
//                    std::this_thread::sleep_for(reasonableTimeout);
                backoffSleep(1000ms, [&](){
                    return connectionPool.poolState().size==2 && connectionPool.poolState().available==1;
                });
                AssertThat(connectionPool.poolState().size, Equals(2u));
                AssertThat(connectionPool.poolState().available, Equals(1u));
                {
                    auto connection2 = connectionPool.get();
                    backoffSleep(1000ms, [&](){
                        return connectionPool.poolState().size==3 && connectionPool.poolState().available==1;
                    });
                    AssertThat(connectionPool.poolState().size, Equals(3u));
                    AssertThat(connectionPool.poolState().available, Equals(1u));

                    auto connection3 = connectionPool.get();
                    backoffSleep(1000ms, [&](){
                        return connectionPool.poolState().size==4 && connectionPool.poolState().available==1;
                    });
                    AssertThat(connectionPool.poolState().size, Equals(4u));
                    AssertThat(connectionPool.poolState().available, Equals(1u));

                    auto connection4 = connectionPool.get();
                    backoffSleep(1000ms, [&](){
                        return connectionPool.poolState().size==5 && connectionPool.poolState().available==1;
                    });
                    AssertThat(connectionPool.poolState().size, Equals(5u));
                    AssertThat(connectionPool.poolState().available, Equals(1u));
                }
                backoffSleep(1000ms, [&](){
                    return connectionPool.poolState().size==3 && connectionPool.poolState().available==2;
                });
                AssertThat(connectionPool.poolState().size, Equals(3u));
                AssertThat(connectionPool.poolState().available, Equals(2u));


                {
                    auto connection2 = connectionPool.get();
                    backoffSleep(1000ms, [&](){
                        return connectionPool.poolState().size==3 && connectionPool.poolState().available==1;
                    });
                    AssertThat(connectionPool.poolState().size, Equals(3u));
                    AssertThat(connectionPool.poolState().available, Equals(1u));

                    auto connection3 = connectionPool.get();
                    backoffSleep(1000ms, [&](){
                        return connectionPool.poolState().size==4 && connectionPool.poolState().available==1;
                    });
                    AssertThat(connectionPool.poolState().size, Equals(4u));
                    AssertThat(connectionPool.poolState().available, Equals(1u));

                    auto connection4 = connectionPool.get();
                    backoffSleep(1000ms, [&](){
                        return connectionPool.poolState().size==5 && connectionPool.poolState().available==1;
                    });
                    AssertThat(connectionPool.poolState().size, Equals(5u));
                    AssertThat(connectionPool.poolState().available, Equals(1u));
                }
                backoffSleep(1000ms, [&](){
                    return connectionPool.poolState().size==3 && connectionPool.poolState().available==2;
                });
                AssertThat(connectionPool.poolState().size, Equals(3u));
                AssertThat(connectionPool.poolState().available, Equals(2u));
            }
            backoffSleep(1000ms, [&](){
                return connectionPool.poolState().size==2 && connectionPool.poolState().available==2;
            });
            AssertThat(connectionPool.poolState().size, Equals(2u));
            AssertThat(connectionPool.poolState().available, Equals(2u));

            {
                auto connection2 = connectionPool.get();
                backoffSleep(1000ms, [&](){
                    return connectionPool.poolState().size==2 && connectionPool.poolState().available==1;
                });
                AssertThat(connectionPool.poolState().size, Equals(2u));
                AssertThat(connectionPool.poolState().available, Equals(1u));
            }
            backoffSleep(1000ms, [&](){
                return connectionPool.poolState().size==2 && connectionPool.poolState().available==2;
            });
            AssertThat(connectionPool.poolState().size, Equals(2u));
            AssertThat(connectionPool.poolState().available, Equals(2u));
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
