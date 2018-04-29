#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include <alchemy/task.h>
#include <alchemy/timer.h>

#define MAXTIMINGS 85

#define ENC_CLK_PIN 1
#define ENC_DT_PIN 2
#define ENC_SW_PIN 3
#define DHTPIN 4

// Tasks 
RT_TASK humitemp;
RT_TASK encoder;

RTIME humitemp_period = 1e8;
RTIME encoder_period = 5e6;

// Encoder variables
unsigned char last_enc_status;
unsigned char flag;
unsigned char current_enc_status;

// Humitemp sensor data
int dht11[5] = {0, 0, 0, 0, 0};
int old[5] = {0, 0, 0, 0, 0};

float timer = 0;
float last_timer = 0;
int count, prev;

void quit(int signal) {
    // Suspend tasks, then delete them.
    rt_task_suspend(&humitemp);
    rt_task_suspend(&encoder);

    printf("Tasks suspended. Deleting...\n");
    rt_task_delete(&humitemp);
    rt_task_delete(&encoder);

    printf("Tasks deleted. Exiting...\n");
    exit(0);
}
/* Attempts to read the encoder data every 50 ms. */
static void read_encoder() {
    rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(encoder_period));  //50ms period
    while (1) {
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
        rt_task_wait_period(NULL); // wait for the next period to run the following
  }
}

/* Attempt to retrieve the data from the humidity/temperature sensor. */
static void read_temp() {
    rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(humitemp_period)); // 1s period
    while (1) {
	    uint8_t laststate = HIGH;
	    uint8_t counter = 0;
    	uint8_t j = 0, i;
    	float f; // fahrenheit

    	dht11[0] = dht11[1] = dht11[2] = dht11[3] = dht11[4] = 0;

    	// pull pin down for 18 milliseconds
    	pinMode(DHTPIN, OUTPUT);
    	digitalWrite(DHTPIN, LOW);
    	delay(18);
    	// then pull it up for 40 microseconds
    	digitalWrite(DHTPIN, HIGH);
    	delayMicroseconds(40); 
    	// prepare to read the pin
    	pinMode(DHTPIN, INPUT);

    	// detect change and read data
    	for ( i=0; i< MAXTIMINGS; i++) {
    		counter = 0;
    		while (digitalRead(DHTPIN) == laststate) {
    			counter++;
    			rt_task_sleep(1);
    			if (counter == 255) {
    				break;
    			}
    		}
    		laststate = digitalRead(DHTPIN);

    		if (counter == 255) break;
    
    		// ignore first 3 transitions
    		if ((i >= 4) && (i%2 == 0)) {
    			// shove each bit into the storage bytes
    			dht11[j/8] <<= 1;
    			if (counter > 16)
    				dht11[j/8] |= 1;
    			j++;
    		}
    	}

    	// check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
    	// print it out if data is good
    	if ((j >= 40) && 
    			(dht11[4] == ((dht11[0] + dht11[1] + dht11[2] + dht11[3]) & 0xFF)) ) {
    		f = dht11[2] * 9. / 5. + 32;
    		printf("Humidity = %d.%d %% Temperature = %d.%d *C (%.1f *F)\n", 
    				dht11[0], dht11[1], dht11[2], dht11[3], f);
            old[0] = dht11[0];
            old[1] = dht11[1];
            old[2] = dht11[2];
            old[3] = dht11[3];
	    }

        // Data bad -- print last good reading
    	else 
    	{
	    	printf("Humidity = %d.%d %% Temperature = %d.%d *C (%.1f *F)\n", 
		    	old[0], old[1], old[2], old[3], f);
    	}   
        int error_code = rt_task_wait_period(NULL); // wait for the next period to run the following
        if (error_code) { printf("Error from rt_task_wait_period for read_encoder %s\n", strerror(-error_code)); }
    }
}

/* Initializes the ISR for the Encoder. */
void init_encoder() {
	pinMode(ENC_SW_PIN, INPUT);
	pinMode(ENC_DT_PIN, INPUT);
	pinMode(ENC_CLK_PIN, INPUT);

	pullUpDnControl(ENC_SW_PIN, PUD_UP);
    if (wiringPiISR(ENC_SW_PIN, INT_EDGE_FALLING, &read_encoder), 0) {
        printf("Unable to initialize ISR\n");
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, quit);
    printf("Initializing tasks.\n");
    if (wiringPiSetup() < 0) printf("WiringPi setup error\n");
    init_encoder();
    printf("Initialization complete.\n");

    // &task, name, stack size, priority, mode);

     rt_task_create(&encoder, "Encoder", 10, 50, 0);
     rt_task_start(&encoder, &read_encoder, 0);

     rt_task_create(&humitemp, "Humitemp", 0, 49, 0);
     rt_task_start(&humitemp, &read_temp, 0);
     while(1); 
     return 0;
}

