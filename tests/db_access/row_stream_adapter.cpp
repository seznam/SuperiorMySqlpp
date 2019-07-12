#include <sstream>
#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp/extras/row_stream_adapter.hpp>
#include <superior_mysqlpp.hpp>

#include "settings.hpp"


using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;

go_bandit([](){
    describe("Test RowStreamAdapter", [&](){
        auto& s = getSettingsRef();
        Connection connection{s.database, s.user, s.password, s.host, s.port};

        it("just works", [&](){
            auto query = connection.makeQuery(
                "SELECT * FROM test_superior_sqlpp.row_stream_adapter"
            );
            query.execute();

            auto result = query.use();
            auto row = result.fetchRow();
            Extras::RowStreamAdapter rowStreamAdapter {row};

            Assert::That(static_cast<bool>(rowStreamAdapter));
            int intValue1 {0};
            rowStreamAdapter >> intValue1;
            AssertThat(intValue1, Equals(42));

            Assert::That(static_cast<bool>(rowStreamAdapter));
            int intValue2 {666};
            rowStreamAdapter >> intValue2;
            AssertThat(intValue2, Equals(0));

            Assert::That(static_cast<bool>(rowStreamAdapter));
            std::string stringValue1;
            rowStreamAdapter >> stringValue1;
            AssertThat(stringValue1, Equals("answer"));

            Assert::That(static_cast<bool>(rowStreamAdapter));
            std::string stringValue2 {"number of the beast"};
            rowStreamAdapter >> stringValue2;
            AssertThat(stringValue2, Equals(""));

            Assert::That(static_cast<bool>(!rowStreamAdapter));
        });
    });
});
