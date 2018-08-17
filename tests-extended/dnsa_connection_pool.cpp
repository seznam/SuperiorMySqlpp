/*
 * Author: Tomas Nozicka
 */

#include <bandit/bandit.h>

#include <thread>
#include <sstream>
#include <fstream>

#include <superior_mysqlpp/dnsa_connection_pool.hpp>
#include <superior_mysqlpp/shared_ptr_pool/sleep_in_parts.hpp>

#include "../tests/db_access/settings.hpp"
#include "../tests/db_access/test_utils.hpp"


using namespace bandit;
using namespace SuperiorMySqlpp;
using namespace std::chrono_literals;


std::string getUniqueHostname()
{
    std::stringstream ss{};
    ss << "tmp-superiormysqlpp-" << std::this_thread::get_id();
    return ss.str();
}

std::string hostname = getUniqueHostname();





void setIpForHostname(const std::string& ip, const std::string& hostname)
{
    systemExecute("echo '" + ip + " " + hostname + "' >> /etc/hosts");
}


/*
 * Inside docker /etc/hosts is a mount point so we can use only those
 * operations not removing its link id.
 */
class HostnameGuard
{
public:
    HostnameGuard()
    {
        systemExecute("cp /etc/hosts /etc/hosts.orig");
    }

    ~HostnameGuard()
    {
        restore();
    }

    void restore()
    {
        systemExecute("cp /etc/hosts.orig /etc/hosts");
    }
};


go_bandit([]() {

    it("reconnects after dns change", [&]() {

            HostnameGuard hostnameGuard{};
            setIpForHostname("1.1.1.1", hostname);

            auto& settings = getSettingsRef();
            static int mysql_opt_timeout_s = 1;
            static bool mysql_opt_reconnect = true;

            auto&& driverOptions = std::make_tuple(
                std::make_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &mysql_opt_timeout_s),
                std::make_tuple(SuperiorMySqlpp::ConnectionOptions::readTimeout, &mysql_opt_timeout_s),
                std::make_tuple(SuperiorMySqlpp::ConnectionOptions::writeTimeout, &mysql_opt_timeout_s),
                std::make_tuple(SuperiorMySqlpp::ConnectionOptions::reconnect, &mysql_opt_reconnect)
            );

            auto&& pool = makeDnsaConnectionPool<true, true, true, true, true>(
                [&]() {
                    return std::async(
                        std::launch::async,
                        [&]() {
                            return std::make_shared<Connection>(
                                settings.database,
                                settings.user,
                                settings.password,
                                hostname,
                                settings.port,
                                driverOptions
                            );
                        });
                },
                hostname
            );

            pool.setMinSpare(10);
            pool.setMaxSpare(20);
            pool.setResourceCountKeeperSleepTime(50ms);
            pool.startResourceCountKeeper();

            pool.setHealthCareJobSleepTime(50ms);
            pool.startHealthCareJob();

            pool.startDnsAwarePoolManagement();

            AssertThat(pool.poolState().available, Equals(0u));
            SuperiorMySqlpp::detail::sleepInParts(1s, 50ms, [](){return true;});
            AssertThat(pool.poolState().available, Equals(0u));

            hostnameGuard.restore();
            setIpForHostname(getSettingsRef().host, hostname);

            // tryPing will use one connection, we must save previous state for later
            // comparison
            SharedPtrPoolState state{};

            // wait until resource count keeper do its job
            backoffSleep(std::chrono::milliseconds(2000 * mysql_opt_timeout_s), [&]() {
                state = pool.poolState();
                return state.available >= 10 && state.available <= 20;
            });

            AssertThat(pool.get()->tryPing(), IsTrue());
            AssertThat(state.available >= 10, IsTrue());
            AssertThat(state.available <= 20, IsTrue());
    });
});
