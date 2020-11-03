### Timestamp

#### Timestamp field and conversion to unix time stamp. 

```c++
// Timestamp field
auto timestamp_field = model_item(key("timestamp"), tag("timestamp"), dbtype("TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP");

// MySQL conversion to unixtime 
auto unixtime_field_mysql = model_item(local<time_t>(), key("UNIX_TIMESTAMP(timestamp)"), tag("unixtime"), direction::read_only, create(false));

// SQLite conversion to unixtime
auto unixtime_field_sqlite = model_item(local<time_t>(), key("strftime('%s',timestamp)"), tag("unixtime"), direction::read_only, create(false));
```

### Relationship

As models take one row there is no clean solution for relations. 
Only one to one relation would be possible to implement for both writing and reading data.

Some ideas how to deal with relations:
- create view for one to one relation and data reading
- use more than one model. Data can be retrieved with custom sql statement with joins and imported to each model. 
