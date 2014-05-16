#include "utility.h"

bool is_input(int a)
{
	if ( a >> ARG_INPUT == 1 )
		return true;

	return false;
}

bool is_output(int a)
{
	if ( ((a >> ARG_OUTPUT) & 0x1) == 1  )
		return true;

	return false;
}

int get_Arg_Type(int a)
{
	return ((a >> 16) & 0xFF);
}

int arg_Length(int a)
{
	int length = a & 0x0000FFFF;
	if ( length == 0 )
		length = 1;

	switch(get_Arg_Type(a))
	{
		case ARG_CHAR:
			length = sizeof(char) * length;
			break;
		case ARG_SHORT:
			length = sizeof(short) * length;
			break;
		case ARG_INT:
			length = sizeof(int) * length;
			break;
		case ARG_LONG:
			length = sizeof(long) * length;
			break;
		case ARG_DOUBLE:
			length = sizeof(double) * length;
			break;
		case ARG_FLOAT:
			length = sizeof(float) * length;
			break;
	}

	return length;
}



int argTypes_Length(int* argTypes )
{
	int count = 0;
	while(*argTypes != 0){
		argTypes++;
		count++;
	}
	return count+1;
}

int* argTypes_Copy(int* argTypes)
{
	int *ret = new int[argTypes_Length(argTypes)];
	int *temp = ret;

	while( *argTypes != 0 )
	{
		*temp = *argTypes;
		argTypes++;
		temp++;

	}

	*temp = 0;
	return ret;
}

void argTypes_Copy(int* argTypes, int* buf)
{
	while( *argTypes != 0 )
	{
		*buf = *argTypes;
		argTypes++;
		buf++;

	}
	*buf = 0;
}

int args_Length(int *argTypes)
{
	int length = 0;
	while( *argTypes != 0 )
	{
		length += arg_Length(*argTypes);
		argTypes++;
	}

	return length;
}

void args_Copy(int* argTypes, void** args, void* buf){
	int length = 0;
	while ( *argTypes != 0 )
	{
		length = arg_Length(*argTypes);
		memcpy(buf, *args, length);
		buf = (void *)((char *)buf + length);
		argTypes++;
		args++;;
	}
}

void** get_Args(int* argTypes, void* buf)
{
	int length = argTypes_Length(argTypes) - 1;
	void** args = new void*[length];
	int i = 0;
	while( *argTypes != 0 )
	{
		args[i] = buf;
		length = arg_Length(*argTypes);
		buf = (void *)((char *)buf + length);
		argTypes++;
		i++;
	}

	return args;
}

bool argTypes_Compare(int* argTypesA, int* argTypesB)
{
	while(true)
	{
		if ( *argTypesA == 0 && *argTypesB == 0 )
			break;
		else if ( *argTypesA == 0 || *argTypesB == 0)
			return false;
		/*
			only check in/out and type and
			check for is scalar vs vector
		*/
		else if ( (*argTypesA & 0xFFFF0000) != (*argTypesB & 0xFFFF0000)
				|| ((*argTypesA & 0xFFFF) == 0 && (*argTypesB & 0xFFFF) != 0)
				|| ((*argTypesB & 0xFFFF) == 0 && (*argTypesA & 0xFFFF) != 0))
			return false;
		
		argTypesA++;
		argTypesB++;
	}

	return true;
}

void copy_Args(int* argTypes, void** dst, void** src)
{
	while (*argTypes != 0)
	{
		if ( is_output(*argTypes) )
		{
			memcpy(*dst, *src, arg_Length(*argTypes));
		}
		dst++;
		src++;
		argTypes++;
	}
}