/* Computer-controlled hot-air balloon 
 * @author	: TONG Haowen Joel
 * @org		: Singapore Acamdemy of Young Engineers and Scientists (SAYES), Science Centre Singapore
 * @date	: 20 Jan 2012
 * @descr	: This code controls an Arduino-powered wireless helium balloon, using a computer and a wireless car key transmitter.
 * @credit	: Receiver / transmitter protocol based on code by Maurice Ribble, http://www.glacialwanderer.com/hobbyrobotics, 8-30-2009
 */ 


#include <SoftwareSerial.h>
#include <Servo.h>

/* uncomment respectively */
//#define TRANSMITTER
#define RECEIVER

/* define the baud rate */
#define BAUD_RATE					2400

/* define opcodes for controlling receiver arduino */
#define OP_ERROR					0x01
#define OP_SUCCESS					0x00

#define OP_LED_OFF					0x06
#define OP_LED_BLINK					0x07


/*** ADVANCED BLOCK MOTOR CONTROL OPCODES ***/				//for easy control of motor commands
#define OP_MOTORS_STOP					0x10
#define OP_MOTORS_MOVE_FWD				0x11
#define OP_MOTORS_MOVE_REV				0x12
#define OP_MOTORS_TURN_LEFT				0x13
#define OP_MOTORS_TURN_RIGHT				0x14
#define OP_MOTORS_STRAFE_FWD_LEFT			0x15
#define OP_MOTORS_STRAFE_FWD_RIGHT			0x16
#define OP_MOTORS_STRAFE_REV_LEFT			0X17
#define OP_MOTORS_STRAFE_REV_RIGHT			0x18


/** MOTOR SPEED CONTROL OPCODES ***/
#define OP_MOTOR_SPEED_25				0x21
#define OP_MOTOR_SPEED_50				0x22
#define OP_MOTOR_SPEED_75				0x23
#define OP_MOTOR_SPEED_100				0x24



#define OP_SERVO_00					0x30
#define OP_SERVO_30					0x31



/* This routine MAPS the keyboard INPUT byte to our blimp's OUTPUT OPCODE */
byte mkBlimpByte(byte key) {
	if (key == '[')		return OP_LED_BLINK;
	else if (key == ']')	return OP_LED_OFF;
	
	else if (key == 'w')	return OP_MOTORS_MOVE_FWD;
	else if (key == 's')	return OP_MOTORS_MOVE_REV;
	else if (key == 'a')	return OP_MOTORS_TURN_LEFT;
	else if (key == 'd')	return OP_MOTORS_TURN_RIGHT;
	else if (key == 'q')	return OP_MOTORS_STRAFE_FWD_LEFT;
	else if (key == 'e')	return OP_MOTORS_STRAFE_FWD_RIGHT;
	else if (key == 'z')	return OP_MOTORS_STRAFE_REV_LEFT;
	else if (key == 'c')	return OP_MOTORS_STRAFE_REV_RIGHT;
	
	else if (key == '1')	return OP_MOTOR_SPEED_25;
	else if (key == '2')	return OP_MOTOR_SPEED_50;
	else if (key == '3')	return OP_MOTOR_SPEED_75;
	else if (key == '4')	return OP_MOTOR_SPEED_100;
	
	else if (key == '0')	return OP_SERVO_00;
	else if (key == '1')	return OP_SERVO_30;
	else			return OP_ERROR;				//if no button is pressed
}



/* Packet as follows: 
 * [INIT] [NETWORK_SIG_0] [NETWORK_SIG_1] [NETWORK_SIG_2] [MSG] [CHKSUM]
 */
#define INIT_SIZE					1
#define NETWORK_SIG_SIZE				3
#define MSG_SIZE					1
#define CHKSUM_SIZE					1
#define PACKET_SIZE					(INIT_SIZE + NETWORK_SIG_SIZE + MSG_SIZE + CHKSUM_SIZE)

const byte INIT					=	0xF0;
const byte CHANNEL				=	0x05;
const byte NETWORK_SIG[NETWORK_SIG_SIZE] 	=	{0x8F, 0xAA, CHANNEL};

SoftwareSerial tx 				=	SoftwareSerial(2,3);




