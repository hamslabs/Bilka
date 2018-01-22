

/*
 * Closed loop control of the take-up reel for the laser cutter
 *
 * This uses an external stepper connected on pins 8,9,10 and 11
 *
 * An ultrasonic sensor with the triggger on pin 5 and the echo on pin 6
 *
 * and a 16x2 LCD connected through i2c with SCL on A5 and SDA on A4
 *
 */

#include <SD.h>
#include <NewPing.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define TRIG_PIN 5
#define ECHO_PIN 6
#define ECHO_INT 0


#define TARGET_DIST 50  // how far in cm is the desired ZERO hight
#define LIMIT_DIST 15	// how much deviation from ZERO is ok

// init the ultra sonic library
NewPing sonar(TRIG_PIN, ECHO_PIN, 200);

// initialize the stepper library on pins 8 through 11:
AccelStepper stepper(AccelStepper::FULL4WIRE, 8, 9, 10, 11);

// Init the i2c LCD control at address 0x27
LiquidCrystal_I2C  lcd(0x27,16,2);


unsigned int pingSpeed = 500; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned long pingTimer;     // Holds the next ping time.

typedef enum
{
	MOTOR_NOT_RUNNING,
	MOTOR_RUNNING_UP,
	MOTOR_RUNNING_DOWN
} MotorStatus_t;



char motorStatusPrefix[] = "Stat: ";
int motorStatusPrefixSize = strlen(motorStatusPrefix);

char *motorStatusMsgs[] =
{
//   012345678901
	"Stopped",
	"Up     ",
	"Down   "
};

MotorStatus_t motorStatus = MOTOR_NOT_RUNNING;

char distString[16];


void DisplayMotorStatus(void)
{
		lcd.setCursor(0,0);
		lcd.print(motorStatusPrefix);
		lcd.print(motorStatusMsgs[motorStatus]);
		Serial.println(motorStatusMsgs[motorStatus]);
}

void setup() {
	// get serial port going
	Serial.begin (115200);

	// set stepper motor param
	stepper.setMaxSpeed(1600.0);
	stepper.setAcceleration(10000.0);

	// lcd init
	lcd.init();
	lcd.backlight();
}


void loop() {
	int motorSpeed;
	long distFromNeutral;
	int dir;
	if (millis() >= pingTimer) {   // pingSpeed milliseconds since last ping, do another ping.
		pingTimer += pingSpeed;      // Set the next ping time.
		sonar.ping_timer(echoCheck); // Send out the ping, calls "echoCheck" function every 24uS where you can check the ping status.
	}
	stepper.run();
}


void echoCheck() { // Timer2 interrupt calls this function every 24uS where you can check the ping status.
	//  Don't do anything here!
	long dist;
	if (sonar.check_timer()) { // This is how you check to see if the ping was received.
    
		dist = sonar.ping_result / US_ROUNDTRIP_CM - TARGET_DIST;

		if ((motorStatus == MOTOR_RUNNING_UP) && (dist < 0))
		{
			stepper.stop();
			motorStatus = MOTOR_NOT_RUNNING;
			DisplayMotorStatus();
		}
		else if ((motorStatus == MOTOR_RUNNING_DOWN) && (dist > 0))
		{
			stepper.stop();
			motorStatus = MOTOR_NOT_RUNNING;
			DisplayMotorStatus();
		} else if ((dist > LIMIT_DIST) && (motorStatus != MOTOR_RUNNING_UP)) {
			stepper.setCurrentPosition(0);
			stepper.moveTo(-100000);
			motorStatus = MOTOR_RUNNING_UP;
			DisplayMotorStatus();
		}
		else if ((dist < -LIMIT_DIST) && (motorStatus != MOTOR_RUNNING_DOWN)) {
			stepper.setCurrentPosition(0);
			stepper.moveTo(100000);
			motorStatus = MOTOR_RUNNING_DOWN;
			DisplayMotorStatus();
		}
		// Here's where you can add code.
		sprintf(distString, "dist: %3d cm", dist);
		Serial.println(distString);
		lcd.setCursor(0,1);
		lcd.print(distString);
	}
// Don't do anything here!
}
