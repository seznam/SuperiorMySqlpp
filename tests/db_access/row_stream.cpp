#include <sstream>
#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp/row_stream.hpp>
#include <superior_mysqlpp.hpp>

#include "settings.hpp"


using namespace bandit;
using namespace SuperiorMySqlpp;

go_bandit([](){
    describe("Test RowStream", [&](){
        auto& s = getSettingsRef();
        Connection connection{s.database, s.user, s.password, s.host, s.port};

        it("just works", [&](){
            {
                auto query = connection.makeQuery(
                    "INSERT INTO test_superior_sqlpp.row_stream("
                        "`int_value_1`,"
                        "`int_value_2`,"
                        "`string_value_1`,"
                        "`string_value_2`) "
                    "VALUE (42, NULL, 'answer', NULL);"
                );
                query.execute();
                do {} while (query.nextResult());
            }

            auto query = connection.makeQuery(
                "SELECT * FROM test_superior_sqlpp.row_stream"
            );
            query.execute();

            auto result = query.use();

            int intValue1 {0};
            int intValue2 {666};
            std::string stringValue1;
            std::string stringValue2 {"do not touch"};
            while (auto row = result.fetchRow()) {
                RowStream {row}
                    >> intValue1
                    >> intValue2
                    >> stringValue1
                    >> stringValue2
                    ;

                AssertThat(intValue1, Equals(42));
                AssertThat(intValue2, Equals(666));
                AssertThat(stringValue1, Equals("answer"));
                AssertThat(stringValue2, Equals("do not touch"));
            }
        });
    });
});
