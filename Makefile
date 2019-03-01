all:
	gcc -Wall -Wextra BMP180.c -o bmp180
clean:
	rm bmp180