#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <string.h>
#include <lcd.h>
#include <assert.h>

#define AMG88xx_PIXEL_ARRAY_SIZE 64
#define AMG88xx_PIXEL_ARRAY_BASE 0x80
#define AMG88xx_PIXEL_TEMP_CONVERSION 0.25

int initRes = 0;

void setup()
{
    if (wiringPiSetup() == -1)
    {
        exit(1);
    }
}

void Display(const float *array, int size)
{
    assert(size == 64);
    for (int y = 0; y < 8; ++y)
    {
       printf(
             "%6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f\n",
             array[8*y+0],
             array[8*y+1],
             array[8*y+2],
             array[8*y+3],
             array[8*y+4],
             array[8*y+5],
             array[8*y+6],
             array[8*y+7]
             );
    }
    printf("\n");
}

int ConvertSigned11BitValue(int val)
{

    // bits 0-10 are the absolute value 
    int absVal = (val & 0x7FF);
    if (val & 0x8000) // 0000 1000 0000 0000 - 12th bit is +/-
                      // a 1 in bit 12 indicates -ve
    {
       return -1 * absVal;
    }
    else
    {
       return absVal;
    }
}


// see https://eewiki.net/display/Motley/Panasonic+Grid+Eye+Memory+Registers#PanasonicGridEyeMemoryRegisters-AverageRegister
// for description of AMG8833 sensor
void GetImage(float *values)
{
    int tval = 0;
    for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; ++i)
    {
        int lsb = wiringPiI2CReadReg8(initRes, AMG88xx_PIXEL_ARRAY_BASE + (i << 1));
        int msb = wiringPiI2CReadReg8(initRes, AMG88xx_PIXEL_ARRAY_BASE + (i << 1) + 1);
        tval = (msb << 8 | lsb);
        // tval = wiringPiI2CReadReg16(initRes, AMG88xx_PIXEL_ARRAY_BASE + (i << 2));
        tval = ConvertSigned11BitValue(tval);
        values[i] = AMG88xx_PIXEL_TEMP_CONVERSION * (float)tval;
    }
}

int main()
{
    setup();
    initRes = wiringPiI2CSetup(0x69);
    assert(initRes != -1);

    float pixels[AMG88xx_PIXEL_ARRAY_SIZE];
    memset(pixels, 0, AMG88xx_PIXEL_ARRAY_SIZE  * sizeof(float));

    // printf("Init: %d", initRes);
    int i = 0;
    while (i < 2)
    {
      GetImage(pixels);
      Display(pixels, AMG88xx_PIXEL_ARRAY_SIZE);
      ++i;
      sleep(5); // 5 seconds
    }
    return 0;
}
