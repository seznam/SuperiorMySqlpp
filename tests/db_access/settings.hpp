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
};

inline Setting& getSettingsRef()
{
    static Setting settings{};
    return settings;
}


inline void systemExecute(std::string command)
{
    auto returnCode = std::system(command.c_str());
    if (returnCode != 0)
    {
        using namespace std::string_literals;
        throw std::runtime_error{"Executing system command '"s + command + "' failed with return code: " + std::to_string(returnCode) + "!"};
    }
}

inline void startMySql()
{
    systemExecute("docker exec " + getSettingsRef().containerId + " supervisorctl start mysqld 1>/dev/null");
}

inline void stopMySql()
{
    systemExecute("docker exec " + getSettingsRef().containerId + " supervisorctl stop mysqld 1>/dev/null");
}

inline void restartMySql()
{
    systemExecute("docker exec " + getSettingsRef().containerId + " supervisorctl restart mysqld 1>/dev/null");
}

inline void waitForMySql()
{
    while (true)
    {
        try
        {
            auto& s = getSettingsRef();
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            SuperiorMySqlpp::Connection connection{"", s.user, s.password, s.host, s.port};
        }
        catch (std::exception& e)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
            continue;
        }

        break;
    }
}
