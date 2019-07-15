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
    describe("Test Loggers", [&](){
        it("has implemented all virtual functions", [&](){
            Loggers::Base base{};
            Loggers::Full full{};
        });

        it("can be set and unset", [&](){
            auto logger = SuperiorMySqlpp::DefaultLogger::getLoggerPtr();
            SuperiorMySqlpp::DefaultLogger::getModifiableInstance().setLoggerPtr(std::make_shared<SuperiorMySqlpp::Loggers::Base>());
            SuperiorMySqlpp::DefaultLogger::getModifiableInstance().setLoggerPtr(std::make_shared<SuperiorMySqlpp::Loggers::Default>());
            SuperiorMySqlpp::DefaultLogger::getModifiableInstance().setLoggerPtr(std::make_shared<SuperiorMySqlpp::Loggers::Full>());
            SuperiorMySqlpp::DefaultLogger::setLoggerPtr(std::move(logger));
        });

    });
});