/* This routine waits for available data before proceeding to read the socket */
void serialWait() { while(Serial.available() == 0) {}; }

/* This routine validates the message packet byte using MSG <XOR> 0xFF */
byte mkChkSum(byte val) { return (val ^ 0xFF); } 

void writeByte(byte val) {
	tx.print(INIT, BYTE);
	tx.print(NETWORK_SIG[0], BYTE);
	tx.print(NETWORK_SIG[1], BYTE);
	tx.print(NETWORK_SIG[2], BYTE);
	tx.print(val, BYTE);
	tx.print(mkChkSum(val),BYTE);

}

byte readByte() {
	int 	pos	= 0;
	byte 	val	= 0x00;
	byte	chksum	= 0x00;
	byte	curByte = 0x00;
	
	while(pos < NETWORK_SIG_SIZE) {
		serialWait();
		curByte = Serial.read();
			if (curByte == NETWORK_SIG[pos]) {
				if (pos == (NETWORK_SIG_SIZE-1)) {
					serialWait();
	      				val	= Serial.read();
                                        serialWait();
	      				chksum	= Serial.read();
	      				if (chksum == mkChkSum(val) )
	      					while(Serial.available())	//clear any remaining data in the Rx buffer to minimise latency
	      						Serial.read();
	      					return val;  			//success			
				} else
	      				pos++;
			}
	}
        return OP_ERROR;							//return error opcode
}


/** transmitter code **/

#ifdef TRANSMITTER

void setup()
{
  tx.begin(BAUD_RATE);
  Serial.begin(115200);										//this is for the PC so we set at the max rate
  
  Serial.println("Type 'b' to start blinking and 's' to stop blinking");
  Serial.print(">");
}

void loop()
{
	byte keyPressed	= Serial.read();
	byte blimpByte 	= mkBlimpByte(keyPressed);
	
	if (blimpByte != OP_ERROR) {
		writeByte(blimpByte);
		Serial.print(keyPressed);
		Serial.println(">");
	} else {										//do these commands if nothing is pressed
		writeByte(OP_MOTORS_STOP);							//prevent latency lag of motor (see Rx code for more info)
	}
	
}
#endif //TRANSMITTER



/** receiver code **/


#ifdef RECEIVER

/*** Use the following pins ***/
/* LED */
#define LED_PIN			13

/* MOTOR PINS */
#define MOTOR_1_ENABLE 		5					//must be PWM pin
#define MOTOR_1_PIN1 		6 
#define MOTOR_1_PIN2 		7

#define MOTOR_2_ENABLE 		10					//must be PWM pin
#define MOTOR_2_PIN1 		9
#define MOTOR_2_PIN2 		8

#define SERVO_1			3					//must be PWM pin

/* MAXIMUM CONST */
#define MOTOR_MAX_SPEED		255					//max speed of motor


/*** BASIC MOTOR CONTROL OPCODES ***/
#define MOTOR_FWD		true
#define MOTOR_REV		false


/*** GLOBAL FLAGS ***/ 
//current state
boolean FLAG_MOTOR_1_STATE		= true;						//true: forward | false: backward
boolean FLAG_MOTOR_2_STATE		= true;						//true: forward | false: backward

//state in memory
boolean FLAG_LAST_MOTOR_1_STATE		= true;
boolean FLAG_LAST_MOTOR_2_STATE		= true;



/*** GLOBAL VARIABLES ***/

//MOTOR//
//current state
int MOTOR_1_SPEED			= MOTOR_MAX_SPEED;				//these define the motor speed (range: 0-255)
int MOTOR_2_SPEED			= MOTOR_MAX_SPEED;

//state in memory
int LAST_MOTOR_1_SPEED			= MOTOR_MAX_SPEED;
int LAST_MOTOR_2_SPEED			= MOTOR_MAX_SPEED;


//COMMANDS//
byte LAST_STATE_BYTE			= OP_ERROR;					//IMPT: the last command state the Blimp was given.  This is required to reduce number of hardware interrupts for better perfomance (esp enabling the motor to run at true PWM speed) 


/*** GLOBAL COMPONENTS ***/
Servo pitchServo;


