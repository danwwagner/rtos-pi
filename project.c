#include <wiringPi.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <alchemy/task.h>
#include <alchemy/timer.h>

#define IR_PIN 0
#define ENC_CLK_PIN 1
#define ENC_DT_PIN 2
#define ENC_SW_PIN 3

RT_TASK ir;
RT_TASK humitemp;
RT_TASK encoder;
RT_TASK dummy;

unsigned char last_enc_status;
unsigned char flag;
unsigned char current_enc_status;

float timer = 0;
float last_timer = 0;

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
    // Set task to be periodic every 15ms
    static int count;
    static int prev;
    // 50ms period for this task.
    rt_task_set_periodic(&encoder, TM_NOW, rt_timer_ns2ticks(5e7));
    while (1) {
        int error_code = rt_task_wait_period(NULL); // wait for the next period to run the following
        if (error_code) { printf("Error from rt_task_wait_period for read_encoder %s\n", error_code); return; }
        last_enc_status = digitalRead(ENC_DT_PIN);
        while(!digitalRead(ENC_CLK_PIN)) {// timing constraint here?
            current_enc_status = digitalRead(ENC_DT_PIN);
            flag = 1;
        }   

        if (flag == 1) {
            flag = 0;
            if (last_enc_status  == 0 && current_enc_status == 1) count++;
            if (last_enc_status == 1 && current_enc_status == 0) count--;
        }
        if (count != prev) printf("Encoder count: %d\n", count);
        prev = count;  
  }
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
}

/* Dummy function for learning how to program periodic tasks in Xenomai 3 */
void dummy_func() {
    rt_task_set_periodic(&dummy, TM_NOW, rt_timer_ns2ticks(7e6));
    while (1) {
        int error = rt_task_wait_period(NULL);
        if (error) printf("Error on rt_task_wait_period, %s\n", strerror(-error));
        printf("Time diff: %.2f\n", timer - last_timer);
        last_timer = timer;
    }
}
int main(int argc, char* argv[]) {
    printf("Starting IR sensor task.\n");
    init_encoder();
    printf("Encoder initialized.\n");
//    init_ir();
    // &task, name, stack size, priority, mode);
//    rt_task_create(&ir, "IR", 0, 50, 0); 

    //rt_task_set_periodic(&ir, TM_NOW, rt_timer_ns2ticks(15000));
   //  rt_task_start(&ir, &readISR, NULL);

    rt_task_create(&encoder, "Encoder", 0, 99, 0);
    rt_task_start(&encoder, &read_encoder, NULL);

    rt_task_create(&dummy, "Dummy", 0, 98, 0);
    rt_task_start(&dummy, &dummy_func, NULL);
       while(1) {
        static time_t t_start = 0;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        if (t_start == 0) t_start = ts.tv_sec;
        timer = (float) (ts.tv_sec - t_start) + ts.tv_nsec * 1.0e-9;
    }
    return 0;
}

