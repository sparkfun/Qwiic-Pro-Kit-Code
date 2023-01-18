/*
  Digital Bubble Level
  SparkFun Electronics
  Modified By: Ho Yun "Bobby" Chan
  Modified Date: Oct 21, 2022
  Originally Written By: Nathan Seidle
  Date: Feb 26, 2021
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  
    6DoF Breakout (ISM330DHCX)                 - https://www.sparkfun.com/products/19764
    9DoF IMU Breakout (ISM330DHCX, MMC5983MA ) - https://www.sparkfun.com/products/19895
    Qwiic Micro OLED                           - https://www.sparkfun.com/products/14532

  This example takes readings from the accelerometer and translates them into the position of a bubble in a bubble
  level, to be displayed on the OLED screen. This code was originally written for the LIS2DH12 and has been adjusted
  for the ISM330DHCX.

  The x and y axes of the OLED display should aligned with the axes of the accelerometer.

   ---------------------
  | y                   |
  | ^                   |
  | |   OLED Screen     |
  | |                   |
  |   -- -- > x         |
   ---------------------
       Display Cable

*/



#include <Wire.h>

// Accelerometer
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_ISM330DHCX.h>  // Include the ISM330DHCX 6DoF Library                                  http://librarymanager/All#SparkFun_ISM330DHCX
SparkFun_ISM330DHCX myISM;
sfe_ism_data_t accelData;
sfe_ism_data_t gyroData;          //gyro isn't used but we included it since it's a 6DoF IC

float accelX = 0;
float accelY = 0;
float accelZ = 1;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// External Display
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED
//#include "icons.h"

#define PIN_RESET 7
#define DC_JUMPER 1
MicroOLED oled(PIN_RESET, DC_JUMPER);
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


// Set target and dot size
int radiusLarge = 10; // Target area 2
int radiusSmall = 4;  // Target area 1
int radiusBubble = 2; // Bubble size

// Set initial roll and pitch measurement
double averagedRoll = 0.0;
double averagedPitch = 0.0;



void setup() {
  // Initialize Output for Print Statements
  Serial.begin(115200);
  Serial.println("SparkFun Accel Example");

  // Initialize I2C Connection
  Wire.begin();
  //Wire.setClock(400000);

  beginDisplay(); //Check if an external Qwiic OLED is attached and display splash screen

  // Check for accelerometer LIS2DH12
  /*  if (accel.begin() == false)
    {
      Serial.println("Accelerometer not detected. Check address jumper and wiring. Freezing...");
      while (1)
        ;
    }

    // Set sample/data rate for accelerometer
    // The larger the avgAmount the faster we should read the sensor
    //accel.setDataRate(LIS2DH12_ODR_100Hz); //6 measurements a second
    accel.setDataRate(LIS2DH12_ODR_400Hz); //25 measurements a second

  */

  //// Check for ISM330DHCX (accelerometr/gyro) and initialize 6DoF settings
  //Note: If you disconnect the 9DoF, you will need to configure the 6DoF settings again.
  if ( !myISM.begin() ) {
    Serial.println("Did not begin."); //Output this message if we are not able to detect the 6DoF
  }

  myISM.deviceReset();

  while ( !myISM.getDeviceReset() ) {
    delay(1);
  }

  Serial.println("Reset.");
  Serial.println("Applying settings.");
  delay(100);

  myISM.setDeviceConfig();
  myISM.setBlockDataUpdate();

  // Set sample/data rate for accelerometer
  // The larger the avgAmount the faster we should read the sensor
  //myISM.setAccelDataRate(ISM_XL_ODR_104Hz); //~6 measurements a second, (i.e. 104/16)
  myISM.setAccelDataRate(ISM_XL_ODR_416Hz); //26 measurements a second, (i.e. 416/16)
  myISM.setAccelFullScale(ISM_2g);

  myISM.setGyroFullScale(ISM_500dps);
  myISM.setGyroDataRate(ISM_GY_ODR_104Hz);

  myISM.setAccelFilterLP2();
  myISM.setAccelSlopeFilter(ISM_LP_ODR_DIV_100);

  myISM.setGyroFilterLP1();
  myISM.setGyroLP1Bandwidth(ISM_MEDIUM);


}

