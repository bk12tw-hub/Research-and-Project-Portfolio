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
using namespace std;

#define TIME_STEP 64
//#define MAX_SPEED 6.28
#define DIST_TO_GO 5
// All the webots classes are defined in the "webots" namespace
using namespace webots;

#include "myRobot.hpp"
#include "myRobotSQ.hpp"
#include "myRobotOA.hpp"
#include "myRobotNA.hpp"
#include "myRobotWP.hpp"
#include <webots/Receiver.hpp>
#include <webots/Display.hpp>
#include <cstring>

//#include <webots/Display.hpp>
//#include <webots/Camera.hpp>

int main(int argc, char **argv) {
  // create the Robot instance.
  myRobotWP *robot = new myRobotWP();
  
  Receiver *receiver;
  receiver = robot->getReceiver("receiver");
  receiver->enable(TIME_STEP);
  //display = robot->getDisplay("DISPLAY");
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
  double t = robot->getTime();
 // printf(timeStep);
  // position sensor
  /*PositionSensor *leftPosSensor = leftMotor->getPositionSensor();
  PositionSensor *rightPosSensor = rightMotor->getPositionSensor();
  leftPosSensor->enable(timeStep);
  rightPosSensor->enable(timeStep);*/
  //srand(time(0));
  robot->step(timeStep);
  //double pos[2];
  //double psVal[8];
  // Main loop:
  // - perform simulation steps until Webots is stopping the controller
  while (robot->step(timeStep) != -1) {
  
    
       //robot->drawVFHHistogram();
      //robot->leftTurn(0.01);
      //robot->step(timeStep);
      //robot->transition(pos);
     //robot->BackwardForDistance(0.8);
        //robot->getVFH();
        //robot->printGPSLoc();
        //North = compass->getValues(); // ← 這行不能少
   // robot->Navigation();
    robot->arraydata();
    robot->transition();
    //std::cout << "Current state: " << navState << ", Distance: " << dist << std::endl;
     //robot->Test();

  };

  // Enter here exit cleanup code.

  delete robot;
  return 0;

}