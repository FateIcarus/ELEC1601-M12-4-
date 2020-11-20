//-----------------------------------------------------------------------------------------------------------//
//                                                                                                           //
//  Slave_ELEC1601_Student_2019_v3                                                                           //
//  The Instructor version of this code is identical to this version EXCEPT it also sets PIN codes           //
//  20191008 Peter Jones                                                                                     //
//                                                                                                           //
//  Bi-directional passing of serial inputs via Bluetooth                                                    //
//  Note: the void loop() contents differ from "capitalise and return" code                                  //
//                                                                                                           //
//  This version was initially based on the 2011 Steve Chang code but has been substantially revised         //
//  and heavily documented throughout.                                                                       //
//                                                                                                           //
//  20190927 Ross Hutton                                                                                     //
//  Identified that opening the Arduino IDE Serial Monitor asserts a DTR signal which resets the Arduino,    //
//  causing it to re-execute the full connection setup routine. If this reset happens on the Slave system,   //
//  re-running the setup routine appears to drop the connection. The Master is unaware of this loss and      //
//  makes no attempt to re-connect. Code has been added to check if the Bluetooth connection remains         //
//  established and, if so, the setup process is bypassed.                                                   //
//                                                                                                           //
//-----------------------------------------------------------------------------------------------------------//

#include <SoftwareSerial.h>   //Software Serial Port
#include <Servo.h>

#define RxD 7
#define TxD 6
#define ConnStatus A1

#define DEBUG_ENABLED  1

// ##################################################################################
// ### EDIT THE LINES BELOW TO MATCH YOUR SHIELD NUMBER AND CONNECTION PIN OPTION ###
// ##################################################################################

int shieldPairNumber = 4;

// CAUTION: If ConnStatusSupported = true you MUST NOT use pin A1 otherwise "random" reboots will occur
// CAUTION: If ConnStatusSupported = true you MUST set the PIO[1] switch to A1 (not NC)

boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin

// #######################################################

// The following two string variable are used to simplify adaptation of code to different shield pairs

String slaveNameCmd = "\r\n+STNA=Slave";   // This is concatenated with shieldPairNumber later

SoftwareSerial blueToothSerial(RxD,TxD);

//DECLARING GLOBAL VARIABLES
Servo servoLeft;
Servo servoRight;


int buzzerPin = 3;

int lOut = 1;
int cOut = 1;
int rOut = 1;

int turnSpeed = 15;
int driveSpeed = 30;

int lPin = A3;
int cPin = A4;
int rPin = A5;

int joyStatus = 1;

//-----------------------------------------------------------------  
void setup()
{
    Serial.begin(9600);
    blueToothSerial.begin(38400);                    // Set Bluetooth module to default baud rate 38400
    
    pinMode(RxD, INPUT);
    pinMode(TxD, OUTPUT);
    pinMode(ConnStatus, INPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(lPin,INPUT);
    pinMode(cPin,INPUT);
    pinMode(rPin,INPUT);
  
    servoLeft.attach(13);                           //SERVO MOTORS
    servoRight.attach(12);

    //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
    //  This prevents running the full connection setup routine if not necessary.

    if(ConnStatusSupported) Serial.println("Checking Slave-Master connection status.");

    if(ConnStatusSupported && digitalRead(ConnStatus)==1)
    {
        Serial.println("Already connected to Master - remove USB cable if reboot of Master Bluetooth required.");
    }
    else
    {
        Serial.println("Not connected to Master.");
        
        setupBlueToothConnection();   // Set up the local (slave) Bluetooth module

        delay(1000);                  // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
    }
    tone(buzzerPin,1000,100);
    delay(500);
    tone(buzzerPin,1000,100);
}

//-----------------------------------------------------------------  
void loop()
{
    char recvChar;
    String str = "";

    while(1)
    {
        int c = 0;
        //SENSOR
        lOut = digitalRead(lPin);
        cOut = digitalRead(cPin);
        rOut = digitalRead(rPin);
        
        if(blueToothSerial.available())   // Check if there's any data sent from the remote Bluetooth shield
        {
            recvChar = blueToothSerial.read();
            
            if (recvChar == 'z'){
              joyStatus = joyStatus*-1;
              tone(buzzerPin, 1000, 200);
              delay(200);
            } 
            
            if (joyStatus == 1){
              if (recvChar == 'f'){
                c = 1;
              } else if (recvChar == 'b'){
                c = -1;
              } else if (recvChar == 'l'){
                c = 2;
              } else if (recvChar == 'r'){
                c = -2;
              } else if (recvChar == 's'){
                c = 0;
              }

            driveCommand(c,1);
                          
            }

            if (joyStatus == -1){
             if (lOut == 1 && cOut == 1 && rOut == 1){
                driveCommand(0,1);
                blueToothSerial.print("Object Found");
                tone(buzzerPin,400,500);
              } else if (lOut == 0 && rOut == 0){
                driveCommand(1,1);
              } else if (lOut == 1 and rOut == 0){
                driveCommand(-2,1);
              } else if (lOut == 0 and rOut == 1){
                driveCommand(2,1);
              } else{
                driveCommand(0,1);
              }
            } 
        }
    }
}
//-----------------------------------------------------------------  
void drive(int l,int r,int t)
{
  servoLeft.writeMicroseconds(l);
  servoRight.writeMicroseconds(r);
  delay(t);
}

//-----------------------------------------------------------------  
void driveCommand(int c,int t)
{
  if (c == 0) 
  {
    drive(1500,1500,t);
  }
  if (c == 1) 
  {
    drive(1500-driveSpeed,1500+driveSpeed,t);
  }
  if (c == -1) 
  {
    drive(1500+driveSpeed,1500-driveSpeed,t);
  }
  if (c == 2) 
  {
    drive(1500-turnSpeed,1500-turnSpeed,t);
  }
  if (c == -2) 
  {
    drive(1500+turnSpeed,1500+turnSpeed,t);
  }
}

//-----------------------------------------------------------------  
void setupBlueToothConnection()
{
    Serial.println("Setting up the local (slave) Bluetooth module.");

    slaveNameCmd += shieldPairNumber;
    slaveNameCmd += "\r\n";

    blueToothSerial.print("\r\n+STWMOD=0\r\n");      // Set the Bluetooth to work in slave mode
    blueToothSerial.print(slaveNameCmd);             // Set the Bluetooth name using slaveNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection should be forbidden here
    blueToothSerial.print("\r\n+STOAUT=1\r\n");      // Permit paired device to connect me
    
    //  print() sets up a transmit/outgoing buffer for the string which is then transmitted via interrupts one character at a time.
    //  This allows the program to keep running, with the transmitting happening in the background.
    //  Serial.flush() does not empty this buffer, instead it pauses the program until all Serial.print()ing is done.
    //  This is useful if there is critical timing mixed in with Serial.print()s.
    //  To clear an "incoming" serial buffer, use while(Serial.available()){Serial.read();}

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the slave Bluetooth inquirable
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("The slave bluetooth is inquirable!");
}
