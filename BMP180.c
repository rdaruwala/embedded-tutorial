#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <math.h>

int main() 
{
	// Create I2C bus
	char *bus = "/dev/i2c-2";
	int file = open(bus, O_RDWR);
	if(file < 0){
		printf("Failed to open the bus.\n");
		exit(1);
	}
	
	// Get I2C device, BME180 I2C address is 0x77(119)
	if(ioctl(file, I2C_SLAVE, 0x77) < 0){
		printf("Failed to set I2C address.\n");
		exit(1);
	}

	// Calibration Cofficients stored in EEPROM of the device
	// Read 22 bytes of data from address 0xAA(170)
	char data[22] = {0};
	
	if(write(file, 0xAA, 1) != 1){
		printf("I2C write error\n");
		exit(1);
	}
	
	if(read(file, data, 22) != 22){
		printf("I2C data read error - Didn't read correct number of bytes\n");
		exit(1);
	}

	// Convert the data
	long AC1 = data[0] * 256 + data[1];
	if (AC1 > 32767) { AC1 -= 65535; }
	long AC2 = data[2] * 256 + data[3];
	if (AC2 > 32767) { AC2 -= 65535; }
	long AC3 = data[4] * 256 + data[5];
	if (AC3 > 32767) { AC3 -= 65535; }
	long AC4 = data[6] * 256 + data[7];
	long AC5 = data[8] * 256 + data[9];
	long AC6 = data[10] * 256 + data[11];
	long B1 = data[12] * 256 + data[13];
	if (B1 > 32767) { B1 -= 65535; }
	long B2 = data[14] * 256 + data[15];
	if (B2 > 32767) { B2 -= 65535; }
	long MB = data[16] * 256 + data[17];
	if (MB > 32767) { MB -= 65535; }
	long MC = data[18] * 256 + data[19];
	if (MC > 32767) { MC -= 65535; }
	long MD = data[20] * 256 + data[21];
	if (MD > 32767) { MD -= 65535; }
	sleep(1);

	// Select measurement control register(0xF4)
	// Enable temperature measurement(0x2E)
	char config[2]= {0};
	config[0] = 0xF4;
	config[1] = 0x2E;
	if(write(file, config, 2) != 2){
		printf("I2C write data error\n");
		exit(1);
	}
	sleep(1);
	
	// Read 2 bytes of data from register(0xF6)
	// temp msb, temp lsb
	if(write(file, 0xF6, 1)){
		printf("I2C write data error\n");
		exit(1);
	}
	
	if(read(file, data, 2) != 2)
	{
		printf("Erorr : Input/output Erorr \n");
		exit(1);
	}

	// Convert the data
	int temp = (data[0] * 256 + data[1]);

	// Select measurement control register(0xf4)
	// Enable pressure measurement, OSS = 1(0x74)
	config[0] = 0xF4;
	config[1] = 0x74;
	
	if(write(file, config, 2) != 2){
		printf("I2C write data error\n");
		exit(1);
	}

	sleep(1);
	
	// Read 3 bytes of data from register(0xF6)
	// pres msb1, pres msb, pres lsb
	if(write(file, 0xF6, 1) != 1){
		printf("I2C write data error\n");
		exit(1);
	}
	
	if(read(file, data, 3) != 3){
		printf("I2C read data error\n");
		exit(1);
	}

	// Convert the data
	double pres = (data[0] * 65536 + (data[1] * 256) + data[2]) / 128;

	// Callibration for Temperature - See datasheet
	double X1 = (temp - AC6) * AC5 / 32768.0;
	double X2 = (MC * 2048.0) / (X1 + MD);
	double B5 = X1 + X2;
	double cTemp = ((B5 + 8.0) / 16.0) / 10.0;
	double fTemp = cTemp * 1.8 + 32;

	// Calibration for Pressure - See datasheet
	double B6 = B5 - 4000;
	X1 = (B2 * (B6 * B6 / 4096.0)) / 2048.0;
	X2 = AC2 * B6 / 2048.0;
	double X3 = X1 + X2;
	double B3 = (((AC1 * 4 + X3) * 2) + 2) / 4.0;
	X1 = AC3 * B6 / 8192.0;
	X2 = (B1 * (B6 * B6 / 2048.0)) / 65536.0;
	X3 = ((X1 + X2) + 2) / 4.0;
	double B4 = AC4 * (X3 + 32768) / 32768.0;
	double B7 = ((pres - B3) * (25000.0));
	double pressure = 0.0;
	if(B7 < 2147483648LU)
	{
		pressure = (B7 * 2) / B4;
	}
	else
	{
		pressure = (B7 / B4) * 2;
	}
	X1 = (pressure / 256.0) * (pressure / 256.0);
	X1 = (X1 * 3038.0) / 65536.0;
	X2 = ((-7357) * pressure) / 65536.0;
	double pressure1 = (pressure + (X1 + X2 + 3791) / 16.0) / 100;

	// Calculate Altitude - See datasheet
	double altitude = 44330 * (1 - pow(pressure1/1013.25, 0.1903));

	// Output data
	printf("Altitude : %.2f m \n", altitude);
	printf("Pressure : %.2f hPa \n", pressure1);
	printf("Temperature in Celsius : %.2f C \n", cTemp);
	printf("Temperature in Fahrenheit : %.2f F \n", fTemp);
}