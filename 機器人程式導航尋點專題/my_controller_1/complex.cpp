#include <complex.h>
#include <iostream>

using namespace std;

#define THRESHOLD 75

int main()
{
   /* numbered 0 to 7, corresponding to sensor locations */
   double theta[8] = {0.30,  0.80,  1.57,  2.64, -2.64, -1.57, -0.80, -0.30};
   double ps[8] = {0, 0, 129, 0, 254, 0, 0, 0};
   double ms[8];
   std::complex<double> z[8];

   std::complex<double> zc = 0;
   double m = 0;
   
   for (int i=0; i<8; i++)
   {
      /* mass = sensor value minus sensor threshold */
      if (ps[i] > THRESHOLD )
      	ms[i] = ps[i] - THRESHOLD;
      else
        ms[i] = 0;
      
      z[i] = polar(ms[i],theta[i]);
      zc += z[i];  // same as zc = zc + z[i];
      m  += ms[i];
   }
   
   double thetac;
   
   if (m == 0)
      thetac = NAN;  // all sensors return below threshold
   else
   {
      zc = zc/m;
      thetac = arg(zc);
   }
   
   cout << real(zc) << " + " << imag(zc) << "j" << endl;
   cout << thetac << endl;
   

   return 0;
}

