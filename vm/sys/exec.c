
#include "sys.h"
// #include <unistd.h>
// #include <sys/wait.h>

// void exit(int code);

// vm_obj_t vm_sys_exec(vm_gc_t *gc, vm_obj_t command)
// {
//     vm_gc_entry_t *ent = vm_obj_to_ptr(command);
//     int size = vm_obj_to_int(vm_gc_sizeof(ent));
//     char **ents = vm_calloc(sizeof(char *) * (size + 1));
//     for (int i = 0; i < size; i++)
//     {
//         vm_obj_t val = vm_gc_get_index(ent, vm_obj_of_int(i));
//         vm_gc_entry_t *sent = vm_obj_to_ptr(val);
//         int slen = vm_obj_to_int(vm_gc_sizeof(sent));
//         ents[i] = vm_calloc(sizeof(char) * (slen + 1));
//         for (int j = 0; j < slen; j++)
//         {
//             ents[i][j] = vm_obj_to_int(vm_gc_get_index(sent, vm_obj_of_int(j)));
//         }
//     }
//     pid_t pid = fork();

//     if (pid == -1)
//     {
//         for (int i = 0; i < size; i++)
//         {
//             vm_free(ents[i]);   
//         }
//         vm_free(ents);
//         return vm_obj_of_dead();
//     } 
//     else if (pid > 0)
//     {
//         int status;
//         waitpid(pid, &status, 0);
//         for (int i = 0; i < size; i++)
//         {
//             vm_free(ents[i]);   
//         }
//         vm_free(ents);
//         return vm_obj_of_int(status);
//     }
//     else 
//     {
//         execvp(ents[0], ents);
//         exit(0);
//     }
// }
