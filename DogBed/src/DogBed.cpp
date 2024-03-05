/* 
 * Project DogBed
 * Author: David Barbour
 * Date: 3/1/24
 */

#include "Particle.h"
#include "IoTClassroom_CNM.h"
#include "neopixel.h"
#include "Colors.h"
#include "math.h"
#include "IoTTimer.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_BME280.h"
#include "Graphic.h"
#include "Button.h"
#include "encoder.h"
#include "wemo.h"

//encoder setup
const int PINA = D8;
const int PINB = D9;
const int BUTTONPIN = D3;
Encoder myEnc (PINA , PINB );
Button encButton(BUTTONPIN,true);

//joystick setup
const int joyHorz = A1;
const int joyVert = A2;
const int joySwitch = D6;
Button joyButton(joySwitch,true);
int newVer;int newHor;
int oldVer;int oldHor;

//temperature setup
Adafruit_BME280 bme;
bool status;

//neo pixel setup
const int PIXELCOUNT = 20;
Adafruit_NeoPixel pixel ( PIXELCOUNT , SPI1 , WS2812B );

//display setup
Adafruit_SSD1306 display(-1);
bool showDisplay=false;

//temperature reading
const int sensorWaitTime=10;
float currentTemp;
IoTTimer waitTimer;
int waitedTime=0;

//Motion detector
const int DETECTPIN=D8;
bool motionDetected=false;

//wemo
int wemoCool=4; 
int wemoHeat=2; 

//hue light bulb
const int BULB=3;

//debugging button
Button debugButton(D4,false);

void PixelFill(int startPixel, int endPixel, int theColor);
void setPixelDisplay(int theState);
void programLogic();

int encPosition;
int lastEncPosition;

int applicationState=0;
int prevApplicationState;
int setupState=0;
int subSetupState=0;

bool setVertDown = false;
bool setVertUp = false;
bool setHorRight = false;
bool setHorLeft = false;

int coolingTemp = 80;
int heatingTemp = 75;

SYSTEM_MODE(MANUAL);

void setup() {

    //turn on wi fi
    WiFi.on();
    WiFi.clearCredentials();
    WiFi.setCredentials("IoTNetwork");
    
    WiFi.connect();
    while(WiFi.connecting()) {
    Serial.printf(".");}

    //start serial monitor
    Serial.begin(9600);
    waitFor (Serial.isConnected,10000);

    //start the display
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.display();
    showDisplay=true;

    //start the neo pixels
    pixel.begin();
    pixel.setBrightness(22);
    pixel.clear();
    pixel.show();

    //set up motion detector
    pinMode(DETECTPIN,INPUT);

    //start bme temp guage
    status=bme.begin (0x76);
    if (status==false ) {
        Serial.printf (" BME280 at address %c failed to start ", 0x76 );}

}


void loop() {

    // bool gotMotion = digitalRead(DETECTPIN);
    // if(gotMotion==true)
    // {
    //     Serial.printf("Motion %i \r",gotMotion);
    // }

    programLogic();

 
}

