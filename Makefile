all:
	gcc -Wall -Wextra BMP180.c -o bmp180 -lm
clean:
	rm bmp180