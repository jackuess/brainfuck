#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_MEMORY_SLOTS 30000
#define MAX_PROGRAM_LEN 1024*64
#define MAX_LOOP_DEPTH 512


enum BFErrCode {
    BF_SUCCESS,
    BF_ERR_OUT_OF_MEMORY,
    BF_ERR_PROGRAM_TOO_LONG,
    BF_ERR_OUT_OF_BOUNDS
};

enum BFCommandType {
    BF_CMD_FORWARD,
    BF_CMD_BACK,
    BF_CMD_INCR,
    BF_CMD_DECR,
    BF_CMD_WRITE,
    BF_CMD_READ,
    BF_CMD_LOOP,
    BF_CMD_ENDLOOP
};

struct BFCommand {
    enum BFCommandType type;
    struct BFCommand *jmp;
};

enum BFErrCode bf_parse_file(struct BFCommand *program, size_t *program_len,
                             size_t program_max_len, FILE *f) {
    const int bufsize = 4096;
    char buf[bufsize];
    size_t num_chars = 0;
    struct BFCommand *open_braces[MAX_LOOP_DEPTH] = {NULL};
    size_t num_open_braces = 0;
    struct BFCommand *sibling = NULL;
    while ((num_chars = fread(&buf[0], 1, bufsize, f)) > 0) {
        if (*program_len + num_chars > program_max_len) {
            return BF_ERR_PROGRAM_TOO_LONG;
        }
        for (unsigned int i = 0; i < num_chars; i++) {
            switch (buf[i]) {
                case '>':
                    program[*program_len].type = BF_CMD_FORWARD;
                    break;
                case '<':
                    program[*program_len].type = BF_CMD_BACK;
                    break;
                case '+':
                    program[*program_len].type = BF_CMD_INCR;
                    break;
                case '-':
                    program[*program_len].type = BF_CMD_DECR;
                    break;
                case '.':
                    program[*program_len].type = BF_CMD_WRITE;
                    break;
                case ',':
                    program[*program_len].type = BF_CMD_READ;
                    break;
                case '[':
                    if (num_open_braces == MAX_LOOP_DEPTH) {
                        return BF_ERR_PROGRAM_TOO_LONG;
                    }
                    open_braces[num_open_braces++] = &program[*program_len];
                    program[*program_len].type = BF_CMD_LOOP;
                    break;
                case ']':
                    sibling = open_braces[--num_open_braces];
                    sibling->jmp = &program[*program_len];
                    program[*program_len].type = BF_CMD_ENDLOOP;
                    program[*program_len].jmp = sibling;
                    break;
                default:
                    continue;
            }
            (*program_len)++;
        }
    }
    return BF_SUCCESS;
}

enum BFErrCode bf_eval(const struct BFCommand *program, ptrdiff_t program_len,
                       uint8_t *mem, uint8_t *mem_end, FILE *out, FILE *in) {
    const struct BFCommand *cmd_ptr = &program[0];
    uint8_t *mem_ptr = mem;
    while (cmd_ptr - program < program_len) {
        switch (cmd_ptr->type) {
            case BF_CMD_FORWARD:
                if (mem_ptr + 1 > mem_end) {
                    return BF_ERR_OUT_OF_BOUNDS;
                }
                mem_ptr++;
                break;
            case BF_CMD_BACK:
                if (mem_ptr == mem) {
                    return BF_ERR_OUT_OF_BOUNDS;
                }
                mem_ptr--;
                break;
            case BF_CMD_INCR:
                (*mem_ptr)++;
                break;
            case BF_CMD_DECR:
                (*mem_ptr)--;
                break;
            case BF_CMD_WRITE:
                putc(*mem_ptr, out);
                break;
            case BF_CMD_READ:
                *mem_ptr = (uint8_t)getc(in);
                break;
            case BF_CMD_LOOP:
                if (*mem_ptr == 0) {
                    cmd_ptr = cmd_ptr->jmp;
                }
                break;
            case BF_CMD_ENDLOOP:
                if (*mem_ptr != 0) {
                    cmd_ptr = cmd_ptr->jmp;
                }
                break;
        }
        cmd_ptr++;
    }
    return BF_SUCCESS;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <PROGRAM>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *src_file = fopen(argv[1], "r");
    if (!src_file) {
        printf("%s could not be opened\n", argv[1]);
        return EXIT_FAILURE;
    }
    struct BFCommand program[MAX_PROGRAM_LEN] = {0};
    size_t num_commands = 0;
    enum BFErrCode err = bf_parse_file(&program[0], &num_commands,
                                       MAX_PROGRAM_LEN, src_file);
    fclose(src_file);
    switch (err) {
        case BF_ERR_OUT_OF_MEMORY:
            printf("READ ERROR: Out of memory\n");
            return EXIT_FAILURE;
        case BF_ERR_PROGRAM_TOO_LONG:
            printf("READ ERROR: Program too long (max: %d)\n", MAX_PROGRAM_LEN);
            return EXIT_FAILURE;
        default:
            break;
    }

    uint8_t mem[NUM_MEMORY_SLOTS] = {0};
    err = bf_eval(&program[0], num_commands, &mem[0], &mem[NUM_MEMORY_SLOTS-1],
                  stdout, stdin);
    switch (err) {
        case BF_ERR_OUT_OF_BOUNDS:
            printf("RUNTIME ERROR: Out of bounds memory access\n");
            return EXIT_FAILURE;
        default:
            break;
    }

    return EXIT_SUCCESS;
}
