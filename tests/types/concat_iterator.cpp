/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <tuple>
#include <vector>
#include <bandit/bandit.h>

#include <superior_mysqlpp.hpp>


using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;
using namespace std::string_literals;



go_bandit([](){
    describe("Test ConcatIterator", [&](){
        it("can work on empty vectors", [&](){
            std::vector<int> a{};
            std::vector<int> b{};
            auto it_a_begin = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), firstTag, a.begin());
            auto it_a_end = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), firstTag, a.end());
            auto it_b_begin = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), secondTag, b.begin());
            auto it_b_end = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), secondTag, b.end());

            AssertThat(it_a_begin, Is().EqualTo(it_a_end).And().EqualTo(it_b_begin).And().EqualTo(it_b_end));

            AssertThat(it_a_begin!=it_a_begin, IsFalse());
            AssertThat(it_a_end!=it_a_end, IsFalse());
            AssertThat(it_b_begin!=it_b_begin, IsFalse());
            AssertThat(it_b_end!=it_b_end, IsFalse());

            AssertThat(it_a_begin!=it_a_end, IsFalse());
            AssertThat(it_a_begin!=it_b_begin, IsFalse());
            AssertThat(it_a_begin!=it_b_end, IsFalse());
            AssertThat(it_a_end!=it_b_begin, IsFalse());
            AssertThat(it_a_end!=it_b_end, IsFalse());
            AssertThat(it_b_begin!=it_b_end, IsFalse());

            AssertThat(distance(it_a_begin, it_a_begin), Equals(0));
            AssertThat(distance(it_a_end, it_a_end), Equals(0));
            AssertThat(distance(it_b_begin, it_b_begin), Equals(0));
            AssertThat(distance(it_b_end, it_b_end), Equals(0));

            AssertThat(distance(it_a_begin, it_a_end), Equals(0));
            AssertThat(distance(it_a_begin, it_b_begin), Equals(0));
            AssertThat(distance(it_a_begin, it_b_end), Equals(0));
            AssertThat(distance(it_a_end, it_a_end), Equals(0));
            AssertThat(distance(it_a_end, it_b_begin), Equals(0));
            AssertThat(distance(it_a_end, it_b_end), Equals(0));
            AssertThat(distance(it_b_begin, it_b_begin), Equals(0));
            AssertThat(distance(it_b_begin, it_b_end), Equals(0));
        });

        it("can work when first vector is empty", [&](){
            std::vector<int> a{};
            std::vector<int> b{1, 2, 3};
            auto it_a_begin = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), firstTag, a.begin());
            auto it_a_end = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), firstTag, a.end());
            auto it_b_begin = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), secondTag, b.begin());
            auto it_b_last = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), secondTag, --b.end());
            auto it_b_end = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), secondTag, b.end());

            AssertThat(it_a_begin==it_a_end, IsTrue());
            AssertThat(it_a_begin==it_b_begin, IsTrue());
            AssertThat(it_a_begin==it_b_last, IsFalse());
            AssertThat(it_a_end==it_b_begin, IsTrue());
            AssertThat(it_a_end==it_b_last, IsFalse());
            AssertThat(it_b_last==it_b_last, IsTrue());


            AssertThat(it_a_begin!=it_a_end, IsFalse());
            AssertThat(it_a_begin!=it_b_begin, IsFalse());
            AssertThat(it_a_begin!=it_b_last, IsTrue());
            AssertThat(it_a_end!=it_b_begin, IsFalse());
            AssertThat(it_a_end!=it_b_last, IsTrue());
            AssertThat(it_b_begin!=it_b_last, IsTrue());

            AssertThat(distance(it_a_begin, it_a_end), Equals(0));
            AssertThat(distance(it_a_begin, it_b_begin), Equals(0));
            AssertThat(distance(it_a_end, it_b_begin), Equals(0));
            AssertThat(distance(it_b_begin, it_b_last), Equals(2));
            AssertThat(distance(it_b_begin, it_b_end), Equals(3));
        });

        it("can work when second vector is empty", [&](){
            std::vector<int> a{1, 2, 3};
            std::vector<int> b{};
            auto it_a_begin = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), firstTag, a.begin());
            auto it_a_last = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), firstTag, --a.end());
            auto it_a_end = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), firstTag, a.end());
            auto it_b_begin = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), secondTag, b.begin());
            auto it_b_end = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), secondTag, b.end());

            AssertThat(it_a_begin==it_a_last, IsFalse());
            AssertThat(it_a_begin==it_b_begin, IsFalse());
            AssertThat(it_a_begin==it_b_end, IsFalse());
            AssertThat(it_a_last==it_b_begin, IsFalse());
            AssertThat(it_a_end==it_b_begin, IsTrue());
            AssertThat(it_a_last==it_b_end, IsFalse());
            AssertThat(it_b_end==it_b_end, IsTrue());


            AssertThat(it_a_begin!=it_a_last, IsTrue());
            AssertThat(it_a_begin!=it_b_begin, IsTrue());
            AssertThat(it_a_begin!=it_b_end, IsTrue());
            AssertThat(it_a_last!=it_b_begin, IsTrue());
            AssertThat(it_a_last!=it_b_end, IsTrue());
            AssertThat(it_b_begin!=it_b_end, IsFalse());

            AssertThat(distance(it_a_begin, it_a_last), Equals(2));
            AssertThat(distance(it_a_begin, it_a_end), Equals(3));
            AssertThat(distance(it_b_begin, it_b_begin), Equals(0));
            AssertThat(distance(it_b_begin, it_b_end), Equals(0));
        });

        it("can work when no vector is empty", [&](){
            std::vector<int> a{1, 2, 3};
            std::vector<int> b{4, 5};
            auto it_a_begin = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), firstTag, a.begin());
            auto it_a_last = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), firstTag, --a.end());
            auto it_a_end = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), firstTag, a.end());
            auto it_b_begin = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), secondTag, b.begin());
            auto it_b_last = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), secondTag, --b.end());
            auto it_b_end = makeConcatIterator(a.begin(), a.end(), b.begin(), b.end(), secondTag, b.end());

            AssertThat(it_a_begin==it_a_last, IsFalse());
            AssertThat(it_a_begin==it_b_begin, IsFalse());
            AssertThat(it_a_begin==it_b_end, IsFalse());
            AssertThat(it_a_last==it_b_begin, IsFalse());
            AssertThat(it_a_end==it_b_begin, IsFalse());
            AssertThat(it_a_last==it_b_end, IsFalse());


            AssertThat(it_a_begin!=it_a_last, IsTrue());
            AssertThat(it_a_begin!=it_b_begin, IsTrue());
            AssertThat(it_a_begin!=it_b_end, IsTrue());
            AssertThat(it_a_last!=it_b_begin, IsTrue());
            AssertThat(it_a_last!=it_b_end, IsTrue());
            AssertThat(it_b_begin!=it_b_end, IsTrue());

            AssertThat(distance(it_a_begin, it_a_last), Equals(2));
            AssertThat(distance(it_a_begin, it_b_begin), Equals(3));
            AssertThat(distance(it_a_begin, it_b_last), Equals(4));
            AssertThat(distance(it_a_begin, it_b_end), Equals(5));
            AssertThat(distance(it_b_begin, it_b_last), Equals(1));
            AssertThat(distance(it_b_begin, it_b_end), Equals(2));

            int s = 0;
            for (auto it=it_a_begin; it!=it_b_end; ++it)
            {
                ++s;
                AssertThat(*it, Equals(s));
            }
            AssertThat(s, Equals(5));
        });
    });
});
