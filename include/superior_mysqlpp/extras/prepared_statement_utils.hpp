#pragma once

#include <superior_mysqlpp/connection.hpp>
#include <superior_mysqlpp/exceptions.hpp>
#include <superior_mysqlpp/prepared_statement.hpp>
#include <superior_mysqlpp/utils.hpp>
#include <type_traits>

namespace SuperiorMySqlpp
{
    namespace detail
    {
        /**
         * @brief Makes prepared statement by deducing ResultBindings template arguments from function's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
         * @tparam validateMode Indicates validate mode level
         * @tparam warnMode Indicates warning mode level
         * @tparam ignoreNullable Disables null type checking
         */
        template <bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode, bool ignoreNullable, typename CallableClass, typename... Args>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, void (CallableClass::*)(Args...) const volatile) {
            return connection.makePreparedStatement<ResultBindings<std::remove_cv_t<std::remove_reference_t<Args>>...>, storeResult, validateMode, warnMode, ignoreNullable>(query);
        }

        /**
         * @brief Makes prepared statement by deducing ResultBindings template arguments from function's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
         * @tparam validateMode Indicates validate mode level
         * @tparam warnMode Indicates warning mode level
         * @tparam ignoreNullable Disables null type checking
         */
        template <bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode, bool ignoreNullable, typename CallableClass, typename... Args>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, void (CallableClass::*)(Args...) volatile) {
            return connection.makePreparedStatement<ResultBindings<std::remove_cv_t<std::remove_reference_t<Args>>...>, storeResult, validateMode, warnMode, ignoreNullable>(query);
        }

        /**
         * @brief Makes prepared statement by deducing ResultBindings template arguments from function's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
         * @tparam validateMode Indicates validate mode level
         * @tparam warnMode Indicates warning mode level
         * @tparam ignoreNullable Disables null type checking
         */
        template <bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode, bool ignoreNullable, typename CallableClass, typename... Args>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, void (CallableClass::*)(Args...) const) {
            return connection.makePreparedStatement<ResultBindings<std::remove_cv_t<std::remove_reference_t<Args>>...>, storeResult, validateMode, warnMode, ignoreNullable>(query);
        }

        /**
         * @brief Makes prepared statement by deducing ResultBindings template arguments from function's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
         * @tparam validateMode Indicates validate mode level
         * @tparam warnMode Indicates warning mode level
         * @tparam ignoreNullable Disables null type checking
         */
        template <bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode, bool ignoreNullable, typename CallableClass, typename... Args>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, void (CallableClass::*)(Args...)) {
            return connection.makePreparedStatement<ResultBindings<std::remove_cv_t<std::remove_reference_t<Args>>...>, storeResult, validateMode, warnMode, ignoreNullable>(query);
        }

        /**
         * @brief Makes prepared statement by deducing ResultBindings template arguments from lambda's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
         * @tparam validateMode Indicates validate mode level
         * @tparam warnMode Indicates warning mode level
         * @tparam ignoreNullable Disables null type checking
         */
        template <bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode, bool ignoreNullable, typename CallableClass>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, CallableClass&&) {
            // This is actually only callable argument type deductor. We don't care about callable type, so we can decay Callable object
            return generatePreparedStatementImpl<storeResult, validateMode, warnMode, ignoreNullable>(connection, query, &std::decay_t<CallableClass>::operator());
        }

        /**
         * @brief Makes prepared statement by deducing ResultBindings template arguments from plain old C function's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
         * @tparam validateMode Indicates validate mode level
         * @tparam warnMode Indicates warning mode level
         * @tparam ignoreNullable Disables null type checking
         */
        template <bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode, bool ignoreNullable, typename... Args>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, void (*)(Args...)) {
            return connection.makePreparedStatement<ResultBindings<std::remove_cv_t<std::remove_reference_t<Args>>...>, storeResult, validateMode, warnMode, ignoreNullable>(query);
        }
    }

    /**
     * @brief Executes query, reads input data and passes them row by row to callback function
     * @param ps Prepared statement object (only static statements are currently supported)
     * @param processingFunction function to be invoked on every row
     *                           Its parameters must correspond with result columns (their types and count)
     */
    template<typename PreparedStatementType, typename Callable, typename = typename std::enable_if<std::is_base_of<detail::StatementBase, std::remove_reference_t<PreparedStatementType>>::value>::type>
    void psReadQuery(PreparedStatementType &&ps, Callable &&processingFunction)
    {
        ps.execute();
        while (ps.fetch())
        {
            invokeViaTuple(processingFunction, ps.getResult());
        }
    }

    /**
     * @brief Builds prepared statement from query string and invokes psReadQuery(ps, processingFunction)
     * @param query Query to be executed (in most cases there will be selections)
     * @param connection Connection handle into database
     * @param processingFunction function to be invoked on every row
     *                           Its parameters must correspond with result columns (their types and count)
     * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
     * @tparam validateMode Indicates validate mode level
     * @tparam warnMode Indicates warning mode level
     * @tparam ignoreNullable Disables null type checking
     */
    template<bool storeResult=detail::PreparedStatementsDefault::getStoreResult(),
             ValidateMetadataMode validateMode=detail::PreparedStatementsDefault::getValidateMode(),
             ValidateMetadataMode warnMode=detail::PreparedStatementsDefault::getWarnMode(),
             bool ignoreNullable=detail::PreparedStatementsDefault::getIgnoreNullable(),
             typename Callable,
             typename ConnType>
    void psReadQuery(const std::string &query, ConnType &&connection, Callable &&processingFunction)
    {
        auto ps = detail::generatePreparedStatementImpl<storeResult, validateMode, warnMode, ignoreNullable>(connection, query, processingFunction);
        psReadQuery(ps, processingFunction);
    }

    /**
     * @brief Reads one and only one row from your own prepared statement into variables
     * @param ps Prepared statement object (only static statements are currently supported)
     * @param values References to variables to be loaded from query
     *               Their type must be compatible with query result types
     * @throws UnexpectedRowCountError if number of results is not exactly 1
     */
    template<typename PreparedStatementType, typename... Args, typename = typename std::enable_if<std::is_base_of<detail::StatementBase, std::remove_reference_t<PreparedStatementType>>::value>::type>
    void psReadValues(PreparedStatementType &&ps, Args&... values)
    {
        ps.execute();

        if (ps.getRowsCount() != 1 || !ps.fetch())
        {
            throw UnexpectedRowCountError("psReadValues() expected exactly one row, got " + std::to_string(ps.getRowsCount()) + " rows", ps.getRowsCount());
        }

        std::tuple<Args&...>(values...) = ps.getResult();
    }

    /**
     * @brief Reads one and only one row from prepared statement (constructed from `query` string) into variables
     * @param query Query to be executed (Ensure you are returning only 1 row)
     * @param connection Connection handle into database
     * @param values Reference to variables to be loaded from query
     *               Their type must be compatible with query result types
     * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
     * @tparam validateMode Indicates validate mode level
     * @tparam warnMode Indicates warning mode level
     * @tparam ignoreNullable Disables null type checking
     */
    template<bool storeResult=detail::PreparedStatementsDefault::getStoreResult(),
             ValidateMetadataMode validateMode=detail::PreparedStatementsDefault::getValidateMode(),
             ValidateMetadataMode warnMode=detail::PreparedStatementsDefault::getWarnMode(),
             bool ignoreNullable=detail::PreparedStatementsDefault::getIgnoreNullable(),
             typename... Args,
             typename ConnType>
    void psReadValues(const std::string &query, ConnType &&connection, Args&... values)
    {
        psReadValues(connection.template makePreparedStatement<ResultBindings<Args...>, storeResult, validateMode, warnMode, ignoreNullable>(query), values...);
    }

}

