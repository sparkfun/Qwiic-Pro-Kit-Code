/****************************************************************************************
  Qwiic Starter Kit Game
  Michelle Shorter @ SparkFun
  Thanks for Code from Owen Lyke @ SparkX, Wes Furuya, and Nathan Seidle @SparkFun
  May 13th 2019
  License: This code is public domain but you buy me a burger if you use this 
  and we meet someday (Beefware License).

  This sketch shows how Qwiic-ly you can make a Pong-like game
  All parts (Redboard , Mini OLED Display, Proximity Sensor, Acceleromenter, 
  and Qwiic Joystick) are  included in the Qwiic Starter Kit (available: 
  https://www.sparkfun.com/products/15349).  The Joystick has different firmware 
  than the SparkX version of the joystick which requires different code.

  Wiring:
  The hardware setup is incredibly simple thanks to the magic of I2C! All you
  have to do is connect all the parts to one another with the Qwiic cables and
  make sure that they are connected to the Redboard. The order in which the
  parts are all strung together doesn't matter!

  Playing the Game:
  Make sure the libraries for the OLED, the Joystick, Accelerometer (MMA8452Q), and 
  Proximity sensor (VCNL4040) are installed (see links below), then compile and upload the 
  code to the Redboard. You'll notice in the loop funtion that P1 (player 1) and P2 (player 2) 
  are controlled by update Paddle functions.  Select which board and which function you 
  want to control each paddle, you will also need to comment out any boards that are not
  being used in the startBoards function.  Then move the corresponding board to control 
  each player. The game does not end, you just keep racking up points (the display does 
  not look great once you hit 100).
****************************************************************************************/

#include <Wire.h>                                     // Qwiic parts use the I2C communication protocol, handled by Wire
#include <SFE_MicroOLED.h>                            // Include the SFE_MicroOLED library                                    http://librarymanager/All#Sparkfun_Micro_OLED_Breakout
#include <SparkFun_MMA8452Q.h>                        // Include the SFE_MMA8452Q library                                     http://librarymanager/All#Sparkfun_MMA8452Q
#include <SparkFun_Qwiic_Joystick_Arduino_Library.h>  // Include the SFE_Qwiic Joystick library                               http://librarymanager/All#SparkFun_joystick
#include <SparkFun_VCNL4040_Arduino_Library.h>        // Include the VCNL4040 Library                                         http://librarymanager/All#SparkFun_VCNL4040
//#define Serial SerialUSB                            // Uncomment for Redboard Turbo, this reroutes all Serial commands to SerialUSB

//////////////////////////
// MicroOLED Definition //
//////////////////////////
#define PIN_RESET 9                     // The library assumes a reset pin is necessary. The Qwiic OLED has RST hard-wired, so pick an arbitrarty IO pin that is not being used
#define DC_JUMPER 1                     // Out-of-the-box the OLED's address jumper is open so use (1). If the jumper is closed then use 0 here
MicroOLED oled(PIN_RESET, DC_JUMPER);   

//////////////////////////////
// Accelerometer Definition //
//////////////////////////////
MMA8452Q accel;                         
int X = 0;        //X acceleration - in gs
int Y = 0;        //Y acceleration - in gs
int Z = 0;        //Z acceleration - in gs

/////////////////////////
// Joystick Definition //
/////////////////////////
JOYSTICK joystick;
byte B = 1;       // Button value
int H = 0;        // Horizontal ADC counts (0-1023)
int V = 0;        // Vertical ADC counts (0-1023)

//////////////////////////
// Proximity Definition //
//////////////////////////
VCNL4040 proximity;
int prox = 0;     //Proximity data from sensor

/////////////////////////////
// Game Variables (Global) //
/////////////////////////////
unsigned int score1 = 0;         //Score for Player 1
unsigned int score2 = 0;         //Score for Player 2


//Playing field - instead of having these calculated all the time, they are available for anyone to use
#define borderSpace 2            //space between edge of screen and paddles
const int oledWidth=64;          //int oledWidth = oled.getLCDWidth();      //If you use a different screen you can run this to check the width and height
const int oledHeight=48;         //int oledHeight = oled.getLCDHeight();    //If you use a different screen you can run this to check the width and height
const int middleX = oledWidth / 2;
const int middleY = oledHeight / 2;
//boolean invertOLED  = true;    //can uncomment to reverse the display 

