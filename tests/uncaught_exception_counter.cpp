/*
 *  Author: Tomas Nozicka
 */

#include <string>
#include <bandit/bandit.h>

#include <superior_mysqlpp/uncaught_exception_counter.hpp>


using namespace bandit;
using namespace snowhouse;
using namespace SuperiorMySqlpp;


template<typename Callable>
struct CustomDestructible
{
    Callable callable;

    template<typename C>
    explicit CustomDestructible(C&& callable)
        : callable{std::forward<C>(callable)}
    {}

    ~CustomDestructible() noexcept(noexcept(callable()))
    {
        callable();
    }
};


template<typename Callable>
auto makeCustomDestuctible(Callable&& callable)
{
    return CustomDestructible<std::remove_reference_t<Callable>>{std::forward<Callable>(callable)};
}

go_bandit([](){
    describe("Test UncaughtExceptionCounter", [&](){
        it("can count caught exceptions", [&](){
            UncaughtExceptionCounter counter{};

            AssertThat(uncaughtExceptions(), Equals(0));
            AssertThat(counter.isNewUncaughtException(), IsFalse());

            try
            {
                AssertThat(uncaughtExceptions(), Equals(0));
                AssertThat(counter.isNewUncaughtException(), IsFalse());
                throw std::exception{};
            }
            catch(std::exception&)
            {
                AssertThat(uncaughtExceptions(), Equals(0));
                AssertThat(counter.isNewUncaughtException(), IsFalse());

                try
                {
                    AssertThat(uncaughtExceptions(), Equals(0));
                    AssertThat(counter.isNewUncaughtException(), IsFalse());
                    throw std::exception{};
                }
                catch(std::exception&)
                {
                    AssertThat(uncaughtExceptions(), Equals(0));
                    AssertThat(counter.isNewUncaughtException(), IsFalse());
                }
            }
        });

        it("can count uncaught exceptions for one active exception", [&](){
            UncaughtExceptionCounter counter{};

            AssertThat(uncaughtExceptions(), Equals(0));
            AssertThat(counter.isNewUncaughtException(), IsFalse());

            /*
             * We must write our results in these variables since we are already unwinding
             * stack and failed assert would mean double exception situation => std::terminate
             */
            int uncaught = -1;
            bool newUncaught = false;
            try
            {
                auto something = makeCustomDestuctible([&](){
                    uncaught = uncaughtExceptions();
                    newUncaught = counter.isNewUncaughtException();
                });
                throw std::exception{};
            }
            catch(std::exception&)
            {
                AssertThat(uncaught, Equals(1));
                AssertThat(counter.isNewUncaughtException(), IsFalse());
                AssertThat(newUncaught, IsTrue());
            }
        });

        it("can count uncaught exceptions when unwinding stack", [&](){
            /*
             * Inner job doesn't throw
             */
            int uncaughtOuter = -1;
            int uncaughtInner = -1;
            bool newUncaughtOuter = false;
            bool newUncaughtInner = true;

            try
            {
                UncaughtExceptionCounter counterOuter{};
                auto something = makeCustomDestuctible([&](){
                    uncaughtOuter = uncaughtExceptions();
                    newUncaughtOuter = counterOuter.isNewUncaughtException();

                    try
                    {
                        UncaughtExceptionCounter counterInner{};
                        auto something = makeCustomDestuctible([&](){
                            uncaughtInner = uncaughtExceptions();
                            newUncaughtInner = counterInner.isNewUncaughtException();
                        });
                    }
                    catch(std::exception&)
                    {
                    }
                });
                throw std::exception{};
            }
            catch(std::exception&)
            {
                AssertThat(uncaughtOuter, Equals(1));
                AssertThat(newUncaughtOuter, IsTrue());
                AssertThat(uncaughtInner, Equals(1));
                AssertThat(newUncaughtInner, IsFalse());
            }

            /*
             * Inner job does throw
             */
            uncaughtOuter = -1;
            uncaughtInner = -1;
            newUncaughtOuter = false;
            newUncaughtInner = false;
            try
            {
                UncaughtExceptionCounter counterOuter{};
                auto something = makeCustomDestuctible([&](){
                    uncaughtOuter = uncaughtExceptions();
                    newUncaughtOuter = counterOuter.isNewUncaughtException();

                    try
                    {
                        UncaughtExceptionCounter counterInner{};
                        auto something = makeCustomDestuctible([&](){
                            uncaughtInner = uncaughtExceptions();
                            newUncaughtInner = counterInner.isNewUncaughtException();
                        });
                        throw std::exception{};
                    }
                    catch(std::exception&)
                    {
                    }
                });
                throw std::exception{};
            }
            catch(std::exception&)
            {
                AssertThat(uncaughtOuter, Equals(1));
                AssertThat(newUncaughtOuter, IsTrue());
                AssertThat(uncaughtInner, Equals(2));
                AssertThat(newUncaughtInner, IsTrue());
            }
        });
    });
});
