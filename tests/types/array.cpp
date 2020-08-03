/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <tuple>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>

#include "../test_utils.hpp"


using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;
using namespace std::string_literals;


go_bandit([](){
    describe("Test ArrayBase", [&](){
        it("can do basic operations", [&](){
            {
                constexpr unsigned long arraySize = 0;
                ArrayBase<arraySize> array{};

                AssertThat(array.begin()==array.cbegin(), IsTrue());
                AssertThat(array.end()==array.cend(), IsTrue());
                AssertThat(array.begin()==array.end(), IsTrue());
                AssertThat(array.cbegin()==array.cend(), IsTrue());
                AssertThat(array.end()==array.endOfStorage(), IsTrue());
                AssertThat(array.cend()==array.cendOfStorage(), IsTrue());

                AssertThat(array.empty(), IsTrue());
                AssertThat(array.size(), Equals(0u));
                AssertThat(array.maxSize(), Equals(arraySize));
            }

            {
                constexpr unsigned long arraySize = 1;
                ArrayBase<arraySize> array{};

                AssertThat(array.begin()==array.cbegin(), IsTrue());
                AssertThat(array.end()==array.cend(), IsTrue());
                AssertThat(array.begin()==array.end(), IsTrue());
                AssertThat(array.cbegin()==array.cend(), IsTrue());
                AssertThat(array.end()+arraySize==array.endOfStorage(), IsTrue());
                AssertThat(array.cend()+arraySize==array.cendOfStorage(), IsTrue());

                AssertThrows(std::out_of_range, array.front());
                AssertThrows(std::out_of_range, array.back());

                AssertThat(array.empty(), IsTrue());
                AssertThat(array.size(), Equals(0u));
                AssertThat(array.maxSize(), Equals(arraySize));


                array.counterRef() = 1;
                array[0] = 'a';


                AssertThat(array.begin()==array.cbegin(), IsTrue());
                AssertThat(array.end()==array.cend(), IsTrue());
                AssertThat(array.begin()+1==array.end(), IsTrue());
                AssertThat(array.cbegin()+1==array.cend(), IsTrue());
                AssertThat(array.end()+arraySize-1==array.endOfStorage(), IsTrue());
                AssertThat(array.cend()+arraySize-1==array.cendOfStorage(), IsTrue());

                AssertThat(array.front(), Equals(*array.begin()));
                AssertThat(array.back(), Equals(*(array.end()-1)));

                AssertThat(array.empty(), IsFalse());
                AssertThat(array.size(), Equals(1u));
                AssertThat(array.maxSize(), Equals(arraySize));

                AssertThat(array[0], Equals('a'));
            }
        });

        it("can be zero-terminated", [&](){
            const std::size_t arraySize = 5;
            std::array<unsigned char, sizeof(ArrayBase<arraySize>)+2> storage;
            std::string input{"123"};

            auto resetStorage = [&storage] {
                storage.fill('x');
                storage.back() = '\0';
            };

            // Basic case without implicit zero terminator
            {
                resetStorage();
                auto ptr = PlacementPtr<ArrayBase<arraySize>>(storage.data());
                auto& array = *ptr;
                AssertThat(std::string(array.data(), arraySize), Equals(std::string("xxxxx")));

                std::copy(input.begin(), input.end(), array.begin());
                array.counterRef() = 3;
                AssertThat(array.size(), Equals(3U));

                AssertThat(std::string(array.data(), arraySize), Equals(std::string("123xx")));
            }

            // With zero terminator
            {
                resetStorage();
                auto ptr = PlacementPtr<ArrayBase<arraySize, true>>(storage.data());
                auto& array = *ptr;
                AssertThat(std::string(array.data()), Equals(std::string("")));

                std::copy(input.begin(), input.end(), array.begin());
                array.counterRef() = 3;
                AssertThat(array.size(), Equals(3U));

                // We used too low-level API to set content, so there is no zero terminator after 3, but safety check at array's end caps us, at least
                AssertThat(std::string(array.data()), Equals(std::string("123xx")));

                array[3] = '\0';
                AssertThat(std::string(array.data()), Equals(std::string("123")));

                // Copy constructor, however, honors zero terminators
                ArrayBase<arraySize, true> arrayCopy{array};
                AssertThat(std::string(arrayCopy.data()), Equals(std::string("123")));
            }
        });
    });
});
