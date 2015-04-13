SuperiorMySQL++
====================================================
Modern C++ wrapper for MySQL C API

**Author**: [Tomas Nozicka](https://github.com/tnozicka), [*Seznam.cz*](https://github.com/seznam)


# Features
 - Modern C++ (currently written in C++14)
 - Minimal overhead
 - Multi-statements queries
 - High performance conversions
 - Advanced prepared statements support with type checks and automatic, typesafe bindings
 - Configurable logging
 - Integrated connection pool
    - Connection management
    - Health checks
    - DNS change checking
 - Extensive and fully automated multi-platform tests (using Docker)

# Status
Currently, we have working library implementation internally at Seznam.cz. It is already used in production code with great results.

**We are commited to making this library opensource** as soon as we are able. There is still some work left on library documentation, refactoring code and decoupling tests and packaging from our internal architecture to fulfill the opensource and multi-platform requirements. 

# ETA
Rough estimate is May 2015.

# Preview

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
