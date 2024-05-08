.phony: all clean build

target := mini_vtysh
CC := g++
RM := rm -rf
CP := cp -rf
TYPE_SRC := .cpp

SRCS := $(wildcard *$(TYPE_SRC))
	
CFLAGS := -lpthread -Wall -g -std=c++11


all:$(target) 
	@echo "complie succeed"
	
$(target):$(SRCS)
	@echo "complie $@" 
	$(CC) $^ -o $@ $(CFLAGS) 

clean:
	$(RM) $(target)
rebuild: clean all
	@echo "rebuild succeed."

