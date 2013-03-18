// --------------------------------------------------------------------
// Copyright (c) 2010 by Terasic Technologies Inc.
// --------------------------------------------------------------------
//
// Permission:
//
//   Terasic grants permission to use and modify this code for use
//   in synthesis for all Terasic Development Boards and Altera Development
//   Kits made by Terasic.  Other use of this code, including the selling
//   ,duplication, or modification of any portion is strictly prohibited.
//
// Disclaimer:
//
//   This VHDL/Verilog or C/C++ source code is intended as a design reference
//   which illustrates how these types of functions can be implemented.
//   It is the user's responsibility to verify their design for
//   consistency and functionality through the use of formal
//   verification methods.  Terasic provides no warranty regarding the use
//   or functionality of this code.
//
// --------------------------------------------------------------------
//
//                     Terasic Technologies Inc
//                     356 Fu-Shin E. Rd Sec. 1. JhuBei City,
//                     HsinChu County, Taiwan
//                     302
//
//                     web: http://www.terasic.com/
//                     email: support@terasic.com
//
// --------------------------------------------------------------------

#include "lib/terasic_includes.h"
#include "lib/accelerometer_adxl345_spi.h"
#include "lib/flash.h"
#include "lib/adc_spi_read.h"
#include "lib/I2C.h"
#include "lib/LSM303.h"
#include "includes.h"

typedef void (* MY_TEST_FUNC)(void);

void DEMO_ACCELEROMETER(void);
void DEMO_ADC(void);
void DEMO_PWM(void);
void DEMO_ENCODERS(void);
void DEMO_COMPASS(void);
void DEMO_EEPROM(void);
void DEMO_EPCS(void);
//static void Encoder0_ISR(void* context, alt_u32 id);

typedef enum{
    T_EERPOM,
    T_EPCS,
    T_GSENSOR,
    T_ADC,
    T_PWM,
    T_ENCODERS,
    T_COMPASS
}LOCAL_TEST_ITEM;

typedef struct{
    LOCAL_TEST_ITEM TestId;
    char szName[64];
    MY_TEST_FUNC Func;
}TEST_ITEM_INFO;

static TEST_ITEM_INFO szTestList[] = {
    {T_GSENSOR, "ACCELEROMETER", DEMO_ACCELEROMETER},
    {T_PWM, "PWM", DEMO_PWM},
    {T_ENCODERS, "ENCODERS", DEMO_ENCODERS},
    {T_COMPASS, "COMPASS", DEMO_COMPASS},
    {T_ADC, "ADC", DEMO_ADC},
    {T_EERPOM, "EEPROM", DEMO_EEPROM},
    {T_EPCS, "EPCS", DEMO_EPCS},
};



//OS_EVENT *QueueEvent;
//void * MyQueue[ 100 ];
bool bKeyPressed = FALSE;
//double encoder0a_count;
//volatile int edge_capture;

static void KEY_ISR(void* context, alt_u32 id){
    if (id == KEY_IRQ){
        bKeyPressed = TRUE;
        printf("Button pressed!\n");

        // clear interrupt flag
        IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE,0);
    }
}

void EnableKeyInterrupt(void){
    int error;

    // enable interrupt, 2-keybutton
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(KEY_BASE,0x03);

    // clear capture flag
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE,0);
    //

    bKeyPressed = FALSE;
    // register interrupt isr
    error = alt_irq_register (KEY_IRQ, 0, KEY_ISR);
    if (error)
        printf("Failed to register interrupt\r\n");

}



void ShowMenu(void){
    int i,num;
    printf("---------------------------------\r\n");
    printf("- Selection function:\r\n");
    num = sizeof(szTestList)/sizeof(szTestList[0]);
    for(i=0;i<num;i++){
        printf("- [%d]%s\r\n", i, szTestList[i].szName);
    }
    printf("---------------------------------\r\n");
}


