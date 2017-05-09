//BoardBathController.ino

//#define NDEBUG                        // enable local debugging information

#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#include <SimpleTimer.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <SimpleTimer.h>

#include <Bounce2.h>
#include <OLED_I2C.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include <avr/eeprom.h>
#include "EEPROMAnything.h"

#define KNOB_ENC_PIN_1 A1    // Rotary encoder input pin 1 (A0)
#define KNOB_ENC_PIN_2 4    // Rotary encoder input pin 2
#define KNOB_BUTTON_PIN A2   // Rotary encoder button pin 
#define BUZZER	6
#define SOCKET1RED 12
#define SOCKET1GREEN 13
#define LEDSTRIP_FRONT 9
#define LEDSTRIP_BACK 5
#define MOTOR 3
#define HEATERRELAY 8
#define TEMPERATURE_PIN 10
#define PUMP 11

extern uint8_t MediumNumbers[];
extern uint8_t SmallFont[];
extern uint8_t RusFont[];
extern uint8_t BigNumbers[];

OLED  myOLED(SDA, SCL, 8);

SimpleTimer Channelstimer;


ClickEncoder *encoder;

OneWire oneWire(TEMPERATURE_PIN);        // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);  // Pass the oneWire reference to Dallas Temperature. 
float lastTemp1 = 0;

void timerIsr() {
  encoder->service();
}

typedef struct {
	uint8_t             desiredTemp=0;
	byte                frontLightLevel=0;
	byte                backLightLevel=0;	
	byte                motorSpeed=0;
} bath_params;

bath_params bathOptions;

/* 
0 - main menu
1 - liquid flush menu
2 - temp settings menu
3 - light settings menu
4 - motor settings menu
*/
uint8_t uiCurrentMenu = 0;
uint8_t uiPreviousMenu = 0;

uint8_t uiMenuCurrentSelection = 0;

float fCurrentTemp = 0;

boolean bIsRunning = false;

unsigned long lastencoderValue = -1;
long encoderValue = -1;

boolean  bInHeld = false;

boolean bRedLedOn = false;
boolean bGreenLedOn = false;

boolean bFlushInProgress = false;

//int iLastMotorSpeed = -1;
int motorspeed = -1;

void setup() 
{

//TCCR1B = TCCR1B & B11111000 | B00000100;
TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM00); //disable FastPWM on Timer0. mills lasts twice longer



TCCR2B = TCCR2B & B11111000 | B00000001; 



     #ifdef NDEBUG
       Serial.begin(115200); 
       Serial.println();
       Serial.println("Begin setup");
     #endif

//Channelstimer.setInterval(1000, timerIsr); 

  encoder = new ClickEncoder(KNOB_ENC_PIN_1, KNOB_ENC_PIN_2, KNOB_BUTTON_PIN, 2);

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 

   pinMode (KNOB_ENC_PIN_1,INPUT);
   pinMode (KNOB_ENC_PIN_2,INPUT);
   //pinMode (KNOB_BUTTON_PIN,INPUT);
   pinMode (BUZZER,OUTPUT);
   pinMode (SOCKET1RED,OUTPUT);
   pinMode (SOCKET1GREEN,OUTPUT);
   pinMode (LEDSTRIP_FRONT,OUTPUT);
   pinMode (LEDSTRIP_BACK,OUTPUT);
   pinMode (MOTOR,OUTPUT);
   pinMode (HEATERRELAY,OUTPUT);
   pinMode (TEMPERATURE_PIN,INPUT);   

digitalWrite(HEATERRELAY, LOW);

  myOLED.begin();

EEPROM_readAnything(0, bathOptions);


Channelstimer.setInterval(1000, checkTemp); 
          		

																																						


drawMenu();


digitalWrite(HEATERRELAY, HIGH); 

analogWrite(LEDSTRIP_BACK, map(bathOptions.backLightLevel, 0, 100, 0, 255));

Timer1.pwm(LEDSTRIP_FRONT, map(bathOptions.frontLightLevel, 0, 100, 0, 1024));


}




