#include <vm/debug.h>

const char *vm_opcode_internal_name(opcode_t op)
{
    switch (op)
    {
    default:
        return "error";
    case OPCODE_EXIT:
        return "exit";
    case OPCODE_STORE_REG:
        return "store_reg";
    case OPCODE_STORE_LOG:
        return "store_log";
    case OPCODE_STORE_NUM:
        return "store_num";
    case OPCODE_STORE_FUN:
        return "store_fun";
    case OPCODE_EQUAL:
        return "equal";
    case OPCODE_EQUAL_NUM:
        return "equal_num";
    case OPCODE_NOT_EQUAL:
        return "not_equal";
    case OPCODE_NOT_EQUAL_NUM:
        return "not_equal_num";
    case OPCODE_LESS:
        return "less";
    case OPCODE_LESS_NUM:
        return "less_num";
    case OPCODE_GREATER:
        return "greater";
    case OPCODE_GREATER_NUM:
        return "greater_num";
    case OPCODE_LESS_THAN_EQUAL:
        return "less_than_equal";
    case OPCODE_LESS_THAN_EQUAL_NUM:
        return "less_than_equal_num";
    case OPCODE_GREATER_THAN_EQUAL:
        return "greater_than_equal";
    case OPCODE_GREATER_THAN_EQUAL_NUM:
        return "greater_than_equal_num";
    case OPCODE_JUMP_ALWAYS:
        return "jump_always";
    case OPCODE_JUMP_IF_FALSE:
        return "jump_if_false";
    case OPCODE_JUMP_IF_TRUE:
        return "jump_if_true";
    case OPCODE_JUMP_IF_EQUAL:
        return "jump_if_equal";
    case OPCODE_JUMP_IF_EQUAL_NUM:
        return "jump_if_equal_num";
    case OPCODE_JUMP_IF_NOT_EQUAL:
        return "jump_if_not_equal";
    case OPCODE_JUMP_IF_NOT_EQUAL_NUM:
        return "jump_if_not_equal_num";
    case OPCODE_JUMP_IF_LESS:
        return "jump_if_less";
    case OPCODE_JUMP_IF_LESS_NUM:
        return "jump_if_less_num";
    case OPCODE_JUMP_IF_GREATER:
        return "jump_if_greater";
    case OPCODE_JUMP_IF_GREATER_NUM:
        return "jump_if_greater_num";
    case OPCODE_JUMP_IF_LESS_THAN_EQUAL:
        return "jump_if_less_than_equal";
    case OPCODE_JUMP_IF_LESS_THAN_EQUAL_NUM:
        return "jump_if_less_than_equal_num";
    case OPCODE_JUMP_IF_GREATER_THAN_EQUAL:
        return "jump_if_greater_than_equal";
    case OPCODE_JUMP_IF_GREATER_THAN_EQUAL_NUM:
        return "jump_if_greater_than_equal_num";
    case OPCODE_INC:
        return "inc";
    case OPCODE_INC_NUM:
        return "inc_num";
    case OPCODE_DEC:
        return "dec";
    case OPCODE_DEC_NUM:
        return "dec_num";
    case OPCODE_ADD:
        return "add";
    case OPCODE_ADD_NUM:
        return "add_num";
    case OPCODE_SUB:
        return "sub";
    case OPCODE_SUB_NUM:
        return "sub_num";
    case OPCODE_MUL:
        return "mul";
    case OPCODE_MUL_NUM:
        return "mul_num";
    case OPCODE_DIV:
        return "div";
    case OPCODE_DIV_NUM:
        return "div_num";
    case OPCODE_MOD:
        return "mod";
    case OPCODE_MOD_NUM:
        return "mod_num";
    case OPCODE_STATIC_CALL:
        return "static_call";
    case OPCODE_CALL:
        return "call";
    case OPCODE_REC:
        return "rec";
    case OPCODE_RETURN:
        return "return";
    case OPCODE_PRINTLN:
        return "println";
    case OPCODE_PUTCHAR:
        return "putchar";
    case OPCODE_ALLOCA:
        return "alloca";
    case OPCODE_ARRAY:
        return "array";
    case OPCODE_LENGTH:
        return "array";
    case OPCODE_INDEX:
        return "index";
    case OPCODE_INDEX_NUM:
        return "index_num";
    }
}

