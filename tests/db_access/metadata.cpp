/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>

#include "settings.hpp"

using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;


std::string queryString = "SELECT `id` AS `as_id`, `name` FROM test_superior_sqlpp.xuser7 ORDER BY `id` ASC";

void validateMetadataId(const ResultField& item)
{
    AssertThat(item.getCatalogName(), Equals("def"));
    AssertThat(item.getCatalogNameView(), Equals("def"));
    AssertThat(item.getCharacterSetNumber(), Equals(63u));
    AssertThat(item.getColumnName(), Equals("as_id"));
    AssertThat(item.getColumnNameView(), Equals("as_id"));
    AssertThat(item.getColumnOriginalName(), Equals("id"));
    AssertThat(item.getColumnOriginalNameView(), Equals("id"));
    AssertThat(item.getDatabaseName(), Equals("test_superior_sqlpp"));
    AssertThat(item.getDatabaseNameView(), Equals("test_superior_sqlpp"));
    AssertThat(item.getDecimalsCount(), Equals(0u));
    AssertThat(item.getDisplayLength(), Equals(11u));
//    AssertThat(item.getMaxLength(), Equals(11));  // works only for prepared statements - should be working also for store, but it doesn't
    AssertThat(item.getFieldType(), Equals(FieldTypes::Long));
    AssertThat(item.getTableName(), Equals("xuser7"));
    AssertThat(item.getTableNameView(), Equals("xuser7"));
    AssertThat(item.getTableOriginalName(), Equals("xuser7"));
    AssertThat(item.getTableOriginalNameView(), Equals("xuser7"));
    AssertThat(item.hasAutoincrement(), Equals(true));
    AssertThat(item.hasDefaultValue(), Equals(true));
    AssertThat(item.hasZerofill(), Equals(false));
    AssertThat(item.isBinary(), Equals(false));
    AssertThat(item.isEnum(), Equals(false));
    AssertThat(item.isNonUniqueKeyPart(), Equals(false));
    AssertThat(item.isNullable(), Equals(false));
    AssertThat(item.isNumeric(), Equals(true));
    AssertThat(item.isPrimaryKeyPart(), Equals(true));
    AssertThat(item.isSet(), Equals(false));
    AssertThat(item.isUniqueKeyPart(), Equals(false));
    AssertThat(item.isUnsigned(), Equals(false));
}

void validateMetadataName(const ResultField& item)
{
    AssertThat(item.getCatalogName(), Equals("def"));
    AssertThat(item.getCatalogNameView(), Equals("def"));
    AssertThat(item.getCharacterSetNumber(), Equals(8u));
    AssertThat(item.getColumnName(), Equals("name"));
    AssertThat(item.getColumnNameView(), Equals("name"));
    AssertThat(item.getColumnOriginalName(), Equals("name"));
    AssertThat(item.getColumnOriginalNameView(), Equals("name"));
    AssertThat(item.getDatabaseName(), Equals("test_superior_sqlpp"));
    AssertThat(item.getDatabaseNameView(), Equals("test_superior_sqlpp"));
    AssertThat(item.getDecimalsCount(), Equals(0u));
    AssertThat(item.getDisplayLength(), Equals(100u));
    AssertThat(item.getMaxLength(), Equals(0u));
    AssertThat(item.getFieldType(), Equals(FieldTypes::VarString));
    AssertThat(item.getTableName(), Equals("xuser7"));
    AssertThat(item.getTableNameView(), Equals("xuser7"));
    AssertThat(item.getTableOriginalName(), Equals("xuser7"));
    AssertThat(item.getTableOriginalNameView(), Equals("xuser7"));
    AssertThat(item.hasAutoincrement(), Equals(false));
    AssertThat(item.hasDefaultValue(), Equals(true));
    AssertThat(item.hasZerofill(), Equals(false));
    AssertThat(item.isBinary(), Equals(false));
    AssertThat(item.isEnum(), Equals(false));
    AssertThat(item.isNonUniqueKeyPart(), Equals(false));
    AssertThat(item.isNullable(), Equals(true));
    AssertThat(item.isNumeric(), Equals(false));
    AssertThat(item.isPrimaryKeyPart(), Equals(false));
    AssertThat(item.isSet(), Equals(false));
    AssertThat(item.isUniqueKeyPart(), Equals(false));
    AssertThat(item.isUnsigned(), Equals(false));
}


go_bandit([](){
    describe("Test metadata", [&](){
        auto& s = getSettingsRef();
        Connection connection{s.database, s.user, s.password, s.host, s.port};

        it("can do StoreQuery metadata", [&](){
            auto query = connection.makeQuery(queryString);
            query.execute();

            StoreQueryResult result{query.store()};

            auto& metadata = result.getMetadata();

            validateMetadataId(metadata[0]);
            validateMetadataName(metadata[1]);
        });

        it("is iterable", [&](){
            auto query = connection.makeQuery(queryString);
            query.execute();

            StoreQueryResult result{query.store()};

            auto& metadata = result.getMetadata();
            for (auto&& item: metadata)
            {
                item.getColumnName();
            }
        });

        it("can do UseQuery metadata", [&](){
            auto query = connection.makeQuery(queryString);
            query.execute();

            UseQueryResult result{query.use()};

            auto& metadata = result.getMetadata();

            validateMetadataId(metadata[0]);
            validateMetadataName(metadata[1]);
        });

        it("can do PreparedStatement metadata", [&](){
            PreparedStatement<ResultBindings<Sql::Int, Nullable<Sql::String>>, ParamBindings<>> preparedStatement(connection.detail_getDriver(), queryString);
            preparedStatement.setUpdateMaxLengthOnStore(true);
            AssertThat(preparedStatement.getUpdateMaxLengthOnStore(), IsTrue());

            preparedStatement.execute();

            auto& metadata = preparedStatement.getResultMetadata();

            validateMetadataId(metadata[0]);
            validateMetadataName(metadata[1]);
        });

        it("can do PreparedStatement metadata without execute", [&](){
            PreparedStatement<ResultBindings<Sql::Int, Nullable<Sql::String>>, ParamBindings<>> preparedStatement(connection.detail_getDriver(), queryString);

            auto& metadata = preparedStatement.getResultMetadata();

            validateMetadataId(metadata[0]);
            validateMetadataName(metadata[1]);
        });

        it("can do DynamicPreparedStatement metadata without execute", [&](){
            DynamicPreparedStatement<> preparedStatement(connection.detail_getDriver(), queryString);

            auto& metadata = preparedStatement.getResultMetadata();

            validateMetadataId(metadata[0]);
            validateMetadataName(metadata[1]);
        });
    });
});


