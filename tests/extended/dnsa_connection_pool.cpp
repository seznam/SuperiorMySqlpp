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
using namespace snowhouse;
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

static constexpr int  mysql_opt_timeout_s = 1;
static constexpr bool mysql_opt_reconnect = true;
static constexpr std::size_t min_spare_connections = 10;
static constexpr std::size_t max_spare_connections = 20;

auto makeTestPool(const Setting &settings, const std::string &hostname) {

    auto driverOptions = std::make_tuple(
        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::connectTimeout, &mysql_opt_timeout_s),
        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::readTimeout, &mysql_opt_timeout_s),
        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::writeTimeout, &mysql_opt_timeout_s),
        std::make_tuple(SuperiorMySqlpp::ConnectionOptions::reconnect, &mysql_opt_reconnect)
    );

    return makeDnsaConnectionPool(
        [=]() {
            return std::async(
                std::launch::async,
                [=]() {
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

}

template <typename T>
void setupTestPool(T &&pool, const std::chrono::milliseconds &job_sleep_period=50ms,
        const bool &start_resource_keeper=true, const bool &start_healthcare=false) {

    pool.setMinSpare(min_spare_connections);
    pool.setMaxSpare(max_spare_connections);


    if(start_healthcare) {
        pool.setHealthCareJobSleepTime(job_sleep_period);
        pool.startHealthCareJob();
    }

    if(start_resource_keeper) {
        pool.setResourceCountKeeperSleepTime(job_sleep_period);
        pool.startResourceCountKeeper();
    }

    pool.startDnsAwarePoolManagement();
}

go_bandit([]() {

    describe("Test dnsa connection pool", [&]() {

        it("reconnects after dns change", [&]() {

            auto&& standardLogger = DefaultLogger::getLoggerPtr();
            auto&& silentLogger = std::make_shared<Loggers::Base>();

            HostnameGuard hostnameGuard{};
            // set "invalid" ip address, connections cannot be created
            setIpForHostname("1.1.1.1", hostname);

            auto settings = getSettingsRef();

            // Silences logging, notably the logSharedPtrPoolResourceCountKeeperAddingResourcesException case
            DefaultLogger::setLoggerPtr(silentLogger);

            auto pool = makeTestPool(settings, hostname);

            setupTestPool(pool);

            // Recover conventional logging
            DefaultLogger::setLoggerPtr(standardLogger);

            // check that we cannot create connections
            AssertThat(pool.poolState().available, Equals(0u));
            SuperiorMySqlpp::detail::sleepInParts(1s, 50ms, [](){return true;});
            AssertThat(pool.poolState().available, Equals(0u));

            // set "valid" ip address, connections can be created now
            hostnameGuard.restore();
            setIpForHostname(getSettingsRef().host, hostname);

            // tryPing will use one connection, we must save previous state for later
            // comparison
            SharedPtrPoolState state{};

            // wait until resource count keeper do its job
            backoffSleep(std::chrono::milliseconds(2000 * mysql_opt_timeout_s), [&]() {
                state = pool.poolState();

                return state.available >= min_spare_connections &&
                       state.available <= max_spare_connections;
            });

            AssertThat(pool.get()->tryPing(), IsTrue());
        });

        it("keeps spare connections", [&]() {

            auto settings = getSettingsRef();
            auto pool = makeTestPool(settings, hostname);

            setIpForHostname(settings.host, hostname);
            setupTestPool(pool);

            // wait until we create enough connections
            backoffSleep(2000ms, [&]() {return pool.poolState().available >= min_spare_connections;});
            AssertThat(pool.poolState().available, Equals(min_spare_connections));

            // use one connection
            auto connection = pool.get();
            connection->tryPing();

            // check that we have at least _min_spare_connections_ spare connections
            std::this_thread::sleep_for(150ms);
            AssertThat(pool.poolState().available >= min_spare_connections, IsTrue());
        });

        it("doesn't keep spare connections until a job creates them", [&]() {

            auto settings = getSettingsRef();
            auto pool = makeTestPool(settings, hostname);

            setIpForHostname(settings.host, hostname);
            setupTestPool(pool, std::chrono::milliseconds(1h));

            // wait until we create enough connections
            backoffSleep(2000ms, [&]() {return pool.poolState().available >= min_spare_connections;});
            AssertThat(pool.poolState().available, Equals(min_spare_connections));

            // use one connection
            auto connection = pool.get();
            connection->tryPing();

            std::this_thread::sleep_for(10ms);
            AssertThat(pool.poolState().available < min_spare_connections, IsTrue());
        });

        it("can log dns error", [&]() {

            auto &settings = getSettingsRef();
            std::string hostname{"definitiely.not.resolvable.hostname"};

            StreamCapture<std::ostream> capture(std::cerr);

            auto &&connectionPool = makeDnsaConnectionPool<true, true, true, true, true>(
                [&]() {
                    return std::async(
                        std::launch::async,
                        [&]() {
                            return std::make_shared<Connection>(
                                settings.database,
                                settings.user,
                                settings.password,
                                settings.host,
                                settings.port
                            );
                        });
                },
                hostname,
                std::make_shared<Loggers::Full>()
            );

            // ensure that at least one log method is called
            connectionPool.startDnsAwarePoolManagement();
            connectionPool.stopDnsAwarePoolManagement();

            // save stream data, restore std::cerr
            std::string streamCaptureResult = capture.getString();
            capture.restore();

            // check that hostname is present in stream data
            AssertThat(streamCaptureResult.find(hostname), !Equals(std::string::npos));
        });

    });


});