const char *vm_opcode_name(opcode_t op)
{
    switch (op)
    {
    default:
        return "error";
    case OPCODE_EXIT:
        return "exit";
    case OPCODE_STORE_REG:
        return "mov";
    case OPCODE_STORE_LOG:
        return "mov";
    case OPCODE_STORE_NUM:
        return "mov";
    case OPCODE_STORE_FUN:
        return "proc";
    case OPCODE_EQUAL:
        return "eq";
    case OPCODE_EQUAL_NUM:
        return "eq";
    case OPCODE_NOT_EQUAL:
        return "ne";
    case OPCODE_NOT_EQUAL_NUM:
        return "ne";
    case OPCODE_LESS:
        return "lt";
    case OPCODE_LESS_NUM:
        return "lt";
    case OPCODE_GREATER:
        return "gt";
    case OPCODE_GREATER_NUM:
        return "gt";
    case OPCODE_LESS_THAN_EQUAL:
        return "le";
    case OPCODE_LESS_THAN_EQUAL_NUM:
        return "le";
    case OPCODE_GREATER_THAN_EQUAL:
        return "ge";
    case OPCODE_GREATER_THAN_EQUAL_NUM:
        return "ge";
    case OPCODE_JUMP_ALWAYS:
        return "j";
    case OPCODE_JUMP_IF_FALSE:
        return "jf";
    case OPCODE_JUMP_IF_TRUE:
        return "jt";
    case OPCODE_JUMP_IF_EQUAL:
        return "jeq";
    case OPCODE_JUMP_IF_EQUAL_NUM:
        return "jeq";
    case OPCODE_JUMP_IF_NOT_EQUAL:
        return "jne";
    case OPCODE_JUMP_IF_NOT_EQUAL_NUM:
        return "jne";
    case OPCODE_JUMP_IF_LESS:
        return "jlt";
    case OPCODE_JUMP_IF_LESS_NUM:
        return "jlt";
    case OPCODE_JUMP_IF_GREATER:
        return "jgt";
    case OPCODE_JUMP_IF_GREATER_NUM:
        return "jgt";
    case OPCODE_JUMP_IF_LESS_THAN_EQUAL:
        return "jle";
    case OPCODE_JUMP_IF_LESS_THAN_EQUAL_NUM:
        return "jle";
    case OPCODE_JUMP_IF_GREATER_THAN_EQUAL:
        return "jge";
    case OPCODE_JUMP_IF_GREATER_THAN_EQUAL_NUM:
        return "jge";
    case OPCODE_INC:
        return "add";
    case OPCODE_INC_NUM:
        return "add";
    case OPCODE_DEC:
        return "sub";
    case OPCODE_DEC_NUM:
        return "sub";
    case OPCODE_ADD:
        return "add";
    case OPCODE_ADD_NUM:
        return "add";
    case OPCODE_SUB:
        return "sub";
    case OPCODE_SUB_NUM:
        return "sub";
    case OPCODE_MUL:
        return "mul";
    case OPCODE_MUL_NUM:
        return "mul";
    case OPCODE_DIV:
        return "div";
    case OPCODE_DIV_NUM:
        return "div";
    case OPCODE_MOD:
        return "mod";
    case OPCODE_MOD_NUM:
        return "mod";
    case OPCODE_CALL:
        return "call";
    case OPCODE_STATIC_CALL:
        return "call";
    case OPCODE_REC:
        return "rec";
    case OPCODE_RETURN:
        return "ret";
    case OPCODE_PRINTLN:
        return "println";
    case OPCODE_PUTCHAR:
        return "putc";
    case OPCODE_ALLOCA:
        return "alloca";
    case OPCODE_ARRAY:
        return "new";
    case OPCODE_LENGTH:
        return "len";
    case OPCODE_INDEX:
        return "nth";
    case OPCODE_INDEX_NUM:
        return "nth";
    }
}

