#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>

#define ADC_ADDR 0x68

// I2C definitions

#define I2C_SLAVE	0x0703
#define I2C_SMBUS	0x0720	/* SMBus-level access */

#define I2C_SMBUS_READ	1
#define I2C_SMBUS_WRITE	0

// SMBus transaction types

#define I2C_SMBUS_QUICK		    0
#define I2C_SMBUS_BYTE		    1
#define I2C_SMBUS_BYTE_DATA	    2
#define I2C_SMBUS_WORD_DATA	    3
#define I2C_SMBUS_PROC_CALL	    4
#define I2C_SMBUS_BLOCK_DATA	    5
#define I2C_SMBUS_I2C_BLOCK_BROKEN  6
#define I2C_SMBUS_BLOCK_PROC_CALL   7		/* SMBus 2.0 */
#define I2C_SMBUS_I2C_BLOCK_DATA    8

// SMBus messages

#define I2C_SMBUS_BLOCK_MAX	32	/* As specified in SMBus standard */
#define I2C_SMBUS_I2C_BLOCK_MAX	32	/* Not specified but we use same structure */


#define	MCP3422_SR_240	0
#define	MCP3422_SR_60	1
#define	MCP3422_SR_15	2
#define	MCP3422_SR_3_75	3

#define	MCP3422_GAIN_1	0
#define	MCP3422_GAIN_2	1
#define	MCP3422_GAIN_4	2
#define	MCP3422_GAIN_8	3


using namespace std;

union i2c_smbus_data
{
  uint8_t  byte ;
  uint16_t word ;
  uint8_t  block [I2C_SMBUS_BLOCK_MAX + 2] ;	// block [0] is used for length + one more for PEC
} ;

struct i2c_smbus_ioctl_data
{
  char read_write ;
  uint8_t command ;
  int size ;
  union i2c_smbus_data *data ;
} ;


static inline int i2c_smbus_access (int fd, char rw, uint8_t command, int size, union i2c_smbus_data *data)
{
  struct i2c_smbus_ioctl_data args ;

  args.read_write = rw ;
  args.command    = command ;
  args.size       = size ;
  args.data       = data ;
  return ioctl (fd, I2C_SMBUS, &args) ;
}

int openDevice(char* devPath, int devAddr)
{
    int fd;
    int r;

    fd = open(devPath, O_RDWR);
    if(fd <= 0) {
		return -1;
	}

    if( ( r = ioctl(fd, I2C_SLAVE, devAddr)) < 0) {
		return -1;
	}

    return fd;
}

void waitForConversion (int fd, unsigned char *buffer, int n)
{
  for (;;)
  {
    read (fd, buffer, n) ;
    if ((buffer [n-1] & 0x80) == 0)
      break ;
    usleep (1000) ;
  }
}

int readValue(int fd, int channel, int sampleRate, int gain)
{
    unsigned char config ;
    unsigned char buffer [4] ;
    int value = 0 ;

    config = 0x80 | (channel << 5) | (sampleRate << 2) | (gain);

    i2c_smbus_access (fd, I2C_SMBUS_WRITE, config, I2C_SMBUS_BYTE, NULL);

  switch (sampleRate)	// Sample rate
  {
    case MCP3422_SR_3_75:			// 18 bits
      waitForConversion (fd, &buffer [0], 4) ;
      value = ((buffer [0] & 3) << 16) | (buffer [1] << 8) | buffer [2] ;
      break ;

    case MCP3422_SR_15:				// 16 bits
      waitForConversion (fd, buffer, 3) ;
      value = (buffer [0] << 8) | buffer [1] ;
      break ;

    case MCP3422_SR_60:				// 14 bits
      waitForConversion (fd, buffer, 3) ;
      value = ((buffer [0] & 0x3F) << 8) | buffer [1] ;
      break ;

    case MCP3422_SR_240:			// 12 bits - default
      waitForConversion (fd, buffer, 3) ;
      value = ((buffer [0] & 0x0F) << 8) | buffer [1] ;
      break ;
  }


    return value;
}

int main(int argc, char **argv)
{
    int fd;
    int value;

    double dVperDiv = 0.000015625;
    int	resistorDividerRatio = 7;

    fd = openDevice("/dev/i2c-2", ADC_ADDR);
    if(fd<0)
        return fd;


    while(true) {
        value = readValue(fd, 0, MCP3422_SR_3_75, MCP3422_GAIN_1);
        printf("%f volts on input\n\n", value * dVperDiv * resistorDividerRatio);
        sleep(1);
    }


}

