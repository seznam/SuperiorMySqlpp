/*
 * settings.hpp
 *
 *  Created on: Aug 25, 2014
 *      Author: tnozicka
 */

#pragma once


#include <cstdlib>
#include <stdexcept>
#include <string>

#include <superior_mysqlpp.hpp>


struct Setting
{
    std::string socket{""};
    std::string host{""};
    std::string user{"root"};
    std::string password{"password"};
    std::string database{"test_superior_sqlpp"};
    std::uint16_t port{0};
    std::string containerId{""};
    bool verbose{false};
};

inline Setting& getSettingsRef()
{
    static Setting settings{};
    return settings;
}


inline void systemExecute(std::string command)
{
    if (getSettingsRef().verbose)
    {
        std::cout << "Executing: " << command << std::endl;
    }
    auto returnCode = std::system(command.c_str());
    if (returnCode != 0)
    {
        using namespace std::string_literals;
        throw std::runtime_error{"Executing system command '"s + command + "' failed with return code: " + std::to_string(returnCode) + "!"};
    }
}

inline void startMySql()
{
    systemExecute("docker exec " + getSettingsRef().containerId + " supervisorctl start mysqld");
}

inline void stopMySql()
{
    systemExecute("docker exec " + getSettingsRef().containerId + " supervisorctl stop mysqld");
}

inline void restartMySql()
{
    systemExecute("docker exec " + getSettingsRef().containerId + " supervisorctl restart mysqld");
}

inline void waitForMySql()
{
    auto& s = getSettingsRef();
    while (true)
    {
        try
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            SuperiorMySqlpp::Connection connection{"", s.user, s.password, s.host, s.port};
        }
        catch (std::exception& e)
        {
            if (s.verbose)
            {
                std::cout << "Connecting to " << s.user << "@" << s.host << ":" << s.port << " failed: " << e.what() << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds{10});
            continue;
        }

        break;
    }
}
