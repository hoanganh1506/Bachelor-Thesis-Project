#pragma once //đảm bảo rằng tệp tiêu đề chỉ được bao gồm một lần duy nhất trong một quá trình biên dịch, dù cho tệp tiêu đề đó có được bao gồm nhiều lần trong các tệp khác nhau

#include <MFRC522.h>
#include <Servo.h>
#include <NTPClient.h>
#include <FirebaseESP8266.h>
#include <LiquidCrystal_I2C.h>
#include "pins_config.h"

void setupWifi();
void appMainInit();

extern String strUID;
extern unsigned int cardCount;

extern Servo myServo;
extern MFRC522 Reader;
extern NTPClient timeClient;
extern LiquidCrystal_I2C lcd; 

extern FirebaseData firebaseData; //Declare the Firebase Data object in the global scope
extern FirebaseAuth auth; //Define the FirebaseAuth data for authentication data
extern FirebaseConfig config; //Define the FirebaseConfig data for config data
extern String path;