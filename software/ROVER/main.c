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

#define ALT_LEGACY_INTERRUPT_API_PRESENT

#include "lib/terasic_includes.h"
#include "lib/accelerometer_adxl345_spi.h"
#include "lib/flash.h"
#include "lib/adc_spi_read.h"
#include "lib/I2C.h"
#include "lib/LSM303.h"


#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    task3_stk[TASK_STACKSIZE];
OS_STK    task4_stk[TASK_STACKSIZE];
OS_STK    task5_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define TASK1_PRIORITY      2
#define TASK2_PRIORITY      3
#define TASK3_PRIORITY      4
#define TASK4_PRIORITY      5
#define TASK5_PRIORITY      1

/* Queue */
#define QueueSize 5
OS_EVENT* MainQueue;

typedef void (* MY_TEST_FUNC)(void);

void Encoder_Task(void* pData);
void PWM_Task(void* pData);
void LSM303_Task(void* pData);
void ADC_Task(void* pData);
void DEMO_EEPROM(void);
void DEMO_EPCS(void);

typedef enum{
    T_EERPOM,
    T_EPCS,
    T_GSENSOR,
    T_ADC,
}LOCAL_TEST_ITEM;

typedef struct{
    LOCAL_TEST_ITEM TestId; 
    char szName[64];
    MY_TEST_FUNC Func;
}TEST_ITEM_INFO;


static TEST_ITEM_INFO szTestList[] = {
    {T_GSENSOR, "ACCELEROMETER", DEMO_ACCELEROMETER},
    {T_ADC, "ADC", DEMO_ADC},
    {T_EERPOM, "EEPROM", DEMO_EEPROM},
    {T_EPCS, "EPCS", DEMO_EPCS},
};


bool bKeyPressed = FALSE;

void KEY_ISR(void* context, alt_u32 id){
    if (id == KEY_IRQ){
        bKeyPressed = TRUE;
        
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
    error = alt_irq_register (KEY_IRQ, 0, (alt_isr_func) KEY_ISR);
    if (error)
        printf("Failed to register interrut\r\n");
    
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



int main(void)
{

    int sel;
    int nNum;
    
    nNum = sizeof(szTestList)/sizeof(szTestList[0]);
    printf("DE-Nano Demo\r\n");
    
    // enble key interrupt
    EnableKeyInterrupt();  
    
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

    OSTaskCreateExt(Encoder_Task,
                      NULL,
                      (void *)&task1_stk[TASK_STACKSIZE-1],
                      TASK1_PRIORITY,
                      TASK1_PRIORITY,
                      task1_stk,
                      TASK_STACKSIZE,
                      NULL,
                      0);

  OSTaskCreateExt(PWM_Task,
				  NULL,
				  (void *)&task2_stk[TASK_STACKSIZE-1],
				  TASK2_PRIORITY,
				  TASK2_PRIORITY,
				  task2_stk,
				  TASK_STACKSIZE,
				  NULL,
				  0);

	OSTaskCreateExt(ADC_Task,
				  NULL,
				  (void *)&task3_stk[TASK_STACKSIZE-1],
				  TASK3_PRIORITY,
				  TASK3_PRIORITY,
				  task3_stk,
				  TASK_STACKSIZE,
				  NULL,
				  0);

	OSTaskCreateExt(LSM303_Task,
				   NULL,
				   (void *)&task4_stk[TASK_STACKSIZE-1],
				   TASK4_PRIORITY,
				   TASK4_PRIORITY,
				   task4_stk,
				   TASK_STACKSIZE,
				   NULL,
				   0);

	OSTaskCreateExt(Navigation_Task,
					   NULL,
					   (void *)&task5_stk[TASK_STACKSIZE-1],
					   TASK5_PRIORITY,
					   TASK5_PRIORITY,
					   task5_stk,
					   TASK_STACKSIZE,
					   NULL,
					   0);

	OS_Start();
    return 0;
    
       
}


void Navigation_Task(void *pData){
    bool bSuccess;
    char * status = malloc(sizeof(char) * BUFFER_SIZE + 1);;
    INT8U err;

    while (1)
    {
    	status = (char *) OSQPend(MainQueue, 0, &err);
    }
}

void LSM303_Task(void *pData){
    bool bSuccess;

    // release i2c pio pin 
    //IOWR_ALTERA_AVALON_PIO_DIRECTION(I2C_SCL_BASE, ALTERA_AVALON_PIO_DIRECTION_OUTPUT);    
    //IOWR_ALTERA_AVALON_PIO_DIRECTION(I2C_SDA_BASE, ALTERA_AVALON_PIO_DIRECTION_INPUT);
    IOWR(SELECT_I2C_CLK_BASE, 0, 0x00);
    
}

void ADC_Task(void* pData){
    int ch = 0, next_ch=0;
    float volt = 0;
    alt_u16 data16;
    
    printf("Monitoring ADC value. Press KEY0 or KEY1 to terminal the monitor process.\r\n");
    ADC_Read(next_ch);
    while(!bKeyPressed){
        next_ch++;
        if (next_ch >= 8)
            next_ch = 0;        
        data16 = ADC_Read(next_ch); // 12-bits resolution
        volt = (float)data16 * 3.3 / 4095.0;
        printf("CH%d=%.2f V\r\n", ch, volt);
        ch = next_ch;
        
        if (volt > 2.0) //close to an object
        {
        	OSQPost(MainQueue, "collision");
        }

    }        

    //read from CH0, switch to CH1 or CH2 when obstacle detected
}

void PWM_Task(void* pData){
	//generate PWM signal and send it through GPIO pins


}

void Encoder_Task(void* pData){
	//read from Encoder GPIO pins

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

