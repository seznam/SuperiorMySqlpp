/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <superior_mysqlpp/prepared_statement_fwd.hpp>

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>
#include <cstring>
#include <cinttypes>
#include <memory>

#include <superior_mysqlpp/prepared_statements/initialize_bindings.hpp>
#include <superior_mysqlpp/prepared_statements/prepared_statement_base.hpp>
#include <superior_mysqlpp/prepared_statements/default_initialize_result.hpp>
#include <superior_mysqlpp/low_level/dbdriver.hpp>
#include <superior_mysqlpp/connection_def.hpp>
#include <superior_mysqlpp/metadata.hpp>
#include <superior_mysqlpp/traits.hpp>
#include <superior_mysqlpp/types/tags.hpp>



namespace SuperiorMySqlpp
{
    template<bool IsParamBinding, typename... Types>
    class Bindings
    {
    public:
        static constexpr auto kArgumentsCount = sizeof...(Types);
        static constexpr bool isParamBinding = IsParamBinding;

        std::tuple<Types...> data;
        std::array<MYSQL_BIND, sizeof...(Types)> bindings;


    public:
        template<typename... Args>
        Bindings(Args&&... args)
            : data{std::forward<Args>(args)...}
        {
            static_assert(sizeof...(Args) == 0 || sizeof...(Args) == sizeof...(Types), "You shall initialize all bindings data or none of them!");
            if (bindings.size() > 0)
            {
                std::memset(bindings.data(), 0, sizeof(bindings));
            }

            update();
        }

        Bindings(const Bindings&) = default;
        Bindings(Bindings&&) = default;
        Bindings& operator=(const Bindings&) = default;
        Bindings& operator=(Bindings&&) = default;
        ~Bindings() = default;

        void update()
        {
            detail::initializeBindings<IsParamBinding, Types...>(bindings, data);
        }
    };

    template<typename... Types>
    using ParamBindings = Bindings<true, Types...>;

    template<typename... Types>
    using ResultBindings = Bindings<false, Types...>;


    template<typename ResultBindings, typename ParamBindings, bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode, bool ignoreNullable>
    class PreparedStatement : public detail::PreparedStatementBase<storeResult, validateMode, warnMode, ignoreNullable>
    {
    private:
        ResultBindings resultBindings;
        static_assert(!ResultBindings::isParamBinding, "Result bindings are not of type ResultBindings<Args...>!");

        ParamBindings paramsBindings;
        static_assert(ParamBindings::isParamBinding, "Param bindings are not of type ParamBindings<Args...>!");

        bool hasValidatedResultMetadata = false;

    private:
        template<typename ResultArgsTuple, typename ParamArgsTuple, std::size_t... RI, std::size_t... PI>
        PreparedStatement(LowLevel::DBDriver& driver,
                          const std::string& query,
                          FullInitTag,
                          ResultArgsTuple&& resultArgsTuple,
                          std::index_sequence<RI...>,
                          ParamArgsTuple&& paramArgsTuple,
                          std::index_sequence<PI...>)
            : detail::PreparedStatementBase<storeResult, validateMode, warnMode, ignoreNullable>{driver.makeStatement()},
              resultBindings{std::get<RI>(std::forward<ResultArgsTuple>(resultArgsTuple))...},
              paramsBindings{std::get<PI>(std::forward<ParamArgsTuple>(paramArgsTuple))...}
        {
            this->statement.prepare(query);

            auto paramCount = this->statement.paramCount();
            if (paramCount != paramsBindings.kArgumentsCount)
            {
                throw PreparedStatementTypeError{"Params count (" + std::to_string(paramCount) +
                        ") in query doesn't match number of arguments (" + std::to_string(paramsBindings.kArgumentsCount) + ")!"};
            }

            if (decltype(paramsBindings)::kArgumentsCount > 0)
            {
                this->statement.bindParam(paramsBindings.bindings.data());
            }
        }

