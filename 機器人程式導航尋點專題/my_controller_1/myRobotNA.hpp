
#include <webots/GPS.hpp>
#include <webots/Compass.hpp>
#define N 1e+8
#define C 2.5 //2
#define C_ 2.5 //0.1 //2.5
#define N_ 0.2 //0.5
#define A 20  //10
#define maxRange 2
#define VFHThreshold 0.0625
//#define baseV 4
#define wpThreshold 0.1

#include <cstdlib>   // for rand()
#include <ctime>     // for time()
#include <iostream>
#include <cstring>
#include <webots/Receiver.hpp>
#include <webots/Camera.hpp>
#include <webots/Display.hpp>

class myRobotNA : public myRobotOA
{
public:
    GPS *gps;
    Compass *compass;
    const double *gpsLoc;
    const double *North;
    double heading, sigmaT;
    Display *display;
   // std::vector<std::vector<double>> waypoints;
    std::vector<std::vector<double>> waypoints = {
        {0.0, 0.0},
        {0.0,  0.0},
    };
    enum NavState { S0_PN, S1_CLOS, S2_NEXT_WP, S3_STOP, S4_HOME};
    NavState navState;
    NavState next_navState;
    //unsigned char navState;
    int currentWp;
    double left_speed, right_speed;
    double baseV, bearHat;
    double H_star[32], traver[32], traver_[32];
    double dist;
    double arriveRadius, closeRadius, bearVFH;
    double b, mki;
    bool allZero;
    Receiver *receiver;
    //float (*wp)[2] = nullptr;
    int numWP = 0;
    const void *packet = NULL;
    double (*array)[2];
    //void *memcpy(void *dest, const void *src, size_t n);
        double targetX, targetY;
    myRobotNA(){
        receiver = getReceiver("receiver");
        int timeStep = getBasicTimeStep();
        currentWp = 0;
        gps = getGPS("gps");
        gps->enable(timeStep);

        compass = getCompass("compass");
        compass->enable(timeStep);

        gpsLoc = gps->getValues();
        navState = S0_PN;
        North = compass->getValues();

        baseV = MAX_SPEED;

    }
    // ------------
    void arraydata() {
      if (receiver->getQueueLength() > 0) {
        const void *packet = receiver->getData();

        numWP = (int)*((unsigned char *)packet);

        int expectedSize = 1 + 2 * numWP * sizeof(float);
        if (receiver->getDataSize() != expectedSize) {
          cerr << "Unexpected packet size! Got: " << receiver->getDataSize()
               << ", expected: " << expectedSize << endl;
          receiver->nextPacket();
          return;
        }

        // packet
        waypoints.clear();
        const float* data = reinterpret_cast<const float*>((char*)packet + 1);
        for (int i = 0; i < numWP; ++i) {
          float x = data[2 * i];
          float y = data[2 * i + 1];
          waypoints.push_back({static_cast<double>(x), static_cast<double>(y)});
        }

        // Debug print
        cout << "** Received " << numWP << " waypoints:\n";
        for (int i = 0; i < numWP; i++) {
          printf("Waypoint %d: (%.2f, %.2f)\n", i, waypoints[i][0], waypoints[i][1]);
        }
        //currentWp = numWP;
        receiver->nextPacket();
      }
    }
    //---------PN & CLOS----------------
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
       //double targetX , targetY;

       if(navState == S4_HOME){
        targetX = 0;
        targetY = 0;
        cout << "time to go home" << endl;
       }else{
        targetX = waypoints[currentWp][0];
        targetY = waypoints[currentWp][1];
       }
       //cout << "[DEBUG] Current WP[" << currentWp << "] = (" << targetX << ", " << targetY << ")" << endl;
       double lamT = atan2(targetY-gpsLoc[1], targetX-gpsLoc[0]); /* obtain λ(t) from localization equations */

       //cout << "lamT" << lamT* (180/acos(-1)) << endl;

       //cout << "getBasicTimeStep" << getBasicTimeStep() << endl;

