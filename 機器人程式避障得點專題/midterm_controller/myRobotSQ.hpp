#include <iostream>
#define S1 1
#define S2 2
#define S3 3
#define S4 4
#define S5 5
#define S6 6
#define S7 7
#define S8 8
#define FORWARD_DIST 8
#define TURN_DIST 2.215
using namespace std;

class myRobotSQ : public myRobot{
private:

   double refPosition[2];

public:
   unsigned char State;
   myRobotSQ(){
      State = S1;
      getPosition(pos);
      refPosition[0] = pos[0];
      refPosition[1] = pos[1];   }
    bool reach(double *pos){
     bool reached=false;
     getPosition(pos);
     switch (State){
       case S1:
       case S3:
       case S5:
       case S7: {
         // THIS IMPLEMENTS U1 INPUT SYMBOL
         reached = ((abs(pos[0]-refPosition[0]) > FORWARD_DIST)||
                    (abs(pos[1]-refPosition[1]) > FORWARD_DIST));
       break; }
       case S2:
       case S4:
       case S6:
       case S8: {
       // THIS IMPLEMENTS U2 INPUT SYMBOL
        reached = ((abs(pos[0]-refPosition[0]) > TURN_DIST)||
          (abs(pos[1]-refPosition[1]) > TURN_DIST));
       break;  }
       default: {
            reached = false;
            break; }
     }
     return reached;   }

    void transition(double *pos){
     unsigned char nextState = State;
     static bool transNext = true;

     switch (State){
       case S1:
       case S3:{
        if (transNext){
         setVelocity(1);
         //printf("state = %d \n", State);
         transNext = false;}
        if (reach(pos)) {
            nextState = State + 1;}
            break;       }

       case S2:
       case S6:{
        if (transNext){
        leftTurn(0.05);
        //printf("state = %d \n", State);
        transNext = false;}
        if (reach(pos)) {
            nextState = State + 1;}
            break;       }

       case S4:
       case S8:{
        if (transNext){
        rightTurn(0.05);
        //printf("state = %d \n", State);
        transNext = false;}
        if (reach(pos)) {
            if (State == 4)
                nextState = State + 1;
            else
                nextState = 9;         }
            break;                       }
       case S5:
       case S7:{
        if (transNext){
        setVelocity(-1);

        transNext = false;             }
        if (reach(pos)) {
            nextState = State + 1;       }
            break;  }
       default:
            break;  }
       if (nextState != State){
           transNext = true;
           refPosition[0] = pos[0];
           refPosition[1] = pos[1];    }
         //  printf("state = %d \n", State);
       State = nextState;                }
};
