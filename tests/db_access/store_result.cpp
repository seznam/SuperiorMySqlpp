/*
 *  Author: Tomas Nozicka
 */

#include <sstream>
#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>
#include <superior_mysqlpp/prepared_statement.hpp>

#include "settings.hpp"

using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;
using namespace SuperiorMySqlpp::StringViewLiterals;
using namespace std::string_literals;


go_bandit([](){
    describe("Test store result", [&](){
        auto& s = getSettingsRef();
        Connection connection{s.database, s.user, s.password, s.host, s.port};

        it("can insert initial data", [&](){
            auto query = connection.makeQuery(
                "INSERT INTO test_superior_sqlpp.xuser3 (`id`, `name`) VALUE (76, 'olda');"
                "INSERT INTO test_superior_sqlpp.xuser3 (`id`, `name`) VALUE (77, 'olda');"
                "INSERT INTO test_superior_sqlpp.xuser3 (`id`, `name`) VALUE (78, 'olda');"
            );
            query.execute();
            do {} while (query.nextResult());
        });

        it("can do store query", [&](){
            auto query = connection.makeQuery(
                "SELECT * FROM test_superior_sqlpp.xuser3 ORDER BY `id` ASC"
            );
            query.execute();

            StoreQueryResult result{query.store()};
            auto rowsCount = result.getRowsCount();
            AssertThat(rowsCount, Equals(3u));

            int i = 76;
            while (Row row = result.fetchRow())
            {
                std::stringstream{} << row << std::endl;
                for (auto&& item: row)
                {
                    volatile std::string s{item};
                }

                AssertThat(row.size(), Equals(2u));
                AssertThat(row[0], Equals(std::to_string(i++)));
                AssertThat(row[1], Equals(std::string("olda")));
            }
        });

        it("can do store multi-statement query", [&](){
            auto query = connection.makeQuery(
                "SELECT * FROM test_superior_sqlpp.xuser3;"
                "SELECT * FROM test_superior_sqlpp.xuser3;"
            );
            query.execute();
            do
            {
                auto result = query.store();
                auto rowsCount = result.getRowsCount();
                AssertThat(rowsCount, Equals(3u));
                int i = 76;
                while (Row row = result.fetchRow())
                {
                    std::stringstream{} << row << std::endl;
                    for (auto&& item: row)
                    {
                        volatile std::string s{item};
                    }

                    AssertThat(row.size(), Equals(2u));
                    AssertThat(row[0], Equals(std::to_string(i++)));
                    AssertThat(row[1], Equals(std::string("olda")));
                }

                std::stringstream{} << "affected rows: " << query.affectedRows() << std::endl;
            } while (query.nextResult());
        });

        it("can check query execution", [&](){
            auto makeQuery = [&connection](){
                return connection.makeQuery("SELECT * FROM test_superior_sqlpp.xuser3 ORDER BY `id` ASC");
            };

            AssertThrows(QueryNotExecuted, makeQuery().affectedRows());
            AssertThrows(QueryNotExecuted, makeQuery().info());

            AssertThrows(QueryNotExecuted, makeQuery().hasMoreResults());
            AssertThrows(QueryNotExecuted, makeQuery().nextResult());
            AssertThrows(QueryNotExecuted, makeQuery().nextResultOrFail());

            AssertThrows(QueryNotExecuted, makeQuery().store());
            AssertThrows(QueryNotExecuted, makeQuery().storeNext());

            AssertThrows(QueryNotExecuted, makeQuery().use());
            AssertThrows(QueryNotExecuted, makeQuery().useNext());

            AssertThrows(QueryNotExecuted, makeQuery().forEachRow([](auto&&){}));
        });

        it("can get column by index", [&](){
            auto query = connection.makeQuery("SELECT `id`, `name` FROM test_superior_sqlpp.xuser3 ORDER BY `id` ASC");
            query.execute();
            auto result = query.store();

            AssertThrows(OutOfRange, result.getColumnIndex(""));
            AssertThrows(OutOfRange, result.getColumnIndex(reinterpret_cast<const char*>("")));
            AssertThrows(OutOfRange, result.getColumnIndex(""s));
            AssertThrows(OutOfRange, result.getColumnIndex(""sv));

            AssertThrows(OutOfRange, result.getColumnIndex("i"));
            AssertThrows(OutOfRange, result.getColumnIndex(reinterpret_cast<const char*>("i")));
            AssertThrows(OutOfRange, result.getColumnIndex("i"s));
            AssertThrows(OutOfRange, result.getColumnIndex("i"sv));

            AssertThrows(OutOfRange, result.getColumnIndex("id "));
            AssertThrows(OutOfRange, result.getColumnIndex(reinterpret_cast<const char*>("id ")));
            AssertThrows(OutOfRange, result.getColumnIndex("id "s));
            AssertThrows(OutOfRange, result.getColumnIndex("id "sv));

            AssertThrows(OutOfRange, result.getColumnIndex("idi"));
            AssertThrows(OutOfRange, result.getColumnIndex(reinterpret_cast<const char*>("idi")));
            AssertThrows(OutOfRange, result.getColumnIndex("idi"s));
            AssertThrows(OutOfRange, result.getColumnIndex("idi"sv));

            AssertThat(result.getColumnIndex("id"), Equals(0u));
            AssertThat(result.getColumnIndex(reinterpret_cast<const char*>("id")), Equals(0u));
            AssertThat(result.getColumnIndex("id"s), Equals(0u));
            AssertThat(result.getColumnIndex("id"sv), Equals(0u));

            AssertThat(result.getColumnIndex("name"), Equals(1u));
            AssertThat(result.getColumnIndex(reinterpret_cast<const char*>("name")), Equals(1u));
            AssertThat(result.getColumnIndex("name"s), Equals(1u));
            AssertThat(result.getColumnIndex("name"sv), Equals(1u));
        });

        it("can work with NULL", [&](){
            connection.makeQuery("INSERT INTO `simple_null` (`id`, `string`) VALUES (1, 'aaa'), (2, NULL)").execute();

            auto query = connection.makeQuery("SELECT `id`, `string` FROM `test_superior_sqlpp`.`simple_null` ORDER BY `id` ASC");
            query.execute();
            auto result = query.store();

            auto row1 = result.fetchRow();
            AssertThat(row1, IsTrue());
            AssertThat(row1.size(), Equals(2u));

            AssertThat(row1[0], IsTrue());
            AssertThat(row1[0].getStringView(), Equals("1"));
            AssertThat(row1[1], IsTrue());
            AssertThat(row1[1].getStringView(), Equals("aaa"));

            auto row2 = result.fetchRow();
            AssertThat(row2, IsTrue());

            AssertThat(row2[0], IsTrue());
            AssertThat(row2[0].getStringView(), Equals("2"));
            AssertThat(row2[1], IsFalse());

            AssertThat(result.fetchRow(), IsFalse());
        });

        it("throws exception if not all result sets had been read", [&](){
            Connection connection{s.database, s.user, s.password, s.host, s.port};
            auto foo = [&](){
                auto query = connection.makeQuery(
                        "SELECT 1;"
                        "SELECT 1;"
                        "SELECT 1;"
                );
                query.execute();

                query.store();
                query.nextResult();

                query.store();
            };

            AssertThrows(LogicError, foo());
        });

        it("does not throw exception if all result sets had been read", [&](){
            auto query = connection.makeQuery(
                "SELECT 1;"
                "SELECT 1;"
                "SELECT 1;"
            );
            query.execute();

            query.store();
            query.nextResult();

            query.store();
            query.nextResult();

            query.store();

            AssertThat(query.hasMoreResults(), IsFalse());
            AssertThat(query.nextResult(), IsFalse());
            AssertThat(query.hasMoreResults(), IsFalse());
        });
    });
});


