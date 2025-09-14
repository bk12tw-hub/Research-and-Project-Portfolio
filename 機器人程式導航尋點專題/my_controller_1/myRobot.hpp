#include <webots/Camera.hpp>
#include <webots/Lidar.hpp>
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

   // Navigation
    Lidar *lidar;
    const float *lidarRangeImage;

    double VFHDensity[32];
    double VFHBearing[32];
    double valleyPos[16], valleyWid[16];
    unsigned char numValley = 0;

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

      // PositionSensor
      leftPosSensor = left_motor->getPositionSensor();
      rightPosSensor = right_motor->getPositionSensor();
      leftPosSensor->enable(timeStep);
      rightPosSensor->enable(timeStep);

      // Lidar
      lidar = getLidar("lidar");
      lidar->enable(timeStep);
      lidar->enablePointCloud();
      }

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

   // leftTurn
   void leftTurn(double speed){
     left_motor->setVelocity(-speed*MAX_SPEED+0.25);
     right_motor->setVelocity(speed*MAX_SPEED-0.15);}

   // rightTurn
   void rightTurn(double speed){
     left_motor->setVelocity(speed*MAX_SPEED-0.15);
     right_motor->setVelocity(-speed*MAX_SPEED+0.25);}
};
