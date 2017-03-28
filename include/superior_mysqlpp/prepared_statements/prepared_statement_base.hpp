/*
 * Author: Tomas Nozicka
 */

#pragma once

#include <vector>
#include <sstream>

#include <superior_mysqlpp/logging.hpp>
#include <superior_mysqlpp/types/optional.hpp>
#include <superior_mysqlpp/low_level/dbdriver.hpp>
#include <superior_mysqlpp/metadata.hpp>
#include <superior_mysqlpp/prepared_statements/validate_metadata_modes.hpp>
#include <superior_mysqlpp/prepared_statements/get_binding_type.hpp>
#include <superior_mysqlpp/types/nullable.hpp>

namespace SuperiorMySqlpp
{
    namespace detail
    {
        class StatementBase
        {
        protected:
            LowLevel::DBDriver::Statement statement;
            Loggers::ConstPointer_t loggerPtr;

        public:
            StatementBase(LowLevel::DBDriver::Statement&& statement, Loggers::ConstPointer_t loggerPtr=DefaultLogger::getLoggerPtr().get())
                : statement{std::move(statement)},
                  loggerPtr{loggerPtr}
            {
            }

            StatementBase(StatementBase&&) = default;
            StatementBase& operator=(StatementBase&&) = default;

            StatementBase(const StatementBase&) = delete;
            StatementBase& operator=(const StatementBase&) = delete;
        };

        template<bool storeResult=true>
        class StoreOrUseResultBase : public StatementBase
        {
        public:
            using StatementBase::StatementBase;


            auto getRowsCount()
            {
                return statement.getRowsCount();
            }

            template<typename T>
            void seekRow(T index)
            {
                statement.seekRow(index);
            }

            void seekRowOffset(MYSQL_ROW_OFFSET offset)
            {
                statement.seekRowOffset(offset);
            }

            auto tellRowOffset()
            {
                return statement.tellRowOffset();
            }

        protected:
            void storeOrUse()
            {
                this->statement.storeResult();
            }
        };

        template<>
        class StoreOrUseResultBase<false> : public StatementBase
        {
        public:
            using StatementBase::StatementBase;


            void storeOrUse() const
            {
            }

            auto getFetchedRowsCount()
            {
                return statement.getRowsCount();
            }
        };


        /**
         * Base class for prepared statements.
         * Contains mostly low level methods communicating directly with underlying mysql C library.
         * Also takes care for prepared statement's argument validation according to PreparedStatement settings.
         *
         * Settings itself are templated, so for different settings, different classes will be created.
         *
         * @tparam storeResult    flag - when true, mysql store result is used
         * @tparam validateMode   enum - says how params or results will be validated (more in {@link ValidateMetadataMode}).
         *                        When params or results doesn't comply with rules set by this option, exception will
         *                        be thrown.
         * @tparam warnMode       enum - says when user will be warned. These is same enum as {@param validateMode}.
         *                        When params or results doesn't comply with rules set by this option a debug message
         *                        will be issued with description, which rule was broken.
         * @tparam ignoreNullable flag - says if {@link #validateResultMetadata()) ignores null value.
         *                        (Feasible only for prepare statement results.)
         *                        In mysql every data type can be also set to null. In C++ this is not the case.
         *                        At least not for primitive types. Usually when we read data from mysql, we also need
         *                        to set some "null" flag. In this library we can easily do it by mapping mysql data
         *                        type to Nullable<T> type, which is simple wrapper (for any data type), that can store
         *                        this "null flag". During result validation, this is also one of the checks - if we are
         *                        able to store this "null flag". Sometimes however we just want to ignore null flag
         *                        (binded result memory then will be not set, where null was sent), that is what this flag
         *                        is for.
         */
        template<bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode, bool ignoreNullable>
        class PreparedStatementBase : public StoreOrUseResultBase<storeResult>//, public ValidateResult<validateMetadataMode>
        {
        protected:
            /**
             * Collection of Nullables which needs to be engaged after
             * successful fetch. Must be filled by inherited class.
             */
            std::vector<detail::NullableBase*> nullableBindings;