void loop() 
{



  ClickEncoder::Button b = encoder->getButton();


 if (b != ClickEncoder::Open) {


    switch (b) {

      case ClickEncoder::DoubleClicked:



      		switch (uiCurrentMenu)
      		{
      			case 0:
      				uiPreviousMenu = uiCurrentMenu;
      				uiCurrentMenu = 1;
      				clickSound();
      				drawMenu();


      			break;

      		}      

        break;
      case ClickEncoder::Held:


      	if (!bInHeld)
      	{
      		bInHeld = true;

      		switch (uiCurrentMenu)
      		{
      			case 0:
      				uiPreviousMenu = uiCurrentMenu;
      				uiCurrentMenu = 2;
      				//tone(BUZZER,1000,5);
      				clickSound();


      				drawMenu();


      			break;

      			case 1:
      				if (!bFlushInProgress)
      				{
	      				uiCurrentMenu = uiPreviousMenu;
	      				clickSound();
	      				drawMenu();
      				}
      				else
      				{
      					clickSound();
      					clickSound();
      					clickSound();      					      					
      				}


      			break; 

      			case 2:
      				EEPROM_writeAnything(0, bathOptions);
      				uiCurrentMenu = uiPreviousMenu;
      				clickSound();
      				drawMenu();


      			break; 

      			case 3:
      				EEPROM_writeAnything(0, bathOptions);
      				uiCurrentMenu = uiPreviousMenu;
      				clickSound();
      				drawMenu();


      			break;

      			case 4:
      				EEPROM_writeAnything(0, bathOptions);
      				uiCurrentMenu = uiPreviousMenu;
      				clickSound();
      				//iLastMotorSpeed = -1;
      				drawMenu();


      			break;

      		}
      	}

        break;  

 		case ClickEncoder::Released:
 			bInHeld = false;
		
 		break;



      case ClickEncoder::Clicked:


      		switch (uiCurrentMenu)
      		{
       			case 0:

      				if ( bIsRunning )
      				{
      					bIsRunning = false;

						if (bGreenLedOn)
						{
							digitalWrite(SOCKET1GREEN, LOW);
							bGreenLedOn = false;
						}

						if (bRedLedOn)
						{
							digitalWrite(SOCKET1RED, LOW);
							bRedLedOn = false;
						}

						digitalWrite(HEATERRELAY, HIGH); 
						analogWrite(MOTOR, 0);   						
      				}
					else
      				{
      					bIsRunning = true;      	
						motorspeed = map(bathOptions.motorSpeed, 0, 100, 166, 255);
						if (bathOptions.motorSpeed < 16)
						{
							analogWrite(MOTOR, map(20, 0, 100, 166, 255));	
							delay(500);
						}
				        analogWrite(MOTOR, motorspeed); 
      				}

      			clickSound();
      			drawMenu();



      			break;

      			case 1:

      				if ( !bFlushInProgress && !bIsRunning) 
      				{
						analogWrite(LEDSTRIP_BACK, map(10, 0, 100, 0, 255));
						Timer1.pwm(LEDSTRIP_FRONT, map(10, 0, 100, 0, 1024));	
						analogWrite(PUMP, 255); 
						bFlushInProgress = false;
      				}
      				else if (bFlushInProgress)
      					{
							analogWrite(PUMP, 0); 
							bFlushInProgress = false;	
							analogWrite(LEDSTRIP_BACK, map(bathOptions.backLightLevel, 0, 100, 0, 255));
							Timer1.pwm(LEDSTRIP_FRONT, map(bathOptions.frontLightLevel, 0, 100, 0, 1024));		
						}									


				clickSound();
      			drawMenu();
      			break;


      			case 3:

      				if ( uiMenuCurrentSelection == 0 )
      				{
      					uiMenuCurrentSelection = 1;
      					    //myOLED.clrRect(86,38,114,58);


      				}
      				else
      				{

      					uiMenuCurrentSelection = 0;
      				}

				clickSound();
      			drawMenu();
      			break;


      			default:

      			break;


      		}

        break;              
    }
  } 

	Channelstimer.run();
checkRotaryEncoder(); 

}