void programLogic()
{
    
    //There are multiple states the application can be in
    // 0 - off
    // 1 - setup mode
    // 2 - waiting to cool
    // 3 - cooling
    // 4 - waiting to head
    // 5 - heating

    //when you're in running mode
    //if the temperature goes above the cooling temperature turn on the fan
    currentTemp = (bme.readTemperature ()*9/5)+32.0; // deg F

    //this is for debugging only
    if (debugButton.isClicked()==true)
    {
        Serial.printf("Start %i, Temp %0.1f%cF\n\n",0,currentTemp,248);
    }

    prevApplicationState = applicationState;
    if(currentTemp>coolingTemp && applicationState>1)
    {
        if (motionDetected==true)
        {
            applicationState=3;
        }
        else
        {
            applicationState=2;
        }
    }

    //when you're in running mode
    //if the temperature goes below the heating temp, turn on 
    if(currentTemp<heatingTemp && applicationState>1)
    {
        if (motionDetected==true)
        {
            applicationState=5;
        }
        else
        {
            applicationState=4;
        }
    }

    //only update the display if something changes
    if(applicationState!=prevApplicationState){showDisplay=true;}


    switch (applicationState)
    {
    case 0: 
        //this is off

        //turn off the neopixels
        setPixelDisplay(0);

        //turn off the display
        if(showDisplay)
        {
            display.clearDisplay();
            display.display();
            showDisplay=false;

            //turn off the heater
            wemoWrite(wemoHeat,LOW);

            //turn off the fan
            wemoWrite(wemoCool,LOW);

        }
          
        //if user clicks on button, start  running process
        if (joyButton.isPressed()==true)
        {
            applicationState=1;
            showDisplay=true;
        }

        break;
    
    case 1:  
        //setup mode

        setPixelDisplay(1);
        newVer = analogRead(joyVert);
        newHor = analogRead(joyHorz);
        switch (setupState)
        {
        case 0: //on off screen
            
            if(showDisplay)
            {
                display.clearDisplay();
                display.drawBitmap(20, 0,graphic_onoff, 80,68, 1);
                display.display();
                showDisplay=false;

                //turn off the heater
                wemoWrite(wemoHeat,LOW);

                //turn off the fan
                wemoWrite(wemoCool,LOW);

            }

            //DOWN set the application state back to off
            if(newVer>3500){setVertDown=true;}
            if(newVer<3500 && setVertDown==true)
            {
                applicationState = 0;
                setVertDown = false;
                showDisplay=true;
            }

            //UP turn it on
            if(newVer<1500){setVertUp=true;}
            if(newVer>1500 && setVertUp==true)
            {
                applicationState = 2;
                setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
                showDisplay=true;
            }


            //RIGHT, go to the cooling teperature screen
            if(newHor>3500){setHorRight=true;}
            if(newHor<3500 && setHorRight==true)
            {
                setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
                setupState = 1;
                showDisplay=true;
            }
            break;

        case 1:
            //set cooling termperature

            //up down arrow
            if (showDisplay)
            {
                display.clearDisplay();
                display.drawBitmap(0, 9,graphic_updown,16,46, 1);
                display.setTextColor(WHITE);
                display.setTextSize(1);
                display.setCursor(20,0);
                display.printf("Set cooling temp");

                display.setTextSize(2);
                display.setCursor(60,25);
                display.printf("%i",coolingTemp);
                display.display();
                showDisplay=false;
            }
            
            //RIGHT, go to the heating teperature screen
            if(newHor>3500){setHorRight=true;}
            if(newHor<3500 && setHorRight==true)
            {
                setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
                setupState = 2;
                showDisplay=true;
            }

            //LEFT, go to on off screen
            if(newHor<1000){setHorLeft=true;}
            if(newHor>1000 && setHorLeft==true)
            {
                setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
                setupState = 0;
                showDisplay=true;
            }

            //UP, increase the cooling termperature
            if(newVer>3000){setVertUp=true;}
            if(newVer<3000 && setVertUp==true)
            {
                setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
                coolingTemp--;

                //keep the temps from overlapping
                if(heatingTemp>=coolingTemp){heatingTemp=coolingTemp-1;}

                showDisplay=true;
            }

            //DOWN, decrease the cooling termperature
            if(newVer<1000){setVertDown=true;}
            if(newVer>1000 && setVertDown==true)
            {
                setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
                coolingTemp++;

                //keep the temps from overlapping
                if(heatingTemp>=coolingTemp){heatingTemp=coolingTemp-1;}

                showDisplay=true;
            }

            break;
        
        case 2:
            //set heting termperature
            if(showDisplay)
            {
                display.clearDisplay();
                display.drawBitmap(0, 9,graphic_updown,16,46, 1);
                display.setTextColor(WHITE);
                display.setTextSize(1);
                display.setCursor(20,0);
                display.printf("Set heating temp");

                display.setTextSize(2);
                display.setCursor(60,25);
                display.printf("%i",heatingTemp);
                display.display();
                showDisplay=false;
            }

            //LEFT, go to on off screen
            if(newHor<1000){setHorLeft=true;}
            if(newHor>1000 && setHorLeft==true)
            {
                setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
                setupState = 1;
                showDisplay=true;
            }

            //UP, increase the heating termperature
            if(newVer>3000){setVertUp=true;}
            if(newVer<3000 && setVertUp==true)
            {
                setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
                heatingTemp--;

                //keep the temps from overlapping
                if(heatingTemp>=coolingTemp){coolingTemp=heatingTemp+1;}

                showDisplay=true;
            }

            //DOWN, increase the heating termperature
            if(newVer<1000){setVertDown=true;}
            if(newVer>1000 && setVertDown==true)
            {
                setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
                heatingTemp++;

                //keep the temps from overlapping
                if(heatingTemp>=coolingTemp){coolingTemp=heatingTemp+1;}

                showDisplay=true;
            }


            break;
        
        default:
            break;
        }
        break;
    
    case 2:  //wating to cool
        setPixelDisplay(2);
        
        //tell the user, it's waiting to cool
        if(showDisplay)
        {
            waitedTime = 0;
            waitTimer.startTimer(1000);   
            motionDetected=0;

            display.clearDisplay();
            display.setTextColor(WHITE);
            display.setTextSize(1);
            display.setCursor(20,0);
            display.printf("Waiting to cool");
            display.display();
            showDisplay=false;
        }

        //do a countdown, so the motion sensor doesn't start reading
        //for 10 seconds
        if (waitedTime < sensorWaitTime)
        {
            if (waitTimer.isTimerReady()==true)
            {
                display.fillRect(50,20, 70,30,BLACK);
                display.setTextColor(WHITE);
                display.setTextSize(2);
                display.setCursor(50,20);
                display.printf("%i",sensorWaitTime-waitedTime);
                display.display();
                waitedTime++;
                waitTimer.startTimer(1000); 
            }
        }
        else
        {
            //read the motion sensor
            display.fillRect(50,20, 70,30,BLACK);
            display.display();
            motionDetected = digitalRead(DETECTPIN);
        }

        break;


    case 3:  //Cooling
        setPixelDisplay(3);
        newHor = analogRead(joyHorz);

         //this is for debugging only
        // if (debugButton.isClicked()==true)
        // {
        //     Serial.printf("Temp %i, temp %i\r",applicationState,currentTemp);
        // }

        //tell the user, it's cooling
        if(showDisplay)
        {
            display.clearDisplay();
            display.setTextColor(WHITE);
            display.setTextSize(2);
            display.setCursor(20,0);
            display.printf("Cooling");
            display.display();
            showDisplay=false;

            //turn on fan here
            wemoWrite(wemoCool,HIGH);
        }

        //LEFT, go back to setup screen
        if(newHor<1000){setHorLeft=true;}
        if(newHor>1000 && setHorLeft==true)
        {
            setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
            applicationState = 1;
            setupState=0;
            waitedTime = 0;
            motionDetected=false;
            showDisplay=true;
        }

        break;

    case 4:  //Wating to heat
        setPixelDisplay(4);

        //tell the user, it's cooling
        if(showDisplay)
        {   waitedTime = 0;
            waitTimer.startTimer(1000); 
            motionDetected=0;

            display.clearDisplay();
            display.setTextColor(WHITE);
            display.setTextSize(1);
            display.setCursor(20,0);
            display.printf("Wating to heat");
            display.display();
            showDisplay=false;
        }

        //do a countdown, so the motion sensor doesn't 
        //start reading for 10 seconds
        if (waitedTime < sensorWaitTime)
        {
            if (waitTimer.isTimerReady()==true)
            {
                display.fillRect(50,20, 70,30,BLACK);
                display.setTextColor(WHITE);
                display.setTextSize(2);
                display.setCursor(50,20);
                display.printf("%i",sensorWaitTime-waitedTime);
                display.display();
                waitedTime++;
                waitTimer.startTimer(1000); 
            }
        }
        else
        {
            //read the motion sensor
            display.fillRect(50,20, 70,30,BLACK);
            display.display();
            motionDetected = digitalRead(DETECTPIN);
        }

        break;

    case 5:  //heating
        setPixelDisplay(5);
        newHor = analogRead(joyHorz);
        
        //this is for debugging only
        // if (debugButton.isClicked()==true)
        // {
        //     Serial.printf("Temp %i, temp %i\r",applicationState,currentTemp);
        // }

        //tell the user, it's cooling
        if(showDisplay)
        {
            display.clearDisplay();
            display.setTextColor(WHITE);
            display.setTextSize(2);
            display.setCursor(20,0);
            display.printf("Heating");
            display.display();
            showDisplay=false;

            //turn on heater here
            wemoWrite(wemoHeat,HIGH);
        }

        //LEFT, go back to setup screen
        if(newHor<1000){setHorLeft=true;}
        if(newHor>1000 && setHorLeft==true)
        {
            setHorRight = false;setHorLeft= false;setVertDown=false;setVertUp=false;
            applicationState = 1;
            setupState=0;
            waitedTime = 0;
            motionDetected=false;
            showDisplay=true;
        }

        break;

    default:
        break;
    }


}


