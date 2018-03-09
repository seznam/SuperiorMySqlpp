/*
 *  Author: Tomas Nozicka
 */

#include <iostream>
#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>

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

        it("can work with huge strings", [&](){
            // About 14kB of test data
            static const char *text = "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Nullam sit amet magna in magna gravida vehicula. Mauris dolor felis, sagittis at, luctus sed, aliquam non, tellus. Nullam dapibus fermentum ipsum. Sed convallis magna eu sem. Aliquam erat volutpat. Aenean vel massa quis mauris vehicula lacinia. Aliquam ornare wisi eu metus. Maecenas sollicitudin. Donec vitae arcu. Curabitur sagittis hendrerit ante. Fusce nibh."
            "Mauris elementum mauris vitae tortor. Aliquam erat volutpat. Donec quis nibh at felis congue commodo. Mauris suscipit, ligula sit amet pharetra semper, nibh ante cursus purus, vel sagittis velit mauris vel metus. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Donec vitae arcu. In enim a arcu imperdiet malesuada. Temporibus autem quibusdam et aut officiis debitis aut rerum necessitatibus saepe eveniet ut et voluptates repudiandae sint et molestiae non recusandae. Nunc tincidunt ante vitae massa. Phasellus et lorem id felis nonummy placerat. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Etiam ligula pede, sagittis quis, interdum ultricies, scelerisque eu."
            "Aliquam id dolor. Aliquam erat volutpat. Integer lacinia. Vivamus ac leo pretium faucibus. Proin mattis lacinia justo. Fusce wisi. Ut tempus purus at lorem. Nam libero tempore, cum soluta nobis est eligendi optio cumque nihil impedit quo minus id quod maxime placeat facere possimus, omnis voluptas assumenda est, omnis dolor repellendus. Maecenas ipsum velit, consectetuer eu lobortis ut, dictum at dui. Fusce tellus odio, dapibus id fermentum quis, suscipit id erat. Fusce consectetuer risus a nunc. Cras elementum. In sem justo, commodo ut, suscipit at, pharetra vitae, orci. Pellentesque pretium lectus id turpis."
            "Maecenas sollicitudin. Aliquam erat volutpat. Nulla accumsan, elit sit amet varius semper, nulla mauris mollis quam, tempor suscipit diam nulla vel leo. Praesent vitae arcu tempor neque lacinia pretium. Nullam sit amet magna in magna gravida vehicula. Sed vel lectus. Donec odio tempus molestie, porttitor ut, iaculis quis, sem. Sed elit dui, pellentesque a, faucibus vel, interdum nec, diam. Maecenas fermentum, sem in pharetra pellentesque, velit turpis volutpat ante, in pharetra metus odio a lectus. Nam sed tellus id magna elementum tincidunt. Fusce nibh."
            "Integer in sapien. Duis viverra diam non justo. Fusce consectetuer risus a nunc. Suspendisse nisl. Donec vitae arcu. Fusce suscipit libero eget elit. Curabitur vitae diam non enim vestibulum interdum. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Phasellus enim erat, vestibulum vel, aliquam a, posuere eu, velit. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Sed convallis magna eu sem. Pellentesque ipsum. Maecenas lorem. Aliquam ante. Integer rutrum, orci vestibulum ullamcorper ultricies, lacus quam ultricies odio, vitae placerat pede sem sit amet enim. Duis ante orci, molestie vitae vehicula venenatis, tincidunt ac pede."
            "Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Phasellus enim erat, vestibulum vel, aliquam a, posuere eu, velit. Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? Nulla non lectus sed nisl molestie malesuada. Itaque earum rerum hic tenetur a sapiente delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut perferendis doloribus asperiores repellat. Nullam at arcu a est sollicitudin euismod. Fusce suscipit libero eget elit. Etiam quis quam. Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? Duis viverra diam non justo. Etiam ligula pede, sagittis quis, interdum ultricies, scelerisque eu. Maecenas lorem. Donec iaculis gravida nulla. Integer rutrum, orci vestibulum ullamcorper ultricies, lacus quam ultricies odio, vitae placerat pede sem sit amet enim. Phasellus rhoncus."
            "Integer lacinia. Nulla est. Mauris suscipit, ligula sit amet pharetra semper, nibh ante cursus purus, vel sagittis velit mauris vel metus. Duis pulvinar. Aliquam id dolor. Duis pulvinar. Integer lacinia. Etiam dui sem, fermentum vitae, sagittis id, malesuada in, quam. Praesent id justo in neque elementum ultrices. Duis pulvinar. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Vivamus porttitor turpis ac leo. Pellentesque pretium lectus id turpis. Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? Nulla non lectus sed nisl molestie malesuada."
            "Pellentesque pretium lectus id turpis. Suspendisse sagittis ultrices augue. Sed convallis magna eu sem. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Sed ac dolor sit amet purus malesuada congue. Fusce dui leo, imperdiet in, aliquam sit amet, feugiat eu, orci. Aenean vel massa quis mauris vehicula lacinia. Proin pede metus, vulputate nec, fermentum fringilla, vehicula vitae, justo. Quisque porta. Vestibulum fermentum tortor id mi. Fusce aliquam vestibulum ipsum. Integer vulputate sem a nibh rutrum consequat. Nam sed tellus id magna elementum tincidunt. Fusce consectetuer risus a nunc. Maecenas lorem. Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur?"
            "Etiam commodo dui eget wisi. Vestibulum fermentum tortor id mi. Aliquam in lorem sit amet leo accumsan lacinia. Nulla est. Mauris dictum facilisis augue. Morbi scelerisque luctus velit. In convallis. Integer malesuada. Aenean placerat. Vivamus ac leo pretium faucibus. Praesent vitae arcu tempor neque lacinia pretium. Phasellus faucibus molestie nisl. Praesent dapibus. Nullam dapibus fermentum ipsum. Mauris dictum facilisis augue. Suspendisse nisl."
            "Aliquam id dolor. Quisque porta. Cras elementum. Nullam lectus justo, vulputate eget mollis sed, tempor sed magna. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Duis ante orci, molestie vitae vehicula venenatis, tincidunt ac pede. In enim a arcu imperdiet malesuada. Aliquam erat volutpat. Morbi scelerisque luctus velit. Aliquam in lorem sit amet leo accumsan lacinia. Maecenas aliquet accumsan leo. Aliquam erat volutpat. Phasellus rhoncus. Sed elit dui, pellentesque a, faucibus vel, interdum nec, diam. Fusce wisi. Aliquam erat volutpat. Mauris dolor felis, sagittis at, luctus sed, aliquam non, tellus."
            "Mauris dolor felis, sagittis at, luctus sed, aliquam non, tellus. Vestibulum fermentum tortor id mi. Etiam dui sem, fermentum vitae, sagittis id, malesuada in, quam. Curabitur vitae diam non enim vestibulum interdum. In enim a arcu imperdiet malesuada. Vivamus luctus egestas leo. Integer malesuada. Nunc auctor. Fusce suscipit libero eget elit. Maecenas fermentum, sem in pharetra pellentesque, velit turpis volutpat ante, in pharetra metus odio a lectus. Duis risus. Nullam faucibus mi quis velit. Morbi imperdiet, mauris ac auctor dictum, nisl ligula egestas nulla, et sollicitudin sem purus in lacus. Etiam egestas wisi a erat. In laoreet, magna id viverra tincidunt, sem odio bibendum justo, vel imperdiet sapien wisi sed libero. Curabitur sagittis hendrerit ante. Nunc dapibus tortor vel mi dapibus sollicitudin."
            "Aliquam erat volutpat. Maecenas lorem. Cras pede libero, dapibus nec, pretium sit amet, tempor quis. Phasellus rhoncus. Vivamus porttitor turpis ac leo. Suspendisse sagittis ultrices augue. Nullam at arcu a est sollicitudin euismod. Praesent vitae arcu tempor neque lacinia pretium. Fusce aliquam vestibulum ipsum. Etiam bibendum elit eget erat. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem."
            "Nulla quis diam. In convallis. Ut tempus purus at lorem. Mauris suscipit, ligula sit amet pharetra semper, nibh ante cursus purus, vel sagittis velit mauris vel metus. Integer vulputate sem a nibh rutrum consequat. Nullam lectus justo, vulputate eget mollis sed, tempor sed magna. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Duis bibendum, lectus ut viverra rhoncus, dolor nunc faucibus libero, eget facilisis enim ipsum id lacus. Quisque porta. Duis bibendum, lectus ut viverra rhoncus, dolor nunc faucibus libero, eget facilisis enim ipsum id lacus. Etiam posuere lacus quis dolor. Phasellus et lorem id felis nonummy placerat."
            "Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Nulla est. Etiam posuere lacus quis dolor. Praesent dapibus. Integer imperdiet lectus quis justo. Etiam posuere lacus quis dolor. Et harum quidem rerum facilis est et expedita distinctio. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Integer tempor. Fusce wisi. Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Curabitur bibendum justo non orci. Integer imperdiet lectus quis justo. Fusce tellus. Etiam neque. Etiam bibendum elit eget erat. Nam quis nulla. Nunc auctor. Duis risus. Vivamus luctus egestas leo."
            "Nunc dapibus tortor vel mi dapibus sollicitudin. Donec quis nibh at felis congue commodo. Nam sed tellus id magna elementum tincidunt. Pellentesque sapien. Nunc tincidunt ante vitae massa. Curabitur ligula sapien, pulvinar a vestibulum quis, facilisis vel sapien. In sem justo, commodo ut, suscipit at, pharetra vitae, orci. Mauris elementum mauris vitae tortor. Nullam dapibus fermentum ipsum. Duis bibendum, lectus ut viverra rhoncus, dolor nunc faucibus libero, eget facilisis enim ipsum id lacus. Aliquam erat volutpat."
            "Nullam dapibus fermentum ipsum. Fusce aliquam vestibulum ipsum. Quisque tincidunt scelerisque libero. Nullam dapibus fermentum ipsum. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Phasellus et lorem id felis nonummy placerat. In sem justo, commodo ut, suscipit at, pharetra vitae, orci. Aliquam ornare wisi eu metus. In laoreet, magna id viverra tincidunt, sem odio bibendum justo, vel imperdiet sapien wisi sed libero. In convallis. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Phasellus enim erat, vestibulum vel, aliquam a, posuere eu, velit. Nullam rhoncus aliquam metus. Phasellus et lorem id felis nonummy placerat. Aliquam in lorem sit amet leo accumsan lacinia. Etiam bibendum elit eget erat. Aliquam erat volutpat. Integer in sapien."
            "Fusce consectetuer risus a nunc. Maecenas lorem. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Fusce tellus. Vivamus luctus egestas leo. Fusce tellus. Nulla pulvinar eleifend sem. Nam libero tempore, cum soluta nobis est eligendi optio cumque nihil impedit quo minus id quod maxime placeat facere possimus, omnis voluptas assumenda est, omnis dolor repellendus. In rutrum. Nulla non lectus sed nisl molestie malesuada. Duis bibendum, lectus ut viverra rhoncus, dolor nunc faucibus libero, eget facilisis enim ipsum id lacus. Aliquam id dolor. In dapibus augue non sapien. Integer imperdiet lectus quis justo."
            "Morbi scelerisque luctus velit. Etiam neque. Praesent id justo in neque elementum ultrices. Aenean vel massa quis mauris vehicula lacinia. Etiam dictum tincidunt diam. Donec ipsum massa, ullamcorper in, auctor et, scelerisque sed, est. Aliquam in lorem sit amet leo accumsan lacinia. Mauris metus. Nulla non lectus sed nisl molestie malesuada. Phasellus enim erat, vestibulum vel, aliquam a, posuere eu, velit."
            "Cras elementum. Pellentesque pretium lectus id turpis. Nulla non arcu lacinia neque faucibus fringilla. Integer vulputate sem a nibh rutrum consequat. Nullam eget nisl. Nullam justo enim, consectetuer nec, ullamcorper ac, vestibulum in, elit. Praesent dapibus. Mauris dolor felis, sagittis at, luctus sed, aliquam non, tellus. Nullam sit amet magna in magna gravida vehicula. Duis viverra diam non justo. Etiam sapien elit, consequat eget, tristique non, venenatis quis, ante. Nulla accumsan, elit sit amet varius semper, nulla mauris mollis quam, tempor suscipit diam nulla vel leo. Integer imperdiet lectus quis justo. Curabitur bibendum justo non orci. Cras pede libero, dapibus nec, pretium sit amet, tempor quis. Pellentesque sapien. Praesent vitae arcu tempor neque lacinia pretium. Phasellus et lorem id felis nonummy placerat. Cras elementum. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas."
            "Etiam dictum tincidunt diam. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Mauris tincidunt sem sed arcu. Aliquam erat volutpat. Maecenas sollicitudin. Mauris dictum facilisis augue. In enim a arcu imperdiet malesuada. Vivamus luctus egestas leo. Phasellus enim erat, vestibulum vel, aliquam a, posuere eu, velit. Morbi leo mi, nonummy eget tristique non, rhoncus non leo. In rutrum. Proin mattis lacinia justo. Etiam sapien elit, consequat eget, tristique non, venenatis quis, ante.";
            {
                auto&& preparedStatement = connection.makePreparedStatement(
                    "INSERT INTO `test_superior_sqlpp`.`hugestring` VALUES (?, ?)",
                    1, Nullable<std::string>{text}
                );
                preparedStatement.execute();

                preparedStatement.setParams(143, Nullable<std::string>{"aaa"});
                preparedStatement.updateParamsBindings();
                preparedStatement.execute();
            }
            {
                auto&& preparedStatement = connection.makePreparedStatement<ResultBindings<int, Nullable<HugeStringData>>>(
                    "SELECT `int_value`, `mytext` FROM `test_superior_sqlpp`.`hugestring` LIMIT 3"
                );
                preparedStatement.execute();
                int id;
                Nullable<HugeStringData> htext{};

                AssertThat(preparedStatement.fetch(), IsTrue());
                std::tie(id, htext) = preparedStatement.getResult();
                AssertThat(htext->getString(), Equals(text));

                AssertThat(preparedStatement.fetch(), IsTrue());
                std::tie(id, htext) = preparedStatement.getResult();
                AssertThat(htext->getString(), Equals("aaa"));
            }
        });
    });
});
