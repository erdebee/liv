// defines pins numbers atmega -> wemos
#include "FS.h"
void(* resetFunc) (void) = 0;

void playThr(String file);
void playNextThrLine(void);
File file;
bool playingFile = false;
int currentInstruction = 0;

//X cable on the CNC shield
const int stepDeg = 16; //Uno 2 or ESP 16   
const int dirDeg  = 0; //Uno 5 or ESP 0

//Y cable on the CNC shield
const int stepRad = 5; //Uno 3 or ESP 5
const int dirRad  = 2; //Uno 6 or ESP 2

const int enPin = 12; //Uno 8 or ESP 12

//Full radius of canvas in millimeter 
const float fullRadiusMM = 150.0;

//Speed of the ball in MM per sec
const float speedMMPerSec = 40.0;
//Limit the degrees per second (in the center area)
const float maxDegPerSec = 18.0; 

//Total revolutions for a full spin 
const float revsServo = 400.0 * 32.0;
const float revsRad = (68.0 / 30.0) * revsServo;
const float revsDeg = (36.0 / 12.0) * revsServo;

const float stepsPerRad = revsRad / 2.0; // servo steps / rad
const float stepsPerDeg = (36.0 / 12.0) * revsServo; // servosteps / deg

bool degCw = true;  //Deg arm clockwise

float degFrom = 0.0; // Degrees original position
float degTo = 0.0; // Degrees desired position
int servoDeg = 0; // Degrees servo position
int servoDegTo = 0; // Degrees desired position

float radFrom = 0.0; // Radius original position
float radTo = 0.0; // Radius desired position ( 1 = max )
int servoRad = 0; // Current servo position for radius
int servoRadTo = 0; // Desired servo position for radius

float radDelta = 0.0;
float degDelta = 0.0;
int moveAdded = 0;

void nextInstruction(float newDeg, float newRad) {
  newRad = newRad * 0.95 ;
  degFrom = degTo;
  radFrom = radTo;
  
  degTo = newDeg;
  radTo = newRad;
  
  radDelta = fabs(radFrom-radTo);
  degDelta = fabs(degFrom-degTo);

  moveAdded = 0;
  
//  Serial.print("Set radius: ");
//  Serial.print(radFrom);
//  Serial.print(" -> ");
//  Serial.print(radTo);
//  Serial.print(" == ");
//  Serial.println(radDelta);
//  Serial.print("Degrees: ");
//  Serial.print(degFrom);
//  Serial.print(" -> ");
//  Serial.print(degTo);
//  Serial.print(" == ");
//  Serial.println(degDelta);
}

