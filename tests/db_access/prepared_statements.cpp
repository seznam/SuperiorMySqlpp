/*
 *  Author: Tomas Nozicka
 */

#include <iostream>
#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>
#include <superior_mysqlpp/extras/prepared_statement_utils.hpp>

#include "settings.hpp"


using namespace bandit;
using namespace SuperiorMySqlpp;
using namespace std::string_literals;



template<ValidateMetadataMode validateMode, typename T, bool debug=false>
bool testColumnType(Connection& connection, const std::string& column)
{
    auto preparedStatement = connection.makePreparedStatement<ResultBindings<T>, true, validateMode, ValidateMetadataMode::Disabled>(
            "SELECT `" + column + "` FROM `test_superior_sqlpp`.`xuser_datatypes`");
    try
    {
        preparedStatement.execute();
    }
    catch (PreparedStatementTypeError& e)
    {
        if (debug)
        {
            std::cerr << e.what() << std::endl;
        }
        return false;
    }
    return true;
}

template<ValidateMetadataMode validateMode, typename T, bool debug=false>
bool testInt32(Connection& connection)
{
    return testColumnType<validateMode, T, debug>(connection, "id");
}

template<ValidateMetadataMode validateMode, typename T, bool debug=false>
bool testUInt32(Connection& connection)
{
    return testColumnType<validateMode, T, debug>(connection, "uid");
}


