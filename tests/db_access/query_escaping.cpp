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


go_bandit([](){
    describe("Test query escaping", [&](){
        auto& s = getSettingsRef();
        Connection connection{s.database, s.user, s.password, s.host, s.port};

        it("works for arithmetic types", [&](){
            {
                auto query = connection.makeQuery();
                query << "XXX " << 77;
                AssertThat(query.getQueryString(), Equals("XXX 77"));
            }
            {
                auto query = connection.makeQuery();
                query << "XXX " << quoteOnly << 77 << " " << 55;
                AssertThat(query.getQueryString(), Equals("XXX '77' 55"));
            }
            {
                auto query = connection.makeQuery();
                query << "XXX " << escape << 77 << " " << 55;
                AssertThat(query.getQueryString(), Equals("XXX 77 55"));
            }
            {
                auto query = connection.makeQuery();
                query << "XXX " << quote << 77 << " " << 55;
                AssertThat(query.getQueryString(), Equals("XXX '77' 55"));
            }
        });

        it("works for other types (strings)", [&](){
            {
                auto query = connection.makeQuery();
                query << "XXX " << "ol'da";
                AssertThat(query.getQueryString(), Equals("XXX ol'da"));
            }
            {
                auto query = connection.makeQuery();
                query << "XXX " << quoteOnly << "ol'da" << " ol'da";
                AssertThat(query.getQueryString(), Equals("XXX 'ol'da' ol'da"));
            }
            {
                auto query = connection.makeQuery();
                query << "XXX " << escape << R"(ol'd\a)" << " ol'da";
                AssertThat(query.getQueryString(), Equals(R"(XXX ol\'d\\a ol'da)"));
            }
            {
                auto query = connection.makeQuery();
                query << "XXX " << quote << R"(ol'd\a)" << " ol'da";
                AssertThat(query.getQueryString(), Equals(R"(XXX 'ol\'d\\a' ol'da)"));
            }
        });
    });
});


