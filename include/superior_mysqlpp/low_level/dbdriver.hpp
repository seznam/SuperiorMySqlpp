/*
 * Author: Tomas Nozicka
 */

#pragma once

#include <mysql/mysql.h>
#include <pthread.h>

#include <stdexcept>
#include <mutex>

#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <type_traits>
#include <cinttypes>
#include <atomic>

#include <superior_mysqlpp/logging.hpp>
#include <superior_mysqlpp/exceptions.hpp>
#include <superior_mysqlpp/utils.hpp>
#include <superior_mysqlpp/types/string_view.hpp>
#include <superior_mysqlpp/low_level/mysql_hacks.hpp>


namespace SuperiorMySqlpp { namespace LowLevel
{
    namespace detail
    {
        class MysqlLibraryInitWrapper
        {
        private:
            MysqlLibraryInitWrapper()
            {
                if (mysql_library_init(0, nullptr, nullptr))
                {
                    throw MysqlInternalError("Could not initialize MYSQL library!");
                }
            }

            ~MysqlLibraryInitWrapper()
            {
                mysql_library_end();
            }

        public:
            static void initialize()
            {
                // Thread-safe initialization
                static MysqlLibraryInitWrapper instance{};
            }
        };
    }


    class DBDriver
    {
    private:
        bool connected = false;
        const std::uint_fast64_t id;
        MYSQL mysql;
        Loggers::SharedPointer_t loggerPtr;

        using size_t = unsigned long long;

    private:
        static auto& getGlobalIdRef()
        {
            static std::atomic<std::uint_fast64_t> globalId{1};
            return globalId;
        }

        MYSQL& getMysql()
        {
            return mysql;
        }

        MYSQL* getMysqlPtr()
        {
            return &mysql;
        }

        void mysqlInit()
        {
            detail::MysqlLibraryInitWrapper::initialize();

            if (mysql_init(getMysqlPtr()) == nullptr)
            {
                throw MysqlInternalError("Could not initialize MYSQL library. (mysql_init failed)");
            }

            if (mysql_thread_safe())
            {
                detail::MySqlThreadRaii::setup();
            }
        }

        void mysqlClose()
        {
            getLogger()->logMySqlClose(id);
            mysql_close(getMysqlPtr());
        }


    public:
        DBDriver(Loggers::SharedPointer_t loggerPtr=DefaultLogger::getLoggerPtr())
            : id{getGlobalIdRef().fetch_add(1)}, loggerPtr{std::move(loggerPtr)}
        {
            mysqlInit();
        }

        ~DBDriver()
        {
            mysqlClose();
        }

        DBDriver(const DBDriver&) = delete;
        DBDriver& operator=(const DBDriver&) = delete;

        DBDriver(DBDriver&&) = default;
        DBDriver& operator=(DBDriver&&) = delete;


        auto& getLoggerPtr()
        {
            return loggerPtr;
        }

        const auto& getLoggerPtr() const
        {
            return loggerPtr;
        }

        template<typename T>
        void setLoggerPtr(T&& value)
        {
            loggerPtr = std::forward<T>(value);
        }

        Loggers::ConstPointer_t getLogger() const
        {
            return loggerPtr.get();
        }


        auto affectedRows()
        {
            return mysql_affected_rows(getMysqlPtr());
        }

        void enableAutocommit(bool value)
        {
            if (mysql_autocommit(getMysqlPtr(), value))
            {
                throw MysqlInternalError("Failed to set autocommit!");
            }
        }

        template<typename T>
        std::string escapeString(const char* original, T originalLength)
        {
            auto buffer = std::make_unique<char[]>(originalLength*2 + 1);
            auto length = mysql_real_escape_string(getMysqlPtr(), buffer.get(), original, originalLength);

            return std::string(buffer.get(), length);
        }

        std::string escapeString(const std::string& original)
        {
            return escapeString(original.c_str(), original.length());
        }

        template<typename T>
        static std::string escapeStringNoConnection(const char* original, T originalLength)
        {
            auto buffer = std::make_unique<char[]>(originalLength*2 + 1);
            auto length = mysql_escape_string(buffer.get(), original, originalLength);

            return std::string(buffer.get(), length);
        }