const char *vm_opcode_format(opcode_t op)
{
    switch (op)
    {
    default:
        return "e";
    case OPCODE_EXIT:
        return "";
    case OPCODE_STORE_REG:
        return "rr";
    case OPCODE_STORE_LOG:
        return "rl";
    case OPCODE_STORE_NUM:
        return "rn";
    case OPCODE_STORE_FUN:
        return "rf";
    case OPCODE_EQUAL:
        return "rrr";
    case OPCODE_EQUAL_NUM:
        return "rrn";
    case OPCODE_NOT_EQUAL:
        return "rrr";
    case OPCODE_NOT_EQUAL_NUM:
        return "rrn";
    case OPCODE_LESS:
        return "rrr";
    case OPCODE_LESS_NUM:
        return "rrn";
    case OPCODE_GREATER:
        return "rrr";
    case OPCODE_GREATER_NUM:
        return "rrn";
    case OPCODE_LESS_THAN_EQUAL:
        return "rrr";
    case OPCODE_LESS_THAN_EQUAL_NUM:
        return "rrn";
    case OPCODE_GREATER_THAN_EQUAL:
        return "rrr";
    case OPCODE_GREATER_THAN_EQUAL_NUM:
        return "rrn";
    case OPCODE_JUMP_ALWAYS:
        return "j";
    case OPCODE_JUMP_IF_FALSE:
        return "jr";
    case OPCODE_JUMP_IF_TRUE:
        return "jr";
    case OPCODE_JUMP_IF_EQUAL:
        return "jrr";
    case OPCODE_JUMP_IF_EQUAL_NUM:
        return "jrn";
    case OPCODE_JUMP_IF_NOT_EQUAL:
        return "jrr";
    case OPCODE_JUMP_IF_NOT_EQUAL_NUM:
        return "jrn";
    case OPCODE_JUMP_IF_LESS:
        return "jrr";
    case OPCODE_JUMP_IF_LESS_NUM:
        return "jrn";
    case OPCODE_JUMP_IF_GREATER:
        return "jrr";
    case OPCODE_JUMP_IF_GREATER_NUM:
        return "jrn";
    case OPCODE_JUMP_IF_LESS_THAN_EQUAL:
        return "jrr";
    case OPCODE_JUMP_IF_LESS_THAN_EQUAL_NUM:
        return "jrn";
    case OPCODE_JUMP_IF_GREATER_THAN_EQUAL:
        return "jrr";
    case OPCODE_JUMP_IF_GREATER_THAN_EQUAL_NUM:
        return "jrn";
    case OPCODE_INC:
        return "rr";
    case OPCODE_INC_NUM:
        return "rn";
    case OPCODE_DEC:
        return "rr";
    case OPCODE_DEC_NUM:
        return "rn";
    case OPCODE_ADD:
        return "rrr";
    case OPCODE_ADD_NUM:
        return "rrn";
    case OPCODE_SUB:
        return "rrr";
    case OPCODE_SUB_NUM:
        return "rrn";
    case OPCODE_MUL:
        return "rrr";
    case OPCODE_MUL_NUM:
        return "rrn";
    case OPCODE_DIV:
        return "rrr";
    case OPCODE_DIV_NUM:
        return "rrn";
    case OPCODE_MOD:
        return "rrr";
    case OPCODE_MOD_NUM:
        return "rrn";
    case OPCODE_STATIC_CALL:
        return "rjc";
    case OPCODE_CALL:
        return "rrc";
    case OPCODE_REC:
        return "rc";
    case OPCODE_RETURN:
        return "r";
    case OPCODE_PRINTLN:
        return "r";
    case OPCODE_PUTCHAR:
        return "r";
    case OPCODE_ALLOCA:
        return "n";
    case OPCODE_ARRAY:
        return "rc";
    case OPCODE_LENGTH:
        return "rr";
    case OPCODE_INDEX:
        return "rrr";
    case OPCODE_INDEX_NUM:
        return "rrn";
    }
}

void vm_print_opcode(int index, opcode_t *bytecode)
{
    opcode_t *head = &bytecode[index];
    opcode_t opcode = *head;
    head++;
    printf("[%i]: ", index);
    printf("%s", vm_opcode_name(opcode));
    const char *fmt = vm_opcode_format(opcode);
    for (int index = 0; fmt[index] != '\0'; index++)
    {
        if (index == 0)
        {
            printf(" ");
        }
        else
        {
            printf(", ");
        }
        switch (fmt[index])
        {
        case 'r':
        {
            printf("r%i", *(reg_t *)head);
            head += sizeof(reg_t);
            break;
        }
        case 'j':
        {
            printf("[%i]", *(int *)head);
            head += sizeof(int);
            break;
        }
        case 'l':
        {
            printf("%s", *(bool *)head ? "1" : "0");
            head += sizeof(bool);
            break;
        }
        case 'c':
        {
            int nargs = *(reg_t *)head;
            head += sizeof(reg_t);
            printf("(");
            for (int argno = 0; argno < nargs; argno++)
            {
                if (argno != 0)
                {
                    printf(", ");
                }
                printf("r%i", *(reg_t *)head);
                head += sizeof(reg_t);
            }
            printf(")");
            break;
        }
        case 'n':
        {
            if (fmod(*(integer_t *)head, 1) == 0)
            {
                printf("%i", *(integer_t *)head);
            }
            else
            {
                printf("%i", *(integer_t *)head);
            }
            head += sizeof(integer_t);
            break;
        }
        case 'f':
        {
            printf("{%i}", *(reg_t *)head);
            head += *(reg_t *)head + sizeof(reg_t);
            break;
        }
        default:
        {
            printf("error: opcode %c\n", fmt[index]);
        }
        }
    }
    printf("\n");
}