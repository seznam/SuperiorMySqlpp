SuperiorMySQL++
====================================================
Modern C++ wrapper for MySQL C API

**Author**: [Tomas Nozicka](https://github.com/tnozicka), [*Seznam.cz*](https://github.com/seznam)


# Installation


## Requirements
 - C++14 compatible compiler (tested GCC>=4.9)
 - MySQL C API dev (libmysqlclient-dev)


## Bootstrap
We use git submodules for our dependencies and git requires you to initialize them manually.
```bash
git submodule update --init --recursive
```

## Build
This is header only library therefore no build step is required.

## Test
Tests require docker(>=1.5.0) for running mysql instances with testing data.
```bash
make -j32 test
```
You may (among other things) specify custom compiler and extra flags.
```bash
make -j32 test CXX=/opt/szn/bin/g++ CXXEF=-Werror LDEF=-Wl,-rpath=/opt/szn/lib/gcc/x86_64-linux-gnu/current
```

## Install
```bash
make -j32 DESTDIR=/usr/local/ install
```

# Packages
We support several packages to be build by default:
 - Debian Jessie
 - Fedora 22

## dbuild (docker-build)
This can build packages in completely clean environment using docker.
You might want to lower the `CONCURRENCY` in case you run out of memory.
```bash
make CONCURRENCY=32 package-fedora-22-dbuild
```
```bash
make CONCURRENCY=32 package-debian-jessie-dbuild
```
## build
Classic packaging.
```bash
make package-fedora-22-build
```
```bash
make package-debian-jessie-build
```



# Features
 - Modern C++ (currently written in C++14)
 - Minimal overhead
 - Multi-statement queries
 - High performance conversions
 - Advanced prepared statements support with type checks and automatic, typesafe bindings
 - Configurable logging
 - Integrated connection pool
    - Connection management
    - Health checks
    - DNS change checking
 - Extensive and fully automated multi-platform tests (using Docker)

# Status
Currently, it is already used at *Seznam.cz* in production code with great results.

The library is thoroughly tested and all tests are fully automated.

In future we are going to add more examples and documentation.

Please help us out by reporting bugs. (https://github.com/seznam/SuperiorMySqlpp/issues)

We appreciate your feedback!

# Preview
Until we create proper examples, you can see all functionality in action by looking at our tests (https://github.com/seznam/SuperiorMySqlpp/tree/master/tests).
Please be aware that tests must validate all possible cases and syntax and should not be taken as reference in these matters.

You can look at some basic examples below:

## Connection
```c++
Connection connection{"<database>", "<user>", "<password>", "<host>", "<port>"};
```

Connections are not thread-safe => use one connection per thread. If you intend to have multiple connection to one server consider using connection pool.

## Connection pool
```c++
auto connectionPool = makeConnectionPool([](){
    return std::make_shared<Connection>("<database>", "<user>", "<password>", "<host>", "<port>");
});
connectionPool.setMinSpare(10);  // optional
connectionPool.setMaxSpare(20);  // optional

// possibly set interval times and other things

connectionPool.startManagementJob();  // optional
connectionPool.startHealthCareJob();  // optional

connectionSharedPtr = connectionPool.get();
```


## Queries
### Simple result
```c++
auto query = connection.makeQuery("INSERT INTO ..");
query.execute();
```

### Store result
```c++
auto query = connection.makeQuery("SELECT * FROM ...");
query.execute();

auto result = query.store();
while (auto row = result.fetchRow())
{
    // process row
}
```

### Use result
```c++
auto query = connection.makeQuery("SELECT * FROM ...");
query.execute();

auto result = query.use();
while (auto row = result.fetchRow())
{
    // process row
}
```

### Escaping
To escape variable manually you may use method connection.escapeString. Preferred way is using query stream manipulators:
```c++
auto query = connection.makeQuery();
query << escape << "ab'cd";  // escape - next argument will be escaped

```

### Multi-statement queries
```c++
auto query = connection.makeQuery(
    "INSERT INTO ...;"
    "INSERT INTO ...;"
    "INSERT INTO ...;"
);
query.execute();
do {} while (query.nextResult());
```

## Prepared statement
Prepared statements **by default automatically check bound types and query metadata** and issue warnings or exceptions if you bound any incompatible types. All C API prepared statements variables types are supported and bindings are set using C++ type system.

These are in fact relatively simple examples. There are a lot of configurations for prepared statement including how strictly do you want to check metadata, allowing some types of implicit conversion and so on.
### Param bindings
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

### Result bindings
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

## Dynamic prepared statement
This type is for situations when you do not know which columns you are going to need at compile time.

### Param bindings
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

### Result bindings
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

## RowStreamAdapter
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

## Transactions
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

## Logging
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

# Current issues
There are problems caused by MySQL C API's bad design which are solved by https://github.com/seznam/SuperiorMySqlpp/blob/master/include/superior_mysqlpp/low_level/mysql_hacks.hpp. This is causing problems with MariaDB which stripped down some symbols from their shared object that we use to fix this bug. (https://github.com/seznam/SuperiorMySqlpp/issues/2)

## ABI tag warnings
GCC supports `-Wabi-tag` that should warn when a type with ABI tag is used in context that not have that ABI tag (https://gcc.gnu.org/onlinedocs/gcc-6.4.0/gcc/C_002b_002b-Dialect-Options.html#C_002b_002b-Dialect-Options). This warning should be used only for building shared libraries.

Mentioned compiler warning informs about situations where library may be theoretically successfully linked with another one built with incompatible ABI. For more info on this topic, see
 - https://developers.redhat.com/blog/2015/02/05/gcc5-and-the-c11-abi/
 - https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html

This warning is triggered many times in this library, for example for each function where `std::string` is returned from the function or for each structure that contains `std::string` -- generally, in each case where the type is not part of the resulting mangled name for given symbol.
Unfortunately, the only way to resolve this warning properly is explicitly put `abi_tag` attribute to every single affected symbol, which seems excessive and does not help readability.
ABI tag incompatibility is transitive to descendants -- that means when you build own library using *SuperiorMySqlpp* you shouldn't use `-Wabi-tag` either.

Notably, our internet research seem to suggest that this issue (linking code built with old and new ABI) is almost never occurring in practice, as all packages for given OS are by convention built with the same version.
Judging from the relative lack of related problems, tutorials or general discussion about this topic, `-Wabi-tag` seems to be generally unused by now.
