#include <webots/DistanceSensor.hpp>
#include <string>
#include <complex.h>
#include <iostream>
#define PSTHRESHOLD 71
#define S9 9
#define S10 10
#define S11 11
#define S12 12
#define S13 13
#define TIME_STEP 64
#define TURN_RATIO 0.7
#define BACKW 0.05
#include <unistd.h>
using namespace std;

class myRobotOA : public myRobotSQ
{
private:
    unsigned char State_;
    float p;
public:
    char SensorName[8][4] = {"ps0","ps1","ps2","ps3", "ps4", "ps5", "ps6", "ps7"};
    double theta[8] = {-0.30,  -0.80,  -1.57,  -2.64, 2.64, 1.57, 0.80, 0.30};
    DistanceSensor *ps[8];
    double thetac;

    myRobotOA(){
        srand((int)time(NULL));
        State_ = S9;
        //State = S1; //not sure
        for (int i = 0; i < 8;i++){
            ps[i] = getDistanceSensor(SensorName[i]);
            ps[i]->enable(TIME_STEP);           }
    }

    void getPositionSensor(double *psValue){
        for (int i = 0; i < 8;i++){
            psValue[i] = ps[i]->getValue();     }
            //cout << "Sensor " << i << ": " << psValue[i] << endl;
    }
    // implement center of mass
    double Complex(double *psValue){
        double ms[8], m = 0;
        complex<double> z[8], zc = 0;
        getPositionSensor(psValue);

        for (int i=0; i<8; i++){
              if (psValue[i] > PSTHRESHOLD )
                ms[i] = psValue[i] - PSTHRESHOLD;
              else
                ms[i] = 0;

              z[i] = polar(ms[i],theta[i]);
              zc += z[i];
              m  += ms[i];
           }

        if (m == 0)
            thetac = NAN;
        else {
            zc = zc / m;
            thetac = arg(zc);
            }
        //cout << "radien = " << arg(zc) << endl;
        return thetac;
    }
    // obstacles detection
    unsigned char detection(double *psValue){
        double a = Complex(psValue);
        unsigned char reached = 0;
        if ( (a > -1.57)&& (a <= -0.01)) //right
            reached= 3;
        else if ((a > -0.015) && (a < 0.015)) //middle
            reached = 2;
        else if ((a < 1.57)&& (a >= 0.01)) //left
            reached = 1;
        else
            reached = 4; //nothing infront
        return reached;
    }
    // avoid scratching the obsstacles-1
    void BackwardForDistance(double distance) {
        double speed = -0.3;
        int steps = (int)((distance / fabs(speed)) / (TIME_STEP / 1000.0));
        for (int i = 0; i < steps; ++i) {
            setVelocity(speed);
            step(TIME_STEP);            }     }

    // avoid scratching the obsstacles-2
    bool isTooClose(double *psValue){
        for (int i = 0; i < 8; i++)     {
                if (psValue[i] > 140.0)
                    return true;        }
        return false;               }

    void Transition(double *psValue){
        double pos[2];
        myRobotSQ::transition(pos);
        unsigned char nextState_ = State_;
        unsigned char reached = detection(psValue);

        if (State > 8){
            switch(State_){
                case S9:{ //free_to_go
                        setVelocity(0.5);
                    if(reached == 1){
                        if (isTooClose(psValue))
                            BackwardForDistance(BACKW);
                        nextState_ = S13;
                    }

                    else if(reached == 2){
                        if (isTooClose(psValue))
                            BackwardForDistance(BACKW);
                        nextState_ = S13;//14;
                    }

                    else if(reached == 3){
                        if (isTooClose(psValue))
                            BackwardForDistance(BACKW);
                        nextState_ = S13;
                    }else nextState_ = S12;
                    break;
                }

                case S10:{ //avoid left
                    rightTurn(TURN_RATIO);
                    if(reached == 4)
                        nextState_ = S9;
                    break;
                }
                case S11:{ //avoid right
                    leftTurn(TURN_RATIO);
                    if(reached == 4)
                        nextState_ = S9;
                    break;
                }
                case S12:{ //goforward random
                    p = float(rand()) / float(RAND_MAX);
                    if (p > 0.95)
                        nextState_ = S10;
                    else if (p > 0.85)
                        nextState_ = S11;
                    else
                        nextState_ = S9;
                    break;}

                case S13:{ //turning random
                   if (isTooClose(psValue))
                        BackwardForDistance(BACKW);
                    usleep(1000);

                    p = float(rand()) / float(RAND_MAX);
                   // printf("p = %f \n", p);
                   printf("reached = %d \n", reached);
                    if (reached == 1 || reached == 3) {
                        nextState_ =  (p > 0.15) ? ((reached == 1) ? S10 : S11)
                                          : ((reached == 1) ? S11 : S10);
                    } else {
                        nextState_ = (p > 0.5) ? S11 : S10;
                    }

                    break;
                }
                default:
                    break;
            }
            printf("state = %d \n", State_);
            State_ = nextState_;
       }
    }
};
