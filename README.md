# dbm

[![Build Status](https://travis-ci.org/ipeperko/dbm.svg?branch=master)](https://travis-ci.org/ipeperko/dbm)

Simple database table models with serialization support written in C++. 

- currently supported databases: SQLite, MySQL
- requires C++17
- build-in lightweight xml serializer
- MIT licence

Note : this library is under development and subject to change. Contributions are welcome.

### Example

Writing and reading data from database into model

```c++
using namespace dbm;

try 
{
    sqlite_session session("hello_dbm.sqlite");
    session.open();
    
    std::string name = "Rambo";
    double score = 42;

    auto name_validator = [](const std::string& s) {
        return !s.is_empty();
    };
    
    model m("test_table",
            {
                { key("id"),    local<int>(1),                  primary(true)   },
                { key("name"),  binding(name, name_validator)                   },
                { key("score"), binding(score)                                  }
            });
            
    // insert or update existing record
    m >> session; 
    
    // read database and insert data into model
    session >> m;
}
catch (std::exception& e)
{
    std::cout << "exception: " << e.what() << "\n";
}
```

### Serializers

##### Build-in XML serializer
```c++
xml_serilizer xml;

// read data from database and serialize
session >> m >> xml;
std::cout << xml.object().to_string(4) << "\n";  

xml.set("name", "Terminator");
xml.set("score", 43);

// deserialize data and write Terminator 43 to database
xml >> m >> session;
```

Note that build-in xml serializer is not fully compatible with the standard.
But if you are ok with lightweight version without comments, namespaces, schemas it works well.

##### Writing custom serializers
See example [nlohmann json serializer](https://github.com/ipeperko/dbm/blob/master/include/nlohmann_json_serializer.hpp)  

See also [nlohmann json](https://github.com/nlohmann/json) project.

### Models

##### Accessing and modifying model items

```c++
model m;

// set table name
m.set_table_name("my_table");

// adding items
m.emplace_back();
m.emplace_back(key("id"));
m.emplace_back( /* add parameters in any order except null and defined should be placed after container */  
        key("name"), tag("Name"), taggable(true), primary(true), required(false), 
        local<int>(), null(true), defined(true)); 

// accessing items through key name 
auto& item3 = m.at("name");     // same as m.at(key("id"))

// accessing items from items vector
auto& item1 = m.items().at(0);
auto& item2 = m.items()[1]; 

// access through iterators
auto it = m.find("name");
auto itb = m.begin();           // same as m.items().begin() 
auto itb = m.end();             // same as m.items().end() 

// remove item
m.erase("name"); // same as m.erase(key("name"))
```

##### Model item parameters overview

Type | Default | Description
--- | --- | ---
key | | Database column name
tag | | Optional tag for serialization. If not defined it will be serialized with same name as 'key'.
primary | false | Defines if database column is primary key.
taggable | true | Defines if item will be serialized.
defined | false | Defines if value has been set. If 'defined' is false it won't be written to db or serializer.
null | true | Defines if value is null.
required | false | Defines if value 'defined' should always be true. Otherwise exception is thrown.
direction | bidirectional | Item configuration for database write/read (bidirectional, read_only, write_only)
local | | Value container with local storage of any supported type.
binding | | Value container with binding.

#### Value container

Library supports containers with local storage and binding containers.

```c++
std::string name;
m.item("name").set(local<std::string>());   // replace container with new local storage
m.item("name").set(binding(name));          // replace with new binding container
```

##### Supported types

- bool
- int
- short
- long
- double
- std::string

##### Binding enums

Binding enums is not directly supported. 
One soulution is to use c style cast with std::underlying_type_t.

```c++
enum class MyEnum : int { one=1, two, three };
MyEnum em;

auto cont = binding((std::underlying_type_t<MyEnum>&) (em));
cont->set(2); // or
cont->set(static_cast<std::underlying_type_t<MyEnum>>(MyEnum::two));
```

##### Parameters 'null' and 'defined'

Storage type        | null      | defined 
----                | ----      | ---- 
Local storage | null      | not defined  
Local storage with value | not null      | defined
Binding             | not null  | defined
Value set success           | null / not null  | defined
Value set failed            | null      | not defined
From string succeeded    | not null  | defined
From string failed          | null      | not defined

```c++
local<int>();                               // null, not defined
local<int>(1);                              // not null, defined
binding<int>(my_int);                       // not null, defined
binding<int>(my_int, nullptr, true, false); // null, not defined (arguments : reference, validator, null, defined)
local<int>()->set(null(false));             // not null
local<int>()->set(defined(true));           // defined
```

##### Validators

```c++
local<int>( [](int val) { return val > 0; } );
local<int>(1, [](int val) { return val > 0; } );    // with default value

std::string name;
auto validate_string = [](const std::string& s) { return !s.empty(); };
binding(name, validate_string );
```

##### Multiple rows result set

Alternatively you can retrieve data from database and pass separate rows to model. 

```c++
sql_rows rows = session.select("SELECT * FROM test_table");
for (const sql_row& row : rows) {
    m << row;         // import data to model
}
```

Note that sql_rows do not store values. Field values are pointers to session internal result set.
If you want to store data for further work use sql_rows_dump.

```c++
sql_rows_dump data = rows;          // store data
sql_rows rows2 = data.restore();    // restore data
```

### Thread safety

dbm classes are not thread safe and should not be used concurrently. 
One solution is to use separate model and session objects for each thread.

### Build

```Batchfile
mkdir build
cmake ..
make
make install
```
CMake automatically searches for dependent libraries mysqlclient and sqlite3 and compiles each driver only if found on the system.

To build tests add -DDBM_BUILD_TESTS=ON to cmake command.

### Exceptions

It is possible to specify your own exception function. 
Note that custom exception must inherit from std::exception.

```c++
config::set_custom_exception([](const std::string& msg) {
    throw MyCustomException(msg);
});

config::set_custom_exception(nullptr); // library default exceptions
``` 

### Todo

- blob support
- sql aliases
- timestamp to unix time conversion
- sql joins
- table creation from models 
- Postgres

