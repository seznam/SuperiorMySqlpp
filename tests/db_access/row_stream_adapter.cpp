#include <sstream>
#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp/converters.hpp>
#include <superior_mysqlpp/extras/row_stream_adapter.hpp>
#include <superior_mysqlpp.hpp>

#include "settings.hpp"

struct NonDefaultConstructible {
    int intValue;
    NonDefaultConstructible(int intValue) : intValue(intValue) {}

    bool operator==(const NonDefaultConstructible &other) const {
        return intValue == other.intValue;
    }
};

namespace SuperiorMySqlpp { namespace Converters {
    template<typename T>
    struct To<T, std::enable_if_t<std::is_same<T, NonDefaultConstructible>::value>> {
        T operator()(const char* str, unsigned int length) {
            return to<int>(str, length);
        }
    };
}}


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

            Assert::That(static_cast<bool>(rowStreamAdapter));
            NonDefaultConstructible ndcValue1 {0};
            rowStreamAdapter >> ndcValue1;
            AssertThat(ndcValue1, Equals(NonDefaultConstructible{42}));

            Assert::That(static_cast<bool>(rowStreamAdapter));
            NonDefaultConstructible ndcValue2 {666};
            AssertThrows(LogicError, (rowStreamAdapter >> ndcValue2));

            Assert::That(static_cast<bool>(rowStreamAdapter));
            int ndcValue2b {666};
            rowStreamAdapter >> ndcValue2b;
            AssertThat(ndcValue2b, Equals(0));

            Assert::That(static_cast<bool>(!rowStreamAdapter));
        });
    });
});
