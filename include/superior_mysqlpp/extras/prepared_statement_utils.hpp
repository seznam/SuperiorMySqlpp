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
         * Converts tuples of arguments into prepared statement via `generate()` method
         * @tparam ResultArgs tuple of arguments from result function
         * @tparam ParamsArgs tuple of arguments from params function
         */
        template<typename ResultArgs, typename ParamsArgs>
        struct ToPreparedStatement;

        template<template<typename...> class ResultTuple, typename... ResultArgs, template<typename...> class ParamsTuple, typename... ParamsArgs>
        struct ToPreparedStatement<ResultTuple<ResultArgs...>, ParamsTuple<ParamsArgs...>>
        {
            /**
             * Constructs prepared statement
             * @param connection Connection handle into database
             * @param query Query for prepared statement
             * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
             * @tparam validateMode Indicates validate mode level
             * @tparam warnMode Indicates warning mode level
             * @tparam ignoreNullable Disables null type checking
             */
            template <bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode, bool ignoreNullable>
            static inline auto generate(Connection &connection, const std::string &query)
            {
                return PreparedStatement<ResultBindings<ResultArgs...>, ParamBindings<ParamsArgs...>, storeResult, validateMode, warnMode, ignoreNullable>
                    { connection, query };
            }
        };

        /**
         * @brief Makes prepared statement by deducing ResultBindings and ParamBindings template arguments from functions's signature
         * @param connection Connection handle into database
         * @param query Query for prepared statement
         * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
         * @tparam validateMode Indicates validate mode level
         * @tparam warnMode Indicates warning mode level
         * @tparam ignoreNullable Disables null type checking
         */
        template <bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode, bool ignoreNullable, typename ResultCallable, typename ParamCallable>
        auto generatePreparedStatement(Connection &connection, const std::string &query, ResultCallable &&, ParamCallable &&)
        {
            static_assert(FunctionInfo<ParamCallable>::arguments_are_lvalue_references::value, "Arguments must be lvalue references");
            return ToPreparedStatement<
                typename FunctionInfo<ResultCallable>::raw_arguments,
                typename FunctionInfo<ParamCallable>::raw_arguments
            >::template generate<storeResult, validateMode, warnMode, ignoreNullable>(connection, query);
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
        auto ps = detail::generatePreparedStatement<storeResult, validateMode, warnMode, ignoreNullable>(connection, query, processingFunction, [](){});
        psReadQuery(ps, processingFunction);
    }

    /**
     * @brief Builds prepared statement from query string and constructs prepared stament with updatable arguments
     * @param query Query to be executed (in most cases there will be selections)
     * @param connection Connection handle into database
     * @param paramsSetter function setting statement's parameters, returning bool if input data are available
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
             typename ResultCallable,
             typename ParamCallable,
             typename ConnType>
    void psQuery(const std::string &query, ConnType &&connection, ParamCallable &&paramsSetter, ResultCallable &&processingFunction)
    {
        auto ps = detail::generatePreparedStatement<storeResult, validateMode, warnMode, ignoreNullable>(connection, query, processingFunction, paramsSetter);

        while (invokeViaTuple(paramsSetter, ps.getParams()))
        {
            ps.updateParamsBindings();
            ps.execute();

            while (ps.fetch())
            {
                invokeViaTuple(processingFunction, ps.getResult());
            }
        }
    }

    /**
     * @brief Builds prepared statement from query string and constructs prepared stament with updatable arguments
     * @param query Query to be executed (in most cases there will be selections)
     * @param connection Connection handle into database
     * @param paramsSetter function setting statement's parameters, returning bool if input data are available
     * @tparam storeResult Boolean indicating if results will be in `store` or `use` mode
     * @tparam validateMode Indicates validate mode level
     * @tparam warnMode Indicates warning mode level
     * @tparam ignoreNullable Disables null type checking
     */
    template<bool storeResult=detail::PreparedStatementsDefault::getStoreResult(),
             ValidateMetadataMode validateMode=detail::PreparedStatementsDefault::getValidateMode(),
             ValidateMetadataMode warnMode=detail::PreparedStatementsDefault::getWarnMode(),
             bool ignoreNullable=detail::PreparedStatementsDefault::getIgnoreNullable(),
             typename ParamCallable,
             typename ConnType>
    void psQuery(const std::string &query, ConnType &&connection, ParamCallable &&paramsSetter)
    {
        auto ps = detail::generatePreparedStatement<storeResult, validateMode, warnMode, ignoreNullable>(connection, query, [](){}, paramsSetter);

        while (invokeViaTuple(paramsSetter, ps.getParams()))
        {
            ps.updateParamsBindings();
            ps.execute();
        }
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