void loop()
{
  getAngles(); // Calculate roll and pitch angles based on the acceleromter readings
  oled.clear(PAGE); // Clear the display's internal memory

  // Set dot position
  double bubbleX = LCDWIDTH / 2 - averagedPitch; // Bubble location on x-axis
  double bubbleY = LCDHEIGHT / 2 - averagedRoll; // Bubble location on y-axis

  // Limit bubble position to edge of screen
  if (bubbleX < radiusBubble) {
    bubbleX = radiusBubble;
  }
  else if (bubbleX > LCDWIDTH - radiusBubble) {
    bubbleX = LCDWIDTH - radiusBubble - 1;
  }
  if (bubbleY < radiusBubble) {
    bubbleY = radiusBubble;
  }
  else if (bubbleY > LCDHEIGHT - radiusBubble) {
    bubbleY = LCDHEIGHT - radiusBubble - 1;
  }

  // Draw circle relative to dot
  oled.circle(LCDWIDTH / 2, LCDHEIGHT / 2, radiusLarge);
  oled.circle(LCDWIDTH / 2, LCDHEIGHT / 2, radiusSmall);
  oled.circleFill(bubbleX, bubbleY, radiusBubble);

  // Display angle/position once bubble is inside larger target area
  if ( sqrt(averagedPitch * averagedPitch + averagedRoll * averagedRoll) < (radiusLarge - radiusBubble))
  {
    oled.setFontType(0); //Set font to smallest

    oled.setCursor(LCDWIDTH / 2 - 21, 0); //x, y
    oled.print("X:");
    oled.print(-averagedPitch);
    oled.setCursor(LCDWIDTH / 2 - 21, LCDHEIGHT - 8); //x, y
    oled.print("Y:");
    oled.print(averagedRoll);


    //    oled.setCursor(LCDWIDTH/2 - 11, 0); //x, y
    //    oled.print(averagedRoll);
    //    if (-averagedPitch < 0) { oled.setCursor(LCDWIDTH - 29, LCDHEIGHT/2 - 3); }
    //    else { oled.setCursor(LCDWIDTH - 23, LCDHEIGHT/2 - 3); }
    //    oled.print(-averagedPitch);
  }

  oled.display();
}

void getAngles()
{
  averagedRoll = 0.0;
  averagedPitch = 0.0;
  const int avgAmount = 16;

  // Average readings after 'avgAmount' samples
  for (int reading = 0 ; reading < avgAmount ; reading++)
  {

    if ( myISM.checkStatus() ) { // Wait for accelerometer connection

      myISM.getAccel(&accelData);
      myISM.getGyro(&gyroData);

      // Retrieve data from accelerometer
      accelX = accelData.xData;   //store in variable in accelX
      //Serial.print("accelX= "); //output accelerometer value along X-axis, uncomment this line for debugging
      //Serial.println(accelX);

      accelY = accelData.yData;   //store in variable in accelY
      //Serial.print("accelY= "); //output accelerometer value along Y-axis, uncomment this line for debugging
      //Serial.println(accelY);

      accelZ = accelData.zData;   //we don't actually use this for the bubble leveler but we'll store it anyway


    }


    // Optional modification: https://www.nxp.com/docs/en/application-note/AN3461.pdf
    //int signZ = constrain(accelZ, -1, 1);
    //double roll = atan2(accelY , signZ * sqrt( accelZ * accelZ + .001 *  abs(accelX) ) ) * 57.3;

    // Calculate roll and pitch angles
    double roll = atan2(accelY , accelZ) * 57.3;
    double pitch = atan2((-accelX) , sqrt(accelY * accelY + accelZ * accelZ)) * 57.3;
    if (constrain(accelZ, -1, 1) == -1) {
      roll = atan2(accelY, -accelZ) * 57.3;  // Invert if upside down
    }

    averagedRoll += roll;
    averagedPitch += pitch;

    // Debug Print Statements
    //Serial.print(roll, 6);
    //Serial.print(", ");
    //Serial.print(pitch, 6);
    //Serial.print(", ");
    //
    //Serial.print(accelX);
    //Serial.print(", ");
    //Serial.print(accelY);
    //Serial.print(", ");
    //Serial.print(accelZ);
    //Serial.println("");
  }

  averagedRoll /= (float)avgAmount;
  averagedPitch /= (float)avgAmount;

  // Debug Print Statements
  //Serial.print(averagedRoll, 6);
  //Serial.print(", ");
  //Serial.print(averagedPitch, 6);
  //Serial.println(", ");

}