/* This routine makes it easier to control the direction and speed of the motors.
 * Information about the motor speed and direction can be obtained by accessing the relevant flags.
 */
void setMotor(int motorNumber, boolean direction, int speed) {			//motorNumber{1|2}, direction{true|false}, speed{0-255}
	if (motorNumber == 1) {
		FLAG_MOTOR_1_STATE	= direction;
		MOTOR_1_SPEED		= speed;
		
		if (direction == MOTOR_FWD) {
			digitalWrite(MOTOR_1_PIN1, HIGH);
  			digitalWrite(MOTOR_1_PIN2, LOW);
  		} else if (direction == MOTOR_REV) {
  			digitalWrite(MOTOR_1_PIN1, LOW);
  			digitalWrite(MOTOR_1_PIN2, HIGH);
  		}	
  		if (speed == 0) {
  			analogWrite(MOTOR_1_ENABLE, 0);
  			digitalWrite(MOTOR_1_PIN1, LOW);
  			digitalWrite(MOTOR_1_PIN2, LOW);
  		} else
  			analogWrite(MOTOR_1_ENABLE, speed);
  		
  	} else if (motorNumber == 2) {
		FLAG_MOTOR_2_STATE	= direction;
		MOTOR_2_SPEED		= speed;
		
		if (direction == MOTOR_FWD) {
			digitalWrite(MOTOR_2_PIN1, HIGH);
  			digitalWrite(MOTOR_2_PIN2, LOW);
  		} else if (direction == MOTOR_REV) {
  			digitalWrite(MOTOR_2_PIN1, LOW);
  			digitalWrite(MOTOR_2_PIN2, HIGH);
  		}	
  		if (speed == 0) {
  			analogWrite(MOTOR_2_ENABLE, 0);
  			digitalWrite(MOTOR_2_PIN1, LOW);
  			digitalWrite(MOTOR_2_PIN2, LOW);
  		} else
  			analogWrite(MOTOR_2_ENABLE, speed);
  	}
}


/* This routine is advanced form of block motor control.  It can move, turn, and strafe. */
void tellMotors(byte command) {
	if (command == OP_MOTORS_MOVE_FWD) {
		setMotor(1,MOTOR_FWD,LAST_MOTOR_1_SPEED);
		setMotor(2,MOTOR_FWD,LAST_MOTOR_2_SPEED);
	
	} else if (command == OP_MOTORS_STOP) {				//this is placed near the beginning of tree as it is the most called routine. 
		setMotor(1,MOTOR_FWD,0);
		setMotor(2,MOTOR_FWD,0);
		
	} else if (command == OP_MOTORS_MOVE_REV) {
		setMotor(1,MOTOR_REV,LAST_MOTOR_1_SPEED);
		setMotor(2,MOTOR_REV,LAST_MOTOR_2_SPEED);
		
	} else if (command == OP_MOTORS_TURN_LEFT) {
		setMotor(1,MOTOR_REV,LAST_MOTOR_1_SPEED);
		setMotor(2,MOTOR_FWD,LAST_MOTOR_2_SPEED);
	
	} else if (command == OP_MOTORS_TURN_RIGHT) {
		setMotor(1,MOTOR_FWD,LAST_MOTOR_1_SPEED);
		setMotor(2,MOTOR_REV,LAST_MOTOR_2_SPEED);
	
	} else if (command == OP_MOTORS_STRAFE_FWD_LEFT) {
		setMotor(1,MOTOR_REV,0);
		setMotor(2,MOTOR_FWD,LAST_MOTOR_2_SPEED);
	
	} else if (command == OP_MOTORS_STRAFE_FWD_RIGHT) {
		setMotor(1,MOTOR_FWD,LAST_MOTOR_1_SPEED);
		setMotor(2,MOTOR_REV,0);
	
	} else if (command == OP_MOTORS_STRAFE_REV_LEFT) {
		setMotor(1,MOTOR_FWD,0);
		setMotor(2,MOTOR_REV,LAST_MOTOR_2_SPEED);
	
	} else if (command == OP_MOTORS_STRAFE_REV_RIGHT) {
		setMotor(1,MOTOR_REV,LAST_MOTOR_1_SPEED);
		setMotor(2,MOTOR_FWD,0);
	}
}


