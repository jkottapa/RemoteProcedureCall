CXX = g++					# compiler
MAKEFILE_NAME = ${firstword ${MAKEFILE_LIST}}	# makefile name
CXXFLAGS = -g -lpthread -Wall -Wno-unused-label -I./lib_src/	# compiler flags


COMMON_SRC=$(wildcard lib_src/*.cpp)
COMMON_OBJECTS=$(COMMON_SRC:.cpp=.o)

RPC_SRC=$(wildcard rpc_src/*.cpp)
RPC_OBJECTS=$(RPC_SRC:.cpp=.o)

BINDER_SRC =$(wildcard binder_src/*.cpp)
BINDER_OBJECTS=$(BINDER_SRC:.cpp=.o) ${COMMON_OBJECTS}
BINDER_EXEC = binder

LIB_OBJECTS=${COMMON_OBJECTS} ${RPC_OBJECTS}
LIB=librpc.a

EXECS=${BINDER_EXEC} ${LIB} 
ALL_OBJECTS=${RPC_OBJECTS} ${BINDER_OBJECTS}

#############################################################

.PHONY : all clean

all : ${EXECS}					# build all executables

${BINDER_EXEC} : ${BINDER_OBJECTS}				# link step 1st executable
	${CXX} $^ ${CXXFLAGS} -o $@

${LIB} : ${LIB_OBJECTS}
	ar rcs $@ $^

#############################################################

${OBJECTS} : ${MAKEFILE_NAME}			# OPTIONAL : changes to this file => recompile

clean :						# remove files that can be regenerated
	rm -f *.d *.o ${EXECS} ${ALL_OBJECTS}
	