void setPixelDisplay(int theState)
{
    static int lastSwitch;
    static bool onOff;
    int brightNess;

    switch (theState)
    {
        case 0:
            //bed is off
            pixel.clear();
            pixel.show();
            setHue(BULB,false);
            break;

        case 1:  
            //setup mode  (blinking white every second)
            if(millis()-lastSwitch>500)
            {
                pixel.setBrightness(10);
                onOff = !onOff;
                if (onOff==true)
                {
                    PixelFill(0,PIXELCOUNT-1,white);
                    setHue(BULB,true,HueOrange,200,255);
                }
                else
                {
                    pixel.clear();
                    setHue(BULB,false);
                }
                pixel.show();
                lastSwitch = millis();
            }
            
            break;

        case 2: 
            // bed is ready to be cold (breath blue)
            PixelFill(0,PIXELCOUNT-1,blue);
            brightNess = 7 * sin(2.0*M_PI*(2.0/5.0)*millis()/1000.0)+ 10;
            pixel.setBrightness(brightNess);
            pixel.show();
            setHue(BULB,true,HueBlue,brightNess,255);
            break;

        case 3:
            //bed is in cold mode (steady blue)
            PixelFill(0,PIXELCOUNT-1,blue);
            pixel.setBrightness(40);
            pixel.show();
            setHue(BULB,true,HueBlue,100,255);
            break;

        case 4:
            // bed is ready to be hot (breathing yellow)
            PixelFill(0,PIXELCOUNT-1,yellow);
            brightNess = 7 * sin(2.0*M_PI*(2.0/5.0)*millis()/1000.0)+ 10;
            pixel.setBrightness(brightNess);
            pixel.show();
            setHue(BULB,true,HueYellow,brightNess,255);
            break;

        case 5:
            //bed is in heat mode (steady yellow)
            PixelFill(0,PIXELCOUNT-1,yellow);
            pixel.setBrightness(40);
            pixel.show();
            setHue(BULB,true,HueYellow,100,255);
            break;

        default:
            //bed is off
            pixel.clear();
            pixel.show();
            setHue(BULB,false);
            break;

    }

}