bool doMove() {
  // step per 1 mm of radius and 1 degree of rotation
  float radDeltaMM = radDelta * fullRadiusMM;

  float degAdded = 0;
  float radAdded = 0;
    
  float nextDegAdded = 0;
  float nextRadAdded = 0;

//    Serial.println("move");
//    Serial.println(moveAdded);
//    Serial.println(radDeltaMM);
//    Serial.println(degDelta);
  bool movesLeft = false;
  //based on current step calculate current deg and next deg
  if (radDeltaMM > 0 && radDeltaMM >= degDelta && moveAdded < radDeltaMM) {
    // RAD leads, DEG follows
//    Serial.println("Rad leads");
    if (degFrom < degTo){
      degAdded = degFrom + ((degDelta / radDeltaMM) * moveAdded);
    }else{
      degAdded = degFrom - ((degDelta / radDeltaMM) * moveAdded);
    }
    if (radFrom < radTo){
      radAdded = (radFrom*fullRadiusMM) + moveAdded;
    }else{
      radAdded = (radFrom*fullRadiusMM) - moveAdded;
    }

    if (degFrom < degTo){
      nextDegAdded = degFrom + ((degDelta / radDeltaMM) * (moveAdded+1));
    }else{
      nextDegAdded = degFrom - ((degDelta / radDeltaMM) * (moveAdded+1));
    }
    if (radFrom < radTo){
      nextRadAdded = (radFrom*fullRadiusMM) + moveAdded + 1;
    }else{
      nextRadAdded = (radFrom*fullRadiusMM) - moveAdded - 1;
    }
//    
//    Serial.print(currentDeg);
//    Serial.print("/Rad: ");
//    Serial.println(currentRad);
    
    movesLeft = true;
  }else if (moveAdded < degDelta && degDelta > 0){
    // DEG leads, RAD follows
//    Serial.println("Deg leads");
   if (degFrom < degTo){
      degAdded = degFrom + moveAdded;
    }else{
      degAdded = degFrom - moveAdded;
    }
    if (radFrom < radTo){
      radAdded = (radFrom*fullRadiusMM) + ((radDeltaMM / degDelta) * moveAdded);
    }else{
      radAdded = (radFrom*fullRadiusMM) - ((radDeltaMM / degDelta) * moveAdded);
    }
    
//    Serial.print(currentDeg);
//    Serial.print("/Rad: ");
//    Serial.print(currentRad);
    
    // DEG leads, RAD follows
   if (degFrom < degTo){
      nextDegAdded = degFrom + moveAdded + 1;
    }else{
      nextDegAdded = degFrom - moveAdded - 1;
    }
    if (radFrom < radTo){
      nextRadAdded = (radFrom*fullRadiusMM) + ((radDeltaMM / degDelta) * (moveAdded + 1));
    }else{
      nextRadAdded = (radFrom*fullRadiusMM) - ((radDeltaMM / degDelta) * (moveAdded + 1));
    }
//
//    Serial.print(", next Deg: ");
//    Serial.print(nextDegAdded);
//    Serial.print("/Rad: ");
//    Serial.print(nextRadAdded);
    movesLeft = true;
  }
  if (movesLeft) {
    
    float mmDistStep = polarDistance(radAdded, degAdded, nextRadAdded, nextDegAdded);

    //Calculate the minimum speed, for center rotation (this would otherwise happen infinitely fast at 0 radius)
    long microSecForStepMin = (long) ((fabs(degAdded - nextDegAdded)/maxDegPerSec) * 1000000.0);
    
    long microSecForStep = microSecForStepMin;
    if (mmDistStep > 0)  {
      microSecForStep = (long) ((1000000.0 / (speedMMPerSec / mmDistStep) ));
    }
    if (microSecForStep < microSecForStepMin) {
      microSecForStep = microSecForStepMin;
  //    Serial.print("Step min applied");
    }
//    Serial.print("radAdded=");
//    Serial.print(radAdded);
//    Serial.print("degAdded=");
//    Serial.print(degAdded);
//    Serial.print("nextRadAdded=");
//    Serial.print(nextRadAdded);
//    Serial.print("nextDegAdded=");
//    Serial.print(nextDegAdded);

    //int servoDegFrom = degToServoPos(radAdded / fullRadiusMM, degAdded);
    //int servoRadFrom = radToServoPos(radAdded / fullRadiusMM, degAdded);
    servoDegTo = degToServoPos(nextRadAdded / fullRadiusMM, nextDegAdded);
    servoRadTo = radToServoPos(nextRadAdded / fullRadiusMM, nextDegAdded);

//    Serial.print("mmDistStep = ");
//    Serial.println(mmDistStep);
    
//    Serial.print("actuateServos(");
//    Serial.print("servoRadFrom=");
//    Serial.print(servoRad);
//    Serial.print(", servoDegFrom=");
//    Serial.print(servoDeg);
//    Serial.print(", servoRadTo=");
//    Serial.print(servoRadTo);
//    Serial.print(", servoDegTo=");
//    Serial.print(servoDegTo);
//    Serial.print(", microSecForStep=");
//    Serial.println(microSecForStep);
    
    actuateServos(servoRad, servoDeg, servoRadTo, servoDegTo, microSecForStep);
    servoDeg = servoDegTo;
    servoRad = servoRadTo;
    moveAdded = moveAdded + 1;
  }
  return movesLeft;
}

float polarDistance(float rad1, float deg1, float rad2, float deg2) {
  return sqrt(sq(rad1) + sq(rad2) - 2 * rad1 * rad2 * cos(degreesToRadians(deg1 - deg2)));
}

/**
 * Travel from servo pos rad1,deg1 to servopos rad2,deg2 in timeSpan microseconds
 */
 long deltaRad = 0;
 long deltaDeg = 0;
 long timePerRad = 0;
 long timePerDeg = 0;
 
