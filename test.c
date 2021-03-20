#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <gc.h>

struct obj_t;
struct func_t;
union value_t;
enum type_t;

typedef struct obj_t obj_t;
typedef struct func_t func_t;
typedef enum type_t type_t;

obj_t run(int argc, obj_t *argv, func_t func);

enum type_t {
    TYPE_NONE,
    TYPE_BOOLEAN,
    TYPE_NUMBER,
    TYPE_POINTER,
};

struct obj_t {
    union {
        bool boolean;
        double number;
        struct {
            obj_t *pointer;
            int length;
        };
    };
    type_t type;
};

struct func_t {
    int stack_used;
    int locals_used;
    int *bytecode;
    obj_t *constants;
};

enum opcode {
    OPCODE_RETURN,
    OPCODE_PUSH,
    OPCODE_POP,
    OPCODE_ARG,
    OPCODE_STORE,
    OPCODE_LOAD,
    OPCODE_ADD,
    OPCODE_SUB,
    // OPCODE_ARRAY,
    // OPCODE_INDEX,
    OPCODE_LT,
    OPCODE_PRINT,
    OPCODE_JUMP,
    OPCODE_IFTRUE,
    OPCODE_IFFALSE,
};

void print(obj_t arg) {
    switch (arg.type)
    {
    case TYPE_NONE:
        printf("none");
        break;
    case TYPE_BOOLEAN:
        if (arg.boolean)
        {
            printf("true");
        }
        else
        {
            printf("false");
        }
        break;
    case TYPE_NUMBER:
        printf("%lg", arg.number);
        break;
    case TYPE_POINTER:
        printf("%zx", (size_t) arg.pointer);
        break;
    }
}

obj_t run(int argc, obj_t *argv, func_t func) {
    int index = 0;
    int op;
    obj_t *stack = alloca((sizeof(obj_t)) * func.stack_used);
    obj_t *locals = alloca((sizeof(obj_t)) * func.locals_used);
    while (true) {
        // printf("%i: %i\n", index, func.bytecode[index]);
        switch (func.bytecode[index++]) {
        case OPCODE_RETURN:
            return *stack;
        case OPCODE_PUSH:
            *(++stack) = func.constants[func.bytecode[index++]];
            break;
        case OPCODE_POP:
            --stack;
            break;
        case OPCODE_ARG:
            *(++stack) = argv[func.bytecode[index++]];
            break;
        case OPCODE_STORE:
            locals[func.bytecode[index++]] = *stack;
            break;
        case OPCODE_LOAD:
            *(++stack) = locals[func.bytecode[index++]];
            break;
        case OPCODE_ADD: {
            double rhs = (*(stack--)).number;
            stack->number += rhs; 
            break;
        }
        case OPCODE_SUB: {
            double rhs = (*(stack--)).number;
            stack->number -= rhs; 
            break;
        }
        case OPCODE_LT: {
            double rhs = (*(stack--)).number;
            *stack = (obj_t) {
                .type = TYPE_BOOLEAN,
                .boolean = stack->number < rhs,
            }; 
            break;
        }
        case OPCODE_PRINT:
            print(*stack);
            printf("\n");
            break;
        case OPCODE_IFTRUE:
            if (stack->boolean) {
                index = func.bytecode[index];
            }
            else {
                index++;
            }
            break;
        case OPCODE_IFFALSE:
            if (!stack->boolean) {
                index = func.bytecode[index];
            }
            else {
                index++;
            }
            break;
        case OPCODE_JUMP:
            index = func.bytecode[index];
            break;
        }
    }
}

int main(int argc, char **argv) {
    func_t func;
    func.stack_used = 16;
    func.locals_used = 1;
    func.bytecode = alloca(sizeof(int) * 64);
    int index = 0;
    func.bytecode[index++] = OPCODE_ARG;
    func.bytecode[index++] = 0;
    func.bytecode[index++] = OPCODE_STORE;
    func.bytecode[index++] = 0;
    int cond_ind = index;
    func.bytecode[index++] = OPCODE_POP;
    func.bytecode[index++] = OPCODE_LOAD;
    func.bytecode[index++] = 0;
    func.bytecode[index++] = OPCODE_ARG;
    func.bytecode[index++] = 1;
    func.bytecode[index++] = OPCODE_LT;
    func.bytecode[index++] = OPCODE_IFFALSE;
    int false_jump = index;
    func.bytecode[index++] = 0;
    func.bytecode[index++] = OPCODE_LOAD;
    func.bytecode[index++] = 0;
    func.bytecode[index++] = OPCODE_PUSH;
    func.bytecode[index++] = 0;
    func.bytecode[index++] = OPCODE_ADD;
    func.bytecode[index++] = OPCODE_STORE;
    func.bytecode[index++] = 0;
    func.bytecode[index++] = OPCODE_POP;
    func.bytecode[index++] = OPCODE_JUMP;
    int end_jump = index;
    func.bytecode[index++] = cond_ind;
    func.bytecode[false_jump] = index;
    func.bytecode[index++] = OPCODE_LOAD;
    func.bytecode[index++] = 0;
    func.bytecode[index++] = OPCODE_RETURN;
    func.constants = alloca(sizeof(obj_t) * 64);
    func.constants[0] = (obj_t){
        .type = TYPE_NUMBER,
        .number = 1,
    };
    int fargc = 2;
    obj_t *fargv = alloca(sizeof(obj_t) * fargc);
    fargv[0] = (obj_t){
        .type = TYPE_NUMBER,
        .number = 0,
    };
    fargv[1] = (obj_t){
        .type = TYPE_NUMBER,
        .number = 1000000000,
    };
    double total = run(fargc, fargv, func).number;
    printf("%lg\n", total);
}