// Ball 
int ballX = oledWidth / 2;       // initial X position of the ball
int ballY = oledHeight / 2;      // initial Y position of the ball
#define ballR 2                  // Ball Radius
int ballXVelocity = 0;           // X velocity of the ball
int ballYVelocity = 0;           // Y velocity of the ball

// Paddles
int P1 = 20;                     // Holds the Y location of Paddle 1
int P2 = 20;                     // Holds the Y location of Paddle 2
#define PaddleWidth 3
#define PaddleHeight 15


void setup() { //Start serial, and all boards, setup a random seed, clear the display
  //Start initializing
  Serial.begin(9600);//for debugging and error messages
  randomSeed(analogRead(A0) + analogRead(A1));    // Provide a seed for the random generator
  setBall(); //setup random start for ball
  startBoards(); //setup OLED, joystick and print errors if possible

  //display blank screen
  oled.clear(ALL);
  oled.display();
  delay(250);

  //display Intro Text, etc.
  //oled.clear(PAGE);
  //printText("Qwiic", 1000);
}//setup
void loop() { //draw everything, get new values for the paddles, move the ball and determine whether a score has occured, repeat
  //display current settings
  oled.clear(PAGE);
  drawField();
  drawPaddles();
  drawBall();
  oled.display();

  //Update Paddles (these functions read from the sensors before updating paddle information)
  //The code does try to initialize all 3 inputs and will hang if one is not connected.  To fix
  //comment out the appropriate section in the 'startBoards' function
  
  //P1 = updatePaddleJoystick();       //uses the Y axis
  P1 = updatePaddleAccelerometer();    //uses the X axis
  //P1 = updatePaddleProximity();      //Will allow you to go off screen if you get too close, range is about 1-3"

  //P2 = updatePaddleJoystick();       //uses the Y axis
  //P2 = updatePaddleAccelerometer();  //uses the X 
  P2 = updatePaddleProximity();        //Will allow you to go off screen if you get too close, range is about 1-3"


  //move the ball (does not display, just changes values)
  moveBall();
}
void setBall() { //set up the ball near the center of the field and start moving in a random direction/speed
  ballYVelocity = random(-2, 3);
  ballXVelocity = random(-2, 3);
  if (ballYVelocity == 0) ballYVelocity = -3; //This makes sure the ball is moving
  if (ballXVelocity == 0) ballXVelocity = -3; //This makes sure the ball is moving
  ballX = oledWidth / 2;    // X position of the ball
  ballY = oledHeight / 2;   // Y position of the ball
  ballX += random(-5, 5); //close to middle, but offset enough to give more variety in where it actually hits
  ballY += random(-5, 5); //close to middle, but offset enough to give more variety in where it actually hits
}
void startBoards() { //runs Wire.begin, and starts all boards. If not using a board you will need to comment out that section
  Wire.begin();
  bool error = false;

  //start screen
  oled.begin();
  Serial.print("OLED Height = "); Serial.print(oledHeight);
  Serial.print("  OLED Width = "); Serial.println(oledWidth);

  //start accelerometer (comment out if not using accelerometer)
  if (accel.begin(Wire) == false) {
    Serial.println("Accelerometer not responding, ");
    error = true;
  }

  //start joystick (comment out if not using joystick)
  if (joystick.begin() == false) {
    Serial.println("Joystick not responding, ");
    error = true;
  }

  //start proximity sensor (comment out if not using proximity sensor)
    if (proximity.begin() == false) {
    Serial.println("Proximity sensor not responding, ");
    error = true;
  }
  
  if (error == true) while (1); //if there is an error, then hang
}
void printText(String text, int time) { //A quick way to display text in the middle of the screen, This does run display
  oled.setFontType(0);
  // Try to set the cursor in the middle of the screen
  oled.setCursor(middleX - (oled.getFontWidth() * (text.length() / 2)), middleY - (oled.getFontWidth() / 2));
  // Print the title:
  oled.print(text);
  oled.display();
  delay(time);
}
void drawField() { //just draws the field (a line around the screen), does not run display
  oled.rect(0, 0, oled.getLCDWidth() , oled.getLCDHeight());
}
void drawPaddles() {//just adds the paddles to the buffer, does not run display
  //oled.rectFill(X,Y,W,H);
  oled.rectFill(borderSpace, P1, PaddleWidth, PaddleHeight);
  oled.rectFill(oled.getLCDWidth() - borderSpace - PaddleWidth, P2, PaddleWidth, PaddleHeight);
}
void drawBall() { //just adds a ball to the buffer at the correct corrdinates, does not run display
  oled.circleFill(ballX, ballY, ballR);
}
void readJoystick() { //updates global variables for the joystick
  H = joystick.getHorizontal();
  V = joystick.getVertical();
  B = joystick.getButton();
}
void readAccel() { //updates global variables for the accelerometer
  X = (int)(accel.getCalculatedX() * 1000); //makes this more usable as it is not an int from -1000 to 1000 (when using it as a tilt sensor with only gravity affecting it)
  Y=accel.getCalculatedY();
  Z=accel.getCalculatedZ();
}
void readProx() { //updates globlal variable for the proximity sensor
  //There are other things this sensor can read such as ambient light that could be added
  prox = proximity.getProximity();
  //Serial.println(prox); //Useful for seeing values to adjust range
}
int updatePaddleJoystick() { //sends back value to be assigned to either P1 or P2
  readJoystick();
  int temp = map(V, 0, 1023, borderSpace, oledHeight - PaddleHeight - borderSpace);
  //Serial.print("Joystick V: "); Serial.println(temp);
  return temp;
}
int updatePaddleAccelerometer() { //sends back value to be assigned to either P1 or P2
  readAccel();
  int temp = map(X, -1000, 1000, borderSpace, oledHeight - PaddleHeight - borderSpace);
  //Serial.print("Accelerometer X: "); Serial.println(temp);
  return temp;
}
int updatePaddleProximity() { //sends back value to be assigned to either P1 or P2
  readProx();
  int temp = map(prox, 2, 4000, borderSpace, oledHeight - PaddleHeight - borderSpace);
  //Serial.print("Proximity: "); Serial.println(temp);
  return temp;
}
void moveBall() {  //This is where most of the work is done.  The ball is moved which means it must check score conditions and reset the ball as well.
  
  if (ballX - ballR > 1 && ballX + ballR < (oledWidth - 2) && ballY - ballR > 1 && ballY + ballR < (oledHeight - 2) ) //ball has not gone off the left side, right side, bottom, top
  {
    //move ball on its current course
    ballX += ballXVelocity;
    ballY += ballYVelocity;
  }
  else if ( ballY - ballR <= 1 || ballY + ballR >= (oledHeight - 2)) //ball is on the top or bottom
  {
    //turn around and keep going
    Serial.println("Turn around Y");
    ballYVelocity = -ballYVelocity;
    ballX += ballXVelocity;
    ballY += ballYVelocity;
  }
  else if ( ballX - ballR <= borderSpace + PaddleWidth && ballY + ballR >= P1 && ballY - ballR <= P1 + PaddleHeight  ) //ball hits left paddle
  {
    //turn around and keep going
    Serial.println("Hit Left Paddle");
    ballXVelocity = -ballXVelocity;
    ballX += ballXVelocity;
    ballY += ballYVelocity;
  }
  else if ( ballX + ballR >= oledWidth - borderSpace - PaddleWidth && ballY + ballR >= P2 && ballY - ballR <= P2 + PaddleHeight ) //ball hits right paddle
  {
    //turn around and keep going
    Serial.println("Hit Right Paddle");
    ballXVelocity = -ballXVelocity;
    ballX += ballXVelocity;
    ballY += ballYVelocity;
  }
  else if (ballX - ballR <= borderSpace ) //ball is on the left edge
  {
    score2++;
    Serial.print("Player 1: "); Serial.print(score1); Serial.print("   Player 2: "); Serial.println(score2);
    delay(500);
    printText("Score2", 1000);
    displayScore();
    setBall();
  }
  else if (ballX + ballR >= (oledWidth - borderSpace)) //ball is on the right edge
  {
    score1++;
    Serial.print("Player 1: "); Serial.print(score1); Serial.print("   Player 2: "); Serial.println(score2);
    delay(500);
    printText("Score1", 1000);
    displayScore();
    setBall();
  }

  //Good general information for debug
  //Serial.print("XV: "); Serial.print(ballXVelocity); Serial.print("  YV: "); Serial.print(ballYVelocity); Serial.print("    BallX: "); Serial.print(ballX); Serial.print("  BallY: "); Serial.println(ballY);
}
void displayScore() { //prints the score
  oled.clear(PAGE);
  oled.setFontType(1);
  oled.setCursor(middleX - (oled.getFontWidth() * (2)),   middleY - (oled.getFontWidth() / 2));
  oled.print(score1); oled.print(":"); oled.print(score2);
  oled.display();
  delay(1500);
}