        static std::string escapeStringNoConnection(const std::string& original)
        {
            return escapeStringNoConnection(original.c_str(), original.length());
        }

        template<typename Iterable>
        std::string makeHexString(const Iterable& from)
        {
            auto fromSize = std::end(from) - std::begin(from);
            auto buffer = std::make_unique<char[]>(fromSize*2 + 1);
            auto length = mysql_hex_string(buffer.get(), from, fromSize);

            return std::string(buffer.get(), length);
        }

        void changeUser(const char* user, const char* password, const char* database)
        {
            using namespace std::string_literals;
            if (mysql_change_user(getMysqlPtr(), user, password, database))
            {
                throw MysqlInternalError("Failed to change user to '"s + user + "' on database '" + database + "'!",
                    mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
            }
        }

        auto getCharacterSetName()
        {
            return mysql_character_set_name(getMysqlPtr());
        }

        void connect(const char* host,
                     const char* user,
                     const char* password,
                     const char* database,
                     unsigned int port,
                     const char* socketName)
        {
            using namespace std::string_literals;

            if (connected)
            {
                mysqlClose();
                mysqlInit();
            }

            getLogger()->logMySqlConnecting(id, host, user, database, port, socketName);
            if (mysql_real_connect(getMysqlPtr(), host, user, password, database, port, socketName, CLIENT_MULTI_STATEMENTS) == nullptr)
            {
                std::stringstream message{};
                message << "Failed to connect to MySQL. (Host: ";
                if (host != nullptr)
                {
                    message << host;
                }
                message << "; User: ";
                if (user != nullptr)
                {
                    message << user;
                }
                message << "; Database: ";
                if (database != nullptr)
                {
                    message << database;
                }
                message << "; Port: " << port;
                message << "; SocketName: ";
                if (socketName != nullptr)
                {
                    message << socketName;
                }

                throw MysqlInternalError(message.str(), mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
            }

            connected = true;
            getLogger()->logMySqlConnected(id);
        }


        bool isConnected() const
        {
            return connected;
        }


        class Result
        {
        private:
            MYSQL_RES* resultPtr;

            void close() noexcept
            {
                if (resultPtr != nullptr)
                {
                    mysql_free_result(resultPtr);
                }
            }

        public:
            explicit Result(MYSQL_RES* resultPtr)
                : resultPtr{resultPtr}
            {
                if (resultPtr == nullptr)
                {
                    throw InternalError("DBDriver result cannot be initialized with nullptr!");
                }
            }

            Result(const Result&) = delete;
            Result& operator=(const Result&) = delete;

            Result(Result&& result) noexcept
            {
                resultPtr = result.resultPtr;
                result.resultPtr = nullptr;
            }

            Result& operator=(Result&& result) noexcept
            {
                static_assert(noexcept(close()), "close() must be noexcept!");
                close();
                resultPtr = result.resultPtr;
                result.resultPtr = nullptr;
                return *this;
            }

            ~Result()
            {
                close();
            }


            void seekRow(size_t index)
            {
                mysql_data_seek(resultPtr, index);
            }

            void seekRowOffset(MYSQL_ROW_OFFSET offset)
            {
                mysql_row_seek(resultPtr, offset);
            }

            auto fetchField()
            {
                return mysql_fetch_field(resultPtr);
            }

            auto fetchFieldDirect(unsigned int index)
            {
                return mysql_fetch_field_direct(resultPtr, index);
            }

            auto fetchFields()
            {
                return mysql_fetch_fields(resultPtr);
            }

            auto fetchLengths()
            {
                return mysql_fetch_lengths(resultPtr);
            }

            auto fetchRow()
            {
                return mysql_fetch_row(resultPtr);
            }

            auto fieldSeek(MYSQL_FIELD_OFFSET offset)
            {
                return mysql_field_seek(resultPtr, offset);
            }

            auto fieldTell()
            {
                return mysql_field_tell(resultPtr);
            }

            auto freeResult()
            {
                return mysql_free_result(resultPtr);
            }

            auto getFieldsCount()
            {
                return mysql_num_fields(resultPtr);
            }

            auto getRowsCount()
            {
                return mysql_num_rows(resultPtr);
            }

            auto tellRowOffset()
            {
                return mysql_row_tell(resultPtr);
            }


            /*
             * DO NOT USE this function unless you want to work with C API directly!
             */
            auto detail_getResultPtr()
            {
                return resultPtr;
            }
        };


        auto getFieldsCount()
        {
            return mysql_field_count(getMysqlPtr());
        }

        static std::string getClientInfo()
        {
            return mysql_get_client_info();
        }

        static auto getClientVersion()
        {
            return mysql_get_client_version();
        }

        std::string getHostInfo()
        {
            return mysql_get_host_info(getMysqlPtr());
        }

        auto getCharacterSetInfo()
        {
            MY_CHARSET_INFO characterSet;
            mysql_get_character_set_info(getMysqlPtr(), &characterSet);
            return characterSet;
        }

        auto getProtocolInfo()
        {
            return mysql_get_proto_info(getMysqlPtr());
        }

        std::string getServerInfo()
        {
            return mysql_get_server_info(getMysqlPtr());
        }

        auto getServerVersion()
        {
            return mysql_get_server_version(getMysqlPtr());
        }

        std::string getSslCipher()
        {
            auto stringPtr = mysql_get_ssl_cipher(getMysqlPtr());
            if (stringPtr == nullptr)
            {
                return {};
            }
            else
            {
                return stringPtr;
            }
        }

        auto getInsertId()
        {
            return mysql_insert_id(getMysqlPtr());
        }

        void execute(const char* queryString, unsigned long length)
        {
            getLogger()->logMySqlQuery(id, StringView{queryString, length});
            if (mysql_real_query(getMysqlPtr(), queryString, length))
            {
                throw MysqlInternalError("Failed to execute query!",
                    mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
            }
        }

        void execute(const std::string& queryString)
        {
            execute(queryString.c_str(), queryString.length());
        }

        bool hasMoreResults()
        {
            return mysql_more_results(getMysqlPtr());
        }


        enum class DriverOptions : std::underlying_type_t<mysql_option>
        {
              connectTimeout = MYSQL_OPT_CONNECT_TIMEOUT,
              compress = MYSQL_OPT_COMPRESS,
              namedPipe = MYSQL_OPT_NAMED_PIPE,
              initCommand = MYSQL_INIT_COMMAND,
              readDefaultFile = MYSQL_READ_DEFAULT_FILE,
              readDefaultGroup = MYSQL_READ_DEFAULT_GROUP,
              setCharsetDir = MYSQL_SET_CHARSET_DIR,
              setCharsetName = MYSQL_SET_CHARSET_NAME,
              localInfile = MYSQL_OPT_LOCAL_INFILE,
              protocol = MYSQL_OPT_PROTOCOL,
              sharedMemoryBaseName = MYSQL_SHARED_MEMORY_BASE_NAME,
              readTimeout = MYSQL_OPT_READ_TIMEOUT,
              writeTimeout = MYSQL_OPT_WRITE_TIMEOUT,
              useResult = MYSQL_OPT_USE_RESULT,
              useRemoteConnection = MYSQL_OPT_USE_REMOTE_CONNECTION,
              useEmbeddedConnection = MYSQL_OPT_USE_EMBEDDED_CONNECTION,
              guessConnection = MYSQL_OPT_GUESS_CONNECTION,
              setClientIp = MYSQL_SET_CLIENT_IP,
              secureAuthentication = MYSQL_SECURE_AUTH,
              reportDataTruncation = MYSQL_REPORT_DATA_TRUNCATION,
              reconnect = MYSQL_OPT_RECONNECT,
              sslVerifyServerCertificate = MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
              pluginDir = MYSQL_PLUGIN_DIR,
              defaultAuthentication = MYSQL_DEFAULT_AUTH,
              enableClearTextPlugin = MYSQL_ENABLE_CLEARTEXT_PLUGIN,
        };

        void setDriverOption(DriverOptions option, const void* argumentPtr)
        {
            if (mysql_options(getMysqlPtr(), static_cast<mysql_option>(option), argumentPtr))
            {
                throw MysqlInternalError("Failed to set option!",
                    mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
            }
        }

        /*
         * true if there are more results
         */
        bool nextResult()
        {
            auto result = mysql_next_result(getMysqlPtr());
            switch (result)
            {
                case 0:
                    return true;
                case -1:
                    return false;
                default:
                    throw MysqlInternalError("Failed to get next result!",
                        mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
            }
        }

        void ping()
        {
            getLogger()->logMySqlPing(id);
            if (mysql_ping(getMysqlPtr()))
            {
                throw MysqlInternalError("Failed to ping server!",
                    mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
            }
        }

        bool tryPing()
        {
            getLogger()->logMySqlPing(id);
            return !mysql_ping(getMysqlPtr());
        }

        std::string queryInfo()
        {
            const char* info = mysql_info(getMysqlPtr());
            if (info == nullptr)
            {
                return {};
            }
            else
            {
                return info;
            }
        }

        void setCharacterSet(char* characterSetName)
        {
            if (mysql_set_character_set(getMysqlPtr(), characterSetName))
            {
                throw MysqlInternalError("Failed to set character set!",
                    mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
            }
        }


        enum class ServerOptions
        {
              multiStatementsOn = MYSQL_OPTION_MULTI_STATEMENTS_ON,
              myltistatementsOff = MYSQL_OPTION_MULTI_STATEMENTS_OFF,
        };

        void setServerOption(ServerOptions option)
        {
            if (mysql_set_server_option(getMysqlPtr(), static_cast<enum_mysql_set_option>(option)))
            {
                throw MysqlInternalError("Failed to set server option!",
                    mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
            }
        }

        void setSsl(
                const char* key,
                const char* certificate,
                const char* certificationAuthority,
                const char* trustedCertificationAuthorityDirectory,
                const char* cipherList)
        {
            mysql_ssl_set(getMysqlPtr(), key, certificate, certificationAuthority,
                    trustedCertificationAuthorityDirectory, cipherList);
        }

        std::string serverStatistics()
        {
            auto statisticsPtr = mysql_stat(getMysqlPtr());
            if (statisticsPtr == nullptr)
            {
                throw MysqlInternalError("Failed to get server statistics!",
                    mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
            }
            return statisticsPtr;
        }

        auto warningsCount()
        {
            return mysql_warning_count(getMysqlPtr());
        }


        /*
         * Prepared statements
         */
        class Statement
        {
        private:
            MYSQL_STMT* statementPtr;
            std::uint_fast64_t driverId;
            std::uint_fast64_t id;
            Loggers::ConstPointer_t loggerPtr;

            static auto& getGlobalIdRef()
            {
                static std::atomic<std::uint_fast64_t> globalId{1};
                return globalId;
            }

            void close() noexcept
            {
                if (statementPtr != nullptr)
                {
                    loggerPtr->logMySqlStmtClose(driverId, id);
                    if (mysql_stmt_close(statementPtr))
                    {
                        throw MysqlInternalError("Failed to close statement!",
                            mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr));
                    }
                }
            }

        public:
            Statement() = delete;

            Statement(MYSQL& mysql, std::uint_fast64_t driverId, Loggers::ConstPointer_t loggerPtr)
                : driverId{driverId}, id{getGlobalIdRef().fetch_add(1)}, loggerPtr{std::move(loggerPtr)}
            {
                statementPtr = mysql_stmt_init(&mysql);
                if (statementPtr == nullptr)
                {
                    throw MysqlInternalError("Could not initialize statement!",
                        mysql_error(&mysql), mysql_errno(&mysql));
                }

            }

            Statement(const Statement&) = delete;
            Statement& operator=(const Statement&) = delete;

            Statement(Statement&& other) noexcept
                : statementPtr{std::move(other).statementPtr},
                  driverId{std::move(other).driverId},
                  id{std::move(other).id},
                  loggerPtr{std::move(other).loggerPtr}
            {
                other.statementPtr = nullptr;
            }

            Statement& operator=(Statement&& other) noexcept
            {
                static_assert(noexcept(close()), "close() must be noexcept!");
                close();
                statementPtr = other.statementPtr;
                other.statementPtr = nullptr;
                return *this;
            }

            ~Statement()
            {
                close();
            }

            auto getDriverId() const
            {
                return driverId;
            }

            auto getId() const
            {
                return id;
            }

            void prepare(const char* query, unsigned long length)
            {
                loggerPtr->logMySqlStmtPrepare(driverId, id, StringView{query, length});
                if (mysql_stmt_prepare(statementPtr, query, length))
                {
                    throw MysqlInternalError("Failed to prepare statement!",
                        mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr));
                }
            }

            void prepare(const std::string& query)
            {
                prepare(query.c_str(), query.length());
            }

            void bindParam(MYSQL_BIND* bindingsArrayPtr)
            {
                if (mysql_stmt_bind_param(statementPtr, bindingsArrayPtr))
                {
                    throw MysqlInternalError("Failed to bind statement's params!",
                        mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr));
                }
            }

            void execute()
            {
                loggerPtr->logMySqlStmtExecute(driverId, id);
                if (mysql_stmt_execute(statementPtr))
                {
                    throw MysqlInternalError("Failed to execute statement!",
                        mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr));
                }
            }

            auto freeResult()
            {
                return mysql_stmt_free_result(statementPtr);
            }

            auto fieldCount()
            {
                return mysql_stmt_field_count(statementPtr);
            }

            auto insertedId()
            {
                return mysql_stmt_insert_id(statementPtr);
            }

            auto resultMetadata()
            {
                auto resultPtr = mysql_stmt_result_metadata(statementPtr);
                if (resultPtr == nullptr)
                {
                    throw MysqlInternalError("Could not get result statement metadata!",
                        mysql_error(resultPtr->handle), mysql_errno(resultPtr->handle));
                }
                return Result{resultPtr};
            }

            void bindResult(MYSQL_BIND* bindingsArrayPtr)
            {
                if (mysql_stmt_bind_result(statementPtr, bindingsArrayPtr))
                {
                    throw MysqlInternalError("Failed to bind statement's result!",
                        mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr));
                }
            }

            void storeResult()
            {
                if (mysql_stmt_store_result(statementPtr))
                {
                    throw MysqlInternalError("Failed to store statement's result!",
                        mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr));
                }
            }

            enum class FetchStatus
            {
                Ok,
                NoMoreData,
                DataTruncated,
            };

            FetchStatus fetchWithStatus()
            {
                auto result = mysql_stmt_fetch(statementPtr);
                switch (result)
                {
                    case 0:
                        return FetchStatus::Ok;

                    case MYSQL_NO_DATA:
                        return FetchStatus::NoMoreData;

                    case 1:
                        throw MysqlInternalError{"Could not fetch statement!",
                        mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr)};

                    case MYSQL_DATA_TRUNCATED:
                        return FetchStatus::DataTruncated;

                    default:
                        throw LogicError{"Internal error!"};
                }
            }

            bool fetch()
            {
                auto result = fetchWithStatus();
                switch (result)
                {
                    case FetchStatus::Ok:
                        return true;

                    case FetchStatus::NoMoreData:
                        return false;

                    case FetchStatus::DataTruncated:
                        throw MysqlDataTruncatedError{"Data truncated while fetching statement!" + [&](){
                            // Identify truncated columns
                            auto&& resultBindingsPtr = statementPtr->bind;
                            auto resultBindingsSize = statementPtr->field_count;
                            assert(resultBindingsSize > 0u);

                            std::vector<std::size_t> truncatedColumns{};
                            std::vector<std::size_t> undetectedColumns{};
                            std::size_t index = 0;
                            for (auto it=resultBindingsPtr; it!=resultBindingsPtr+resultBindingsSize; ++it)
                            {
                                if (it->error == &it->error_value)
                                {
                                    if (it->error_value)
                                    {
                                        truncatedColumns.emplace_back(index);
                                    }
                                }
                                else
                                {
                                    undetectedColumns.emplace_back(index);
                                }
                                ++index;
                            }

                            std::string result{" Truncated columns: "};
                            if (truncatedColumns.size() == 0)
                            {
                                result += "None.";
                            }
                            else
                            {
                                result += "[";
                                result += toString(truncatedColumns);
                                result += "].";
                            }

                            if (undetectedColumns.size() > 0)
                            {
                                result += " Following columns truncation state could not have been detected since you have set custom error pointer: ";
                                result += toString(undetectedColumns);
                                result += "!!!";
                            }

                            return result;
                        }()};

                    default:
                        throw LogicError{"Internal error!"};
                }
            }

            void fetchColumn(MYSQL_BIND* bindings, unsigned int column, unsigned long offset)
            {
                if (mysql_stmt_fetch_column(statementPtr, bindings, column, offset))
                {
                    throw MysqlInternalError("Failed to fetch statement's column! Invalid column number!");
                }
            }

            auto affectedRows()
            {
                return mysql_stmt_affected_rows(statementPtr);
            }

            auto paramCount()
            {
                return mysql_stmt_param_count(statementPtr);
            }

            void sendLongData(unsigned int paramNumber, const char* data, unsigned long length)
            {
                if (mysql_stmt_send_long_data(statementPtr, paramNumber, data, length))
                {
                    throw MysqlInternalError("Failed to store statement's result!",
                        mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr));
                }
            }

            auto sendLongData(unsigned int paramNumber, const std::string& data)
            {
                return sendLongData(paramNumber, data.c_str(), data.length());
            }

            std::string sqlState()
            {
                return mysql_stmt_sqlstate(statementPtr);
            }

            void seekRow(size_t index)
            {
                mysql_stmt_data_seek(statementPtr, index);
            }

            void seekRowOffset(MYSQL_ROW_OFFSET offset)
            {
                mysql_stmt_row_seek(statementPtr, offset);
            }

            auto tellRowOffset()
            {
                return mysql_stmt_row_tell(statementPtr);
            }

            auto getRowsCount()
            {
                return mysql_stmt_num_rows(statementPtr);
            }

            enum class Attributes : std::underlying_type_t<enum_stmt_attr_type>
            {
                  updateMaxLength = STMT_ATTR_UPDATE_MAX_LENGTH,
                  cursorType = STMT_ATTR_CURSOR_TYPE,
                  prefetchRows = STMT_ATTR_PREFETCH_ROWS,
            };

            template<typename T>
            void setAttribute(Attributes attribute, const T& argument)
            {
                if (mysql_stmt_attr_set(statementPtr, static_cast<enum_stmt_attr_type>(attribute), &argument))
                {
                    throw MysqlInternalError("Failed to set statement attribute!",
                        mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr));
                }
            }

            template<typename T>
            void getAttribute(Attributes attribute, T& argument)
            {
                if (mysql_stmt_attr_get(statementPtr, static_cast<enum_stmt_attr_type>(attribute), &argument))
                {
                    throw MysqlInternalError("Failed to get statement attribute!",
                        mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr));
                }
            }

