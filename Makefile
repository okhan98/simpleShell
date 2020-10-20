# Omair Khan, Ethan Turner
# Student IDs: 914135359, 915729654

all: sshell.c
	gcc sshell.c -Wall -Werror -Wextra -o sshell

clean:
	rm -rf sshell
