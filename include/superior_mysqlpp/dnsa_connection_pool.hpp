/*
 * Author: Tomas Nozicka
 */

#pragma once

/*
 * DO NOT include this header in superior_mysqpp.hpp or it will create dependency on boost.
 * This file also has separated automatic tests requiring Boost and elevated privileges.
 */


#include <chrono>


#include <superior_mysqlpp/connection_pool.hpp>
#include <superior_mysqlpp/shared_ptr_pool/sleep_in_parts.hpp>
#include <superior_mysqlpp/dnsa_connection_pool.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address.hpp>


namespace SuperiorMySqlpp
{
    namespace detail
    {
        /**
         * Utility class that provides a method for DNS resolving.
         * Class depends on boost::asio library.
         */
        class DnsResolver
        {
        public:
            DnsResolver() = default;
            DnsResolver(const DnsResolver&) = default;
            DnsResolver(DnsResolver&&) = default;
            ~DnsResolver() = default;

            DnsResolver& operator=(const DnsResolver&) = default;
            DnsResolver& operator=(DnsResolver&&) = default;

            /**
             * Resolve a hostname to ip addresses.
             *
             * @tparam const reference to std::string
             * @param hostname
             * @return Vector of strings that represent ip addresses for given hostname.
             */
            std::vector<std::string> resolve(const std::string& hostname)
            {
                boost::asio::io_service ioService{};
                std::vector<std::string> addresses{};

                // setup the resolver query
                boost::asio::ip::tcp::resolver resolver{ioService};
                boost::asio::ip::tcp::resolver::query query{hostname, ""};

                // prepare response iterator
                boost::asio::ip::tcp::resolver::iterator destination{resolver.resolve(query)};
                boost::asio::ip::tcp::resolver::iterator end{};
                boost::asio::ip::tcp::endpoint endpoint{};

                while (destination != end)
                {
                    endpoint = *destination++;
                    addresses.emplace_back(endpoint.address().to_string());
                }

                return addresses;
            }
        };

        /**
         * Mix in class that checks and logs DNS changes.
         * It is used as pool management class in DnsaConnectionPool.
         */
        template<typename Base, bool terminateOnFailure>
        class DnsAwarePoolManagement
        {
        private:
            /** Flag that specifies if jobThread is running. */
            std::atomic<bool> enabled{false};
            /** Sleep period between DNS resolutions.
             *  Sleep can be aborted if _enabled_ is set to false.
             */
            std::chrono::milliseconds sleepTime{std::chrono::seconds{10}};
            /* DNS resolution thread */
            std::thread jobThread{};
            /* Hostname to resolve */
            std::string hostname;

        public:
            DnsAwarePoolManagement(std::string hostname)
                : hostname{std::move(hostname)}
            {}

            DnsAwarePoolManagement(const DnsAwarePoolManagement&) = delete;

            /**
             * Initializes pool from other pool.
             * It starts DNS resolution job if job is running in other instance.
             */
            DnsAwarePoolManagement(DnsAwarePoolManagement&& other)
                : enabled{[&](){ other.stopDnsAwarePoolManagement(); return other.enabled.load(); }()},
                  sleepTime{std::move(other).sleepTime},
                  hostname{std::move(other).hostname}
            {
                if (enabled)
                {
                    startDnsAwarePoolManagement();
                }
            }

            /**
             * Stops DNS check job if it is running.
             */
            ~DnsAwarePoolManagement()
            {
                stopDnsAwarePoolManagement();
            }

            DnsAwarePoolManagement& operator=(const DnsAwarePoolManagement&) = delete;
            DnsAwarePoolManagement& operator=(DnsAwarePoolManagement&&) = delete;

        private:
            /**
             * Returns reference to base class where is this mixin class used.
             */
            Base& getBase()
            {
                return *static_cast<Base*>(this);
            }

            /**
             * Returns const reference to class where is this mixin class used.
             */
            const Base& getBase() const
            {
                return *static_cast<const Base*>(this);
            }

            /**
             * This function is called when unexpected error is thrown during dns resolution error handling.
             * It's called only if logger throws exception.
             */
            void onUnexpectedError(void) const noexcept {
                if(terminateOnFailure) {
                    std::terminate();
                }
            }

            /**
             * This function is run as parallel thread.
             * We cannot afford to let any exceptions to be propagated from logging functions
             * since it would result in call to std::terminate().
             */
            void job(void) const {
                try {
                    runJobLoop();
                } catch(std::exception &e) {
                    getBase().getLogger()->logSharedPtrPoolDnsAwarePoolManagementError(getBase().getId(), e);
                    onUnexpectedError();
                } catch(...) {
                    getBase().getLogger()->logSharedPtrPoolDnsAwarePoolManagementError(getBase().getId());
                    onUnexpectedError();
                }
            }

