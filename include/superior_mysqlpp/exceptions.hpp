/*
 * Author: Tomas Nozicka
 */

#pragma once

#include <string>
#include <stdexcept>

namespace SuperiorMySqlpp
{
    class SuperiorMySqlppError : public std::runtime_error
    {
    public:
        explicit SuperiorMySqlppError(const std::string& message)
            : std::runtime_error{message}
        {
        }
    };


    class MysqlInternalError : public SuperiorMySqlppError
    {
    public:
        explicit MysqlInternalError(const std::string &message):
            SuperiorMySqlppError(message),
            errorCode{0}
        {
        }

        MysqlInternalError(const std::string& message, const char* mysqlError, unsigned int errorCode)
            : SuperiorMySqlppError(message + "\nMysql error: " + std::string(mysqlError)), errorCode{errorCode}
        {
        }

        unsigned int getErrorCode() {
            return errorCode;
        }

    private:
        unsigned int errorCode;
    };

    class MysqlDataTruncatedError: public SuperiorMySqlppError
    {
    public:
        using SuperiorMySqlppError::SuperiorMySqlppError;
    };

    class RuntimeError : public SuperiorMySqlppError
    {
    public:
        using SuperiorMySqlppError::SuperiorMySqlppError;
    };

    class LogicError : public SuperiorMySqlppError
    {
    public:
        using SuperiorMySqlppError::SuperiorMySqlppError;
    };

    class OutOfRange : public SuperiorMySqlppError
    {
    public:
        using SuperiorMySqlppError::SuperiorMySqlppError;
    };

    class InternalError : public SuperiorMySqlppError
    {
    public:
        using SuperiorMySqlppError::SuperiorMySqlppError;
    };

    class PreparedStatementTypeError : public SuperiorMySqlppError
    {
    public:
        using SuperiorMySqlppError::SuperiorMySqlppError;
    };

    class DynamicPreparedStatementTypeError : public SuperiorMySqlppError
    {
    public:
        using SuperiorMySqlppError::SuperiorMySqlppError;
    };

    class DynamicPreparedStatementLogicError : public SuperiorMySqlppError
    {
    public:
        using SuperiorMySqlppError::SuperiorMySqlppError;
    };

    class QueryError : public SuperiorMySqlppError
    {
    public:
        using SuperiorMySqlppError::SuperiorMySqlppError;
    };

    class QueryNotExecuted : public QueryError
    {
    public:
        using QueryError::QueryError;
    };
}
