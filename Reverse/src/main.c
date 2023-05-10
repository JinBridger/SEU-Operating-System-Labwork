#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

char** reverse_text(char** text, int line_count) {
    char** ret = ( char** )malloc(sizeof(char*) * line_count);
    if(!ret) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    for (int i = 0; i < line_count; ++i) {
        ret[i] = text[line_count - i - 1];
    }
    return ret;
}

void get_output(FILE* file, char** text, int line_count) {
    for (int i = 0; i < line_count; ++i) {
        fprintf(file, "%s\n", text[i]);
    }
}

char** get_input(FILE* file, int* line_cnt) {
    char** text = ( char** )malloc(sizeof(char*) * 100);
    if(!text) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    for (int i = 0; i < 100; ++i) {
        text[i] = ( char* )malloc(sizeof(char) * 100);
        if(!text[i]) {
            fprintf(stderr, "malloc failed\n");
            exit(1);
        }
    }
    int line_count = 0;
    for (; fscanf(file, "%s", text[line_count]) != -1; ++line_count)
        ;
    *line_cnt = line_count;
    return text;
}

int main(int argc, char* argv[]) {
    char* input_file_path  = NULL;
    char* output_file_path = NULL;

    if (argc == 2) {  // input file -> stdout
        input_file_path = argv[1];
    }
    if (argc == 3) {  // input file -> output file
        input_file_path  = argv[1];
        output_file_path = argv[2];
    }
    if (argc > 3) {  // too many arguments
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(1);
    }

    FILE* input_file  = stdin;
    FILE* output_file = stdout;

    if (input_file_path != NULL) {  // open input file
        input_file = fopen(input_file_path, "r");
        if (input_file == NULL) {
            fprintf(stderr, "reverse: cannot open file '%s'\n", input_file_path);
            exit(1);
        }
    }
    if (output_file_path != NULL) {  // open output file
        output_file = fopen(output_file_path, "w");
        if (output_file == NULL) {
            fprintf(stderr, "reverse: cannot open file '%s'\n", output_file_path);
            exit(1);
        }
    }

    // compare input and output file
    if (input_file_path != NULL && output_file_path != NULL) {
        struct stat input_file_stat, output_file_stat;
        stat(input_file_path, &input_file_stat);
        stat(output_file_path, &output_file_stat);

        if(input_file_stat.st_ino == output_file_stat.st_ino) {
            fprintf(stderr, "reverse: input and output file must differ\n");
            exit(1);
        }
    }

    int cnt = 0;

    char** test_text = get_input(input_file, &cnt);
    if (cnt == 0)
        exit(1);
    get_output(output_file, reverse_text(test_text, cnt), cnt);
}