void DEMO_PWM(void){
	double duty = 0;
	duty = 0.75;

	//reset PWM pins
	IOWR(PWM_0_BASE, 0, 0);
	IOWR(PWM_1_BASE, 0, 0);
	IOWR(PWM_2_BASE, 0, 0);
	IOWR(PWM_3_BASE, 0, 0);
	//alt_u16 L298_IN = PWM_0_BASE;
	//alt_u16 L298_IN_ALT = PWM_1_BASE;

	while (1)
	{
		//IOWR(PWM_0_BASE, 0, 1 * 31250);

		IOWR(PWM_0_BASE, 0, 0.25 * 31250);
		usleep(2000000); //sleep 2 seconds
		IOWR(PWM_0_BASE, 0, 0.50 * 31250);
		usleep(3000000);
		IOWR(PWM_0_BASE, 0, 0.75 * 31250);
		usleep(4000000);
		IOWR(PWM_0_BASE, 0, 1 * 31250);
		usleep(3000000);
		IOWR(PWM_0_BASE, 0, 0.75 * 31250);
		usleep(4000000);
		IOWR(PWM_0_BASE, 0, 0.5 * 31250);
		usleep(3000000);
		IOWR(PWM_0_BASE, 0, 0); //STOP
		usleep(3000000);

		//reverse direction
		IOWR(PWM_1_BASE, 0, 0.65 * 31250);
		usleep(3000000);
		IOWR(PWM_1_BASE, 0, 0.9 * 31250);
		usleep(8000000);
		IOWR(PWM_1_BASE, 0, 0.7 * 31250);
		usleep(3000000);
		IOWR(PWM_1_BASE, 0, 0);
		usleep(2000000);
		//IOWR(PWM_2_BASE, 0, 0.75 * 31250);
        //usleep(2000000);
	}
}


void DEMO_ENCODERS(void){
	//read from pins like for PWM
	double encode_0_a, encode_1_a; //encode_b;
	double counts_0_new, counts_0_old;
	double counts_1_new, counts_1_old;
	counts_0_old = 0;
	counts_1_old = 0;

	/* Init motor */
	IOWR(PWM_0_BASE, 0, 0);
	IOWR(PWM_1_BASE, 0, 0);
	IOWR(PWM_2_BASE, 0, 0);
	IOWR(PWM_3_BASE, 0, 0);

	IOWR(PWM_0_BASE, 0, 1 * 31250);
	//IOWR(PWM_2_BASE, 0, 0.5 * 31250);

	//64 counts per revolution, @ 100% is 80rev/min
	//H-bridge slows voltage output, motors see around 8V max
	//(64 counts per revolution of motor shaft) * (131 revolutions of motor shaft per revolutions of output shaft) * (1 revolution of output shaft per 2*PI*4 distance)
	// = 210712.9025
	//    60 / (duty * 80)
/// 43 increments (12V), 1.3333 revs per second
/*
	counting only the rising edges of channel A = half of counting the rising/falling edges of channel A+B
	So 64 counts => 32 counts per revolution
*/

	while (1)
	{
		//1st motor
		counts_0_new = ((double) IORD(ENCODER_0A_BASE, 0)) / 32;
		encode_0_a = (counts_0_new-counts_0_old);
		//encode_b = (((double) IORD(ENCODER_0B_BASE, 0)) / 32) - encode_b;
		if (encode_0_a < 0){
			encode_0_a = counts_0_new;
		}
		counts_0_old = counts_0_new;

		//2nd motor
		counts_1_new = ((double) IORD(ENCODER_1A_BASE, 0)) / 32;
		encode_1_a = (counts_1_new-counts_1_old);
		if (encode_1_a < 0){
			encode_1_a = counts_1_new;
		}
		counts_1_old = counts_1_new;

		printf("Motor 0 encoder A value is :( %f ) \n", encode_0_a);
		printf("Motor 0 revolutions per second :( %f ) \n", encode_0_a / 64);
		printf("Motor 1 encoder A value is :( %f ) \n", encode_1_a);
		printf("Motor 1 revolutions per second :( %f ) \n", encode_1_a / 64);

		//printf("Current encoder B value is :( %f ) \n", encode_b / 32);
		usleep(1000000);
	}
}

void DEMO_COMPASS(void){

    // set clock as output
    //IOWR_ALTERA_AVALON_PIO_DIRECTION(I2C_SCL_BASE, ALTERA_AVALON_PIO_DIRECTION_OUTPUT);
    //IOWR(SELECT_I2C_CLK_BASE, 0, 0x01);

    //if (I2C_MultipleRead(I2C_SCL_BASE, I2C_SDA_BASE, DeviceAddr, ControlAddr, szBuf, sizeof(szBuf))){
	LSM303_Init();
	enableDefault();
}

