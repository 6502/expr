CC = g++

ifeq ($(DEBUG), 1)
CCOPTS = -Wall -g -O0
else
CCOPTS = -Wall -O3
endif

CCOPTS_11 = $(CCOPTS) -std=c++0x

all: test_expr test_expr_11 test

clean:
	rm -f test_expr test_expr_11 test.pgm *.gcov *.gcda *.gcno

test_expr: test_expr.cpp expr.h
	$(CC) $(CCOPTS) -otest_expr test_expr.cpp

test_expr_11: test_expr.cpp expr.h
	$(CC) $(CCOPTS_11) -otest_expr_11 test_expr.cpp

test: test_expr test_expr_11
	./test_expr
	./test_expr_11

coverage:
	g++ -Wall -O0 -g -coverage -otest_expr test_expr.cpp
	./test_expr
	gcov test_expr