void drawMenu()
{

  myOLED.clrScr();
  String strtemp;

	switch (uiCurrentMenu)
	{
		case 0:
  			myOLED.setFont(SmallFont);
			myOLED.drawLine(64,0, 64, 50);
			myOLED.drawLine(0,10, 128, 10);
			myOLED.drawLine(0,50, 128, 50);

			strtemp = String("Temp ");
			strtemp.concat(bathOptions.desiredTemp);  
			strtemp.concat(" C");  	

  			myOLED.print(strtemp, 0, 0);

 			strtemp = String("LL ");
			strtemp.concat(bathOptions.frontLightLevel);
			strtemp.concat("/");
			strtemp.concat(bathOptions.backLightLevel);			  

  			myOLED.print(strtemp, 65, 0);

  			myOLED.setFont(BigNumbers);

 			strtemp = String(bathOptions.motorSpeed);
 			if (bathOptions.motorSpeed <10) myOLED.print(strtemp, 25, 18);
 			if (bathOptions.motorSpeed >= 10 && bathOptions.motorSpeed < 100 ) myOLED.print(strtemp, 15, 18);
 			if (bathOptions.motorSpeed == 100) myOLED.print(strtemp, 5, 18);

  			myOLED.setFont(BigNumbers);

 			strtemp = String(round(fCurrentTemp));
   			myOLED.print(strtemp, 80, 18);

  			myOLED.setFont(SmallFont);
			strtemp = String("o");
						
   			myOLED.print(strtemp, 110, 15);

   			if ( bIsRunning )
   			{
 				strtemp = String("Running");
 			}
 			else
 			{
 				strtemp = String("Stopped");

 			}
   			myOLED.print(strtemp, 45, 55);


		break;

		case 1:
 			myOLED.setFont(SmallFont);
			strtemp = String("FLUSH LIQUID");
						
   			myOLED.print(strtemp, 32, 0);
			

 			if (bFlushInProgress)
 			{
 				myOLED.print("STOP ", 45, 25);
 			}
 			else
 			{
 				myOLED.print("START", 45, 25);
 			}



		break;

		case 2:

 			myOLED.setFont(SmallFont);
			strtemp = String("TARGET TEMPERATURE");
   			myOLED.print(strtemp, 12, 0);

  			myOLED.setFont(BigNumbers);

 			strtemp = String(bathOptions.desiredTemp);

 			myOLED.print(strtemp, 55, 18);


  			myOLED.setFont(SmallFont);
			strtemp = String("o");
						
   			myOLED.print(strtemp, 85, 15);

 			myOLED.setFont(SmallFont);
			strtemp = String("current temp: ");
 			strtemp.concat(round(fCurrentTemp));	
			strtemp.concat("C");					
   			myOLED.print(strtemp, 12, 55);

		break;

		case 3:

 			myOLED.setFont(SmallFont);
			strtemp = String("FRONT LIGHT LEVEL");
   			myOLED.print(strtemp, 15, 0);
			strtemp = String("FRONT LIGHT LEVEL");
   			myOLED.print(strtemp, 15, 33);

  			myOLED.setFont(MediumNumbers);

 			strtemp = String(bathOptions.frontLightLevel);  

 			if (bathOptions.frontLightLevel <10) myOLED.print(strtemp, 65, 12);
 			if (bathOptions.frontLightLevel >= 10 && bathOptions.frontLightLevel < 100 ) myOLED.print(strtemp, 55, 12);
 			if (bathOptions.frontLightLevel == 100) myOLED.print(strtemp, 45, 12);

 			strtemp = String(bathOptions.backLightLevel);  

 			if (bathOptions.backLightLevel <10) myOLED.print(strtemp, 65, 44);
 			if (bathOptions.backLightLevel >= 10 && bathOptions.backLightLevel < 100 ) myOLED.print(strtemp, 55, 44);
 			if (bathOptions.backLightLevel == 100) myOLED.print(strtemp, 45, 44);


      				if ( uiMenuCurrentSelection == 0 )
      				{
      					myOLED.drawRect(44,10,90,30);

      				}
      				else
      				{
      					myOLED.drawRect(44,42,90,62);
      				}

	  		


		break;

		case 4:

 			myOLED.setFont(SmallFont);
			strtemp = String("MIXER SPEED");
						
   			myOLED.print(strtemp, 32, 0);

  			myOLED.setFont(BigNumbers);

 			strtemp = String(bathOptions.motorSpeed);

 			if (bathOptions.motorSpeed <10) myOLED.print(strtemp, 65, 25);
 			if (bathOptions.motorSpeed >= 10 && bathOptions.motorSpeed < 100 ) myOLED.print(strtemp, 55, 25);
 			if (bathOptions.motorSpeed == 100) myOLED.print(strtemp, 45, 25);
   				   			

		break;

	}

myOLED.update();


}



void checkTemp()
{

byte value =2;
// Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();

  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)

       delay(conversionTime+5);


 float temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(0) * 10.)) / 10.;

          		#ifdef NDEBUG                
                Serial.print ("Temp1: ");
          	    Serial.println (temperature); 
          	    #endif

 	if (temperature == -127.00 || temperature == 85.00 ) 
 	{
 		temperature = fCurrentTemp / 2;
	}

		fCurrentTemp = temperature;


       temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(1) * 10.)) / 10.;

          		#ifdef NDEBUG                
                Serial.print ("Temp2: ");
          	    Serial.println (temperature); 
          	    #endif    

 	if (temperature == -127.00 || temperature == 85.00 ) 
 	{
		 temperature = fCurrentTemp;
	}


	//fCurrentTemp = (fCurrentTemp + temperature)/2 - 1.90;
	
	fCurrentTemp = (fCurrentTemp + temperature)/2 + 5.40;

