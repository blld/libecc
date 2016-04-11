
binary := ./bin/libecc
sources := $(wildcard ../src/*.c) $(wildcard ../src/builtin/*.c)
objects := $(sources:../src/%.c=./object/%.o)

#

warn  ?= -Wall
lto   ?= $(shell echo "main(){}" | gcc -E -flto - >/dev/null 2>&1 && echo "-flto")
optim ?= -Os -fstrict-aliasing
debug ?= -DNDEBUG=1
libs  ?= -lm

.PHONY: debug
debug : debug := -DDEBUG=1
debug : optim := -O0

CFLAGS += $(warn) $(lto) $(optim) $(debug)

##

all: ./object/builtin ./bin $(binary)
debug: all
clean:
	rm -rf ./bin ./object

test: $(binary)
	$(binary) --test

./object/builtin ./bin:
	@mkdir -p $@

$(binary): $(objects)
	@echo "   [LD] $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(libs) $^ -o $@

./object/%.o: ../src/%.c
	@echo "   [CC] $@"
	@$(CC) $(CFLAGS) -I../src/ -c $< -o $@
