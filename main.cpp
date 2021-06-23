#include"mbed.h"
#include "bbcar.h"
#include "bbcar_rpc.h"
#include "stdlib.h"
#include "string.h"
#include <math.h>
PwmOut pin5(D5), pin6(D6);
Ticker servo_ticker;
BufferedSerial pc(USBTX,USBRX); //tx,rx
BufferedSerial uart(A1,A0); //tx,rx
DigitalInOut pin10(D10);
BufferedSerial xbee(D1, D0);

BBCar car(pin5, pin6, servo_ticker);

Thread t1;
EventQueue queue(64 * EVENTS_EVENT_SIZE);

char recvall[50], recvall2[50], tmp[50];
int now;
int re = 1;

void Follow();

int main(){
    xbee.set_baud(9600);
   char recv[1];
   uart.set_baud(9600);
   t1.start(callback(&queue, &EventQueue::dispatch_forever));
   queue.call(Follow);
    int a = 0;
   while(1){
      if(uart.readable()){
            uart.read(recv, sizeof(recv));
            if(recv[0] == '(') now = 0;
            else if(recv[0] == ')') {
                tmp[now] = recv[0];
                if(re) strcpy(recvall, tmp);
                strcpy(tmp, "");
            }
            else if (recv[0] != ',') {
                tmp[now++] = recv[0];
            }
      }
   }
}

void Follow(){
    char n[4][4];
    int x1, x2, y1, y2;
    int tz;
    int i = 0;
    int j = 0;
    int count = 0;
    int dx, dy;
    int deg;
    int len;
    float a = 2.3;
    int turn = 0;
    float r;
    bool stop = false;
    char buff[25];
    int mission = 0;
    sprintf(buff, "FIRST Stage\r\n");     // Start the from the start line
    xbee.write(buff, 12);                     // Print it on the screen using the xbee

    while(1) {               // Read in the information from the openMV
        re = 0;
        i = 0;
        tz = 0;
        len = strlen(recvall);
        if (recvall[0] == 'r') {
            i = 0;
            j = 0;
            count = 0;
            while(recvall[i] != ')') {
                if (recvall[i] == ' ') {
                    j++;
                    count = 0;
                    i++;
                }
                n[j][count] = recvall[i];
                count++;
                i++;
            }
            if (len != 0) {
                x1 = atoi(n[0]);
                y1 = atoi(n[1]);
                x2 = atoi(n[2]);
                y2 = atoi(n[3]);
                if (y1 > y2) {
                    int temp;
                    temp = x1;
                    x1 = x2;
                    x2 = temp;
                }
            }
        }
        else {
            i = 1;
            while(recvall[i] != ')') {
                n[0][i-1] = recvall[i];
                i++;
            }
            tz = atoi(n[0]);           // The distance between the april tag
            printf("tz = %d\n", tz);
        }

        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                n[i][j] = '\0';
            }
        }
        for (i = 0; i < 50; i++) {
            recvall[i] = '\0';
        }

        re = 1;
        dx = x1 - x2;
        dy = y1 - y2;
        double r = (float) sqrt(dx*dx + dy*dy);
        // Detect the line
        if (r != 0) {
            if (x1 < 68) {
                car.turn(35, 1);
                ThisThread::sleep_for(50ms);
            }
            else if (x1 > 92) {
                car.turn(-35, 1);
                ThisThread::sleep_for(50ms);
            }
            else {
                car.goStraight(35);
            }

            // Detect the april tag
            if (tz >= -3 && tz != 0) {
                if (mission == 0) {
                    sprintf(buff, "SECOND Stage\r\n");    // Start to cross the obstacle
                    xbee.write(buff, 13);

                    // the speed and distance is fixed
                    // turn left and go straight
                    car.turn(-100, 1);
                    ThisThread::sleep_for(650ms);
                    car.goStraight(100);
                    ThisThread::sleep_for(2500ms);
                    car.stop();
                    ThisThread::sleep_for(1000ms);
                    for (int i = 0; i < 2; i++) {
                        // turn right and go straight twice
                        car.turn(100, 1);
                        ThisThread::sleep_for(800ms);
                        car.goStraight(100);
                        ThisThread::sleep_for(3000ms);
                        car.stop();
                        ThisThread::sleep_for(1000ms);
                    }

                    car.turn(-100, 1);    // turn left to detect the second line
                    ThisThread::sleep_for(450ms);

                    mission = 1;
                    sprintf(buff, "END1\r\n");
                    xbee.write(buff, 6);
                }
                else if (mission == 1){
                    car.stop();
                    ThisThread::sleep_for(1000ms);
                    sprintf(buff, "END2\r\n");
                    xbee.write(buff, 6);
                }
            }
        }
        ThisThread::sleep_for(50ms);
    }
}