        public:
            using size_t = unsigned long long;

        public:
            using StoreOrUseResultBase<storeResult>::StoreOrUseResultBase;

        private:
            Optional<ResultMetadata> resultMetadata;

            /**
             * Why we need this method? When Nullable type is created and
             * initialised using operator* in dynamic prepared statement
             * its internal flag engaged is NOT set to true. This leads to
             * buggy behavior of isValid method in Nullable class.
             *
             * We need to call this in every fetch because only
             * nullables which are not null are engaged.
             */
            void engageNullables()
            {
                for(auto* nullable : nullableBindings) {
                    if (!nullable->isNull()) {
                        nullable->engage();
                    }
                }
            }

        public:

            const ResultMetadata& getResultMetadata()
            {
                if (!resultMetadata)
                {
                    resultMetadata.emplace(std::move(this->statement.resultMetadata()));
                }
                return *resultMetadata;
            }

            ResultMetadata& getModifiableResultMetadata()
            {
                return const_cast<ResultMetadata&>(getResultMetadata());
            }

            auto fetchWithStatus()
            {
                auto status = this->statement.fetchWithStatus();

                if (status == LowLevel::DBDriver::Statement::FetchStatus::Ok) {
                    engageNullables();
                }

                return status;
            }

            auto fetch()
            {
                auto ok = this->statement.fetch();

                if (ok) {
                    engageNullables();
                }

                return ok;
            }

            /**
             * TODO: Statement.fetchColumn is void and can throw
             * -> do we need bool as a return value here?
             * @return
             */
            bool fetchColumn()
            {
                this->statement.fetchColumn();
                engageNullables();
                return true;
            }

            auto sendLongData(unsigned int paramNumber, const char* data, unsigned long length)
            {
                return this->statement.sendLongData(paramNumber, data, length);
            }

            auto sendLongData(unsigned int paramNumber, const std::string& data)
            {
                return this->statement.sendLongData(paramNumber, data);
            }

            void setPrefetchRowCount(unsigned long count)
            {
                this->statement.setAttribute(LowLevel::DBDriver::Statement::Attributes::prefetchRows, count);
            }

            unsigned long getPrefetchRowCount()
            {
                unsigned long result;
                this->statement.getAttribute(LowLevel::DBDriver::Statement::Attributes::prefetchRows, result);
                return result;
            }

            void setUpdateMaxLengthOnStore(my_bool value)
            {
                this->statement.setAttribute(LowLevel::DBDriver::Statement::Attributes::updateMaxLength, value);
            }

            bool getUpdateMaxLengthOnStore()
            {
                my_bool result = 0;
                this->statement.getAttribute(LowLevel::DBDriver::Statement::Attributes::updateMaxLength, result);
                return result;
            }