void DEMO_ACCELEROMETER(void){

	/*
    bool bSuccess;
    alt_16 szXYZ[3];
    alt_u8 id;
    const int mg_per_digi = 4;

    // release i2c pio pin
    //IOWR_ALTERA_AVALON_PIO_DIRECTION(I2C_SCL_BASE, ALTERA_AVALON_PIO_DIRECTION_OUTPUT);
    //IOWR_ALTERA_AVALON_PIO_DIRECTION(I2C_SDA_BASE, ALTERA_AVALON_PIO_DIRECTION_INPUT);
    IOWR(SELECT_I2C_CLK_BASE, 0, 0x00);

    // configure accelerometer as +-2g and start measure
    bSuccess = ADXL345_SPI_Init(GSENSOR_SPI_BASE);
    if (bSuccess){
        // dump chip id
        bSuccess = ADXL345_SPI_IdRead(GSENSOR_SPI_BASE, &id);
        if (bSuccess)
            printf("id=%02Xh\r\n", id);
    }

    if (bSuccess)
        printf("Monitor Accerometer Value. Press KEY0 or KEY1 to terminal the monitor process.\r\n");

    while(bSuccess && !bKeyPressed){
        if (ADXL345_SPI_IsDataReady(GSENSOR_SPI_BASE)){
            bSuccess = ADXL345_SPI_XYZ_Read(GSENSOR_SPI_BASE, szXYZ);
            if (bSuccess){
                printf("X=%d mg, Y=%d mg, Z=%d mg\r\n", szXYZ[0]*mg_per_digi, szXYZ[1]*mg_per_digi, szXYZ[2]*mg_per_digi);
                // show raw data,
                //printf("X=%04x, Y=%04x, Z=%04x\r\n", (alt_u16)szXYZ[0], (alt_u16)szXYZ[1],(alt_u16)szXYZ[2]);
                usleep(1000*1000);
            }
        }
    }

    if (!bSuccess)
        printf("Failed to access accelerometer\r\n");
*/
}

void DEMO_ADC(void){
    int ch = 0, next_ch=0;
    alt_u16 data16;
    float volt = 0;

    printf("Monitor ADC Value. Press KEY0 or KEY1 to terminal the monitor process.\r\n");
    ADC_Read(next_ch);
    while(!bKeyPressed){
        //next_ch++;
        //if (next_ch >= 8)
        //    next_ch = 0;
        data16 = ADC_Read(next_ch); // 12-bits resolution
        volt = (float)data16 * 3.3 / 4095.0;
	    printf("CH%d=%.2f V\r\n", ch, volt);
	    usleep(1000000);
	    //ch = next_ch;
    }
}

int main(void)
{
    int sel;
    int nNum;

    nNum = sizeof(szTestList)/sizeof(szTestList[0]);
    printf("DE-Nano Demo\r\n");

    while(1){
        ShowMenu();
        printf("Select:");
        scanf("%d", &sel);
        if (sel >= 0 && sel < nNum){
            bKeyPressed = FALSE;
            printf("Demo %s\r\n",szTestList[sel].szName);
            szTestList[sel].Func();
        }else{
            printf("Invalid Selection\r\n");
        }

        sel++;
    }

    return 0;
}


void DEMO_EEPROM(void){
    alt_u8 szBuf[16];
    int i,Num;
    const alt_u8 DeviceAddr = 0xA0;
    const alt_u8 ControlAddr = 00;

    // set clock as output
    //IOWR_ALTERA_AVALON_PIO_DIRECTION(I2C_SCL_BASE, ALTERA_AVALON_PIO_DIRECTION_OUTPUT);
    IOWR(SELECT_I2C_CLK_BASE, 0, 0x01);

    if (I2C_MultipleRead(I2C_SCL_BASE, I2C_SDA_BASE, DeviceAddr, ControlAddr, szBuf, sizeof(szBuf))){
        Num = sizeof(szBuf)/sizeof(szBuf[0]);
        for(i=0;i<Num;i++){
            printf("Addr[%02d] = %02xh\r\n", i, szBuf[i]);
        }
    }else{
        printf("Failed to access EEPROM\r\n");
    }
}

void DEMO_EPCS(void){
    alt_u32 MemSize;
    MemSize = Flash_Size(EPCS_NAME);
    printf("EPCS Size:%d Bytes (%d MB)\r\n", (int)MemSize, (int)MemSize/1024/1024);

}