// void pixelState(int theState)
// {
//     static int lastSwitch;
//     static bool onOff;
//     int brightNess;

//     switch (theState)
//     {
//         case 0:  
//             //setup mode  (blinking white every second)
//             if(millis()-lastSwitch>500)
//             {
//                 pixel.setBrightness(10);
//                 onOff = !onOff;
//                 if (onOff==true)
//                 {
//                     PixelFill(0,PIXELCOUNT-1,orange);
//                 }
//                 else
//                 {
//                     pixel.clear();
//                 }
//                 pixel.show();
//                 lastSwitch = millis();
//             }
            
//             break;

//         case 1: 
//             // bed is ready to be cold (breath blue)
//             PixelFill(0,PIXELCOUNT-1,blue);
//             int brightNess = 7 * sin(2.0*M_PI*(2.0/5.0)*millis()/1000.0)+ 10;
//             pixel.setBrightness(brightNess);
//             pixel.show();
//             break;

//         case 2:
//             //bedi is in cold mode (steady blue)
//             PixelFill(0,PIXELCOUNT-1,blue);
//             pixel.setBrightness(20);
//             pixel.show();
//             break;
//         // case 2:
//         //     PixelFill(0,PIXELCOUNT-1,blue);
//         //     pixel.setBrightness(10);
//         //     pixel.show();
//         //     break;
        
//         // case 3:
//         //     PixelFill(0,PIXELCOUNT-1,yellow);
//         //     brightNess = 7 * sin(2.0*M_PI*(2.0/5.0)*millis()/1000.0)+ 10;
//         //     pixel.setBrightness(brightNess);
//         //     pixel.show();
//         //     break;

//         // case 4:
//         //     PixelFill(0,PIXELCOUNT-1,yellow);
//         //     pixel.setBrightness(10);
//         //     pixel.show();
//         //     break;

//         // default:
//         //     pixel.clear();
//         //     pixel.show();
//         //     break;
//     }
// }


void PixelFill(int startPixel, int endPixel, int theColor)
{
  int theLoop;
  for (theLoop=startPixel;theLoop<=endPixel;theLoop++)
  {
    pixel.setPixelColor(theLoop,theColor);
  }
}
