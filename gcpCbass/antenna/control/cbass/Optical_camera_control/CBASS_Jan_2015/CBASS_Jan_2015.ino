#include <Servo.h> // includes library
Servo myservo; //create servo object to control a servo
int START; // a variable for your starting position
int STOP; // a variable for yout ending postion
int pos; // a variable for current position
char PC_command = ' '; // a variable for you pc command
int switchpin1_input = 4; // the switch
int switchpin1_output = 2; // switch power source
int switchstate = 0; // variable for reading the switch state
int transistor = 7; // transistor base pin
int cam_transistor = 8; // transistor base pin to power camera

void setup(){

  // initialize serial communication:
  Serial.begin(9600);
  
  // initialise the pins as input or output:
  pinMode(switchpin1_input, INPUT);
  pinMode(switchpin1_output, OUTPUT);
  pinMode(transistor, OUTPUT);
  pinMode(cam_transistor, OUTPUT);

}

void loop(){
  
  digitalWrite(switchpin1_output, HIGH);
  switchstate = digitalRead(switchpin1_input);
  PC_command = Serial.read(); // read the oldest byte in the serial buffer:

// Status query:
  if (PC_command == 'S'){
    PC_command = ' ';
    if (switchstate == HIGH){
      Serial.println("C\r");
      } 
      else if(switchstate == LOW){
        Serial.println("O\r"); 
      }
      else{
        Serial.println("U\r");
      }
  }


// Power camera:
  if (PC_command == 'P'){
    digitalWrite(cam_transistor, HIGH);
  }
  
// Kill camera:
  if (PC_command == 'D'){
    digitalWrite(cam_transistor, LOW);
  }
  
// Close shutter:
  if (PC_command == 'C'){
    PC_command = ' ';
    if (switchstate == LOW){
      myservo.attach(9); // attaches the servo on pin 9 to the servo object
      digitalWrite(transistor, HIGH);
      digitalWrite(cam_transistor, LOW);
      START = 180;
      STOP = 45;

      for (pos = START; pos >= STOP; pos -=1) // goes from start to stop
        {
          myservo.write(pos); // tell servo to move to position described by pos
          delay(50); // waits 20 ms for servo to reach position
          if (pos < 50){
            delay(1000);
            myservo.detach();
            digitalWrite(transistor, LOW);
            break;
          }
        }
      }
    }

    
// Open shutter:
    if (PC_command == 'O'){
      PC_command = ' ';
      myservo.attach(9); // attaches the servo on pin 9 to the servo object
      digitalWrite(transistor, HIGH);
      digitalWrite(cam_transistor, HIGH);
      START = 50;
      STOP = 180;

      for (pos = START; pos <= STOP; pos +=1) // goes from start to stop
      {
        myservo.write(pos); // tell servo to move to position described by pos
        delay(50); // waits 20 ms for servo to reach position
        if (pos > 175)
        {  
          delay(1000);
          myservo.detach();
          break;
        }
      }
    }
}



