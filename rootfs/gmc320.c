/* sensor:
 * Based on some code from Christoph Haas
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <time.h>
#include <inttypes.h>

int serial_port;

static  int BOOL_TRUE = 1;
static  int BOOL_FALSE = 0;

struct gyro_sensor {
	 	uint16_t x;
		uint16_t y;
		uint16_t z;
		uint8_t event;  // always 0xaa
		};

typedef struct gyro_sensor Gyro_Sensor;

int gmc_write(int device, const char *cmd) {
	return  write(device, cmd, strlen(cmd));
}

int gmc_read(int device, char *buf, int length) {
	return read(device, buf, length);
}

int gmc_flush(int device) {
	char ch;

	// flush input stream (max 100 bytes)
	for (int i = 0; i < 100; i++) {
		if (read(device, &ch, 1) == 0)
			return BOOL_TRUE;
	}
	return BOOL_FALSE;
}


// Return:   None
int gmc_set_heartbeat_off(int device) {
	char cmd[] = "<HEARTBEAT0>>";

	if (gmc_write(device, cmd) == (ssize_t) strlen(cmd))
		return gmc_flush(device);
	return BOOL_FALSE;
}


int gmc_open(const char *device, int baud) {
	int gc_fd = -1;
	struct termios tio;

	memset(&tio, 0, sizeof(struct termios));
	tio.c_cflag = CS8 | CREAD | CLOCAL;		// 8n1
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 5;

	int tio_baud = B115200;
	switch(baud) {
		case 9600: tio_baud = B9600; break;
		case 19200: tio_baud = B19200; break;
		case 38400: tio_baud = B38400; break;
		case 57600: tio_baud = B57600; break;
		case 115200: tio_baud = B115200; break;
	}

	if ((gc_fd = open(device, O_RDWR)) != -1) {
		if (cfsetspeed(&tio, tio_baud) == 0) { // set baud speed
			if (tcsetattr(gc_fd, TCSANOW, &tio) == 0) { // apply baud speed change
				if (gmc_set_heartbeat_off(gc_fd)) { // disable heartbeat, we use polling
					return gc_fd;
				}
			}
		} else { // something failed
			close(gc_fd);
			gc_fd = -1;
		}
	}
	return gc_fd;
}


void gmc_close(int device) {
	close(device);
}


// Return:   A 16 bit unsigned integer is returned. In total 2 bytes data return from GQ GMC unit. The first byte is MSB byte data and second byte is LSB byte data.
int gmc_get_cpm(int device) {
	char cmd[] = "<GETCPM>>";
	char buf[2] = { 0 };

	gmc_flush(serial_port);
	if (gmc_write(device, cmd) == (ssize_t) strlen(cmd))
		gmc_read(device, buf, 2);
	else
		printf("write error");

	return buf[0] * 256 + buf[1];
}


// Return:  A 32 bit unsigned integer is returned from GMC-500+ (and newer devices?).
uint32_t gmc_get_cpm32(int device) {
	char cmd[] = "<GETCPM>>";
	char buf[4] = { 0 };

	gmc_flush(serial_port);
	if (gmc_write(device, cmd) == (ssize_t) strlen(cmd))
		gmc_read(device, buf, 4);
	else
		printf("write error");

	return (uint32_t)buf[0] << 24 |
      (uint32_t)buf[1] << 16 |
      (uint32_t)buf[2] << 8  |
      (uint32_t)buf[3];
}


// Return: Four bytes celsius degree data in hexdecimal: BYTE1,BYTE2,BYTE3,BYTE4
float gmc_get_temperature(int device) {
	char cmd[] = "<GETTEMP>>";
	char buf[4] = { 0 };

	gmc_flush(serial_port);
	if (gmc_write(device, cmd) == (ssize_t) strlen(cmd)) {
		gmc_read(device, buf, 4);
	} else
		printf("write error");

	int sign =  buf[2] == 0 ? 1 : -1;
	float temp = buf[0];
	temp += (float) buf[1] / 10;
	temp = temp * sign;
	return temp;
}


// Return:   one byte voltage value of battery (X 10V)
int gmc_get_volt(int device) {
	char cmd[] = "<GETVOLT>>";
	char buf[1] = { 0 };

	gmc_flush(serial_port);
	if (gmc_write(device, cmd) == (ssize_t) strlen(cmd)) {
		gmc_read(device, buf, 1);
	} else
		printf("write error");

	return buf[0];
}


// Seven bytes gyroscope data in hexdecimal: BYTE1,BYTE2,BYTE3,BYTE4,BYTE5,BYTE6,BYTE7
//	Here: BYTE1,BYTE2 are the X position data in 16 bits value. The first byte is MSB byte data and second byte is LSB byte data.
//	      BYTE3,BYTE4 are the Y position data in 16 bits value. The first byte is MSB byte data and second byte is LSB byte data.
//	      BYTE5,BYTE6 are the Z position data in 16 bits value. The first byte is MSB byte data and second byte is LSB byte data.
//	      BYTE7 always 0xAA
int gmc_get_gyro(int device, Gyro_Sensor *gyro) {
	const char cmd[] = "<GETGYRO>>";

	if (gmc_write(device, cmd) == (ssize_t) strlen(cmd)) {
		gmc_read(device, (char *) &gyro->x, 7);
		return BOOL_TRUE;
	}
	return BOOL_FALSE;
}


// Return: serial number in 7 bytes.
float gmc_get_serial(int device, char *serialNum) {
	char cmd[] = "<GETSERIAL>>";
	char buf[8] = {0};

	gmc_flush(serial_port);
	if (gmc_write(device, cmd) == (ssize_t) strlen(cmd)) {
		gmc_read(device, buf, 7);
		sprintf(serialNum,"%0x%0x%0x%0x%0x%0x%0x", (unsigned char)buf[0], (unsigned char)buf[1],(unsigned char)buf[2],(unsigned char)buf[3],(unsigned char)buf[4],(unsigned char)buf[5],(unsigned char)buf[6]);
		return BOOL_TRUE;
	}
	return BOOL_FALSE;
}


//Return:   total 14 bytes ASCII chars from GQ GMC unit. It includes 7 bytes hardware model and 7 bytes firmware version
int gmc_get_version(int device, char *version) {
	char cmd[] = "<GETVER>>";
	char buf[16] = {0};

	if (gmc_write(device, cmd) == (ssize_t) strlen(cmd)) {
		gmc_read(device, buf, 14);
		buf[15] = 0;
		strncpy(version, buf, 15);
		return BOOL_TRUE;
	}
	return BOOL_FALSE;
}


// <SETDATETIME[YYMMDDHHMMSS]>>
int setDateTime(int device) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char cmd[40];
	sprintf(cmd,"<SETDATETIME%d%02d%02d%02d%02d%02d>>", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	gmc_write(device,cmd);
}


int main(int argc, char *argv[]) {

	if (argc != 3) {
		printf("You need to specify the serial device and baudrate used by the GMC radation monitor i.e. %s /dev/ttyUSB0 115200\n", argv[0]);
		return 1;
	}

	serial_port = gmc_open(argv[1], (int)argv[2]);
	if (serial_port == -1) {
		printf("Cannot open specified serial device\n");
		return 1;
	};
	
	printf("{");
	
	char version[20];
	gmc_get_version(serial_port, version);
	printf(" \"model\": \"%s\",", version);

	char serialNumber[20];
	gmc_get_serial(serial_port, serialNumber);
	printf(" \"serial\": \"%s\",",serialNumber);

    time_t rawtime;
    struct tm *timeinfo;
    char dateTimeBuffer[80]; // Buffer to hold the formatted string
    time(&rawtime);
    timeinfo = localtime(&rawtime); // Convert to local time structure
    strftime(dateTimeBuffer, sizeof(dateTimeBuffer), "%Y-%m-%d %H:%M:%S", timeinfo);  // Format the time as "YYYY-MM-DD HH:MM:SS"
    printf(" \"time_of_reading\": \"%s\",", dateTimeBuffer);

	uint32_t cpm32 = gmc_get_cpm32(serial_port);
	printf(" \"cpm\": %"PRIu32",",cpm32);

	// TODO: is this accurate?
	float uSvHr = cpm32 / 150.00;
	printf(" \"μSv/h\": %.2f,", uSvHr);

	float temp = gmc_get_temperature(serial_port);
	printf(" \"temp\": %.1f,", temp);

	float volt = gmc_get_volt(serial_port)/10;
	printf(" \"volt\": %0.1f,", volt);

	Gyro_Sensor gyro;
	gmc_get_gyro(serial_port, &gyro);
	printf(" \"gyro_x\": %d, \"gyro_y\": %d, \"gyro_z\": %d", gyro.x, gyro.y, gyro.z);

	printf(" }");
	printf("\r\n");
	
	setDateTime(serial_port);
	gmc_close(serial_port);

	return 0; // success
};
