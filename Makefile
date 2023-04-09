all: abdullahsh.c
	gcc -fsanitize=address -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Werror -std=c17 -Wpedantic -O0 -g abdullahsh.c -o abdullahsh

clean:
	rm -rf *.o abdullahsh d