/* this routine controls motor speed */
void setSpeed(int speed1, int speed2) {
	MOTOR_1_SPEED = LAST_MOTOR_1_SPEED = speed1;		//assign to current and memory speed
	MOTOR_2_SPEED = LAST_MOTOR_2_SPEED = speed2;
}


void setup()
{
  Serial.begin(BAUD_RATE);
  pinMode(LED_PIN, OUTPUT);
  
  pinMode(MOTOR_1_PIN1, OUTPUT);
  pinMode(MOTOR_1_PIN2, OUTPUT);
  
  pinMode(MOTOR_2_PIN1, OUTPUT);
  pinMode(MOTOR_2_PIN2, OUTPUT);
  
  digitalWrite(LED_PIN, LOW);

}

void loop() {
  
	byte theByte = 0x00;
	theByte = readByte();
  
  	if (theByte == LAST_STATE_BYTE) {} //forward branch prediction to improve code efficiency
  	else {  		

		if (theByte == OP_LED_BLINK) 			// Check to see if we got the 71 test number
			digitalWrite(LED_PIN, HIGH);	
		
		else if (theByte == OP_LED_OFF)
	  		digitalWrite(LED_PIN, LOW);
	  		
	  	/*** start of advanced motor control ***/
	  	else if (theByte == OP_MOTORS_MOVE_FWD)
			tellMotors(OP_MOTORS_MOVE_FWD);
	
		else if (theByte == OP_MOTORS_MOVE_REV)
			tellMotors(OP_MOTORS_MOVE_REV);
		
		else if (theByte == OP_MOTORS_TURN_LEFT)
			tellMotors(OP_MOTORS_TURN_LEFT);
		
		else if (theByte == OP_MOTORS_TURN_RIGHT)
			tellMotors(OP_MOTORS_TURN_RIGHT);
		
		else if (theByte == OP_MOTORS_STRAFE_FWD_LEFT)
			tellMotors(OP_MOTORS_STRAFE_FWD_LEFT);
		
		else if (theByte == OP_MOTORS_STRAFE_FWD_RIGHT)
			tellMotors(OP_MOTORS_STRAFE_FWD_RIGHT);
		
		else if (theByte == OP_MOTORS_STRAFE_REV_LEFT)
			tellMotors(OP_MOTORS_STRAFE_REV_LEFT);
		
		else if (theByte == OP_MOTORS_STRAFE_REV_RIGHT)
			tellMotors(OP_MOTORS_STRAFE_REV_RIGHT);	
	
		/*** end of advanced motor control ***/
	
	
		/*** start of motor speed control ***/	
		else if (theByte == OP_MOTOR_SPEED_25)
			setSpeed(0.25*MOTOR_MAX_SPEED,0.25*MOTOR_MAX_SPEED);
		else if (theByte == OP_MOTOR_SPEED_50)
			setSpeed(0.5*MOTOR_MAX_SPEED,0.5*MOTOR_MAX_SPEED);
		else if (theByte == OP_MOTOR_SPEED_75)
			setSpeed(0.75*MOTOR_MAX_SPEED,0.75*MOTOR_MAX_SPEED);		
		else if (theByte == OP_MOTOR_SPEED_100)
			setSpeed(MOTOR_MAX_SPEED,MOTOR_MAX_SPEED);			
		/*** end of motor speed control ***/
	

	  	else if (theByte == OP_SERVO_00) {		//set servo to stationary
	  		pitchServo.attach(SERVO_1);  		
	  		pitchServo.write(90);
	  		delay(1000);
	  		pitchServo.detach();		
	  		
	  	} else if (theByte == OP_SERVO_30) {		//pitch servo to 30 degrees
	  		pitchServo.attach(SERVO_1);
	   		pitchServo.write(0);
	   		delay(1000);
	  		
	  	} else if (theByte == OP_MOTORS_STOP) {		//this prevents the motor from "latency" lag, ie immediate stop when there is no keyboard input.
			tellMotors(OP_MOTORS_STOP);
	  	}
	  	
		LAST_STATE_BYTE = theByte;			//update the last state to the current state
	}

}
#endif //RECEIVER

