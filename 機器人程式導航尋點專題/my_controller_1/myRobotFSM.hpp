#include <webots/GPS.hpp>
#include <webots/Compass.hpp>
#define N 1e+8
#define C 3.5
#define a 10
#define b 0.2
#define VFHThreshold 10
//#define baseV 4
#define wpThreshold 0.1
//#define closeRadius 0.3;   // Switch from PN to CLOS
//#define arriveRadius 0.05; // Declare WP reached

struct valStruct {
  int iStart;
  int iEnd;
  double sigMid;
  double width;
};
class myRobotFSM : public myRobotOA
{
public:
    valStruct vals[32];
    GPS *gps;
    Compass *compass;
    const double *gpsLoc;
    const double *North;
    double heading;
    std::vector<std::vector<double>> waypoints = {
        {0.5, -2.0},
        {1.0,  1.0},
        {-1.0, 1.7},
        {-1.1, -1.5}
    };
    enum NavState { S0_PN, S1_CLOS, S2_NEXT_WP, S3_STOP };
    NavState navState;
    //unsigned char navState;
    int currentWp;
    double left_speed, right_speed;
    double baseV;

    myRobotNA(){
        int timeStep = getBasicTimeStep();
        currentWp = 0;
        gps = getGPS("gps");
        gps->enable(timeStep);

        compass = getCompass("compass");
        compass->enable(timeStep);

        gpsLoc = gps->getValues();
        navState = S0_PN;
        North = compass->getValues();

        baseV = 0.5*MAX_SPEED;
    }

    void printGPSLoc(){
        cout << "GPS location: X = " << gpsLoc[0]
        << ", Y = " << gpsLoc[1] << endl;
    }

    void getHeading(){
        double lambda = atan2(North[1], North[0]);
        heading = 1.5708 - lambda;
    }
    //
    double dLamT()
    {
       static double oldLamT;
       double targetX = waypoints[currentWp][0];
       double targetY = waypoints[currentWp][1];
       //cout << "[DEBUG] Current WP[" << currentWp << "] = (" << targetX << ", " << targetY << ")" << endl;
       double lamT = atan2(targetY-gpsLoc[1], targetX-gpsLoc[0]); /* obtain λ(t) from localization equations */

       //cout << "lamT" << lamT* (180/acos(-1)) << endl;

       //cout << "getBasicTimeStep" << getBasicTimeStep() << endl;

       double dLamT = (lamT - oldLamT)/ (1000*getBasicTimeStep());
       //double dLamT = (lamT - oldLamT) / (getBasicTimeStep() / 1000.0); // 秒為單位
       oldLamT = lamT; /* store as λ(t-1) for next time step */
       return dLamT;
    }

    void PNcommand(){
        North = compass->getValues();
        gpsLoc = gps->getValues();
        //cout << "[DEBUG] GPS = (" << gpsLoc[0] << ", " << gpsLoc[1] << ")" << endl;

        double lambda_dot = dLamT();
        //double maxDeltaV = MAX_SPEED - baseV;
        //cout << "lambda_dot= " << lambda_dot << endl;

        double psi_dot_c = N * lambda_dot;
        double deltaV = 0.5 * psi_dot_c;
        //cout << "deltaV" << deltaV << endl;

        if(lambda_dot > 0){
            left_speed = (baseV - deltaV);
            right_speed = (baseV + deltaV);
        }
        else if (lambda_dot < 0){
            left_speed = (baseV - deltaV);
            right_speed = (baseV + deltaV);
        }
        //if (deltaV > maxDeltaV) deltaV = maxDeltaV;
        //if (deltaV < -maxDeltaV) deltaV = -maxDeltaV;

        /*cout << "λ˙T = " << lambda_dot << ", ψ˙c = " << psi_dot_c
         << ", ΔV = " << deltaV << endl;*/
        //cout << "deltaV" << deltaV << endl;
        //cout << "Set Velocity: L = " << left_speed << ", R = " << right_speed << endl;
        //left_speed = (baseV + deltaV);
        //right_speed = (baseV - deltaV);

        if (left_speed > MAX_SPEED) left_speed = MAX_SPEED;
        else if (left_speed < -MAX_SPEED) left_speed = -MAX_SPEED;

        if (right_speed > MAX_SPEED) right_speed = MAX_SPEED;
        else if (right_speed < -MAX_SPEED) right_speed = -MAX_SPEED;

        //cout << "left_speed: " << left_speed << endl;
        //cout << "right_speed: " << right_speed << endl;

        setVelocity(left_speed/MAX_SPEED, right_speed/MAX_SPEED);
    }

