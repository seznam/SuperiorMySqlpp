/*
 *  Author: Tomas Nozicka
 */

#include <iostream>
#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>

#include "settings.hpp"


using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;


template<ValidateMetadataMode validateMode, typename T, bool debug=false>
bool testColumnType(Connection& connection, const std::string& column)
{
    auto preparedStatement = connection.makeDynamicPreparedStatement<true, validateMode, ValidateMetadataMode::Disabled>(
        "SELECT `" + column + "` FROM `test_superior_sqlpp`.`xuser_datatypes`"
    );
    preparedStatement.execute();
    T value{};
    preparedStatement.bindResult(0, value);
    try
    {
        preparedStatement.updateResultBindings();
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
    describe("Test dynamic stmt", [&](){
        auto& s = getSettingsRef();
        Connection connection{s.database, s.user, s.password, s.host, s.port};

        it("can do simple statement", [&](){
            DynamicPreparedStatement<> preparedStatement{
                connection.detail_getDriver(),
                "INSERT INTO `test_superior_sqlpp`.`xuser5` (`id`, `name`) "
                "VALUES (11, 'Tomas')"
            };

            preparedStatement.execute();
        });

        it("can do statement only with arguments", [&](){
            DynamicPreparedStatement<> preparedStatement{
                connection.detail_getDriver(),
                "INSERT INTO `test_superior_sqlpp`.`xuser5` (`id`, `name`) "
                "VALUES (?, 'Kokot')"
            };

            int id = 12;
            preparedStatement.bindParam(0, id);

            preparedStatement.updateParamBindings();

            preparedStatement.execute();
        });

        it("can do statement only with arguments repeatedly", [&](){
            DynamicPreparedStatement<> preparedStatement{
                connection.detail_getDriver(),
                "INSERT INTO `test_superior_sqlpp`.`xuser5` (`id`, `name`) "
                "VALUES (?, 'Tomas')"
            };

            for (int i=13; i!=17; ++i)
            {
                preparedStatement.bindParam(0, i);
                preparedStatement.updateParamBindings();
                preparedStatement.execute();
            }
        });

        it("can do statement only with result", [&](){
            auto preparedStatement = connection.makeDynamicPreparedStatement(
                "SELECT `id` FROM `test_superior_sqlpp`.`xuser5`"
            );

            preparedStatement.execute();

            int id = -1;
            preparedStatement.bindResult(0, id);

            preparedStatement.updateResultBindings();

            for (int i=11; i<17; ++i)
            {
                AssertThat(preparedStatement.fetch(), Equals(true));

                AssertThat(id, Equals(i));
            }
        });

        it("can bind strings", [&](){
            auto preparedStatement = connection.makeDynamicPreparedStatement(
                "SELECT `id`, CAST(`name` as CHAR) FROM `test_superior_sqlpp`.`xuser5` WHERE `id`=? AND `name`=? AND `name`=? AND `name`=? AND `name`=?"
            );

            int id = 12;
            StringData sd{"Kokot"};
            std::string string{"Kokot"};
            const char cca[] = "Kokot";
            const char* ccp = static_cast<const char*>(cca);
            preparedStatement.bindParam(0, id);
            preparedStatement.bindParam(1, sd);
            preparedStatement.bindParam(2, string);
            preparedStatement.bindParam(3, cca);
            preparedStatement.bindParam(4, ccp);

            preparedStatement.updateParamBindings();

            preparedStatement.execute();

            Nullable<StringData> sname;
            preparedStatement.bindResult(0, id);
            preparedStatement.bindResult(1, sname);

            preparedStatement.updateResultBindings();

            int count = 0;
            while (preparedStatement.fetch())
            {
                AssertThat(sname.isValid(), IsTrue());
                auto name = sname->getString();

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

        it("can detect out of range bindings", [&](){
            auto preparedStatement = connection.makeDynamicPreparedStatement(
                "SELECT `id` FROM `test_superior_sqlpp`.`xuser5` WHERE 1=1"
            );
            preparedStatement.execute();
            int id=42;
            AssertThrows(OutOfRange, preparedStatement.bindParam(0, id));
            preparedStatement.bindResult(0, id);
            AssertThrows(OutOfRange, preparedStatement.bindResult(1, id));
        });

        it("can validate nullable value", [&](){
            int id = 666;
            std::string name{"Diablo"};

            {
                auto preparedStatement = connection.makeDynamicPreparedStatement(
                    "INSERT INTO `test_superior_sqlpp`.`nullable` VALUES (?,?)"
                    );

                preparedStatement.bindParam(0, id);
                preparedStatement.bindParam(1, name);
                preparedStatement.updateParamBindings();
                preparedStatement.execute();
            }

            {
                auto preparedStatement = connection.makeDynamicPreparedStatement(
                    "SELECT `nullable_id`,`nullable_name` FROM `test_superior_sqlpp`.`nullable` WHERE `nullable_id` = ?"
                    );
                preparedStatement.bindParam(0, id);
                preparedStatement.updateParamBindings();

                preparedStatement.execute();

                Nullable<Sql::Int> nullable_id;
                Nullable<Sql::String> nullable_name;
                preparedStatement.bindResult(0, nullable_id);
                preparedStatement.bindResult(1, nullable_name);
                preparedStatement.updateResultBindings();

                AssertThat(preparedStatement.fetch(), IsTrue());

                AssertThat(nullable_id.isValid(), IsTrue());
                AssertThat(nullable_id.value(), Equals(id));

                AssertThat(nullable_name.isValid(), IsTrue());
                AssertThat(nullable_name.value(), Equals(name));
            }
        });

        it("can validate that all results are binded", [&](){
            auto preparedStatement = connection.makeDynamicPreparedStatement(
                "SELECT `id`,`uid` FROM `test_superior_sqlpp`.`xuser_datatypes`"
            );

            preparedStatement.execute();

            int id;
            preparedStatement.bindResult(0, id);

            AssertThrows(PreparedStatementBindError, preparedStatement.updateResultBindings());
        });
    });
});