            /**
             * It does dns resolution of given hostname.
             * Logs if previously resolved ip addresses differ from new ones.
             */
            void runJobLoop() const
            {
                std::vector<std::string> lastIpAddresses{};

                while (enabled)
                {
                    getBase().getLogger()->logSharedPtrPoolDnsAwarePoolManagementCycleStart(getBase().getId());

                    try
                    {
                        //We do intentionally make always new connection to DNS server.
                        DnsResolver resolver{};
                        auto ipAddresses = resolver.resolve(hostname);


                        // Check if ip addresses match to last resolved ip addresses.
                        if (!std::equal(lastIpAddresses.begin(), lastIpAddresses.end(), ipAddresses.begin(), ipAddresses.end()))
                        {
                            getBase().getLogger()->logSharedPtrPoolDnsAwarePoolManagementChangeDetected(getBase().getId());

                            getBase().clearPool();
                            lastIpAddresses = std::move(ipAddresses);
                        }
                    }
                    // catch all dns related errors
                    catch (std::exception& e)
                    {
                        getBase().getLogger()->logSharedPtrPoolDnsAwarePoolManagementCheckError(getBase().getId(), e);
                    }
                    catch (...)
                    {
                        getBase().getLogger()->logSharedPtrPoolDnsAwarePoolManagementCheckError(getBase().getId());
                    }

                    getBase().getLogger()->logSharedPtrPoolDnsAwarePoolManagementCycleEnd(getBase().getId());

                    sleepInParts(sleepTime, std::chrono::milliseconds{50}, [&](){ return enabled.load(); });
                }

                getBase().getLogger()->logSharedPtrPoolDnsAwarePoolManagementStopped(getBase().getId());
            }

        public:
            /**
             * Stops DNS resolution check.
             */
            void stopDnsAwarePoolManagement()
            {
                if (jobThread.joinable())
                {
                    enabled = false;
                    jobThread.join();
                }
                enabled = false;
            }

            /**
             * Starts DNS resolution check.
             */
            void startDnsAwarePoolManagement()
            {
                /* We must stop possibly running thread otherwise ~thread/operator= shall call std::terminate! */
                stopDnsAwarePoolManagement();
                enabled = true;
                jobThread = std::thread{&DnsAwarePoolManagement::job, std::cref(*this)};
            }

            /**
             * Returns true if jobThread is in joinable state.
             */
            auto isDnsAwarePoolManagementThreadRunning() const
            {
                // XXX: this is ambigious, it can return false if jobThread is detached
                return jobThread.joinable();
            }

            /**
             * Returns id of jobThread.
             */
            auto getDnsAwarePoolManagementThreadId() const
            {
                return jobThread.get_id();
            }

            /**
             * Returns DNS resolution sleepTime
             */
            auto getDnsAwarePoolManagementSleepTime() const
            {
                return sleepTime;
            }

            /**
             * Sets DNS resolution sleepTime
             */
            void setDnsAwarePoolManagementSleepTime(decltype(sleepTime) value)
            {
                sleepTime = value;
            }
        };
    }


    template<typename Base>
    using DnsAwarePoolManagement = detail::DnsAwarePoolManagement<Base, true>;


    template<
        typename SharedPtrFactory,
        typename ResourceHealthCheck=void,
        bool invalidateResourceOnAccess=false,
        /* Resource count keeper */
        bool enableResourceCountKeeper=true,
        bool terminateOnResourceCountKeeperFailure=false,
        /* Health care job */
        bool enableHealthCareJob=true,
        bool terminateOnHealthCareJobFailure=false
    >
    using DnsaSharedPtrPool = SharedPtrPool<
            /* Basic pool */
            SharedPtrFactory,
            ResourceHealthCheck,
            invalidateResourceOnAccess,
            /* Resource count keeper */
            enableResourceCountKeeper,
            terminateOnResourceCountKeeperFailure,
            /* Health care job */
            enableHealthCareJob,
            terminateOnHealthCareJobFailure,
            /* Pool management */
            DnsAwarePoolManagement
    >;



    template<
        typename SharedPtrFactory,
        bool invalidateResourceOnAccess=false,
        /* Resource count keeper */
        bool enableResourceCountKeeper=true,
        bool terminateOnResourceCountKeeperFailure=false,
        /* Health care job */
        bool enableHealthCareJob=true,
        bool terminateOnHealthCareJobFailure=false
    >
    using DnsaConnectionPool = ConnectionPool<
            SharedPtrFactory,
            invalidateResourceOnAccess,
            /* Resource count keeper */
            enableResourceCountKeeper,
            terminateOnResourceCountKeeperFailure,
            /* Health care job */
            enableHealthCareJob,
            terminateOnHealthCareJobFailure,
            /* Pool management */
            DnsAwarePoolManagement
    >;


    template<
        bool invalidateResourceOnAccess=false,
        /* Resource count keeper */
        bool enableResourceCountKeeper=true,
        bool terminateOnResourceCountKeeperFailure=false,
        /* Health care job */
        bool enableHealthCareJob=true,
        bool terminateOnHealthCareJobFailure=false,
        typename SharedPtrFactory,
        typename LoggerPtr=decltype(DefaultLogger::getLoggerPtr())
    >
    auto makeDnsaConnectionPool(SharedPtrFactory&& factory, std::string hostname, LoggerPtr&& loggerPtr=DefaultLogger::getLoggerPtr())
    {
        return DnsaConnectionPool<
            std::decay_t<SharedPtrFactory>,
            invalidateResourceOnAccess,
            /* Resource count keeper */
            enableResourceCountKeeper,
            terminateOnResourceCountKeeperFailure,
            /* Health care job */
            enableHealthCareJob,
            terminateOnHealthCareJobFailure
        >
        {
            std::forward_as_tuple(std::forward<SharedPtrFactory>(factory)),
            std::forward_as_tuple(),
            std::forward_as_tuple(std::move(hostname)),
            std::forward<LoggerPtr>(loggerPtr)
        };
    }
}
