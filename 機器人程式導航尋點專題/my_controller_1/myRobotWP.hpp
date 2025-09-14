
#include <webots/GPS.hpp>
#include <webots/Compass.hpp>
#define GO_HOME_THRESHOLD 600

class myRobotWP : public myRobotNA
{
public:
    NavState next_navState;
    double psVal[8];
    bool goHomeFlag = false;
    bool reachHomeFlag = false;
    double targetX, targetY;

    myRobotWP(){
        State_ = S9;
        navState = S0_PN;
        next_navState = navState;
    }

    void transition()  /** This is myRobotWP::transition() **/
    {
        /** Localization, VFH codes go here **/
        double currentTime = getTime();
        //cout << "currentTime" << currentTime <<  endl;
        if (currentTime > GO_HOME_THRESHOLD && !reachHomeFlag) {
            goHomeFlag = true;
            navState = S4_HOME;
            cout << "[FSM] 時間已到 → 強制回家 (0,0)\n";

        }else{
        findClosestWaypoint();
        }

        getHeading();
        getVFH();

        //gpsLoc = gps->getValues();
       // North = compass->getValues();
       // double targetX, targetY;
        /* Call superclass methods */
        getPositionSensor(psVal);
        myRobotOA::Transition(psVal);
        if (!goHomeFlag){
            targetX = waypoints[currentWp][0];
            targetY = waypoints[currentWp][1];
        }
         // cout << "[DEBUG] Current WP_Navigation[" << currentWp << "] = (" << targetX << ", " << targetY << ")" << endl;
    // Compute Euclidean distance to current waypoint
        double dx = targetX - gpsLoc[0];
        double dy = targetY - gpsLoc[1];
        dist = sqrt(dx * dx + dy * dy);
        cout << "dist" << dist << endl;
        closeRadius = (2*baseV)/(acos(-1)*C);   // Switch from PN to CLOS
        arriveRadius = 0.5;
        cout << "[NAV] WP: " << currentWp << ", Dist = " << dist << ", State = " << navState << endl;

        next_navState = navState;

        if (State_ == S9) /* free to go: do nav */
        {
            switch (navState)
            {
                case S0_PN:{
                    PNcommand();
                    if (dist < closeRadius) {
                        next_navState = S1_CLOS;

                        //cout << "[FSM] S0 → S1: Switching to CLOS\n";
                    }
                    break;
                }
                case S1_CLOS:{
                    CLOScommand();
                    cout << "dist" << dist << endl;
                    if (dist < arriveRadius) {
                        next_navState = S2_NEXT_WP;
                        cout << "[FSM] S1 → S2: Waypoint reached\n";
                    }
                    break;
                }
                case S2_NEXT_WP:{
                    //arraydata();
                    findClosestWaypoint();
                    //currentWp += 1;
                    cout << "currentWp" << currentWp << endl;
                    if (currentWp == static_cast<int>(waypoints.size())) {
                        currentWp = 20;
                        //cout << "[FSM] Navigation Complete: All waypoints reached.\n";
                        next_navState = S4_HOME;  // Stop
                    } else {
                        next_navState = S0_PN;
                        //cout << "[FSM] S2 → S0: Moving to next WP\n";
                    }
                    break;
                }
                case S3_STOP:{
                    setVelocity(0, 0);  // Stop
                    cout << "[FSM] 到達原點，停止移動。\n";
                    break;
                }
                case S4_HOME: {
                   targetX = 0;// waypoints = (0, 0);
                   targetY = 0;

                   if (dist < closeRadius) {
                    CLOScommand();  // 使用 Pure Pursuit 或 CLOS 均可
                    cout << "CLOS_ing" << endl;
                   if (dist < 0.03) {
                        reachHomeFlag = true;
                        next_navState = S3_STOP;
                        }

                    }else{
                        PNcommand();
                        cout << "PN_ing" << endl;
                    }
                    //cout << "dist" << dist << endl;

                break;
      }
                default:
                    break;
            }
            cout << "next_navState: " << next_navState << endl;
            navState = next_navState;
        }
    }
};