            /*
             * true if there are more results
             */
            bool nextResult()
            {
                auto result = mysql_stmt_next_result(statementPtr);
                switch (result)
                {
                    case 0:
                        return true;
                    case -1:
                        return false;
                    default:
                        throw MysqlInternalError("Failed to get next statement's result!",
                            mysql_stmt_error(statementPtr), mysql_stmt_errno(statementPtr));
                }
            }

            /*
             * DO NOT USE this function unless you want to work with C API directly!
             */
            auto detail_getStatementPtr()
            {
                return statementPtr;
            }
        };

        Statement makeStatement()
        {
            return {mysql, id, loggerPtr.get()};
        }


        auto storeResult()
        {
            auto result = mysql_store_result(getMysqlPtr());
            if (result == nullptr)
            {
                auto fieldsCount = getFieldsCount();
                if (fieldsCount == 0)
                {
                    // this query should have returned no result (it's INSERT, UPDATE, ...)
                    throw LogicError("You cann't store result for query that shall have no result!!");
                }
                else
                {
                    throw MysqlInternalError("Failed to store result!",
                        mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
                }
            }
            return Result{result};
        }

        auto useResult()
        {
            auto result = mysql_use_result(getMysqlPtr());
            if (result == nullptr)
            {
                throw MysqlInternalError("Failed to use result!",
                    mysql_error(getMysqlPtr()), mysql_errno(getMysqlPtr()));
            }
            return Result{result};
        }

        auto getId() const
        {
            return id;
        }


        /*
         * DO NOT USE this function unless you want to work with C API directly!
         */
        MYSQL* detail_getMysqlPtr()
        {
            return getMysqlPtr();
        }
    };


    inline bool operator==(const DBDriver& lhs, const DBDriver& rhs)
    {
        return lhs.getId() == rhs.getId();
    }
}}
