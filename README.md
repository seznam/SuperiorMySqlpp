# SuperiorMySQL++

- [![Build Status](https://travis-ci.org/seznam/SuperiorMySqlpp.svg?branch=devel)](https://travis-ci.org/seznam/SuperiorMySqlpp)
- [![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v3-blue.svg)](LICENSE)

Modern C++ wrapper for MySQL C API

**Author**: [Tomas Nozicka](https://github.com/tnozicka), [*Seznam.cz*](https://github.com/seznam)

Note: This library currently doesn't support MySQL library version 8 and newer.

## Features

- Modern C++ (currently written in C++14)
- Minimal overhead
- Header only
- Multi-statement queries
- High performance conversions
- Advanced prepared statements support with type checks and automatic, typesafe bindings
- Configurable logging
- Integrated connection pool
  - Connection management
  - Health checks
  - DNS change checking
- Extensive and fully automated multi-platform tests (using Docker)

## Status

Currently, it is already used at *Seznam.cz* in production code with great results.

The library is thoroughly tested and all tests are fully automated.

In future we are going to add more examples and documentation.

Please help us out by reporting bugs. (<https://github.com/seznam/SuperiorMySqlpp/issues>)

We appreciate your feedback!

## Installation

### Requirements

- C++14 compatible compiler (tested GCC>=4.9)
- MySQL C API dev (libmysqlclient-dev)

### Additional requirements for testing

- socat
- Boost System [for TEST_EXTENDED only] (libboost-system-dev)

### Bootstrap

We use git submodules for our dependencies and git requires you to initialize them manually.

Submodules are currently used only for testing, so initializing is optional.

```bash
git submodule update --init --recursive
```

### Test

Tests require docker(>=1.5.0) for running mysql instances with testing data.

Tests are disabled by default, you can enable them by adding `-DTEST_ENABLE=TRUE` to cmake comamnd.

Following additional sub-tests are enabled by default: `TEST_ODR` and `TEST_EXTENDED`.

```bash
mkdir build && cd build
cmake .. -DTEST_ENABLE=TRUE
cmake --build . -j <number of concurrent jobs>
```

## Preview

Until we create proper examples, you can see all functionality in action by looking at our tests (<https://github.com/seznam/SuperiorMySqlpp/tree/devel/tests>).

Please be aware that tests must validate all possible cases and syntax and should not be taken as reference in these matters.

You can look at some basic examples below:

### Connection

Connection can be constructed either by passing all necessary arguments directly to `Connection` constructor or by passing `ConnectionConfiguration` object.

```c++
Connection connection{"<database>", "<user>", "<password>", "<host>", "<port>"};
```

`ConnectionConfiguration` contains static factory methods used to build proper connection configuration for desired purpose, e.g. encrypted connection over TCP or connection over Unix socket.

```c++
auto tcpConfig = ConnectionConfiguration::getTcpConnectionConfiguration("<database>", "<user>", "<password>", "<host>", "<port>");
auto tcpSslConfig = ConnectionConfiguration::getSslTcpConnectionConfiguration(SslConfiguration{}, "<database>", "<user>", "<password>", "<host>", "<port>");
auto socketConfig = ConnectionConfiguration::getSocketConnectionConfiguration("<database>", "<user>", "<password>", "<socket>");
auto socketSslConfig = ConnectionConfiguration::getSslSocketConnectionConfiguration(SslConfiguration{}, "<database>", "<user>", "<password>", "<socket>");

Connection connection{tcpConfig};
```

Connections are not thread-safe => use one connection per thread. If you intend to have multiple connection to one server consider using connection pool.

### Connection pool

```c++
auto connectionPool = SuperiorMySqlpp::makeConnectionPool([](){
    return std::async(std::launch::async,
        [&]() {
            uint16_t port = 3306;
            return std::make_shared<SuperiorMySqlpp::Connection>(
                "<database>", "<user>", "<password>", "<host>", port);
        }
    );
});
connectionPool.setMinSpare(10);  // optional
connectionPool.setMaxSpare(20);  // optional

// possibly set interval times and other things

// Starts the parallel job to keep the number of connections in given range
connectionPool.startResourceCountKeeper();  // optional
// Starts the parallel job to auto-restart broken connections
connectionPool.startHealthCareJob();  // optional

std::shared_ptr<SuperiorMySqlpp::Connection> connection = connectionPool.get();
```

### Queries

#### Simple result

```c++
auto query = connection.makeQuery("INSERT INTO ..");
query.execute();
```

#### Store result

```c++
auto query = connection.makeQuery("SELECT * FROM ...");
query.execute();

auto result = query.store();
while (auto row = result.fetchRow())
{
    // process row
}
```

#### Use result

```c++
auto query = connection.makeQuery("SELECT * FROM ...");
query.execute();

auto result = query.use();
while (auto row = result.fetchRow())
{
    // process row
}
```

#### Escaping

To escape variable manually you may use method `connection.escapeString`. Preferred way is using query stream manipulators:

```c++
auto query = connection.makeQuery();
query << escape << "ab'cd";  // escape - next argument will be escaped

```

#### Multi-statement queries

```c++
auto query = connection.makeQuery(
    "INSERT INTO ...;"
    "INSERT INTO ...;"
    "INSERT INTO ...;"
);
query.execute();
do {} while (query.nextResult());
```

### Prepared statement

Prepared statements **by default automatically check bound types and query metadata** and issue warnings or exceptions if you bound any incompatible types. All C API prepared statements variables types are supported and bindings are set using C++ type system.

These are in fact relatively simple examples. There are a lot of configurations for prepared statement including how strictly do you want to check metadata, allowing some types of implicit conversion and so on.

#### Param bindings

```c++
// type of prepared statements parameters is deduced automatically from arguments
auto preparedStatement = connection.makePreparedStatement(
    "INSERT INTO ... VALUES (?)", 0
);
preparedStatement.execute();

// or if you want multiple inserts
for (auto i=1; i<10; ++i)
{
    std::get<0>(preparedStatement.getParams()) = i
    preparedStatement.execute();
}
```

#### Result bindings

```c++
auto preparedStatement = connection.makePreparedStatement<ResultBindings<Sql::Int, Sql::Int>>(
    "SELECT `id`, `money` FROM ..."
);
preparedStatement.execute();
while (preparedStatement.fetch())
{
    // you can use std::tie
    Sql::Int id, money;
    std::tie(id, money) = preparedStatement.getResult();

    // or directly use e.g. id as:
    preparedStatement.getResult().get<0>()
}
```

### Dynamic prepared statement

This type is for situations when you do not know which columns you are going to need at compile time.

#### Param bindings

```c++
auto preparedStatement = connection.makeDynamicPreparedStatement(
        "INSERT INTO ... VALUES (?)"
);
for (auto id=0; i<10; ++i)
{
    preparedStatement.bindParam(0, id);
    preparedStatement.updateParamBindings();
    preparedStatement.execute();
}
```

#### Result bindings

```c++
auto preparedStatement = connection.makeDynamicPreparedStatement(
        "SELECT `id` FROM ..."
);
preparedStatement.execute();
int id = -1;
preparedStatement.bindResult(0, id);
preparedStatement.updateResultBindings();
while (preparedStatement.fetch())
{
    // do something with id
}
```

#### Convenience read functions

##### psParamQuery

Invokes psQuery with param setter only.

```c++
psParamQuery(connection, "INSERT INTO ... (col1, col2, ...) VALUES (?, ?, ...)", [&](T1 &col1, T2& col2, ...) -> bool {
    col1 = ...;
    col2 = ...;
    return true; // Or false, if we want to stop
});
```

##### psResultQuery

```c++
psResultQuery(connection, "SELECT ... FROM ...", <Callable>);
```

Where callable can be C function, lambda, or member function, however in the last case you need to use
wrapper, for example wrapMember function (located in *superior_mysqlpp/extras/member_wrapper.hpp*).

```c++
psResultQuery(connection, "SELECT ... FROM ...", [&](int arg1, int arg2){});
```

```c++
void processRow(int arg1, int arg2) {}
psResultQuery(connection, "SELECT ... FROM ...", &processRow);
```

```c++
class ProcessingClass {
public:
    void processRow(int arg1, int arg2) {}
};

ProcessingClass pc;
psResultQuery(connection, "SELECT ... FROM ...", wrapMember(&pc, &ProcessingClass::processRow));
```

This method doesn't throw exceptions, however query execution and row fetching can still fail,
resulting in exception.

##### psReadValues

```c++
auto preparedStatement = connection.makePreparedStatement<ResultBindings<Sql::Int, Sql::Int>>("SELECT ... FROM ...");
int arg1, arg2;

psReadValues(preparedStatement, arg1, arg2);
```

or

```c++
int arg1, arg2;
psReadValues("SELECT ... FROM ...", connection, arg1, arg2);
```

Note: This function is made only for reading single row. In case you are reading more than one row,
an *UnexpectedRowCountError* exception is thrown.

##### sQuery

```c++
int myData = 0;

psQuery(
    connection,
    "SELECT ?",
    [&](int &value) -> bool {
        value = myData;
        return (myData++) < 5; // Return true, if data are set, false otherwise (no more input data available)
    },
    [&](int value) {
        printf("Got value: %d\n", value);
    }
)
```

### RowStreamAdapter

Syntactic sugar is provided for extracting values from `Row` using a familiar stream operator.
When a NULL value is encountered, value is default-constructed.
Extracting non-existent values is undefined behaviour.

```c++
auto row = ...
std::string s;
int i = 0;
Extras::RowStreamAdapter {row}
    >> s
    >> i
    ;
```

### Transactions

Library automatically detects exceptions and does commit or rollback as appropriate.

(This is actually quite sophisticated since you must detect if the exception occurred between transaction ctor and dtor (even if there is already active exception). C++17 helps us by introducing std::uncaught_exceptions (instead of std::uncaught_exception), but today we are forced to use internal compiler structures.)

```c++
{
    Transaction transaction{connection};
    connection.makeQuery("INSERT INTO ...;").execute();
}
// or you can specify transaction characteristics or isolation level
{
    Transaction transaction{connection, TransactionCharacteristics::ReadOnly, IsolationLevel::RepeatableRead};
    connection.makeQuery("INSERT INTO ...;").execute();
}
```

### Logging

The library has build-in support for custom logging. In default configuration it logs only warnings and errors to std::cerr.

You may choose some of the library predefined loggers:

```c++
// Log all event to std::cout and std::cerr
auto&& logger = std::make_shared<Loggers::Full>();
DefaultLogger::setLoggerPtr(std::move(logger));
```

Or define your own:

```c++
class MyLogger final : public Loggers::Base
{
    using Base::Base;
    virtual ~MyLogger() override
    {
        // do something
    }

    virtual void logWarning(const std::string& message) const override
    {
        // do something
    }

    // a lot of logging methods like: logMySqlConnecting, logMySqlConnected, logMySqlClose, logMySqlCommit, ...
}

auto&& logger = std::make_shared<MyLogger>();
DefaultLogger::setLoggerPtr(std::move(logger));
```

## Current issues

There are problems caused by MySQL C API's bad design which are solved by <https://github.com/seznam/SuperiorMySqlpp/blob/master/include/superior_mysqlpp/low_level/mysql_hacks.hpp>. This is causing problems with MariaDB which stripped down some symbols from their shared object that we use to fix this bug. (<https://github.com/seznam/SuperiorMySqlpp/issues/2>)

### ABI tag warnings

GCC supports `-Wabi-tag` that should warn when a type with ABI tag is used in context that not have that ABI tag (<https://gcc.gnu.org/onlinedocs/gcc-6.4.0/gcc/C_002b_002b-Dialect-Options.html#C_002b_002b-Dialect-Options>). This warning should be used only for building shared libraries.

Mentioned compiler warning informs about situations where library may be theoretically successfully linked with another one built with incompatible ABI. For more info on this topic, see

- <https://developers.redhat.com/blog/2015/02/05/gcc5-and-the-c11-abi/>
- <https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html>

This warning is triggered many times in this library, for example for each function where `std::string` is returned from the function or for each structure that contains `std::string` -- generally, in each case where the type is not part of the resulting mangled name for given symbol.
Unfortunately, the only way to resolve this warning properly is explicitly put `abi_tag` attribute to every single affected symbol, which seems excessive and does not help readability.
ABI tag incompatibility is transitive to descendants -- that means when you build own library using *SuperiorMySqlpp* you shouldn't use `-Wabi-tag` either.

Notably, our internet research seem to suggest that this issue (linking code built with old and new ABI) is almost never occurring in practice, as all packages for given OS are by convention built with the same version.
Judging from the relative lack of related problems, tutorials or general discussion about this topic, `-Wabi-tag` seems to be generally unused by now.

### MariaDB compatibility

MariaDB connector/C 10.2 upwards can be used instead of MySQL connector/C.

Older versions are not supported due to some issues, for instance:

- memory leaks when ConnectionPool is used and cannot be handled by `mysql_hacks.hpp` because missing symbols
- missing `MARIADB_VERSION_ID` -- we are not able detect whether MariaDB is used
- failing truncation detection test (depending on used version)

#### Known bugs

MariaDB 10.2.8 has a broken MySQL compatibility symlink (libmysqlclient.so), therefore you need to link directly with MariaDB client lib (`-lmariadb`) instead of using usual symlink (`-lmysqlclient`).
