#include <webots/Camera.hpp>
#define MAX_SPEED 6.28

class myRobot : public Robot
{
public:
   // declare some variables that represent properties of this new class, e.g.
   Motor *left_motor, *right_motor;
   PositionSensor *leftPosSensor, *rightPosSensor;

   //camera
   Camera *camera;
   int timeStep;
   double pos[2] = {0, 0};
   //  define the constructor for myRobot; this constructor is called whenever a myRobot object is instantiated.
   myRobot(){
      int timeStep = getBasicTimeStep();
      camera = getCamera("camera");
      camera->enable(timeStep);

      // initialize the object, e.g.
      left_motor = getMotor("left wheel motor");
      right_motor = getMotor("right wheel motor");

      left_motor->setPosition(INFINITY);
      right_motor->setPosition(INFINITY);

      leftPosSensor = left_motor->getPositionSensor();
      rightPosSensor = right_motor->getPositionSensor();
      leftPosSensor->enable(timeStep);
      rightPosSensor->enable(timeStep);}

   void getPosition(double *pos){
      pos[0] = leftPosSensor->getValue();
      pos[1] = rightPosSensor->getValue();}

   // new method
   void setVelocity(double left_speed, double right_speed){
     left_motor->setVelocity(left_speed*MAX_SPEED);
     right_motor->setVelocity(right_speed*MAX_SPEED);}


   // overload setVelocity
   void setVelocity(double speed){
     double LSpeed, RSpeed;
     LSpeed = speed*MAX_SPEED;
     RSpeed = speed*MAX_SPEED;
     if (LSpeed >= MAX_SPEED)
        LSpeed = MAX_SPEED;
     if (RSpeed >= MAX_SPEED)
        RSpeed = MAX_SPEED;
    left_motor->setVelocity(LSpeed);
    right_motor->setVelocity(RSpeed);
     }

   /* void backwardForDuration(int duration_ms) {
        int steps = duration_ms / TIME_STEP;
        for (int i = 0; i < steps; ++i) {
            setVelocity(-1);
            step(TIME_STEP);  // 推進模擬時間
        }
    }*/

   // leftTurn
   void leftTurn(double speed){
     //backward(500);
    /* for (int i = 0; i < 50; i++) {
        setVelocity(-0.8);
        step(timeStep);
     }*/
   //  backwardForDuration(30);
     left_motor->setVelocity(-speed*MAX_SPEED);
     right_motor->setVelocity(speed*MAX_SPEED);}

   // rightTurn
   void rightTurn(double speed){
    // backward(500);
   /*  for (int i = 0; i < 50; i++) {
        setVelocity(-0.8);
        step(timeStep);
     }*/
    // backwardForDuration(30);
     left_motor->setVelocity(speed*MAX_SPEED);
     right_motor->setVelocity(-speed*MAX_SPEED);}
};
