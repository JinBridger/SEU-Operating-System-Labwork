#ifndef __utils_h__
#define __utils_h__

struct data_node_t {
    int                 proceed;
    char*               value;
    struct data_node_t* next;
};

struct info_node_t {
    int                 proceed;
    char*               info;
    struct data_node_t* data;
    struct info_node_t* next;
};

struct partition_t {
    struct info_node_t* info_head;
};

void insert_info(struct partition_t* part, char* key);

void insert_data(struct info_node_t* info, char* value);

#endif