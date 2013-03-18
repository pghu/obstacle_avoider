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
#include "includes.h"
//#include "lib/LSM303.h"

#define   TASK_STACKSIZE       2048
#define   QUEUE_SIZE 		   10
#define   MAX_BUFFER_SIZE      255

/* Error Definitions */
#define BLOCKED_FRONT 0
#define NOTMOVING 1


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

/* Queues */
OS_EVENT* MainQueue;
OS_EVENT* MotorQueue;
void* QueueBaseAddress[QUEUE_SIZE];


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
        printf("Failed to register interrupt\r\n");
    
}


void Navigation_Task(void *pData){
    //char * status = malloc(sizeof(char) * MAX_BUFFER_SIZE + 1);;
    int status = 0;
    INT8U err;

    while (1)
    {
    	status = (int) OSQPend(MainQueue, 0, &err); //wait on messages from queue and act accordingly

    	switch (status){
    		case BLOCKED_FRONT:
    			//
    			break;
    		default:
    			break;
    	}


    	OSTimeDlyHMSM(0, 0, 0.05, 0);
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
    int ch = 0;
    float volt = 0;
    alt_u16 data16;
    
    printf("Monitoring ADC value. Press KEY0 or KEY1 to terminal the monitor process.\r\n");
    ADC_Read(ch);
    while(!bKeyPressed){

        //if (next_ch >= 8)
        //    next_ch = 0;

        data16 = ADC_Read(ch); // 12-bits resolution
        volt = (float)data16 * 3.3 / 4095.0;
        printf("CH%d=%.2f V\r\n", ch, volt);
        
        if (volt < 2.0 && volt > 1.0) //close to an object
        {
        	OSQPost(MainQueue, BLOCKED_FRONT);

        	//post and alert navigation task of the left and right
        }

    }        

    //read from CH0, switch to CH1 or CH2 when obstacle detected
}


void PWM_Task(void* pData){
	//generate PWM signal and send it through GPIO pins

	//keep going forward while path is clear
	double duty = 0;
	duty = 0.75;

	IOWR(PWM_0_BASE, 0, 0.5 * 31250); //100%
	usleep(1000000);


	while (1)
	{
		IOWR(PWM_0_BASE, 0, 1 * 31250); //100%
		usleep(4000000);
	}

}

void Encoder_Task(void* pData){
	double encode_0_a, encode_1_a;
	double counts_0_new, counts_0_old;
	double counts_1_new, counts_1_old;
	counts_0_old = 0;
	counts_1_old = 0;

	while (1)
	{
		//1st motor
		counts_0_new = ((double) IORD(ENCODER_0A_BASE, 0)) / 32;
		encode_0_a = (counts_0_new-counts_0_old);

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

		//post to MainTaskQueue if rev/sec is 0 for multiple seconds

		/*
		printf("Motor 0 encoder A value is :( %f ) \n", encode_0_a);
		printf("Motor 0 revolutions per second :( %f ) \n", encode_0_a / 64);
		printf("Motor 1 encoder A value is :( %f ) \n", encode_1_a);
		printf("Motor 1 revolutions per second :( %f ) \n", encode_1_a / 64);
		*/
		usleep(1000000);
	}
}



int main(void){

	MainQueue = OSQCreate(&QueueBaseAddress[0], QUEUE_SIZE);

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

	//OS_Start();
    return 0;

    //	LSM303_Init();
	//enableDefault();
}