void actuateServos(int rad1, int deg1, int rad2, int deg2, long microSec) {
  long deltaRad = abs(rad1 - rad2);
  long deltaDeg = abs(deg1 - deg2);
  long timePerRad = 0; 
  if (deltaRad > 0){
    timePerRad = microSec / deltaRad / 2; // divided by 2 so that we do high/low in one window
  }
  long timePerDeg = 0;
  if (deltaDeg > 0) {
    timePerDeg = microSec / deltaDeg / 2; 
  }
  
  long nextRadTime = timePerRad;
  long nextDegTime = timePerDeg;

  int currRad = rad1;
  int currDeg = deg1;

//  Serial.print("Move servos rad1:");
//  Serial.print(rad1);
//  Serial.print(" deg1:");
//  Serial.print(deg1);
//  Serial.print(" rad2:");
//  Serial.print(rad2);
//  Serial.print(" deg2:");
//  Serial.print(deg2);
//  Serial.print(" deltaDeg:");
//  Serial.print(deltaDeg);
//  Serial.print(" deltaRad:");
//  Serial.print(deltaRad);
//  Serial.print(" timePerDeg:");
//  Serial.print(timePerDeg);
//  Serial.print(" timePerRad:");
//  Serial.print(timePerRad);
//  
//  Serial.print(" microSec: ");
//  Serial.println(microSec);
  
  bool radInProgress = deltaRad > 0 && ((rad1 < rad2) && (currRad < rad2) || (rad1 > rad2) && (currRad > rad2));
  bool degInProgress = deltaDeg > 0 && ((deg1 < deg2) && (currDeg < deg2) || (deg1 > deg2) && (currDeg > deg2));
  
  while (radInProgress || degInProgress) {
    wdt_reset();
    radInProgress = deltaRad > 0 && ((rad1 < rad2) && (currRad < rad2) || (rad1 > rad2) && (currRad > rad2));
    degInProgress = deltaDeg > 0 && ((deg1 < deg2) && (currDeg < deg2) || (deg1 > deg2) && (currDeg > deg2));
    //calculate whats next up, and pause until then, then actuate and do it again
    
    long timeToWait = 0;
    if (radInProgress) {
      if (!degInProgress || nextRadTime <= nextDegTime) {
        // rad is next up
        timeToWait = nextRadTime;
      }
    } 
    if (degInProgress) {
      if (!radInProgress || nextRadTime > nextDegTime) {
        // deg is next up
        timeToWait = nextDegTime;
      }
    }

    if (degInProgress){ nextDegTime -= timeToWait;}
    if (radInProgress){ nextRadTime -= timeToWait;}
    delayMicroseconds(timeToWait);
    
    if (nextRadTime <= 0) {
      nextRadTime = nextRadTime + timePerRad;
      digitalWrite(dirRad,rad1 > rad2);
      bool radLatch = !digitalRead(stepRad);
      digitalWrite(stepRad,radLatch);
      if (radLatch) {
        if (rad1 < rad2) { 
          currRad++;
        }else{
          currRad--;
        } 
      }
    }
    if (nextDegTime <= 0) {
      nextDegTime = nextDegTime + timePerDeg;
      digitalWrite(dirDeg,deg1 < deg2);
      bool degLatch = !digitalRead(stepDeg);
      digitalWrite(stepDeg,degLatch);
      if (degLatch) {
        if (deg1 < deg2) { 
          currDeg++;
        }else{
          currDeg--;
        }
      }
    }
  }
}

float radiansToDegrees(float rad) {
  return rad * 57296 / 1000;  
}

float degreesToRadians(float deg) {
  return deg / 57296 * 1000;  
}

float angleArm(float radius) {
  return radiansToDegrees(asin(radius) * 2.0);
}

float radiusArm(int angle) {
  return sin(degreesToRadians(angle / 2));
}

//Calculate servo position of "radius servo" for given radius and degrees
int radToServoPos(float radius, float deg) {
  float angleArmVal = angleArm(radius);
  
  float correctionDeg = angleArmVal / 2.0;

  //Correct the rad servo for the rotations of the DEG servo
  float correctionRad = 0;
  if (deg - correctionDeg != 0){
    correctionRad = revsServo / 360.0 * (deg - correctionDeg);
  }

  float revsArm = ((angleArmVal / 360.0) * revsRad);
  
  return (int) (revsArm + correctionRad);
}

//Calculate servo position of "degrees Servo" for given radius and degrees
int degToServoPos(float radius, float deg) {
  //correct deg position for the arm angle
  float correctionDeg = angleArm(radius) / 2.0;

  return (int) ((deg - correctionDeg) / 360.0 * revsDeg);
}


void setup() {
  Serial.begin(19200);
  // Sets the two pins as Outputs
  pinMode(stepDeg,OUTPUT);
  pinMode(dirDeg,OUTPUT);
  
  pinMode(stepRad,OUTPUT);
  pinMode(dirRad,OUTPUT);
 
  pinMode(enPin,OUTPUT);
  digitalWrite(enPin,LOW);
  SPIFFS.begin();

  playThr("/SpiralBezier.thr");
}

void loop() {
  if (!doMove() && playingFile) {
    playNextThrLine();
  }
}