if (uiCurrentMenu == 0)
{
	drawMenu();
}


	if ( bIsRunning )
	{
		if ( ( fCurrentTemp  ) <= bathOptions.desiredTemp + 1 )
		{
			if (bGreenLedOn)
			{
				digitalWrite(SOCKET1GREEN, LOW);
				bGreenLedOn = false;
			}

			if (!bRedLedOn)
			{
				digitalWrite(SOCKET1RED, HIGH);
				bRedLedOn = true;
				digitalWrite(HEATERRELAY, LOW); 
			}


		}
		else
		{
			if (bRedLedOn)
			{
				digitalWrite(SOCKET1RED, LOW);
				bRedLedOn = false;

			}

			if (!bGreenLedOn)
			{
				digitalWrite(SOCKET1GREEN, HIGH);
				bGreenLedOn = true;
				digitalWrite(HEATERRELAY, HIGH); 
				beepComplete(5000);				


			}

		}


			//if (iLastMotorSpeed != motorspeed)
			//{

			//	iLastMotorSpeed = motorspeed;
				//analogWrite(MOTOR, motorspeed); 

			//}

	}


}



void checkRotaryEncoder() 
{
	//delay(10);



   encoderValue += encoder->getValue();

   int16_t val = 0;
   int16_t itemVal = 0;
   String sitemVal;
  


  if (encoderValue != lastencoderValue)
  {


        val = encoderValue - lastencoderValue;

        switch (uiCurrentMenu){

		case 0:

			if ( val > 0)
			{

				//display motor settings menu
				uiPreviousMenu = uiCurrentMenu;
				uiCurrentMenu = 4;
				//drawMenu();
			}

			if ( val < 0)
			{

				//display motor settings menu
				uiPreviousMenu = uiCurrentMenu;
				uiCurrentMenu = 3;
				//drawMenu();
			}

		break;


		case 1:

		break;

		case 2:

					bathOptions.desiredTemp = bathOptions.desiredTemp + val;
					if ( bathOptions.desiredTemp < 20) bathOptions.desiredTemp = 20;
					if ( bathOptions.desiredTemp > 70) bathOptions.desiredTemp = 70;	
					
					//drawMenu();
		break;

		case 3:

			switch (uiMenuCurrentSelection)
			{
				case 0:
					bathOptions.frontLightLevel = bathOptions.frontLightLevel + val;
					if ( bathOptions.frontLightLevel < 0) bathOptions.frontLightLevel = 0;
					if ( bathOptions.frontLightLevel > 100) bathOptions.frontLightLevel = 100;	
					//analogWrite(LEDSTRIP_FRONT, map(bathOptions.frontLightLevel, 0, 100, 0, 255));	
					Timer1.setPwmDuty(LEDSTRIP_FRONT,  map(bathOptions.frontLightLevel, 0, 100, 0, 1024));			
				break;

				case 1:
					bathOptions.backLightLevel = bathOptions.backLightLevel + val;
					if ( bathOptions.backLightLevel < 0) bathOptions.backLightLevel = 0;
					if ( bathOptions.backLightLevel > 100) bathOptions.backLightLevel = 100;	
					analogWrite(LEDSTRIP_BACK, map(bathOptions.backLightLevel, 0, 100, 0, 255));	

				break;



			}

				//drawMenu();
		break;

		case 4:

			bathOptions.motorSpeed = bathOptions.motorSpeed + val;
			if ( bathOptions.motorSpeed < 0) bathOptions.motorSpeed = 0;
			if ( bathOptions.motorSpeed > 100) bathOptions.motorSpeed = 100;	
			motorspeed = map(bathOptions.motorSpeed, 0, 100, 166, 255);
			if (bIsRunning)
			{

				if (bathOptions.motorSpeed < 16 && val > 0)
				{
					analogWrite(MOTOR, map(20, 0, 100, 166, 255));	
					delay(500);
				}
				analogWrite(MOTOR, motorspeed);	
			}
			
			//analogWrite

			//drawMenu();
		break;		



								}




lastencoderValue = encoderValue;
drawMenu();

  }


}//checkRotaryEncoder() 



void beepComplete(unsigned char delayms){
  analogWrite(BUZZER, 20);      // значение должно находится между 0 и 255
                           // поэкспериментируйте для получения хорошего тона
  delay(3000);          // пауза delayms мс
  analogWrite(BUZZER, 0);       // 0 - выключаем пьезо
  //delay(delayms);          // пауза delayms мс   
}


void clickSound()
{

  analogWrite(BUZZER, 30);      // значение должно находится между 0 и 255
                           // поэкспериментируйте для получения хорошего тона
  delay(600);          // пауза delayms мс
  analogWrite(BUZZER, 0);       // 0 - выключаем пьезо

}


