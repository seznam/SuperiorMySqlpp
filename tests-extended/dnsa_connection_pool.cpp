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

std::string hostname = "tmp-superiormysqlpp-" + getUniqueHostname();

auto makePool()
{
    return makeDnsaConnectionPool([&](){
               auto& s = getSettingsRef();
               static unsigned int timeout = 1;
               return std::async(std::launch::async, [&](){ return std::make_shared<Connection>("", s.user, s.password, hostname, s.port,
                       std::make_tuple(
                           std::make_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &timeout),
                           std::make_tuple(SuperiorMySqlpp::ConnectionOptions::readTimeout, &timeout),
                           std::make_tuple(SuperiorMySqlpp::ConnectionOptions::writeTimeout, &timeout)
                       )
                   );
               });
           },
           hostname
    );
}


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
    describe("Test dnsa connection pool", [&]() {
        it("works", [&]() {
            HostnameGuard hostnameGuard{};

            setIpForHostname("1.1.1.1", hostname);
            auto pool = makePool();

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

            backoffSleep(1000ms, [&](){
                return pool.poolState().available>10 && pool.poolState().available<20;
            });
            AssertThat(pool.poolState().available!=0, IsTrue());
            AssertThat(pool.get()->tryPing(), IsTrue());
            AssertThat((pool.poolState().available>10 && pool.poolState().available<20), IsTrue());
        });
    });
});