        public:
            template<typename ResultBindings>
            void validateResultMetadata(const ResultBindings& resultBindings)
            {
                using namespace std::string_literals;
                using std::to_string;
                using std::begin;
                using std::end;

                if (validateMode==ValidateMetadataMode::Disabled && warnMode==ValidateMetadataMode::Disabled)
                {
                    return;
                }

                auto&& metadata = this->getResultMetadata();

                std::size_t index = 0;
                auto bindingsIt=begin(resultBindings);
                auto bindingsEndIt=end(resultBindings);
                auto metadataIt=begin(metadata);
                auto metadataEndIt=end(metadata);
                while (bindingsIt!=bindingsEndIt && metadataIt!=metadataEndIt)
                {
                    auto&& binding = *bindingsIt;
                    auto&& resultMetadata = *metadataIt;

                    auto&& bindingType = toFieldType(binding.buffer_type);
                    bool bindingIsUnsigned = binding.is_unsigned;
                    auto&& resultType = resultMetadata.getFieldType();
                    bool resultIsUnsigned = resultMetadata.isUnsigned();

                    if (bindingType == FieldTypes::Null)
                    {
                        continue;
                    }


                    if (!isCompatible<validateMode>(resultType, resultIsUnsigned, bindingType, bindingIsUnsigned))
                    {
                        throw PreparedStatementTypeError{"Result types at index " + std::to_string(index) + " don't match!\n"
                                                         "In validate mode " + getValidateMetadataModeName(validateMode) + ":\n"
                                                         "expected type =" + detail::getBindingTypeFullName(bindingType, bindingIsUnsigned) +
                                                         "= is not compatible with "
                                                         "result type =" + detail::getBindingTypeFullName(resultType, resultIsUnsigned) + "="};
                    }

                    if (!isCompatible<warnMode>(resultType, resultIsUnsigned, bindingType, bindingIsUnsigned))
                    {
                        auto ptr = this->loggerPtr;
                        ptr->logMySqlStmtMetadataWarning(
                            this->statement.getDriverId(),
                            this->statement.getId(),
                            index,
                            warnMode,
                            bindingType,
                            bindingIsUnsigned,
                            resultType,
                            resultIsUnsigned
                        );
                    }

                    // We usually don't want to read nullable value into non-nullable one
                    // However it is possible, because mysql doesn't need to write this flag, to user provided place
                    // (see in mysql_stmt_bind_result(...) definition in file "libmysql.c".
                    // In this case however the result value is not set. We only ignore possible null value and we do
                    // it only when we explicitly want to.
                    if (resultMetadata.isNullable() && binding.is_null == nullptr && !ignoreNullable)
                    {
                        if (validateMode != ValidateMetadataMode::Disabled)
                        {
                            throw PreparedStatementTypeError{"Result types at index " + std::to_string(index) + " don't match!\n"
                                                             "You can't read nullable value into non-nullable one!!!"};
                        }
                        else
                        {
                            this->loggerPtr->logMySqlStmtMetadataNullableToNonNullableWarning(this->statement.getDriverId(), this->statement.getId(), index);
                        }
                    }

                    // next step
                    ++bindingsIt;
                    ++metadataIt;
                    ++index;
                }

                auto throwError = [](auto bindingsSize, auto metadataSize, auto&& excessiveTypes){
                    std::stringstream message{};
                    message << "Result bindings count (" << bindingsSize
                            << ") doesn't match number of returned fields (" << metadataSize <<  ")!"
                            << "Excessive types: ";
                    for (auto&& type: excessiveTypes)
                    {
                        message << detail::getBindingTypeFullName(std::get<0>(type), std::get<1>(type)) << ", ";
                    }
                    throw PreparedStatementTypeError(message.str());
                };

                if (bindingsIt != bindingsEndIt)
                {
                    auto distance = std::distance(bindingsIt, bindingsEndIt);
                    auto bindingsSize = index + distance;
                    auto metadataSize = index;
                    std::vector<std::tuple<FieldTypes, bool>> excessiveTypes{};
                    excessiveTypes.reserve(distance);
                    do
                    {
                        excessiveTypes.emplace_back(toFieldType(bindingsIt->buffer_type), bindingsIt->is_unsigned);
                        ++bindingsIt;
                    } while (bindingsIt!=bindingsEndIt);
                    throwError(bindingsSize, metadataSize, excessiveTypes);
                }
                else if (metadataIt != metadataEndIt)
                {
                    auto distance = std::distance(metadataIt, metadataEndIt);
                    auto bindingsSize = index;
                    auto metadataSize = index + distance;
                    std::vector<std::tuple<FieldTypes, bool>> excessiveTypes{};
                    excessiveTypes.reserve(distance);
                    do
                    {
                        excessiveTypes.emplace_back(metadataIt->getFieldType(), metadataIt->isUnsigned());
                        ++metadataIt;
                    } while (metadataIt != metadataEndIt);
                    throwError(bindingsSize, metadataSize, excessiveTypes);
                }
            }

        public:
            LowLevel::DBDriver::Statement& detail_getStatementRef() && = delete;
            LowLevel::DBDriver::Statement& detail_getStatementRef() &
            {
                return this->statement;
            }
        };
    }
}


