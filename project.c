#include <wiringPi.h>
#include <stdio.h>
#include  <signal.h>
#include <unistd.h>

#include <alchemy/task.h>
#include <alchemy/timer.h>

#define IR_PIN 0
#define ENC_CLK_PIN 1
#define ENC_DT_PIN 2
#define ENC_SW_PIN 3

RT_TASK ir;
RT_TASK humitemp;
RT_TASK encoder;

unsigned char last_enc_status;
unsigned char flag;
unsigned char current_enc_status;
int count = 0;

void detectISR() {
    printf("Received infrared data.\n");
}

void init_ir() {
    printf("Inside readISR\n");
    if (wiringPiSetup() == -1) {
        printf("WiringPi setup failed!");
    }
    
    if (wiringPiISR(IR_PIN, INT_EDGE_FALLING, &detectISR) == -1) {
        printf("ISR setup failure!");
    }
}

void read_encoder() {
    last_enc_status = digitalRead(ENC_DT_PIN);
    while(!digitalRead(ENC_CLK_PIN)) {// timing constraint here?
        current_enc_status = digitalRead(ENC_DT_PIN);
        flag = 1;
    }

    if (flag == 1) {
        flag = 0;
        if (last_enc_status  == 0 && current_enc_status == 1) count++;
        else if (last_enc_status == 1 && current_enc_status == 0) count--;
    }
    printf("Encoder count: %d\n", count);
}

void init_encoder() {
	if(wiringPiSetup() < 0){
		fprintf(stderr, "Unable to setup wiringPi:%s\n",strerror(errno));
	}

	pinMode(ENC_SW_PIN, INPUT);
	pinMode(ENC_DT_PIN, INPUT);
	pinMode(ENC_CLK_PIN, INPUT);

	pullUpDnControl(ENC_SW_PIN, PUD_UP);
    if (wiringPiISR(ENC_SW_PIN, INT_EDGE_FALLING, &read_encoder) , 0) {
        printf("Unable to initialize ISR\n");
    }
    printf("End of init_encoder\n");
}


int main(int argc, char* argv[]) {
    printf("Starting IR sensor task.\n");
    init_encoder();
//    init_ir();
    // &task, name, stack size, priority, mode);
//    rt_task_create(&ir, "IR", 0, 50, 0); 

    // causes issues; next set to a periodic task every 15 ms
    //rt_task_set_periodic(&ir, TM_NOW, rt_timer_ns2ticks(15000));
    // rt_task_start(&ir, &readISR, 0);
    rt_task_create(&encoder, "Encoder", 1, 50, 0);
    rt_task_start(&encoder, &read_encoder, 0);
    while(1);
    return 0;
}

