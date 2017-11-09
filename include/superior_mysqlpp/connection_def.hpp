/*
 * Author: Tomas Nozicka
 */

#pragma once

#include <string>
#include <memory>
#include <utility>
#include <tuple>


#include <superior_mysqlpp/logging.hpp>
#include <superior_mysqlpp/prepared_statement_fwd.hpp>
#include <superior_mysqlpp/dynamic_prepared_statement_fwd.hpp>
#include <superior_mysqlpp/low_level/dbdriver.hpp>
#include <superior_mysqlpp/types/tags.hpp>
#include <superior_mysqlpp/types/optional.hpp>
#include <superior_mysqlpp/config.hpp>


namespace SuperiorMySqlpp
{
    class Query;

    using ConnectionOptions = LowLevel::DBDriver::DriverOptions;


    class Connection
    {
    protected:
        LowLevel::DBDriver driver;

    protected:
        inline void setSslConfiguration(const SslConfiguration& sslConfig) noexcept
        {
            driver.setSsl(sslConfig.keyPath, sslConfig.certificatePath, sslConfig.certificationAuthorityPath,
                          sslConfig.trustedCertificateDirPath, sslConfig.allowableCiphers);
        }

    public:
        template<typename... OptionTuples>
        Connection(const std::string& database, const std::string& user, const std::string& password="",
                   const std::string& host="localhost", std::uint16_t port=3306,
                   std::tuple<OptionTuples...> optionTuples=std::make_tuple(),
                   Loggers::SharedPointer_t loggerPtr=DefaultLogger::getLoggerPtr())
            : driver{std::move(loggerPtr)}
        {
            setOptions(std::move(optionTuples));
            driver.connect(host.c_str(), user.c_str(), password.c_str(), database.c_str(), port, nullptr);
        }

        template<typename... OptionTuples>
        Connection(const SslConfiguration& sslConfig,
                   const std::string& database, const std::string& user, const std::string& password, const std::string& host, std::uint16_t port,
                   std::tuple<OptionTuples...> optionTuples=std::make_tuple(),
                   Loggers::SharedPointer_t loggerPtr=DefaultLogger::getLoggerPtr())
            : driver{std::move(loggerPtr)}
        {
            setOptions(std::move(optionTuples));
            setSslConfiguration(sslConfig);
            driver.connect(host.c_str(), user.c_str(), password.c_str(), database.c_str(), port, nullptr);
        }

        template<typename... OptionTuples>
        Connection(const std::string& database, const std::string& user, const std::string& password, const std::string& socketPath,
                   std::tuple<OptionTuples...> optionTuples=std::make_tuple(),
                   Loggers::SharedPointer_t loggerPtr=DefaultLogger::getLoggerPtr())
            : driver{std::move(loggerPtr)}
        {
            setOptions(std::move(optionTuples));
            driver.connect(nullptr, user.c_str(), password.c_str(), database.c_str(), 0, socketPath.c_str());
        }

        template<typename... OptionTuples>
        Connection(const SslConfiguration& sslConfig,
                   const std::string& database, const std::string& user, const std::string& password, const std::string& socketPath,
                   std::tuple<OptionTuples...> optionTuples=std::make_tuple(),
                   Loggers::SharedPointer_t loggerPtr=DefaultLogger::getLoggerPtr())
            : driver{std::move(loggerPtr)}
        {
            setOptions(std::move(optionTuples));
            setSslConfiguration(sslConfig);
            driver.connect(nullptr, user.c_str(), password.c_str(), database.c_str(), 0, socketPath.c_str());
        }

        template<typename... OptionTuples>
        Connection(ConnectionConfiguration config,
                   std::tuple<OptionTuples...> optionTuples=std::make_tuple(),
                   Loggers::SharedPointer_t loggerPtr=DefaultLogger::getLoggerPtr())
            : driver{std::move(loggerPtr)}
        {
            setOptions(std::move(optionTuples));

            if (config.sslConfig)
            {
                setSslConfiguration(config.sslConfig.value());
            }

            if (config.usingSocket)
            {
                driver.connect(nullptr, config.user.c_str(), config.password.c_str(), config.database.c_str(), 0, config.target.c_str());
            }
            else
            {
                driver.connect(config.target.c_str(), config.user.c_str(), config.password.c_str(), config.database.c_str(), config.port, nullptr);
            }
        }


        ~Connection() = default;

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        Connection(Connection&&) = default;
        Connection& operator=(Connection&&) = default;


        auto& getLoggerPtr()
        {
            return driver.getLoggerPtr();
        }

        const auto& getLoggerPtr() const
        {
            return driver.getLoggerPtr();
        }

        template<typename T>
        void setLoggerPtr(T&& value)
        {
            driver.setLoggerPtr(std::forward<T>(value));
        }

        Loggers::ConstPointer_t getLogger() const
        {
            return driver.getLogger();
        }


        template<typename RBindings=ResultBindings<>,
                 bool storeResult=detail::PreparedStatementsDefault::getStoreResult(),
                 ValidateMetadataMode validateMode=detail::PreparedStatementsDefault::getValidateMode(),
                 ValidateMetadataMode warnMode=detail::PreparedStatementsDefault::getWarnMode(),
                 bool ignoreNullable=detail::PreparedStatementsDefault::getIgnoreNullable(),
                 typename... Args>
        PreparedStatement<RBindings,
                          ParamBindings<std::decay_t<Args>...>,
                          storeResult,
                          validateMode,
                          warnMode,
                          ignoreNullable
                          > makePreparedStatement(const std::string&, Args&&...) && = delete;

