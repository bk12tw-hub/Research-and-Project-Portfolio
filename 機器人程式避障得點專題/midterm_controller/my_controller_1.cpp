// File:          epuck_go_forward.cpp
// Date:
// Description:
// Author:
// Modifications:

// You may need to add webots include files such as
// <webots/DistanceSensor.hpp>, <webots/Motor.hpp>, etc.
// and/or to add some other includes

#include <webots/Robot.hpp>
#include <webots/Motor.hpp>
#include <webots/PositionSensor.hpp>
#include <iostream>
//#include <webots/robot.h>


#define TIME_STEP 64
//#define MAX_SPEED 6.28
#define DIST_TO_GO 5
// All the webots classes are defined in the "webots" namespace
using namespace webots;

#include "myRobot.hpp"
#include "myRobotSQ.hpp"
#include "myRobotOA.hpp"

int main(int argc, char **argv) {
  // create the Robot instance.
  myRobotOA *robot = new myRobotOA();
  //robot->setPosition(0,0)
  // get motor devices
  //Motor *leftMotor = robot->getMotor("left wheel motor");
  //Motor *rightMotor = robot->getMotor("right wheel motor");
  // set the target position of the motor
  //leftMotor->setPosition(INFINITY);
  //rightMotor->setPosition(INFINITY);
  
  /* set up the motor speeds at 10% of the MAX_SPEED
  leftMotor->setVelocity(0.1 * MAX_SPEED);
  rightMotor->setVelocity(-0.1 * MAX_SPEED);*/
  
  // get the time step of the current world.
  int timeStep = (int)robot->getBasicTimeStep();
 // printf(timeStep);
  // position sensor
  /*PositionSensor *leftPosSensor = leftMotor->getPositionSensor();
  PositionSensor *rightPosSensor = rightMotor->getPositionSensor();
  leftPosSensor->enable(timeStep);
  rightPosSensor->enable(timeStep);*/
  
  robot->step(timeStep);
  double pos[2];
  double psVal[8];
 
  
  // Main loop:
  // - perform simulation steps until Webots is stopping the controller
  while (robot->step(timeStep) != -1) {

      //robot->leftTurn(0.01);
      //robot->step(timeStep);
      //robot->transition(pos);
     //robot->BackwardForDistance(0.8);
      robot->Transition(psVal);
     //robot->Test();

  };

  // Enter here exit cleanup code.

  delete robot;
  return 0;

}