#!/bin/env python
import array
import sys


def bf_tokenize(program):
    tokens = []
    open_braces = []
    for char in program:
        if char in [">", "<", "+", "-", ".", ","]:
            tokens.append(char)
        elif char == "[":
            open_braces.append(len(tokens))
            tokens.append(None)
        elif char == "]":
            sibling_pos = open_braces.pop()
            tokens[sibling_pos] = ("[", len(tokens))
            tokens.append((char, sibling_pos))
    return tokens


def bf_eval(program, mem, stdout, stdin):
    command_ptr = 0
    program_len = len(program)
    mem_len = len(mem)
    mem_ptr = 0

    while command_ptr < program_len:
        command = program[command_ptr]

        if command == ">":
            mem_ptr = (mem_ptr + 1) % mem_len
        elif command == "<":
            mem_ptr = (mem_ptr - 1) % mem_len
        elif command == "+":
            mem[mem_ptr] = (mem[mem_ptr] + 1) % 256
        elif command == "-":
            mem[mem_ptr] = (mem[mem_ptr] - 1) % 256
        elif command == ".":
            stdout.write(chr(mem[mem_ptr]))
            stdout.flush()
        elif command == ",":
            mem[mem_ptr] = ord(stdin.read(1))
        else:
            command, jmp = command
            if command == "[" and mem[mem_ptr] == 0:
                command_ptr = jmp
            elif command == "]" and mem[mem_ptr] != 0:
                command_ptr = jmp

        command_ptr += 1

    return mem


if __name__ == "__main__":
    with open(sys.argv[1]) as f:
        memory = array.array("B", [0] * 30000)
        bf_eval(bf_tokenize(f.read()), memory, sys.stdout, sys.stdin)
