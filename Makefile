all: constparser

constparser: cp.cpp
	clang++ -g -Wall -Wextra -Werror cp.cpp -o $@

check: test.txt constparser
	./constparser < test.txt > test.res
	diff test.res test.expected

