#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>

#define  RoAPin    1
#define  RoBPin    2
#define  SWPin     3

/* File converted to header from Sunfounder. */
static volatile int globalCounter = 0 ;

unsigned char flag;
unsigned char Last_RoB_Status;
unsigned char Current_RoB_Status;

void btnISR(void)
{
	globalCounter = 0;
}

void rotaryDeal(void)
{
	Last_RoB_Status = digitalRead(RoBPin);

	while(!digitalRead(RoAPin)){
		Current_RoB_Status = digitalRead(RoBPin);
		flag = 1;
	}

	if(flag == 1){
		flag = 0;
		if((Last_RoB_Status == 0)&&(Current_RoB_Status == 1)){
			globalCounter ++;	
		}
		if((Last_RoB_Status == 1)&&(Current_RoB_Status == 0)){
			globalCounter --;
		}
	}
}

void init_encoder(void* arg)
{
	if(wiringPiSetup() < 0){
		fprintf(stderr, "Unable to setup wiringPi:%s\n",strerror(errno));
	}

	pinMode(SWPin, INPUT);
	pinMode(RoAPin, INPUT);
	pinMode(RoBPin, INPUT);

	pullUpDnControl(SWPin, PUD_UP);

    if(wiringPiISR(SWPin, INT_EDGE_FALLING, &btnISR) < 0){
		fprintf(stderr, "Unable to init ISR\n",strerror(errno));	
	}
	
	int tmp = 0;
	while(1){
		rotaryDeal();
		if (tmp != globalCounter){
			printf("%d\n", globalCounter);
			tmp = globalCounter;
		}
	}

}
