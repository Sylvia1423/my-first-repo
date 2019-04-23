CC = gcc

CFLAGS  = -std=c11 -fopenmp -O2 -ggdb -pedantic -Wall -Wextra
CFLAGS += -Waggregate-return -Wcast-align -Wcast-qual -Wdeclaration-after-statement -Wdouble-promotion -Wduplicated-cond -Wfloat-equal -Wformat=2 -Winit-self -Winline -Wjump-misses-init -Wlogical-op -Wmissing-declarations -Wmissing-format-attribute -Wmissing-include-dirs -Wmissing-noreturn -Wmissing-prototypes -Wnested-externs -Wnull-dereference -Wold-style-definition -Wredundant-decls -Wrestrict -Wshadow -Wstrict-prototypes -Wswitch-default -Wswitch-enum -Wundef -Wuninitialized -Wwrite-strings
LDLIBS = -lm

PROGRAMS = checkpoint

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)