       double dLamT = (lamT - oldLamT)/ (1000*getBasicTimeStep());
       //double dLamT = (lamT - oldLamT) / (getBasicTimeStep() / 1000.0); // 秒為單位
       oldLamT = lamT; /* store as λ(t-1) for next time step */
       return dLamT;
    }
    void getSigmaT(){
        North = compass->getValues();
        gpsLoc = gps->getValues();
        getHeading();
         targetX = waypoints[currentWp][0];
         targetY = waypoints[currentWp][1];
        double lambdaT = atan2(targetY - gpsLoc[1], targetX - gpsLoc[0]);
        sigmaT = lambdaT - heading;
    }
    void PNcommand(){
        North = compass->getValues();
        gpsLoc = gps->getValues();
        //cout << "[DEBUG] GPS = (" << gpsLoc[0] << ", " << gpsLoc[1] << ")" << endl;
        // CLOS bias
        getBearHatFromValley();
        getSigmaT();
        findValley(0);
        double psi_dot_c;

        bool allZero = true;
        for (int i = 0; i < 32; ++i) {
            if (VFHDensity[i] != 0.0) {
                allZero = false;
                break;
            }
        }

        double min_diff = 1e9;
        int best_idx = -1;

        for (int i = 0; i < numValley; ++i) {
            // restrict to -pi ~ pi
            double diff = valleyPos[i] - sigmaT;
            while (diff > acos(-1)) diff -= 2 * acos(-1);
            while (diff < -acos(-1)) diff += 2 * acos(-1);

            if (fabs(diff) < min_diff) {
                min_diff = fabs(diff);
                best_idx = i;
            }
        }

        //
        if (best_idx != -1) {
            bearHat = valleyPos[best_idx];
            cout << "Selected valley " << best_idx << " with angle = " << bearHat << endl;
        }
        //cout << "bearHat" << bearHat << endl;
        //cout << "sigmaT" << sigmaT << endl;
        double deltaBear = bearHat - sigmaT;
        double lambda_dot = dLamT();
        //cout << "targetX: " << targetX << endl;
        //cout << "targetY: " << targetY << endl;
        //double maxDeltaV = MAX_SPEED - baseV;
        //cout << "lambda_dot= " << N * lambda_dot << endl;
        //cout << "deltaBear= " << C_ * deltaBear << endl;

        if (allZero) {
            cout << "[INFO] VFHDensity 全部為 0，採取特定動作..." << endl;
            psi_dot_c = N * lambda_dot;
        }
        else{
            psi_dot_c = N * lambda_dot + C_ * deltaBear;
        }

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
        getBearHatFromValley();

       // double targetX , targetY;
        //cout << "navState_IN_clos" << navState << endl;
        //cout << "next_navState_IN_clos" << next_navState << endl;
       if(navState == S4_HOME){
        targetX = 0;
        targetY = 0;
        cout << "time to go home" << endl;
       }else{
        targetX = waypoints[currentWp][0];
        targetY = waypoints[currentWp][1];
       }
        //double targetX = waypoints[currentWp][0];
       // double targetY = waypoints[currentWp][1];
      //  cout << "[DEBUG] Current WP_Navigation[" << currentWp << "] = (" << targetX << ", " << targetY << ")" << endl;

        double lambdaT = atan2(targetY - gpsLoc[1], targetX - gpsLoc[0]);
        //cout << "heading" << heading << endl;
        double sigmaT = lambdaT - heading;

        if (sigmaT > acos(-1)) sigmaT = sigmaT - 2 * acos(-1);
        else if (sigmaT < -acos(-1)) sigmaT = sigmaT + 2 * acos(-1);
        //cout << "sigmaT" << sigmaT << endl;
        double deltaBear = sigmaT - bearHat;

        double lambda_dot = dLamT();
        double psi_dot_c = C * deltaBear + N_ * lambda_dot;
        double deltaV = 0.5 * psi_dot_c;
       // cout << "deltaV" << deltaV << endl;s
        cout << "deltaBear" << C * deltaBear << endl;
        cout << "lambda_dot" << N_ * lambda_dot << endl;
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
    // Navigation for test---------------
  /*  void Navigation(){
        gpsLoc = gps->getValues();
        North = compass->getValues();
       // double targetX, targetY;

        if (currentWp!=10){
            targetX = waypoints[currentWp][0];
            targetY = waypoints[currentWp][1];
        }
        //cout << "[DEBUG] GPS = (" << gpsLoc[0] << ", " << gpsLoc[1] << ")" << endl;
        cout << "[DEBUG] Current WP_Navigation[" << currentWp << "] = (" << targetX << ", " << targetY << ")" << endl;

    // Compute Euclidean distance to current waypoint
        double dx = targetX - gpsLoc[0];
        double dy = targetY - gpsLoc[1];
        dist = sqrt(dx * dx + dy * dy);
        //cout << "dist" << dist << endl;
        closeRadius = (2*baseV)/(acos(-1)*C);   // Switch from PN to CLOS
        arriveRadius = 0.5;
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
                if (currentWp == static_cast<int>(waypoints.size())) {
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

    //------------------VFH--------------------------
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
        double dist = computeDistance(currentWp);
        // Compute Euclidean distance to current waypoint
       /*  targetX = waypoints[currentWp][0];
         targetY = waypoints[currentWp][1];
        double dx = targetX - gpsLoc[0];
        double dy = targetY - gpsLoc[1];
        double dist = sqrt(dx * dx + dy * dy);*/

        //cout << "lidarRangeImage : " << lidarRangeImage << endl;
        for (int k=0; k < 32; k++)
        {
            VFHDensity[k] = 0;
            for (int i=0; i<8; i++)
            {
             //cout << "lidarRangeImage : " << lidarRangeImage[8*k + i] << endl;

             if(dist < lidarRangeImage[8*k + i])
                b = A/dist;
             else
                b = A/maxRange;

             mki = (lidarRangeImage[8*k + i] == INFINITY) ?
                0 : (double)(A - b * lidarRangeImage[8*k + i]);  /* equation (1) */

             mki = std::max(0.0, A - b * lidarRangeImage[8*k + i]);
             VFHDensity[k] += mki;                                   // equation (2)
             //cout << VFHDensity[k] << endl;
            // cout << "Sector " << k << ": " << VFHDensity[k] << endl;
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
        //setVelocity(0,0);
        int iStart = -1, iEnd = 31;
        getBearing();
        getVFH();
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
            //cout << "numValley = " << static_cast<int>(numValley) << endl;
            //cout << "iStart: " << iStart << "iEnd: " << iEnd << endl;
            //cout << "VFHBearing[iStart]" << VFHBearing[iStart] << endl;
            valleyPos[numValley] = (VFHBearing[iStart] + VFHBearing[iEnd]) / 2.0;
            //cout << "valleyPos[numValley]: " << valleyPos[numValley] << endl;
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

    void getBearHatFromValley() {
        getVFH();
        getBearing();
        numValley = 0;
        findValley(0); // find all valleys first

        // non-deterministic
        vector<int> validSectors;
        double sumValleyDensity = 0.0;

        // Step 1: sum all valley Density
        for (int i = 0; i < numValley; ++i) {
            for (int j = 0; j < 32; ++j) {
                if (VFHBearing[j] >= valleyPos[i] - valleyWid[i] / 2.0 &&
                    VFHBearing[j] <= valleyPos[i] + valleyWid[i] / 2.0){

                    validSectors.push_back(j);
                    sumValleyDensity += VFHDensity[j];
                }
            }
        }

        // Step 2: normalize + build cumulative prob.
        vector<double> normalized;
        vector<double> cum;
        double cumVal = 0;

        for (int idx : validSectors) {
            double norm = (1 - (VFHDensity[idx] / sumValleyDensity)) / validSectors.size();
            cumVal += norm;
            normalized.push_back(norm);
            cum.push_back(cumVal);
        }

        // Step 3: random sample
        double p = static_cast<double>(rand()) / RAND_MAX;
        for (size_t i = 0; i < cum.size(); ++i) {
            if (p <= cum[i]) {
                bearHat = VFHBearing[validSectors[i]];
                break;
            }
        }
    }

    double computeDistance(int i) {
        gpsLoc = gps->getValues();
        North = compass->getValues();
        targetX = waypoints[i][0];
        targetY = waypoints[i][1];
        double dx = targetX - gpsLoc[0];
        double dy = targetY - gpsLoc[1];
        //dist = sqrt(dx * dx + dy * dy);
        return std::sqrt(dx * dx + dy * dy);
    }

    void findClosestWaypoint() {
        gpsLoc = gps->getValues();
        int nearestIndex = 0;
        double minDist = 1e+7;  // 設為一個大數，確保第一次比較能更新
        for (int i = 0; i < numWP; ++i) {
            double dist = computeDistance(i);
            if (dist < minDist) {
                minDist = dist;
                nearestIndex = i;
                cout << "dist" << dist << endl;
                cout << "nearestIndex" << nearestIndex << endl;
            }
        }

        currentWp = nearestIndex;
        cout << "[FSM] 找到最近的 waypoint 編號：" << currentWp << endl;
    }
};
