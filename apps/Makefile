CC=gcc
DEPS = ../disk/disk_controller.h ../io/File.h
OBJ = ../disk/disk_controller.o ../io/File.o
CFLAGS=-pedantic-errors -Wall -Werror -std=c11 -g

all: clean test01 test02 test03 test04 test05 test06 test07

test01:
	$(CC) -pedantic-errors -Werror -Wall -std=c11 -g test01.c ../disk/disk_controller.c ../io/File.c -o test01

test02: 
	$(CC) -pedantic-errors -Werror -Wall -std=c11 -g test02.c ../disk/disk_controller.c ../io/File.c -o test02

test03:
	$(CC) -pedantic-errors -Werror -Wall -std=c11 -g test03.c ../disk/disk_controller.c ../io/File.c -o test03
	
test04:
	$(CC) -pedantic-errors -Werror -Wall -std=c11 -g test04.c ../disk/disk_controller.c ../io/File.c -o test04

test05:
	$(CC) -pedantic-errors -Werror -Wall -std=c11 -g test05.c ../disk/disk_controller.c ../io/File.c -o test05
	
test06:
	$(CC) -pedantic-errors -Werror -Wall -std=c11 -g test06.c ../disk/disk_controller.c ../io/File.c -o test06
	
test07:
	$(CC) -pedantic-errors -Werror -Wall -std=c11 -g test07.c ../disk/disk_controller.c ../io/File.c -o test07


clean:
	rm -f *.o ../disk/disk_controller.o ../io/File.o test01 test02 test03 test04 test05 test06 test07