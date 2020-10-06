# Omair Khan
# Student ID: 914135359

all: sshell.c
	gcc sshell.c -Wall -Werror -Wextra -o sshell

clean:
				rm -rf sshell