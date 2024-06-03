# Redis-Clone
I am trying to build a redis like application in c++ but on a small scale using my knowledge of computer networks and data structure.

Why Redis is Faster??
    Redis uses RAM of the system instead of traditional disk database which is way more faster.

Redis is a single threaded application which leverages IO Multiplexing to hangle large amount of requests. Since multithreading involve deadlock detection and prevention overhead which can affect performance of redis significantly.

Since Redis store data in memory(RAM) it can store data in in-memory data structures like linked list and hash tables to access data more effiently.

There is one drawback the size of database cannot exceed the size of memory so mainly it is used for caching the data.
