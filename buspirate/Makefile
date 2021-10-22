
SRCS=buspirate.c test.c tsl2561sparkfun.c
OBJS=$(subst .c,.o,${SRCS})

CFLAGS=-Wall -g -O2 -I.

LDFLAGS=-lm

all: depend yasl_cli libyasl_buspirate.so

yasl_cli: ${OBJS}
	${CC} $(LDFLAGS) -o $@ $^

libyasl_buspirate.so: buspirate.c
	${CC} $(LDFLAGS)  -fPIC -c $^
	${LD} -shared  -o $@ buspirate.o

.PHONY: clean
clean:
	rm -f *~ *.o yasl_cli depend.d *.so yasl/*~

.PHONY: compile_commands.json
compile_commands.json:
	make clean
	compiledb make

depend:
	$(CC) -M ${SRCS} > depend.d

-include depend.d
