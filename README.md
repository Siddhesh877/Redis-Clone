# Redis-Clone
I am trying to build a redis like application in c++ but on a small scale using my knowledge of computer networks and data structure.

Why Redis is Faster??
    Redis uses RAM of the system instead of traditional disk database which is way more faster.

Redis is a single threaded application which leverages IO Multiplexing to hangle large amount of requests. Since multithreading involve deadlock detection and prevention overhead which can affect performance of redis significantly.

Since Redis store data in memory(RAM) it can store data in in-memory data structures like linked list and hash tables to access data more effiently.

There is one drawback the size of database cannot exceed the size of memory so mainly it is used for caching the data.

Why to build Redis?
    To understand TCP why, TCP provides a continuous stream of bytes. Unlike messages, a byte stream has no boundaries within it, which is a major difficulty in understanding TCP.

Protocol used:
    A simple text-based protocol where first 4 characters in buffer will tell the length of message followed by message itself.


References:
https://build-your-own.org/redis

https://youtu.be/WuwUk7Mk80E?si=ctP0YwXGlka45276

https://copyconstruct.medium.com/nonblocking-i-o-99948ad7c957

https://copyconstruct.medium.com/the-method-to-epolls-madness-d9d2d6378642