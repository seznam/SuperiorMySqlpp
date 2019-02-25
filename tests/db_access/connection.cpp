/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <memory>
#include <thread>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>

#include "settings.hpp"


using namespace bandit;
using namespace SuperiorMySqlpp;


go_bandit([](){
    describe("Test connection", [&](){
        auto& s = getSettingsRef();

        it("will initialize", [&](){
                auto& s = getSettingsRef();
                Connection connection{s.database, s.user, s.password, s.host, s.port};

                unsigned int connectTimeout = 1;
                connection.setOption(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &connectTimeout);
        });


        it("will can have pre-connect arguments", [&](){
                auto& s = getSettingsRef();
                unsigned int timeout = 1;
                bool reconnect = true;

                Connection connection0_1{s.database, s.user, s.password, s.host, s.port,
                    std::make_tuple()
                };

                Connection connection0_2{s.database, s.user, s.password, s.host, s.port,
                    std::forward_as_tuple()
                };

                Connection connection1{s.database, s.user, s.password, s.host, s.port,
                    std::make_tuple(
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &timeout)
                    )
                };

                Connection connection2{s.database, s.user, s.password, s.host, s.port,
                    std::make_tuple(
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &timeout),
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::readTimeout, &timeout),
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::writeTimeout, &timeout),
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::reconnect, &reconnect)
                    )
                };

                Connection connection3{s.database, s.user, s.password, s.host, s.port,
                    std::forward_as_tuple(
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &timeout),
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::readTimeout, &timeout),
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::writeTimeout, &timeout),
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::reconnect, &reconnect)
                    )
                };

                Connection connection4{s.database, s.user, s.password, s.host, s.port,
                    std::forward_as_tuple(
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &timeout),
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::readTimeout, &timeout),
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::writeTimeout, &timeout),
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::reconnect, &reconnect)
                    )
                };

                Connection connection5{s.database, s.user, s.password, s.host, s.port,
                    std::make_tuple(
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &timeout),
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::readTimeout, &timeout),
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::writeTimeout, &timeout),
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::reconnect, &reconnect)
                    )
                };

                Connection connection6{s.database, s.user, s.password, s.host, s.port,
                    std::make_tuple(
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &timeout),
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::readTimeout, &timeout),
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::writeTimeout, &timeout),
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::reconnect, &reconnect)
                    )
                };

                Connection connection7{s.database, s.user, s.password, s.host, s.port,
                    std::forward_as_tuple(
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &timeout),
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::readTimeout, &timeout),
                        std::forward_as_tuple(SuperiorMySqlpp::ConnectionOptions::writeTimeout, &timeout),
                        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::reconnect, &reconnect)
                    )
                };
        });

        it("can be moved between threads", [&](){
            std::shared_ptr<Connection> connectionPtr1{};
            AssertThat(connectionPtr1.get()==nullptr, IsTrue());

            std::thread{[&connectionPtr1, &s](){
                connectionPtr1 = std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port);
            }}.join();
            AssertThat(connectionPtr1.get()!=nullptr, IsTrue());


            auto connectionPtr2 = std::make_shared<Connection>(s.database, s.user, s.password, s.host, s.port);
            AssertThat(connectionPtr2.get()!=nullptr, IsTrue());

            std::thread{[&connectionPtr2](){
                auto ptr = std::move(connectionPtr2);
            }}.join();
            AssertThat(connectionPtr2.get()==nullptr, IsTrue());
        });

        it("can use SSL", [&](){
            Connection connection{SslConfiguration{}, s.database, s.user, s.password, s.host, s.port};
        });

        it("can use TCP ConnectionConfiguration without SSL", [&](){
            ConnectionConfiguration tcpConfig = ConnectionConfiguration::getTcpConnectionConfiguration(s.database, s.user, s.password, s.host, s.port);

            Connection connection{tcpConfig};
        });

        it("can use TCP ConnectionConfiguration with SSL", [&](){
            ConnectionConfiguration tcpConfig = ConnectionConfiguration::getSslTcpConnectionConfiguration(SslConfiguration{}, s.database, s.user, s.password, s.host, s.port);

            Connection connection{tcpConfig};
        });

        it("can use socketed ConnectionConfiguration without SSL", [&](){
            ConnectionConfiguration socketConfig = ConnectionConfiguration::getSocketConnectionConfiguration(s.database, s.user, s.password, s.socket);

            Connection connection{socketConfig};
        });

        it("can use socketed ConnectionConfiguration with SSL", [&](){
            ConnectionConfiguration socketConfig = ConnectionConfiguration::getSslSocketConnectionConfiguration(SslConfiguration{}, s.database, s.user, s.password, s.socket);

            Connection connection{socketConfig};
        });

        it("does not crash when stealing connection", [&]() {
            auto& s = getSettingsRef();
            Connection connection{s.database, s.user, s.password, s.host, s.port};
            Connection conn2 = std::move(connection);
        });
    });
});


