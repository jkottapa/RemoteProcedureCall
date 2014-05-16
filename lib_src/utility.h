#ifndef _UTILITY_
#define _UTILITY_

#include <cstring>
#include "../rpc_src/rpc.h"

int argTypes_Length(int* argTypes);
int* argTypes_Copy(int* argTypes);
void argTypes_Copy(int* argTypes, int* buf);
bool argTypes_Compare(int* argsTypeA, int* argsTypeB);

bool is_input(int a);
bool is_output(int a);
int arg_Length(int a);
int get_Arg_Type(int a);
int args_Length(int *argTypes);
void args_Copy(int* argTypes, void** args, void* buf);
void** get_Args(int* argTypes, void* buf);
void copy_Args(int* argTypes, void** dst, void** src);

#endif
