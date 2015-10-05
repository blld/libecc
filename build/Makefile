
ifeq ($(OS),Windows_NT)
binary := ./bin/libecc.exe
else
binary := ./bin/libecc
endif

binary := ./bin/libecc
sources := $(wildcard ../src/*.c) $(wildcard ../src/object/*.c)
objects := $(sources:../src/%.c=./object/%.o)

std ?= -std=c99
warn ?= -Wall -pedantic
opts ?= -Os -fomit-frame-pointer -fstrict-aliasing
flto ?= -ftlo
libs ?= -lm

CFLAGS += $(std) $(warn) $(opts) $(flto) -DNDEBUG=1

##

all: ./object/object ./bin $(binary)

clean:
	rm -rf ./bin ./object

test: $(binary)
	$(binary) --test

./object/object ./bin:
	@mkdir -p $@

$(binary): $(objects)
	@echo "   [LD] $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(libs) $^ -o $@

./object/%.o: ../src/%.c
	@echo "   [CC] $@"
	@$(CC) $(CFLAGS) -I../src/ -c $< -o $@
