all: bytearray.c
	gcc -c -Wall -Werror -fPIC bytearray.c -lyaslapi
	gcc -shared -o libbytearray.so bytearray.o -lyaslapi
