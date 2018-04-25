/*
 *  Author: Tomas Nozicka
 */

#include <bandit/bandit.h>
#include <iostream>

#include "db_access/settings.hpp"
#include <superior_mysqlpp/logging.hpp>


// test logger's lifetime
class Singleton
{
private:
    Singleton() = default;

public:
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;

    ~Singleton() noexcept
    {
        SuperiorMySqlpp::DefaultLogger::getLoggerPtr()->logMySqlQuery(-1, "Testing singletons destruction order.");
    }


    static Singleton& getInstance()
    {
        static Singleton singleton{};
        return singleton;
    }
};


void printUsage()
{
    std::cerr << "Usage: ./tester <MySqlIpAddress> <MySqlPort> <MySqlContainerId> <LocalFwdSocket> [bandit-options...]" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        std::cerr << "Less than 4 arguments!" << std::endl;
        printUsage();
        return 1;
    }

    auto& s = getSettingsRef();
    s.host = argv[1];
    try
    {
        s.port = static_cast<std::uint16_t>(std::stoi(argv[2]));
    }
    catch(std::logic_error& e)
    {
        std::cerr << "Unable to parse port from: \"" << argv[2] << "\"." << std::endl;
        printUsage();
        return 1;
    }
    s.containerId = argv[3];
    s.socket = argv[4];

    Singleton::getInstance();

//    SuperiorMySqlpp::DefaultLogger::getModifiableInstance().setLoggerPtr(std::make_shared<SuperiorMySqlpp::Loggers::Full>());

    std::cout << "Waiting for MySQL to become ready..." << std::endl;
    waitForMySql();
    std::cout << "MySQL is ready." << std::endl;

    return bandit::run(argc-4, argv+4);
}

