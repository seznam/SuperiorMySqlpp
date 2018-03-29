#pragma once

#include <superior_mysqlpp/connection.hpp>
#include <superior_mysqlpp/exceptions.hpp>
#include <superior_mysqlpp/prepared_statement.hpp>
#include <superior_mysqlpp/utils.hpp>
#include <type_traits>

namespace SuperiorMySqlpp
{
    namespace
    {
        /**
         * @brief Makes prepared statement from function's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         */
        template <typename CallableClass, typename... Args>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, void (CallableClass::*)(Args...) const volatile) {
            return connection.makePreparedStatement<ResultBindings<std::remove_cv_t<std::remove_reference_t<Args>>...>>(query);
        }

        /**
         * @brief Makes prepared statement from function's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         */
        template <typename CallableClass, typename... Args>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, void (CallableClass::*)(Args...) volatile) {
            return connection.makePreparedStatement<ResultBindings<std::remove_cv_t<std::remove_reference_t<Args>>...>>(query);
        }

        /**
         * @brief Makes prepared statement from function's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         */
        template <typename CallableClass, typename... Args>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, void (CallableClass::*)(Args...) const) {
            return connection.makePreparedStatement<ResultBindings<std::remove_cv_t<std::remove_reference_t<Args>>...>>(query);
        }

        /**
         * @brief Makes prepared statement from function's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         */
        template <typename CallableClass, typename... Args>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, void (CallableClass::*)(Args...)) {
            return connection.makePreparedStatement<ResultBindings<std::remove_cv_t<std::remove_reference_t<Args>>...>>(query);
        }

        /**
         * @brief Makes prepared statement from lambda's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         */
        template <typename CallableClass>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, CallableClass&&) {
            // This is actually only callable argument type deductor. We don't care about callable type, so we can decay Callable object
            return generatePreparedStatementImpl(connection, query, &std::decay_t<CallableClass>::operator());
        }

        /**
         * @brief Makes prepared statement from plain old C function's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         */
        template <typename... Args>
        auto generatePreparedStatementImpl(Connection &connection, const std::string &query, void (*)(Args...)) {
            return connection.makePreparedStatement<ResultBindings<std::remove_cv_t<std::remove_reference_t<Args>>...>>(query);
        }
    }

    /**
     * @brief Executes query, reads input data and passes them to callback function
     * @param ps Prepared statement object
     * @param processingFunction function to be invoked on every row
     */
    template<typename PreparedStatementType, typename Callable>
    void psReadQuery(PreparedStatementType &&ps, Callable &&processingFunction)
    {
        ps.execute();
        while(ps.fetch())
        {
            invokeViaTuple(processingFunction, ps.getResult());
        }
    }

    /**
     * @brief Builds prepared statement from string and invokes psReadQuery(ps, processingFunction)
     * @param query Query to be executed (in most cases there will be selections)
     * @param connection Connection handle into database
     * @param processingFunction function to be invoked on every row
     */
    template<typename Callable>
    void psReadQuery(const std::string &query, Connection &connection, Callable &&processingFunction)
    {
        auto ps = generatePreparedStatementImpl(connection, query, processingFunction);
        psReadQuery(ps, processingFunction);
    }

    /**
     * @brief Reads one and only one row from query (from your own instance) into variables
     * @param ps Prepared statement object
     * @param values References to variables to be loaded from query
     * @throws std::runtime_error if more than one row is being loaded
     */
    template<typename PreparedStatementType, typename... Args, typename = typename std::enable_if<std::is_base_of<detail::StatementBase, PreparedStatementType>::value>::type>
    void psReadValues(PreparedStatementType &&ps, Args&... values)
    {
        ps.execute();

        if(ps.getRowsCount() != 1 || !ps.fetch())
        {
            throw UnexpectedMultipleRowsError("psReadValues() can read only one row per query!");
        }

        std::tuple<Args&...>(values...) = ps.getResult();
    }

    /**
     * @brief Reads one and only one row from query (constructed from `query` string) into variables
     * @param query Query to be executed (Ensure you are returning only 1 row)
     * @param connection Connection handle into database
     * @param values Reference to variables to be loaded from query
     */
    template<typename... Args>
    void psReadValues(const std::string &query, Connection &connection, Args&... values)
    {
        psReadValues(connection.makePreparedStatement<ResultBindings<Args...>>(query), values...);
    }

}

