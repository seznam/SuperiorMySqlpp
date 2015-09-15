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


        template<bool storeResult, ValidateMetadataMode validateMode, ValidateMetadataMode warnMode>
        class PreparedStatementBase : public StoreOrUseResultBase<storeResult>//, public ValidateResult<validateMetadataMode>
        {
        public:
            using size_t = unsigned long long;

        public:
            using StoreOrUseResultBase<storeResult>::StoreOrUseResultBase;

        private:
            Optional<ResultMetadata> resultMetadata;

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
                return this->statement.fetchWithStatus();
            }

            auto fetch()
            {
                return this->statement.fetch();
            }

            bool fetchColumn()
            {
                return this->statement.fetchColumn();
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

                    // We cannot read nullable value into non-nullable one
                    if (resultMetadata.isNullable() && binding.is_null == nullptr)
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


