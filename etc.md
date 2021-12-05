
### Timestamp - dirty solution

Conversion to unix time stamp example

```c++
// Model
model m("test_timestamp");

// This is a normal text field with string local storage
// Field is read-only as we are writing from another item
auto& text_timestamp = m.emplace_back(
        key("timestamp"),
        direction::read_only,
        dbtype("TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP")
        );

// Write-only virtual field for writing timestamp as unixtime
auto& unixtime_write = m.emplace_back(
        dbm::local<std::string>(),
        dbm::key("timestamp"),
        dbm::direction::write_only, // Write-only
        dbm::valquotes(false),      // value quotes while writing disabled as we are writing functions
        dbm::create(false)          // will not be created in database - exists only in model
        );
```

MySQL specific unixtime read item
```c++
// MySQL read-only field for reading timestamp as unix time 
auto& unixtime_read = m.emplace_back(
        local<time_t>(), 
        key("UNIX_TIMESTAMP(timestamp)"), 
        tag("unixtime"), 
        direction::read_only,       // read-only 
        create(false)               // will not be created in database - exists only in model
        );
```

SQLite specific unixtime read item
```c++
// SQLite read-only field for reading timestamp as unix time
auto unixtime_read = m.emplace_back(
        local<time_t>(), 
        key("strftime('%s',timestamp)"), 
        tag("unixtime"), 
        direction::read_only, 
        create(false)
        );
```

Reading data
```c++
// Data will be written in 'text_timestamp' as text formatted time and 'unixtime_read'
// while 'unixtime_write' remains unchanged
session >> m;
```

MySQL set unixitme
```c++
unixtime_write->set_value("FROM_UNIXTIME(12345678)");
```

SQLite set unixtime

```c++
unixtime_write->set_value("datetime(12345678, 'unixepoch')");
```

Write data

```c++
// Write value
m >> session;
// Read data into read-only fields
session >> m;
// unixtime_read value should be now 12345678, text_timestamp value should be '1970-05-23 21:21:18'
```

### Relationship

As models can take only one row at a time there is no clean solution for relations 
(only one to one relation would be possible to implement for both writing and reading data).

Some ideas how to deal with relations:
- create view for one to one relation and data reading
- use more than one model. Data can be retrieved with custom sql statement with joins and imported to each model. 


### To do

- binary, blob support
- pool, prepared statement, transaction docs
- Postgres