    void CLOScommand(){
        North = compass->getValues();
        gpsLoc = gps->getValues();

        getHeading();
        //cout << heading << endl;
        double targetX = waypoints[currentWp][0];
        double targetY = waypoints[currentWp][1];
        cout << "[DEBUG] Current WP_Navigation[" << currentWp << "] = (" << targetX << ", " << targetY << ")" << endl;

        double lambdaT = atan2(targetY - gpsLoc[1], targetX - gpsLoc[0]);
        cout << "heading" << heading << endl;
        double sigmaT = lambdaT - heading;

        if (sigmaT > acos(-1)) sigmaT = sigmaT - 2 * acos(-1);
        else if (sigmaT < -acos(-1)) sigmaT = sigmaT + 2 * acos(-1);
        cout << "sigmaT" << sigmaT << endl;

        double psi_dot_c = C * sigmaT;
        double deltaV = 0.5 * psi_dot_c;
       // cout << "deltaV" << deltaV << endl;s

        //double maxDeltaV = MAX_SPEED - baseV;
        //if (deltaV > maxDeltaV) deltaV = maxDeltaV;
        //if (deltaV < -maxDeltaV) deltaV = -maxDeltaV;
        left_speed = (baseV - deltaV);
        right_speed = (baseV + deltaV);
        if (left_speed > MAX_SPEED) left_speed = MAX_SPEED;
        else if (left_speed < -MAX_SPEED) left_speed = -MAX_SPEED;

        if (right_speed > MAX_SPEED) right_speed = MAX_SPEED;
        else if (right_speed < -MAX_SPEED) right_speed = -MAX_SPEED;

        /*cout << "sigma˙T = " << lambda_dot << ", ψ˙c = " << psi_dot_c
         << ", ΔV = " << deltaV << endl;*/
         //cout << "Set Velocity: L = " << (baseV + deltaV)/MAX_SPEED << ", R = " << (baseV - deltaV)/MAX_SPEED << endl;
        //setVelocity(0, 0);
        setVelocity(left_speed/MAX_SPEED,right_speed/MAX_SPEED);

    }
    void Navigation(){
        gpsLoc = gps->getValues();
        North = compass->getValues();
        double targetX, targetY;

        if (currentWp!=10){
            targetX = waypoints[currentWp][0];
            targetY = waypoints[currentWp][1];
        }
        //cout << "[DEBUG] GPS = (" << gpsLoc[0] << ", " << gpsLoc[1] << ")" << endl;
        cout << "[DEBUG] Current WP_Navigation[" << currentWp << "] = (" << targetX << ", " << targetY << ")" << endl;

    // Compute Euclidean distance to current waypoint
        double dx = targetX - gpsLoc[0];
        double dy = targetY - gpsLoc[1];
        double dist = sqrt(dx * dx + dy * dy);
        //cout << "dist" << dist << endl;
        double closeRadius = (2*baseV)/(acos(-1)*C);   // Switch from PN to CLOS
        double arriveRadius = 0.015;
        cout << "[NAV] WP: " << currentWp << ", Dist = " << dist << ", State = " << navState << endl;

        NavState next_navState;
        next_navState = navState;
    // === FSM Switch ===
        switch (navState) {
            case S0_PN:{
                PNcommand();
                if (dist < closeRadius) {
                    next_navState = S1_CLOS;
                    cout << "[FSM] S0 → S1: Switching to CLOS\n";
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
                currentWp += 1;
                cout << "currentWp" << currentWp << endl;
                if (currentWp == waypoints.size()) {
                    currentWp = 10;
                    cout << "[FSM] Navigation Complete: All waypoints reached.\n";
                    next_navState = S3_STOP;  // Stop
                } else {
                    next_navState = S0_PN;
                    cout << "[FSM] S2 → S0: Moving to next WP\n";
                }
                break;
            }
            case S3_STOP:{
                setVelocity(0, 0);  // Stop
                break;
            }
            default:
                break;
        }

        navState = next_navState;
        cout << "[FSM] State\n" << navState << endl;;
    }*/
    //
    void printRangeImage()
    {
        lidarRangeImage = lidar->getRangeImage();
        for (int i=0; i<256; i+=16)
            printf("%f\t", lidarRangeImage[i]);
        printf("\n");
    }

    void getVFH()
    {
        //cout << a << endl;
        //cout << b << endl;
        lidarRangeImage = lidar->getRangeImage();
        cout << "lidarRangeImage : " << lidarRangeImage << endl;
        for (int k=0; k < 32; k++)
        {
            VFHDensity[k] = 0;
            for (int i=0; i<8; i++)
            {
             double mki = (lidarRangeImage[8*k + i] == INFINITY) ?
                   0 : (double)(a - b * lidarRangeImage[8*k + i]);  /* equation (1) */
             VFHDensity[k] += mki;                                  /* equation (2) */
             //cout << VFHDensity[k] << endl;
             cout << "Sector " << k << ": " << VFHDensity[k] << endl;
            }
        }
    }

    void getBearing()
    {
        for (int k = 0; k < 32; k++) {
            VFHBearing[k] = 1.24 - 0.08 * k;
        }
    }

    void findValley(int k){
        int iStart = -1, iEnd = 31;
        for (int i=k; i<32; i++)
        {
            /* find the first index i where VFHDensity[i] < threshold;
               store i as iStart. */
            /* If iStart is found, continue and find the first index i where
               VFHDensity[i] >= threshold;
               store i-1 as iEnd.*/
            /* Terminate the for loop with break when iEnd is found. */
            /* If iEnd is not found, the valley ends at the last sector,
               so iEnd = 31. */
            if (VFHDensity[i] < VFHThreshold) {
                iStart = i;
                for (int j = iStart + 1; j < 32; ++j) {
                    if (VFHDensity[j] >= VFHThreshold) {
                        iEnd = j - 1;
                        break;
                    }else iEnd = 31;
                }
                break;
            }
        }
        if (iStart != -1)
        {
            /* Calculate width and central position using iStart and iEnd, store these at
               valleyPos[numValley] and vallyWid[numValley]. */
            /* Increment the number of valleys */
            valleyPos[numValley] = (VFHBearing[iStart] + VFHBearing[iEnd]) / 2.0;
            valleyWid[numValley] = fabs(VFHBearing[iStart] - VFHBearing[iEnd]);
            numValley += 1;
        }
        if (iStart == -1 || iEnd == 31)
        {
              ; /* Do nothing. Recursion ends here.*/
        }
        else
        {
              /* Repeat, starting at the next sector. */
              findValley(iEnd + 1);
        }
    }

};