    public:
        template<typename... RArgs, typename... PArgs, template<typename...> class ResultArgsTuple, template<typename...> class ParamArgsTuple>
        PreparedStatement(LowLevel::DBDriver& driver,
                          const std::string& query,
                          FullInitTag tag,
                          ResultArgsTuple<RArgs...>&& resultArgsTuple,
                          ParamArgsTuple<PArgs...>&& paramArgsTuple)
            : PreparedStatement
              {
                  driver, query, tag,
                  std::forward<ResultArgsTuple<RArgs...>>(resultArgsTuple),
                  std::make_index_sequence<sizeof...(RArgs)>{},
                  std::forward<ParamArgsTuple<PArgs...>>(paramArgsTuple),
                  std::make_index_sequence<sizeof...(PArgs)>{}
              }
        {
        }

        template<typename... Args>
        PreparedStatement(LowLevel::DBDriver& driver, const std::string& query, Args&&... params)
            : PreparedStatement
              {
                  driver, query, fullInitTag,
                  std::forward_as_tuple(),
                  std::forward_as_tuple(std::forward<Args>(params)...)
              }
        {
            InitializeResult(resultBindings.data);
        }

        template<typename ResultArgsTuple, typename ParamArgsTuple>
        PreparedStatement(Connection& connection,
                          const std::string& query,
                          FullInitTag tag,
                          ResultArgsTuple&& resultArgsTuple,
                          ParamArgsTuple&& paramArgsTuple)
            : PreparedStatement
              {
                  connection.detail_getDriver(), query, tag,
                  std::forward<ResultArgsTuple>(resultArgsTuple),
                  std::forward<ParamArgsTuple>(paramArgsTuple)
              }
        {
        }

        template<typename... Args>
        PreparedStatement(Connection& connection, const std::string& query, Args&&... params)
            : PreparedStatement
              {
                  connection, query, fullInitTag,
                  std::forward_as_tuple(),
                  std::forward_as_tuple(std::forward<Args>(params)...)
              }
        {
            InitializeResult(resultBindings.data);
        }

        PreparedStatement(const PreparedStatement&) = default;
        PreparedStatement(PreparedStatement&&) = default;
        PreparedStatement& operator=(const PreparedStatement&) = default;
        PreparedStatement& operator=(PreparedStatement&&) = default;

        ~PreparedStatement() = default;

        /*
         * Call this function before execute only when some parameter size or memory address has changed.
         */
        void updateParamsBindings()
        {
            if (decltype(paramsBindings)::kArgumentsCount > 0)
            {
                paramsBindings.update();
                this->statement.bindParam(paramsBindings.bindings.data());
            }
        }

        auto& getParams() noexcept
        {
            return paramsBindings.data;
        }

        const auto& getParams() const noexcept
        {
            return paramsBindings.data;
        }

        auto& getResult() noexcept
        {
            return resultBindings.data;
        }

        const auto& getResult() const noexcept
        {
            return resultBindings.data;
        }

        template<typename... Args>
        void setParams(Args&&... args)
        {
            paramsBindings.data = std::forward_as_tuple(std::forward<Args>(args)...);
        }

        static constexpr auto getParamsCount() noexcept
        {
            return decltype(paramsBindings)::kArgumentsCount;
        }

        static constexpr auto getResultCount() noexcept
        {
            return decltype(resultBindings)::kArgumentsCount;
        }

        void execute()
        {
            this->statement.execute();

            auto fieldCount = this->statement.fieldCount();
            if (fieldCount != resultBindings.kArgumentsCount)
            {
                throw PreparedStatementTypeError{"Result types count (" + std::to_string(resultBindings.kArgumentsCount) +
                        ") in query doesn't match number of returned fields (" + std::to_string(fieldCount) + ")!"};
            }

            if (decltype(resultBindings)::kArgumentsCount > 0)
            {
                if (!hasValidatedResultMetadata)
                {
                    this->validateResultMetadata(resultBindings.bindings);
                    this->hasValidatedResultMetadata = true;
                }

                // TODO: check if this might be cached when calling execute multiple times
                this->statement.bindResult(resultBindings.bindings.data());
            }

            this->storeOrUse();
        }
    };
}

