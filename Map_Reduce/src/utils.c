#include "utils.h"

#include <stdlib.h>
#include <string.h>

void insert_info(struct partition_t* part, char* key) {
    struct info_node_t* new_info = ( struct info_node_t* )malloc(sizeof(struct info_node_t));

    new_info->info    = ( char* )malloc(sizeof(char) * (strlen(key) + 1));
    new_info->data    = NULL;
    new_info->proceed = 0;
    new_info->next    = NULL;
    strcpy(new_info->info, key);

    new_info->next  = part->info_head;
    part->info_head = new_info;
}

void insert_data(struct info_node_t* info, char* value) {
    struct data_node_t* new_node = ( struct data_node_t* )malloc(sizeof(struct data_node_t));
    new_node->value              = ( char* )malloc(sizeof(char) * (strlen(value) + 1));
    new_node->proceed            = 0;
    strcpy(new_node->value, value);
    new_node->next = info->data;
    info->data     = new_node;
}