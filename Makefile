# Makefile for NotThatCPL interpreter : Centurion Programming Language

CCP := g++

CFLAGS += -Wall -Wextra -Wpedantic -DNOTTHATCPL_WITH_DEBUGGING
#CFLAGS += -Wall -Wextra -Wpedantic

ifeq "$(MAKECMDGOALS)" "debug"
   CFLAGS += -O0 -g
else
   CFLAGS += -O2
   BFLAGS += -s
endif

.PHONY: all clean release debug
all: bin/CPL


clean:
	rm bin/CPL | true
	rm -rf obj/*.o | true

release: all


debug: all


bin/CPL: obj/CPL.o obj/Screen.o obj/SigInt.o obj/Compiler.o obj/Execution.o | bin
	$(CCP) $(CFLAGS) $(BFLAGS) -o bin/CPL obj/*.o

obj/CPL.o: NotThatCPL.cpp | obj
	$(CCP) $(CFLAGS) -c -o obj/CPL.o NotThatCPL.cpp

obj/Screen.o: Screen.cpp | obj
	$(CCP) $(CFLAGS) -c -o obj/Screen.o Screen.cpp

obj/SigInt.o: SigInt.cpp | obj
	$(CCP) $(CFLAGS) -c -o obj/SigInt.o SigInt.cpp

obj/Compiler.o: Compiler.cpp | obj
	$(CCP) $(CFLAGS) -c -o obj/Compiler.o Compiler.cpp

obj/Execution.o: Execution.cpp | obj
	$(CCP) $(CFLAGS) -c -o obj/Execution.o Execution.cpp

bin:
	mkdir bin

obj:
	mkdir obj
