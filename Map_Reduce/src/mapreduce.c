#include "mapreduce.h"

#include "utils.h"

#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

pthread_mutex_t*    partition_locks;
struct partition_t* partitions;
int                 num_partitions;
Mapper              mapper;
Reducer             reducer;
pthread_t*          pthreads;  // -1 denotes available

struct mapper_arg_t {
    int   id;
    char* arg;
};

struct reducer_arg_t {
    int   id;
    char* key;
    int   partition_id;
};

char* MR_GetNext(char* key, int partition_number) {
    struct info_node_t* info_ptr = partitions[partition_number].info_head;
    for (; info_ptr != NULL; info_ptr = info_ptr->next) {
        if (strcmp(info_ptr->info, key) == 0) {
            if (info_ptr->proceed == 1)
                return NULL;
            struct data_node_t* data_ptr = info_ptr->data;
            for (; data_ptr != NULL; data_ptr = data_ptr->next) {
                if (data_ptr->proceed == 0) {
                    data_ptr->proceed = 1;
                    return data_ptr->value;
                }
            }
            info_ptr->proceed = 1;
            return NULL;
        }
    }
    return NULL;
}

void* MR_MapperAdapt(void* arg) {
    struct mapper_arg_t* pass_arg = ( struct mapper_arg_t* )arg;
    pthread_detach(pthread_self());
    mapper(pass_arg->arg);
    pthreads[pass_arg->id] = -1;
    return NULL;
}

void* MR_ReducerAdapt(void* arg) {
    struct reducer_arg_t* pass_arg = ( struct reducer_arg_t* )arg;
    pthread_detach(pthread_self());
    reducer(pass_arg->key, MR_GetNext, pass_arg->partition_id);
    pthreads[pass_arg->id] = -1;
    return NULL;
}

unsigned long MR_DefaultHashPartition(char* key, int num_partitions) {
    unsigned long hash = 5381;
    int           c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

void MR_Run(int argc, char* argv[], Mapper map, int num_mappers, Reducer reduce, int num_reducers, Partitioner partition) {
    // create partition locks
    num_partitions = num_reducers;

    partition_locks = ( pthread_mutex_t* )malloc(sizeof(pthread_mutex_t) * num_partitions);
    partitions      = ( struct partition_t* )malloc(sizeof(struct partition_t) * num_partitions);
    for (int i = 0; i < num_partitions; ++i) {
        pthread_mutex_t tmp     = PTHREAD_MUTEX_INITIALIZER;
        partition_locks[i]      = tmp;
        partitions[i].info_head = NULL;
    }

    int current_work = 1;
    int total_work   = argc - 1;

    mapper   = map;
    pthreads = ( pthread_t* )malloc(sizeof(pthread_t) * num_mappers);

    for (int i = 0; i < num_mappers; ++i) {
        pthreads[i] = -1;
    }
    while (current_work <= total_work) {
        for (int i = 0; i < num_mappers; ++i) {
            // ensure whether the thread is ended
            if (pthreads[i] == -1) {
                if (current_work <= total_work) {
                    struct mapper_arg_t* pass_arg = ( struct mapper_arg_t* )malloc(sizeof(struct mapper_arg_t));
                    pass_arg->arg                 = argv[current_work++];
                    pass_arg->id                  = i;
                    pthread_create(&pthreads[i], NULL, MR_MapperAdapt, pass_arg);
                }
                continue;
            }
        }
    }
    // waiting all mappers finished
    int all_finished = 0;
    while (all_finished == 0) {
        all_finished = 1;
        for (int i = 0; i < num_mappers; ++i) {
            if (pthreads[i] == -1)
                continue;
            else
                all_finished = 0;
        }
    }

    // for (int i = 0; i < num_partitions; ++i) {
    //     if (partitions[i].info_head != NULL) {
    //         printf("partition: %d\n", i);
    //         struct info_node_t* info = partitions[i].info_head;
    //         for (; info != NULL; info = info->next) {
    //             printf("info: %s\n", info->info);
    //             struct data_node_t* data = info->data;
    //             for (; data != NULL; data = data->next) {
    //                 printf("data: %s\n", data->value);
    //             }
    //         }
    //     }
    // }

    reducer = reduce;
    free(pthreads);
    pthreads = ( pthread_t* )malloc(sizeof(pthread_t) * num_reducers);

    for (int i = 0; i < num_reducers; ++i) {
        pthreads[i] = -1;
    }
    int finished_reduce = 0;
    while (finished_reduce == 0) {
        finished_reduce = 1;
        for (int i = 0; i < num_reducers; ++i) {
            if (pthreads[i] != -1) {
                finished_reduce = 0;
                continue;
            }
            struct info_node_t* info_ptr = partitions[i].info_head;
            for (; info_ptr != NULL; info_ptr = info_ptr->next) {
                if (info_ptr->proceed == 0)
                    break;
            }
            if (info_ptr == NULL) {
                continue;
            }
            finished_reduce                = 0;
            struct reducer_arg_t* pass_arg = ( struct reducer_arg_t* )malloc(sizeof(struct reducer_arg_t));
            pass_arg->id                   = i;
            pass_arg->key                  = info_ptr->info;
            pass_arg->partition_id         = i;
            pthread_create(&pthreads[i], NULL, MR_ReducerAdapt, pass_arg);
        }
    }
}

void MR_Emit(char* key, char* value) {
    unsigned long partition_index = MR_DefaultHashPartition(key, num_partitions);
    pthread_mutex_lock(&partition_locks[partition_index]);
    // iterate
    struct info_node_t* info_ptr = partitions[partition_index].info_head;
    for (; info_ptr != NULL; info_ptr = info_ptr->next) {
        if (strcmp(info_ptr->info, key) == 0) {
            insert_data(info_ptr, value);
            return;
        }
    }
    insert_info(&partitions[partition_index], key);
    insert_data(partitions[partition_index].info_head, value);
    pthread_mutex_unlock(&partition_locks[partition_index]);
}
