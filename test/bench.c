/*
 * bench.c
 *
 *  Created on: Jan 23, 2014
 *      Author: fan
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <hiredis/hiredis.h>



int parse_conf(const char* path);
void sync_send();
void async_send();

struct server_addr {
    char addr[32];
    uint16_t port;
};

struct server_addr server_list[5];
int server_list_len = 0;
int size = 10000;
float delay = 1;

int parse_conf(const char* path)
{
    FILE* fp = NULL;
    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("open conf");
        return -1;
    }
    char buf[32] = {0};
    int i = 0;
    while (fgets(buf, 32, fp) != NULL) {
        char* addr = strtok(buf, ":\n");
        strncpy(server_list[i].addr, addr, sizeof(server_list[i]));
        char* port = strtok(NULL, ":\n");
        server_list[i].port = atoi(port);
        i++;
    }
    return i;
}

void sync_send()
{
    redisContext* c = NULL;
    redisReply* reply;

    int i = 0;
    for (;;) {

        i = (i + 1) % server_list_len;
        sleep(1);
        // connect idgen server
        char* addr = server_list[i].addr;
        uint16_t port = server_list[i].port;
        printf("Trying to connect to %s:%d\n", addr, port);
        c = redisConnect(addr, port);
        if (c == NULL || c->err) {
            if (c) {
                printf("Connection error:%s\n", c->errstr);
                redisFree(c);
            }
            continue;
        }

        // reset a and connect to leader
        printf("Connected.\n");
        printf("Reset key a = 0\n");
        reply = redisCommand(c, "SET a 0");
        if (reply->type == REDIS_REPLY_ERROR) {
            printf("%s\n", reply->str);
            freeReplyObject(reply);
            redisFree(c);
            continue;
        }
        printf("%s\n", reply->str);
        freeReplyObject(reply);

        int cnt = 0;
        // loop send
        for (;;) {
            reply = redisCommand(c, "INCR a");
            if (reply == NULL) {
                break;
            }

            if (reply->type == REDIS_REPLY_ERROR) {
                printf("%s\n", reply->str);
                freeReplyObject(reply);
                redisFree(c);
                continue;
            } else {
                if (++cnt >= size) {
                    printf("%"PRIu64"\n", reply->integer);
                    cnt = 0;
                }
                freeReplyObject(reply);
            }
        }
    }
}

void async_send()
{

}

int main(int argc, char* argv[])
{
    char conf[32] = "raft.conf";
    int async  = 0;

    int o;
    while ((o = getopt(argc, argv, "s:d:f:ah")) != -1) {
        switch (o) {
        case 's':
            sscanf(optarg, "%d", &size);
            break;
        case 'd':
            sscanf(optarg, "%f", &delay);
            break;
        case 'f':
            strncpy(conf, optarg, 32);
            break;
        case 'a':
            async = 1;
            break;
        case 'h':
            printf("useage: %s -s print_size -d delay_in_ms -f conf_file\n", argv[0]);
            return 0;
        }
    }
    printf("size:%d\n", size);
    printf("delay:%f ms\n", delay);
    printf("conf:%s\n", conf);

    if ((server_list_len = parse_conf(conf)) == -1) {
        return -1;
    }

    int i = 0;
    for (i=0; i<server_list_len; i++) {
        printf("server%d - %s:%"PRIu16"\n", i, server_list[i].addr, server_list[i].port);
    }

    if (async) {
        async_send();
    } else {
        sync_send();
    }


    return 0;
}


