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

Data Structure used to store key-value pair
    I have used map initially but for more efficiency I am using hash table.
    Hash table is basiclly an array of fixed number of slots. A key is hashed and the slot index is typically just "hash(key)%size" OR "hash(key) & (size-1)"
    To resolve the issue of collision there are 2 ways: 1.Open addressing: If a slot is occupied then find other slot for value.
    2.Chaining: store collection of values which have colliding keys.

    I am using chaining hash table.
    load factor = (number of keys)/(number of slots)

How to compile server and run?
    g++ server.cpp hashtable.cpp -o server
    ./server

Serilization
    Searlization is the process of converting an object or data structure into format that can easily stored or transmitted and then reconstructed later.This is crucial for data exchange especially between different platforms or different languages.
    The serilization scheme that i am using can be summarized as 'Type Length Value'(TLV):
        Type: the type of value.
        Length: length of data for strings or arrays.
        Value: at last value is encoded.
    Advantage of TLV:1. can be decoded without a schema like JSON of XML.
    2. It can encode arbitrarily nested data.


References:
https://build-your-own.org/redis

https://youtu.be/WuwUk7Mk80E?si=ctP0YwXGlka45276

https://copyconstruct.medium.com/nonblocking-i-o-99948ad7c957

https://copyconstruct.medium.com/the-method-to-epolls-madness-d9d2d6378642