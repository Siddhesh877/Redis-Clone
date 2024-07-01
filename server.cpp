#include <iostream>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <vector>
#include <sys/epoll.h>
#include <string>
#include<map>
#include "hashtable.h"

#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) );})


static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static bool cmd_is(const std::string &word,const char *cmd)
{
    if(0 == strcasecmp(word.c_str(),cmd))
    {
        return true;
    }
    return false;
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}

const size_t k_max_msg = 4096;

enum 
{
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,  // mark the connection for deletion
};

enum
{
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

//the structure for the key
static struct
{
    HMap db;
}g_data;

//structure for the key
struct Entry
{
    struct HNode node;
    std::string key;
    std::string val;
};

static bool entry_eq(HNode *lhs,HNode *rhs)
{
    struct Entry *le = container_of(lhs,struct Entry,node);
    struct Entry *re = container_of(rhs,struct Entry,node);
    return le->key == re->key;
}

static uint64_t str_hash(const uint8_t *data,size_t len)
{
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

static uint32_t do_get(std::vector<std::string> &cmd,uint8_t *res,uint32_t *reslen)
{
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(),key.key.size());

    HNode *node = hm_lookup(&g_data.db,&key.node,&entry_eq);
    if(!node)
    {
        return RES_NX;
    }

    const std::string &val = container_of(node,Entry,node)->val;
    assert(val.size()<=k_max_msg);
    memcpy(res,val.data(),val.size());
    *reslen = (uint32_t)val.size();
    return RES_OK;
}

static uint32_t do_set(std::vector<std::string> &cmd,uint8_t *res,uint32_t *reslen)
{
    (void) res;
    (void) reslen;

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if (node) {
        container_of(node, Entry, node)->val.swap(cmd[2]);
    } else {
        Entry *ent = new Entry();
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        ent->val.swap(cmd[2]);
        hm_insert(&g_data.db, &ent->node);
    }
    return RES_OK;
}

static uint32_t do_del(std::vector<std::string> &cmd,uint8_t *res, uint32_t *reslen)
{
    (void)res;
    (void)reslen;

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_pop(&g_data.db, &key.node, &entry_eq);
    if (node) {
        delete container_of(node, Entry, node);
    }
    return RES_OK;
}


struct Conn {
    int fd = -1;
    uint32_t state = 0;     // either STATE_REQ or STATE_RES
    // buffer for reading
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];
    // buffer for writing
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

struct ServerContext {
    int epoll_fd;
    std::vector<Conn *> fd2conn;
};

static void conn_put(ServerContext &ctx, struct Conn *conn) {
    if (ctx.fd2conn.size() <= (size_t)conn->fd) {
        ctx.fd2conn.resize(conn->fd + 1);
    }
    ctx.fd2conn[conn->fd] = conn;
}

static int32_t accept_new_conn(ServerContext &ctx, int fd) {
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1;  // error
    }