go_bandit([](){
    describe("Test stmt", [&](){
        auto& s = getSettingsRef();
        Connection connection{s.database, s.user, s.password, s.host, s.port};

        it("can create binding structures", [&](){
            ResultBindings<int, unsigned int, int, unsigned int> ba{};
            ResultBindings<> emptyBinding{};
            ParamBindings<unsigned int, long> bindingArray{4u, 7L};
        });

        it("can do simple statement", [&](){
            PreparedStatement<ResultBindings<>, ParamBindings<>> preparedStatement{
                connection.detail_getDriver(),
                "INSERT INTO `test_superior_sqlpp`.`xuser` (`id`, `name`) "
                "VALUES (11, 'Tomas')"
            };

            preparedStatement.execute();
        });

        it("can do statement only with arguments", [&](){
            PreparedStatement<ResultBindings<>, ParamBindings<int>> preparedStatement{
                connection.detail_getDriver(),
                "INSERT INTO `test_superior_sqlpp`.`xuser` (`id`, `name`) "
                "VALUES (?, 'Kokot')", 12
            };

            preparedStatement.execute();
        });

        it("can do statement only with arguments repeatedly", [&](){
            PreparedStatement<ResultBindings<>, ParamBindings<int>> preparedStatement{
                connection.detail_getDriver(),
                "INSERT INTO `test_superior_sqlpp`.`xuser` (`id`, `name`) "
                "VALUES (?, 'Tomas')"
            };

            for (int i=13; i!=17; ++i)
            {
                std::get<0>(preparedStatement.getParams()) = i;
                preparedStatement.execute();
            }
        });

        it("can do statement only with result", [&](){
            auto preparedStatement = connection.makePreparedStatement<ResultBindings<int>>(
                    "SELECT `id` FROM `test_superior_sqlpp`.`xuser`"
            );

            preparedStatement.execute();

            auto rowsCount = preparedStatement.getRowsCount();
            AssertThat(rowsCount, Equals(6u));


            for (int i=11; i<17; ++i)
            {
                AssertThat(preparedStatement.fetch(), Equals(true));

                int id;
                std::tie(id) = preparedStatement.getResult();

                AssertThat(id, Equals(i));
            }
        });

        it("can bind strings", [&](){
            auto preparedStatement = connection.makePreparedStatement<ResultBindings<int, Nullable<StringDataBase<42>>>>(
                "SELECT `id`, CAST(`name` as CHAR) FROM `test_superior_sqlpp`.`xuser` WHERE `id`=? AND `name`=? AND `name`=? AND `name`=? AND `name`=?",
                fullInitTag,
                std::forward_as_tuple(5, Nullable<StringDataBase<42>>{inPlace}),
                std::forward_as_tuple(12, StringData{"Kokot"}, std::string{"Kokot"}, std::string{"Kokot"}, static_cast<const char*>("Kokot"))
            );

            preparedStatement.execute();

            int count = 0;
            while (preparedStatement.fetch())
            {
                int id;
                Nullable<StringDataBase<42>> sname;
                std::tie(id, sname) = preparedStatement.getResult();

                std::string name{};
                if (sname)
                {
                    name = sname->getString();
                }
                else
                {
                    name = "NULL";
                }

                AssertThat(id, Equals(12));
                AssertThat(name, Equals("Kokot"));

                ++count;
            }
            AssertThat(count, Equals(1));
        });

        it("can work with Nullable", [&](){
            {
                auto preparedStatement = connection.makePreparedStatement(
                    "INSERT INTO `test_superior_sqlpp`.`user_nullable` VALUES (?, ?), (?, ?), (?, ?), (?, ?), (?, ?)",
                    42, Nullable<StringDataBase<42>>{}, 43, Nullable<StringDataBase<42>>{inPlace, "neco"},
                    52, Nullable<std::string>{inPlace, "aaaa"}, 53, std::string{"bbbbbbb"},
                    60, Nullable<std::string>{}
                );
                preparedStatement.execute();
            }

            auto preparedStatement = connection.makePreparedStatement<ResultBindings<int, Nullable<StringDataBase<42>>>>(
                "SELECT `id`, `nullable_name` FROM `test_superior_sqlpp`.`user_nullable` WHERE `id` IN (42, 43, 52, 53, 60) ORDER BY `id` ASC"
            );
            preparedStatement.execute();

            int id = 55;
            Nullable<StringDataBase<42>> nullable{inPlace, "---------------------------------"};

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, nullable) = preparedStatement.getResult();
            AssertThat(id, Equals(42));
            AssertThat(nullable.isValid(), IsFalse());

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, nullable) = preparedStatement.getResult();
            AssertThat(id, Equals(43));
            AssertThat(nullable.isValid(), IsTrue());
            AssertThat(*nullable, Equals("neco"));

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, nullable) = preparedStatement.getResult();
            AssertThat(id, Equals(52));
            AssertThat(nullable.isValid(), IsTrue());
            AssertThat(*nullable, Equals("aaaa"));

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, nullable) = preparedStatement.getResult();
            AssertThat(id, Equals(53));
            AssertThat(nullable.isValid(), IsTrue());
            AssertThat(*nullable, Equals("bbbbbbb"));

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, nullable) = preparedStatement.getResult();
            AssertThat(id, Equals(60));
            AssertThat(nullable.isValid(), IsFalse());
        });

        it("can rebind nullable arguments and read them back", [&](){
            {
                auto&& preparedStatement = connection.makePreparedStatement(
                    "INSERT INTO `test_superior_sqlpp`.`user_nullable` VALUES (?, ?)",
                    1, Nullable<std::string>{"Kokot"}
                );
                preparedStatement.execute();

                preparedStatement.setParams(2, Nullable<std::string>{});
                preparedStatement.updateParamsBindings();
                preparedStatement.execute();

                preparedStatement.setParams(3, Nullable<std::string>{"aaa"});
                preparedStatement.updateParamsBindings();
                preparedStatement.execute();
            }

            auto&& preparedStatement = connection.makePreparedStatement<ResultBindings<int, Nullable<StringData>>>(
                "SELECT `id`, `nullable_name` FROM `test_superior_sqlpp`.`user_nullable` WHERE `id` IN (1, 2, 3) ORDER BY `id` ASC"
            );
            preparedStatement.execute();

            int id;
            Nullable<StringData> sname{};

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, sname) = preparedStatement.getResult();
            AssertThat(id, Equals(1));
            AssertThat(sname->getString(), Equals("Kokot"));

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, sname) = preparedStatement.getResult();
            AssertThat(id, Equals(2));
            AssertThat(sname.isValid(), IsFalse());

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, sname) = preparedStatement.getResult();
            AssertThat(id, Equals(3));
            AssertThat(sname->getString(), Equals("aaa"));
        });

        it("can rebind string arguments and read them back", [&](){
            {
                auto&& preparedStatement = connection.makePreparedStatement(
                    "INSERT INTO `test_superior_sqlpp`.`xuser` (`id`, `name`) VALUES (?, ?)",
                    142, StringData{"Kokot"}
                );
                preparedStatement.execute();

                preparedStatement.setParams(143, StringData{"aaa"});
                preparedStatement.updateParamsBindings();
                preparedStatement.execute();

                preparedStatement.setParams(144, StringData{""});
                preparedStatement.updateParamsBindings();
                preparedStatement.execute();
            }

            auto&& preparedStatement = connection.makePreparedStatement<ResultBindings<int, Nullable<StringDataBase<42>>>>(
                "SELECT `id`, `name` FROM `test_superior_sqlpp`.`xuser` WHERE `id` IN (142, 143, 144)"
            );
            preparedStatement.execute();

            int id;
            Nullable<StringDataBase<42>> sname;
            std::string name{};

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, sname) = preparedStatement.getResult();
            AssertThat(id, Equals(142));
            AssertThat(sname->getString(), Equals("Kokot"));

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, sname) = preparedStatement.getResult();
            AssertThat(id, Equals(143));
            AssertThat(sname->getString(), Equals("aaa"));

            AssertThat(preparedStatement.fetch(), IsTrue());
            std::tie(id, sname) = preparedStatement.getResult();
            AssertThat(id, Equals(144));
            AssertThat(sname->getString(), Equals(""));
        });

        it("can get and set prefetch row count", [&](){
            auto preparedStatement = connection.makePreparedStatement<ResultBindings<int, Nullable<StringData>>, false>(
                "SELECT `id`, CAST(`name` as CHAR) FROM `test_superior_sqlpp`.`xuser` WHERE `id`=? AND `name`=? AND `name`=? AND `name`=? AND `name`=?",
                12, StringData("Kokot"), std::string("Kokot"), std::string("Kokot"), static_cast<const char*>("Kokot")
            );

            unsigned long prefetchCount = preparedStatement.getPrefetchRowCount();
            AssertThat(prefetchCount, Equals(1u));

            preparedStatement.setPrefetchRowCount(100);

            prefetchCount = preparedStatement.getPrefetchRowCount();
            AssertThat(prefetchCount, Equals(100u));

            preparedStatement.execute();

            int count = 0;
            while (preparedStatement.fetch())
            {
                int id;
                Nullable<StringData> sname;
                std::tie(id, sname) = preparedStatement.getResult();

                std::string name{};
                if (sname)
                {
                    name = sname->getString();
                }
                else
                {
                    name = "NULL";
                }

                AssertThat(id, Equals(12));
                AssertThat(name, Equals("Kokot"));

                ++count;
            }
            AssertThat(count, Equals(1));
        });

        it("can validate metadata", [&](){
            AssertThat((testInt32<ValidateMetadataMode::Strict, signed char>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::Strict, short int>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::Strict, int>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::Strict, long>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::Strict, long long>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::Strict, unsigned char>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::Strict, unsigned short int>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::Strict, unsigned int>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::Strict, unsigned long>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::Strict, unsigned long long>(connection)), IsFalse());


            AssertThat((testInt32<ValidateMetadataMode::ArithmeticPromotions, signed char>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticPromotions, short int>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticPromotions, int>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticPromotions, long>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticPromotions, long long>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticPromotions, unsigned char>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticPromotions, unsigned short int>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticPromotions, unsigned int>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticPromotions, unsigned long>(connection)), IsFalse());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticPromotions, unsigned long long>(connection)), IsFalse());

            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticPromotions, signed char>(connection)), IsFalse());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticPromotions, short int>(connection)), IsFalse());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticPromotions, int>(connection)), IsFalse());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticPromotions, long>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticPromotions, long long>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticPromotions, unsigned char>(connection)), IsFalse());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticPromotions, unsigned short int>(connection)), IsFalse());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticPromotions, unsigned int>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticPromotions, unsigned long>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticPromotions, unsigned long long>(connection)), IsTrue());


            AssertThat((testInt32<ValidateMetadataMode::ArithmeticConversions, signed char>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticConversions, short int>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticConversions, int>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticConversions, long>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticConversions, long long>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticConversions, unsigned char>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticConversions, unsigned short int>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticConversions, unsigned int>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticConversions, unsigned long>(connection)), IsTrue());
            AssertThat((testInt32<ValidateMetadataMode::ArithmeticConversions, unsigned long long>(connection)), IsTrue());

            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticConversions, signed char>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticConversions, short int>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticConversions, int>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticConversions, long>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticConversions, long long>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticConversions, unsigned char>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticConversions, unsigned short int>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticConversions, unsigned int>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticConversions, unsigned long>(connection)), IsTrue());
            AssertThat((testUInt32<ValidateMetadataMode::ArithmeticConversions, unsigned long long>(connection)), IsTrue());
        });

        it("can validate metadata with nullable", [&](){
            {
                auto preparedStatement = connection.makePreparedStatement<ResultBindings<int>, true, ValidateMetadataMode::Strict, ValidateMetadataMode::Disabled>(
                    "SELECT `nullable_id` FROM `test_superior_sqlpp`.`xuser_datatypes`"
                );
                AssertThrows(PreparedStatementTypeError, preparedStatement.execute());
            }
            {
                auto preparedStatement = connection.makePreparedStatement<ResultBindings<Nullable<int>>, true, ValidateMetadataMode::Strict, ValidateMetadataMode::Disabled>(
                    "SELECT `id` FROM `test_superior_sqlpp`.`xuser_datatypes`"
                );
                preparedStatement.execute();
            }
        });

        it("can work with blobs", [&](){
            {
                auto preparedStatement = connection.makePreparedStatement(
                    "INSERT INTO `test_superior_sqlpp`.`binary_data` (`id`, `blob`, `binary`, `varbinary`) VALUES (?,?,?,?)",
                    42, BlobData{"ab0cd"}, BlobData{"ef\0gh"}, BlobData{"ij\0kl"}
                );
                preparedStatement.execute();

                preparedStatement.getParams() = std::make_tuple(43, BlobData{}, BlobData{}, BlobData{});
                preparedStatement.updateParamsBindings();
                preparedStatement.execute();
            }

            {
                auto preparedStatement = connection.makePreparedStatement<ResultBindings<int, BlobData, BlobData, BlobData>>(
                    "SELECT `id`, `blob`, `binary`, `varbinary` FROM `test_superior_sqlpp`.`binary_data` ORDER BY `id`"
                );
                preparedStatement.execute();

                int id{};
                BlobData blob{}, binary{}, varbinary{};

                AssertThat(preparedStatement.fetch(), IsTrue());
                std::tie(id, blob, binary, varbinary) = preparedStatement.getResult();
                AssertThat(id, Equals(42));
                AssertThat(blob.size(), Equals(5u));
                AssertThat(binary.size(), Equals(10u));
                AssertThat(varbinary.size(), Equals(5u));
                AssertThat(blob.getStringView()=="ab0cd"s, IsTrue());
                AssertThat(binary.getStringView(), Equals("ef\0gh\0\0\0\0\0"s));
                AssertThat(varbinary.getStringView(), Equals("ij\0kl"s));

                AssertThat(preparedStatement.fetch(), IsTrue());
                std::tie(id, blob, binary, varbinary) = preparedStatement.getResult();
                AssertThat(id, Equals(43));
                AssertThat(blob.size(), Equals(0u));
                AssertThat(binary.size(), Equals(10u));
                AssertThat(varbinary.size(), Equals(0u));
            }
        });

        it("can work with decimal", [&](){
            {
                auto preparedStatement = connection.makePreparedStatement(
                    "INSERT INTO `test_superior_sqlpp`.`decimal_data` (`sd`, `sd_2`, `sd_5`, `sd_5_1`, `sd_6_3`) VALUES (?,?,?,?,?)",
                    42.42, 42.42, DecimalData{"42.42"}, 42.42f, 42.42
                );
                preparedStatement.execute();
            }

            {
                auto preparedStatement = connection.makePreparedStatement<ResultBindings<
                    DecimalData, DecimalData, DecimalData, DecimalData, DecimalData>, true,
                    ValidateMetadataMode::ArithmeticConversions, ValidateMetadataMode::Disabled>(
                        "SELECT `sd`, `sd_2`, `sd_5`, `sd_5_1`, `sd_6_3` FROM `test_superior_sqlpp`.`decimal_data`"
                );
                preparedStatement.execute();

                int count = 0;
                while (preparedStatement.fetch())
                {
                    DecimalData sd{}, sd_2{}, sd_5{}, sd_5_1{}, sd_6_3{};
                    std::tie(sd, sd_2, sd_5, sd_5_1, sd_6_3) = preparedStatement.getResult();

                    AssertThat(sd.getStringView(), Equals("42"));
                    AssertThat(sd_2.getStringView(), Equals("42"));
                    AssertThat(sd_5.getStringView(), Equals("42"));
                    AssertThat(sd_5_1.getStringView(), Equals("42.4"));
                    AssertThat(sd_6_3.getStringView(), Equals("42.420"));

                    ++count;
                }
                AssertThat(count, Equals(1));
            }
        });

        it("can work with date, time, datetime, timestamp", [&](){
            {
                auto preparedStatement = connection.makePreparedStatement(
                    "INSERT INTO `test_superior_sqlpp`.`time` (`id`, `date`, `time`, `datetime`, `timestamp`) VALUES (?,?,?,?,?)",
                    1, Date{2015, 12, 31}, Time{100, 59, 58, true},
                    Datetime{3000, 12, 31, 23, 58, 59, 111}, Timestamp{2038, 01, 19, 03, 14, 07, 222}
                );
                preparedStatement.execute();
            }
            return;
            {
                auto preparedStatement = connection.makePreparedStatement<ResultBindings<
                    Sql::Int, Date, Time, Datetime, Timestamp>>(
                    "SELECT `id`, `date`, `time`, `datetime`, `timestamp` FROM `test_superior_sqlpp`.`time` WHERE `id`=1"
                );
                preparedStatement.execute();


                AssertThat(preparedStatement.fetch(), IsTrue());

                int id;
                Date date{};
                Time time{};
                Datetime datetime{};
                Timestamp timestamp{};
                std::tie(id, date, time, datetime, timestamp) = preparedStatement.getResult();

                AssertThat(id, Equals(1));

                AssertThat(date.getYear(), Equals(2015u));
                AssertThat(date.getMonth(), Equals(12u));
                AssertThat(date.getDay(), Equals(31u));

                AssertThat(time.getHour(), Equals(100u));
                AssertThat(time.getMinute(), Equals(59u));
                AssertThat(time.getSecond(), Equals(58u));

                AssertThat(datetime.getYear(), Equals(3000u));
                AssertThat(datetime.getMonth(), Equals(12u));
                AssertThat(datetime.getDay(), Equals(31u));
                AssertThat(datetime.getHour(), Equals(23u));
                AssertThat(datetime.getMinute(), Equals(58u));
                AssertThat(datetime.getSecond(), Equals(59u));
                AssertThat(datetime.getSecondFraction(), Equals(111u));

                AssertThat(timestamp.getYear(), Equals(2038u));
                AssertThat(timestamp.getMonth(), Equals(01u));
                AssertThat(timestamp.getDay(), Equals(19u));
                AssertThat(timestamp.getHour(), Equals(03u));
                AssertThat(timestamp.getMinute(), Equals(14u));
                AssertThat(timestamp.getSecond(), Equals(07u));
                AssertThat(timestamp.getSecondFraction(), Equals(222u));


                AssertThat(preparedStatement.fetch(), IsFalse());
            }
        });

        it("can detect truncation errors", [&](){
            auto preparedStatement = connection.makePreparedStatement<ResultBindings<StringDataBase<0>>>(
                "SELECT `data` FROM `test_superior_sqlpp`.`truncation_table`"
            );
            preparedStatement.execute();
            AssertThrows(MysqlDataTruncatedError, preparedStatement.fetch());
        });

        it("can avoid truncation errors", [&](){
            // 1. method - store result and allocate buffers before fetch
            {
                auto preparedStatement = connection.makePreparedStatement<ResultBindings<StringDataBase<10>>>(
                    "SELECT `data` FROM `test_superior_sqlpp`.`truncation_table`"
                );
                preparedStatement.setUpdateMaxLengthOnStore(true);
                AssertThat(preparedStatement.getUpdateMaxLengthOnStore(), IsTrue());

                // calls mysql_store() and updates max_length variable in metadata
                preparedStatement.execute();

                auto&& resultMetadata = preparedStatement.getResultMetadata();
                auto maxLength = resultMetadata.front().getMaxLength();
                AssertThat(maxLength, Equals(10u));

                assert(maxLength<=std::get<0>(preparedStatement.getResult()).maxSize());

                preparedStatement.fetch();
            }

            // 2. method - fetch to empty buffer and reallocate
            // TODO: fill in test according to http://dev.mysql.com/doc/refman/5.6/en/mysql-stmt-fetch.html
            // when library will support dynamic types in prepared statements
            {
            }
        });

    });
    describe("Test prepared statement utils", [&](){
        auto& s = getSettingsRef();
        Connection connection{s.database, s.user, s.password, s.host, s.port};

        it("can pass query string into psReadValues", [&](){
            int id{};
            BlobData blob{}, binary{}, varbinary{};
            psReadValues("SELECT `id`, `blob`, `binary`, `varbinary` FROM `test_superior_sqlpp`.`binary_data` ORDER BY `id` LIMIT 1", connection, id, blob, binary, varbinary);

            AssertThat(id, Equals(42));
            AssertThat(blob.size(), Equals(5u));
            AssertThat(binary.size(), Equals(10u));
            AssertThat(varbinary.size(), Equals(5u));
            AssertThat(blob.getStringView()=="ab0cd"s, IsTrue());
            AssertThat(binary.getStringView(), Equals("ef\0gh\0\0\0\0\0"s));
            AssertThat(varbinary.getStringView(), Equals("ij\0kl"s));
        });

        it("throws exception in psReadValues when zero rows is returned", [&](){
            int id{};
            AssertThrows(UnexpectedRowCountError,
                psReadValues("SELECT `id` FROM `test_superior_sqlpp`.`binary_data` LIMIT 0", connection, id)
            );
        });

        it("throws exception in psReadValues when more than one row is returned", [&](){
            int id{};
            AssertThrows(UnexpectedRowCountError,
                psReadValues("SELECT `id` FROM `test_superior_sqlpp`.`binary_data`", connection, id)
            );
        });

        it("can work with psResultQuery (valid types, passing query string)", [&](){
            psResultQuery(connection, "SELECT `id`, `blob`, `binary`, `varbinary` FROM `test_superior_sqlpp`.`binary_data` ORDER BY `id` LIMIT 1",
              [&](int id, const BlobData &blob, const BlobData &binary, const BlobData &varbinary) {
                AssertThat(id, Equals(42));
                AssertThat(blob.size(), Equals(5u));
                AssertThat(binary.size(), Equals(10u));
                AssertThat(varbinary.size(), Equals(5u));
                AssertThat(blob.getStringView()=="ab0cd"s, IsTrue());
                AssertThat(binary.getStringView(), Equals("ef\0gh\0\0\0\0\0"s));
                AssertThat(varbinary.getStringView(), Equals("ij\0kl"s));
            });
        });

        it("can work with prepared statement helper functions - psResultQuery (invalid data types)", [&](){
            AssertThrows(PreparedStatementTypeError, psResultQuery(connection, "SELECT `id`, `blob`, `binary`, `varbinary` FROM `test_superior_sqlpp`.`binary_data` ORDER BY `id` LIMIT 1",
                [&](int , int, const BlobData &, const BlobData &) {}));
        });

        it("can work with psQuery helper function", [&](){
            // Pretend that this is some kind of container on the function's input
            std::array<int, 4> foreignKeys = {{ 1, 2, 3, 4 }};

            std::array<std::vector<int>, 4> ids;

            auto foreignKeysIterator = std::begin(foreignKeys);

            // Select ids, where foreign key is equal to some value (?)
            psQuery(
                connection,
                "SELECT `id`, `f_id` FROM `psquery_test` WHERE `f_id` = ?",
                [&](int &f_id) -> bool {
                    if (foreignKeysIterator == std::end(foreignKeys))
                    {
                        return false;
                    }

                    f_id = *foreignKeysIterator++;
                    return true;
                },
                [&](int id, int f_id)
                {
                    ids[f_id - 1].emplace_back(id);
                }
            );

            AssertThat(ids[0].size(), Equals(4UL));
            AssertThat(ids[1].size(), Equals(2UL));
            AssertThat(ids[2].size(), Equals(1UL));
            AssertThat(ids[3].size(), Equals(1UL));
        });
    });
});


