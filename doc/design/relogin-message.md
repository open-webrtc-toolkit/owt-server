Server and client both maintain an **internal sequence number** for
sent/received messages once a connection established.

For Server:

1.  After a client has logged in, allocate a sequence number with
    initial value 0 for this connection.

2.  Once a message is sent (no matter success or not), bind the message
    with current sequence number and time, save them in a buffer.
    Increase the current sequence number by 1. Discard the old messages after
    a certain period if necessary.

3.  Once client “relogin”, sent those messages with sequence number in
    buffer in response.

For Client:

1.  After login succeed, allocate a sequence number with initial
    value 0.

2.  Once a message is received, increase the current sequence number
    by 1.

3.  Once client “relogin” succeed, filter messages with sequence
    number &gt;= client sequence number, process those messages in
    order. Update its own current sequence number.


