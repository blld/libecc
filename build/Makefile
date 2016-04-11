
binary := ./bin/libecc
sources := $(wildcard ../src/*.c) $(wildcard ../src/builtin/*.c)
objects := $(sources:../src/%.c=./object/%.o)

#

warn  ?= -Wall
opts  ?= -Os -fstrict-aliasing
lto   ?= $(shell echo "main(){}" | gcc -E -flto - >/dev/null 2>&1 && echo "-flto")
debug ?= -DNDEBUG=1
libs  ?= -lm

CFLAGS += $(warn) $(opts) $(lto) $(debug)

##

all: ./object/builtin ./bin $(binary)

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
