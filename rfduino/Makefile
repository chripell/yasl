
SRCS=server.cpp protocol.c

include rfduino.mk

.PHONY: compile_commands.json
compile_commands.json:
	make clean
	compiledb make
