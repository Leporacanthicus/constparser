all: constparser

constparser: cp.cpp
	clang++ -g -Wall -Wextra -Werror cp.cpp -o $@
