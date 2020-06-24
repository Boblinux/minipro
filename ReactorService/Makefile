DIR_INC = ./inc
DIR_SRC = ./src
DIR_OBJ = ./obj
DIR_BIN = ./bin

SRC = $(wildcard ${DIR_SRC}/*.cc)
OBJ = $(patsubst %.cc,${DIR_OBJ}/%.o,$(notdir ${SRC}))

TARGET = main readBigFile

BIN_TARGET = ${DIR_BIN}/${TARGET}

CC = g++
CFLAGS = -std=c++11 -g -Wall -I${DIR_INC}

${BIN_TARGET}:${OBJ}
	$(CC) -pthread -Wl,--no-as-needed -lssl -lcrypto -lboost_system  -lboost_thread $(OBJ)  -o $@

${DIR_OBJ}/%.o:${DIR_SRC}/%.cc
	$(CC) $(CFLAGS) -c  $< -o $@ 

.PHONY:clean
clean:
	rm -rf ${DIR_OBJ}/*.o 