        template<typename RBindings=ResultBindings<>,
                 bool storeResult=detail::PreparedStatementsDefault::getStoreResult(),
                 ValidateMetadataMode validateMode=detail::PreparedStatementsDefault::getValidateMode(),
                 ValidateMetadataMode warnMode=detail::PreparedStatementsDefault::getWarnMode(),
                 bool ignoreNullable=detail::PreparedStatementsDefault::getIgnoreNullable(),
                 typename... Args>
        PreparedStatement<RBindings,
                          ParamBindings<std::decay_t<Args>...>,
                          storeResult,
                          validateMode,
                          warnMode,
                          ignoreNullable
                          > makePreparedStatement(const std::string&, Args&&...) &;



        template<typename RBindings=ResultBindings<>,
                 bool storeResult=detail::PreparedStatementsDefault::getStoreResult(),
                 ValidateMetadataMode validateMode=detail::PreparedStatementsDefault::getValidateMode(),
                 ValidateMetadataMode warnMode=detail::PreparedStatementsDefault::getWarnMode(),
                 bool ignoreNullable=detail::PreparedStatementsDefault::getIgnoreNullable(),
                 template<typename...> class RArgsTuple, template<typename...> class PArgsTuple,
                 typename... RArgs, typename... PArgs
                 >
        PreparedStatement<RBindings,
                          ParamBindings<std::decay_t<PArgs>...>,
                          storeResult,
                          validateMode,
                          warnMode,
                          ignoreNullable
                          > makePreparedStatement(
                const std::string&, FullInitTag, RArgsTuple<RArgs...>&&, PArgsTuple<PArgs...>&&) && = delete;

        template<typename RBindings=ResultBindings<>,
                 bool storeResult=detail::PreparedStatementsDefault::getStoreResult(),
                 ValidateMetadataMode validateMode=detail::PreparedStatementsDefault::getValidateMode(),
                 ValidateMetadataMode warnMode=detail::PreparedStatementsDefault::getWarnMode(),
                 bool ignoreNullable=detail::PreparedStatementsDefault::getIgnoreNullable(),
                 template<typename...> class RArgsTuple, template<typename...> class PArgsTuple,
                 typename... RArgs, typename... PArgs
                 >
        PreparedStatement<RBindings,
                          ParamBindings<std::decay_t<PArgs>...>,
                          storeResult,
                          validateMode,
                          warnMode,
                          ignoreNullable
                          > makePreparedStatement(
                const std::string&, FullInitTag, RArgsTuple<RArgs...>&&, PArgsTuple<PArgs...>&&) &;




        template<bool storeResult=detail::PreparedStatementsDefault::getStoreResult(),
                 ValidateMetadataMode validateMode=detail::PreparedStatementsDefault::getValidateMode(),
                 ValidateMetadataMode warnMode=detail::PreparedStatementsDefault::getWarnMode(),
                 bool ignoreNullable=detail::PreparedStatementsDefault::getIgnoreNullable()>
        DynamicPreparedStatement<storeResult, validateMode, warnMode, ignoreNullable> makeDynamicPreparedStatement(const std::string& query) && = delete;

        template<bool storeResult=detail::PreparedStatementsDefault::getStoreResult(),
                 ValidateMetadataMode validateMode=detail::PreparedStatementsDefault::getValidateMode(),
                 ValidateMetadataMode warnMode=detail::PreparedStatementsDefault::getWarnMode(),
                 bool ignoreNullable=detail::PreparedStatementsDefault::getIgnoreNullable()>
        DynamicPreparedStatement<storeResult, validateMode, warnMode, ignoreNullable> makeDynamicPreparedStatement(const std::string& query) &;


        template<typename... Args>
        Query makeQuery(Args&&... args) && = delete;

        template<typename... Args>
        Query makeQuery(Args&&... args) &;


        std::string escapeString(const std::string& original)
        {
            return driver.escapeString(original);
        }

        static std::string escapeStringNoConnection(const std::string& original)
        {
            return LowLevel::DBDriver::escapeStringNoConnection(original);
        }

        void changeUser(const char* user, const char* password, const char* database)
        {
            driver.changeUser(user, password, database);
        }

        void ping()
        {
            driver.ping();
        }

        bool tryPing()
        {
            return driver.tryPing();
        }

        auto getId() const
        {
            return driver.getId();
        }

        using ServerOptions = LowLevel::DBDriver::ServerOptions;

        void setServerOption(ServerOptions option)
        {
            driver.setServerOption(option);
        }

        void setOption(ConnectionOptions option, const void* argumentPtr)
        {
            driver.setDriverOption(option, argumentPtr);
        }

        template<typename... OptionTuples, std::size_t... I>
        void detail_setOptions(std::tuple<OptionTuples...> mainTuple, std::index_sequence<I...>)
        {
            static_cast<void>(mainTuple);  // this prevents unused variable error if mainTuple is empty
            /*
             * This magic is doing for each argument to call setOption.
             * Order of evaluation is guaranteed by the standard.
             */
            using IntArray = int[];
            static_cast<void>(IntArray{(setOption(
                        std::get<0>(std::get<I>(mainTuple)),
                        std::get<1>(std::get<I>(mainTuple))
            ), 0)..., 0});
        }

        template<typename... OptionTuples>
        void setOptions(std::tuple<OptionTuples...> mainTuple)
        {
            detail_setOptions(std::move(mainTuple), std::index_sequence_for<OptionTuples...>{});
        }


        /*
         * DO NOT USE this function unless you want to work with wrapped C API directly!
         */
        LowLevel::DBDriver& detail_getDriver() && = delete;
        LowLevel::DBDriver& detail_getDriver() &
        {
            return driver;
        }
    };


    inline bool operator==(const Connection& lhs, const Connection& rhs)
    {
        return lhs.getId() == rhs.getId();
    }
}
