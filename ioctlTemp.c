#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

int i2cHandle = 0;

void SetReadMode()
{
    i2cHandle = open("/dev/i2c-1", O_RDWR);
    // wiringPiI2CWrite(initRes, 0xE3); // no hold master

    if (i2cHandle < 0)
    {
      printf("could not open i2c-1 file\n");
      return;
    }
    if (ioctl(i2cHandle, I2C_SLAVE, 0x40) < 0)
    {
      printf("could not open i2c slave sensor at address 0x40\n");
      return;
    }

    char writeBuffer[1] = {0xE3};
    if(write(i2cHandle, writeBuffer, 1)!=1)
    {
       perror("Failed to set the mode\n");
       return;
    }

    close(i2cHandle);
}

void ReadTemp()
{

    printf("Reading temp\n");
    i2cHandle = open("/dev/i2c-1", O_RDONLY);

    if (i2cHandle < 0)
    {
      printf("could not open i2c-1 file\n");
      return;
    }
    if (ioctl(i2cHandle, I2C_SLAVE, 0x40) < 0)
    {
      printf("could not open i2c slave sensor at address 0x40\n");
      return;
    }


    char msbb[1];
    char lsbb[1];
    char chkb[1];
    read(i2cHandle, msbb, 1);
    read(i2cHandle, lsbb, 1);
    read(i2cHandle, chkb, 1);
    printf("0x%x 0x%x\n", msbb[0], lsbb[0]);
    int msb = msbb[0], lsb = lsbb[0];
    if (msb <= 0 || lsb <= 0) // || msb != lsb)
    {
      printf("Invalid Read.\n");
      return;
    }
    int temp1  = (msb << 8) | lsb;
    float resultCelsius = (175.72*temp1/(float)65536) - 46.85;
    printf("Read %6.2f celsius %6.2f fahrenheit\n", resultCelsius, resultCelsius*1.8 + 32);

    close(i2cHandle);
}

int main()
{
    SetReadMode();
    for (int i = 0; i < 5; ++i)
    {
        ReadTemp();
        sleep(3);
    }
    return 0;

}