    // set the new connection fd to nonblocking mode
    fd_set_nb(connfd);
    // creating the struct Conn
    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
    if (!conn) {
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(ctx, conn);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = connfd;
    if (epoll_ctl(ctx.epoll_fd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
        die("epoll_ctl: connfd");
    }

    return 0;
}

static void state_req(Conn *conn);
static void state_res(Conn *conn);

//data structure for the key space for now using map.
static std::map<std::string,std::string> g_map;

// static uint32_t do_get(const std::vector<std::string> &cmd, uint8_t *res,uint32_t *reslen)
// {
//     if(g_map.count(cmd[1])==0)
//     {
//         return RES_NX;
//     }
//     std::string &val = g_map[cmd[1]];//doubt
//     assert(val.size()<= k_max_msg);
//     memcpy(res,val.data(),val.size());
//     *reslen = (uint32_t)val.size();
//     return RES_OK;
// }

// static uint32_t do_set(const std::vector<std::string> &cmd,uint8_t *res,uint32_t *reslen)
// {
//     (void)res;
//     (void)reslen;
//     g_map[cmd[1]]=cmd[2];
//     return RES_OK;
// }
// static uint32_t do_del(const std::vector<std::string> &cmd,uint8_t *res,uint32_t *reslen)
// {
//     (void)res;
//     (void)reslen;
//     g_map.erase(cmd[1]);
//     return RES_OK;
// }

static int32_t parse_req(const uint8_t *data, size_t len,std::vector<std::string> &out)
{
    if(len<4)
    {
        return -1;
    }
    uint32_t n=0;
    memcpy(&n,&data[0],4);//doubt
    if(n>k_max_msg)
    {
        return -1;
    }

    size_t pos = 4;
    while(n--)
    {
        if(pos+4 > len)
        {
            return -1;
        }
        uint32_t sz = 0;
        memcpy(&sz,&data[pos],4);
        if(pos+4+sz > len)
        {
            return -1;
        }
        out.push_back(std::string((char *)&data[pos+4],sz));//doubt
        pos+= 4+sz;
    }
    if(pos!=len)
    {
        return -1; //trailing garbage
    }
    return 0;
}

static int32_t do_request(const uint8_t *req,uint32_t reqlen,uint32_t *rescode,uint8_t *res, uint32_t *reslen)
{
    std::vector<std::string> cmd;
    if(0!=parse_req(req,reqlen,cmd))
    {
        msg("bad req");
        return -1;
    }
    if(cmd.size()==2 && cmd_is(cmd[0],"get"))
    {
        *rescode = do_get(cmd,res,reslen);
    }
    else if(cmd.size()==3 && cmd_is(cmd[0],"set"))
    {
        *rescode = do_set(cmd,res,reslen);
    }
    else if(cmd.size()==2 && cmd_is(cmd[0],"del"))
    {
        *rescode = do_del(cmd, res,reslen);
    }
    else
    {
        //cmd is not recognized
        *rescode = RES_ERR;
        const char *msg = "Unknown cmd";
        strcpy((char *)res,msg); //doubt
        *reslen = strlen(msg);
        return 0;
    }
    return 0;

}

static bool try_one_request(Conn *conn) {
    // try to parse a request from the buffer
    if (conn->rbuf_size < 4) {
        // not enough data in the buffer. Will retry in the next iteration
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg) {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }
    if (4 + len > conn->rbuf_size) {
        // not enough data in the buffer. Will retry in the next iteration
        return false;
    }

    // got one request, do something with it
    // printf("client says: %.*s\n", len, &conn->rbuf[4]);

    // generating echoing response
    // memcpy(&conn->wbuf[0], &len, 4);
    // memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    // conn->wbuf_size = 4 + len;

    //handle the request, generate response
    uint32_t rescode = 0;
    uint32_t wlen = 0;
    int32_t err = do_request(&conn->rbuf[4], len, &rescode,&conn->wbuf[4+4],&wlen);
    if(err)
    {
        conn->state = STATE_END;
        return false;
    }
    wlen += 4;
    memcpy(&conn->wbuf[0], &wlen, 4);
    memcpy(&conn->wbuf[4], &rescode, 4);
    conn->wbuf_size = 4 + wlen;


    // remove the request from the buffer.
    // note: frequent memmove is inefficient.
    // note: need better handling for production code.
    size_t remain = conn->rbuf_size - 4 - len;
    if (remain) {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    // change state
    conn->state = STATE_RES;
    state_res(conn);

    // continue the outer loop if the request was fully processed
    return (conn->state == STATE_REQ);
}

static bool try_fill_buffer(Conn *conn) {
    // try to fill the buffer
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
        return false;
    }
    if (rv < 0) {
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            msg("unexpected EOF");
        } else {
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    // Try to process requests one by one.
    // Why is there a loop? Please read the explanation of "pipelining".
    while (try_one_request(conn)) {}
    return (conn->state == STATE_REQ);
}

static void state_req(Conn *conn) {
    while (try_fill_buffer(conn)) {}
}

static bool try_flush_buffer(Conn *conn) {
    ssize_t rv = 0;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
        return false;
    }
    if (rv < 0) {
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);
    if (conn->wbuf_sent == conn->wbuf_size) {
        // response was fully sent, change state back
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }
    // still got some data in wbuf, could try to write again
    return true;
}

static void state_res(Conn *conn) {
    while (try_flush_buffer(conn)) {}
}

static void connection_io(Conn *conn) {
    if (conn->state == STATE_REQ) {
        state_req(conn);
    } else if (conn->state == STATE_RES) {
        state_res(conn);
    } else {
        assert(0);  // not expected
    }
}




int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(3000);
    addr.sin_addr.s_addr = INADDR_ANY;
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    // set the listen fd to nonblocking mode
    fd_set_nb(fd);

    // create epoll instance
    ServerContext ctx;
    ctx.epoll_fd = epoll_create1(0);
    if (ctx.epoll_fd == -1) {
        die("epoll_create1");
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(ctx.epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        die("epoll_ctl: listen_sock");
    }

    // the event loop
    std::vector<struct epoll_event> events(16);
    while (true) {
        int nfds = epoll_wait(ctx.epoll_fd, events.data(), events.size(), -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            die("epoll_wait");
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == fd) {
                // new connection
                accept_new_conn(ctx, fd);
            } else {
                // existing connection
                Conn *conn = ctx.fd2conn[events[i].data.fd];
                connection_io(conn);
                if (conn->state == STATE_END) {
                    // client closed normally, or something bad happened.
                    // destroy this connection
                    ctx.fd2conn[conn->fd] = NULL;
                    close(conn->fd);
                    free(conn);
                } else {
                    struct epoll_event ev;
                    ev.events = (conn->state == STATE_REQ) ? EPOLLIN : EPOLLOUT;
                    ev.events |= EPOLLET;
                    ev.data.fd = conn->fd;
                    if (epoll_ctl(ctx.epoll_fd, EPOLL_CTL_MOD, conn->fd, &ev) == -1) {
                        die("epoll_ctl: conn_fd");
                    }
                }
            }
        }
    }

    close(fd);
    close(ctx.epoll_fd);
    return 0;
}
