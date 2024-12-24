#include <SPI.h>
#include <Servo.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

#include "main.h"
#include "user_access.h"
#include "card_control.h"
#include "secrets.h"

// secret configurations (wifi + firebase)
#define WIFI_SSID      SECRET_WIFI_SSID
#define WIFI_PASS      SECRET_WIFI_PASS
#define FIREBASE_HOST  SECRET_FIREBASE_HOST
#define FIREBASE_AUTH  SECRET_FIREBASE_AUTH

// UTC Offset for Vietnam is +7h which means 25200 seconds
#define UTC_OFFSET_VIETNAM_IN_SEC 25200 

unsigned int cardCount = 0;    // Holds index of the card if find in Database
bool readSuccess = false;      // Boolean variable to check whether card is readed Successfully or not
String strUID;                 // Holds Card UID in String format 

// UDP Client Instance for NTP Server
WiFiUDP udpClient;

// NTP Client Instance for connecting to NTP Server
NTPClient timeClient(udpClient, "pool.ntp.org", UTC_OFFSET_VIETNAM_IN_SEC);

// Servo Motor Instance
Servo myServo;                       

// MFRC522 Reader Instance
MFRC522 Reader(MFRC522_SLAVE_SELECT_PIN,MFRC522_RESET_PIN);

// LCD1602 Instance
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

// Firebase Instance
FirebaseData firebaseData; //Declare the Firebase Data object in the global scope
FirebaseAuth auth; //Define the FirebaseAuth data for authentication data
FirebaseConfig config; //Define the FirebaseConfig data for config data
String path = "/";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
 
  setupWifi();
  mainInit();

  Serial.println("|************ IoT Smart Park System ************|");
  Serial.println();

  // Desciption of our Smart Park System 
  Serial.println("This is RFID Out Block");
  Serial.println();
  lcd.setCursor(3, 0);
  lcd.print("IoT Smart");
  lcd.setCursor(1, 1);
  lcd.print("Parking System"); 
}

void loop() {
  do {
    readSuccess = getCardID();
    checkAndOpenOutDoor();
    delay(200);
  }while(!readSuccess); // Block till Card read

  // If scanned card exists then check for the balance and grant the extry     
  if(isCardFind(strUID)==true) {
    String cardType;
    bool isVehicleIn;
    if (Firebase.getString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/CardInformation/CardType")) {
      cardType = firebaseData.stringData();
    }
    if (cardType == "Guest") {
      if(Firebase.getBool(firebaseData, path + ("/RFID/Cards/Card" + String(cardCount) + "/IsVehicleIn"))){
        isVehicleIn = firebaseData.boolData();
        if(isVehicleIn){
          // Thank you for parking
          Serial.println("Thank you for parking");
          // Cho bảo vệ xem
          Firebase.setString(firebaseData, path + "/RFID/ExitStatus", "Card " + String(cardCount) + " -" + " Thank you for parking");
          // Cho khách xem
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("Thank You");
          lcd.setCursor(2, 1);
          lcd.print("For Parking!");
          grantExit();
        }
        else{
          // Card has not entered
          Serial.println("Card has not entered");
          // Cho bảo vệ xem
          Firebase.setString(firebaseData, path + "/RFID/ExitStatus", "Card " + String(cardCount) + " -" + " Card has not entered");
          // Cho khách xem
          lcd.clear();
          lcd.setCursor(4, 0);
          lcd.print("Card Has");
          lcd.setCursor(2, 1);
          lcd.print("Not Entered!");
          accessDenied();
        } 
      }
    }
    if (cardType == "Member") {
      int balance;
      if(Firebase.getInt(firebaseData, path + ("/RFID/Cards/Card" + String(cardCount) + "/Balance"))) {
        balance = firebaseData.intData();
        if(balance >= 500){
          if(Firebase.getBool(firebaseData, path + ("/RFID/Cards/Card" + String(cardCount) + "/IsVehicleIn"))){
            isVehicleIn = firebaseData.boolData();
            if(isVehicleIn){
              grantExit();
            }
            else{
              // Card has not entered
              Serial.println("Card has not entered");
              // Cho bảo vệ xem
              Firebase.setString(firebaseData, path + "/RFID/ExitStatus", "Card " + String(cardCount) + " -" + " Card has not entered");
              // Cho khách xem
              lcd.clear();
              lcd.setCursor(4, 0);
              lcd.print("Card Has");
              lcd.setCursor(2, 1);
              lcd.print("Not Entered!");
              accessDenied();
            }  
          }
        }
        else {
          // Card doesn't have sufficient Balance
          Serial.println("Card doesn't have sufficient balance. Please recharge it");
          // Cho bảo vệ xem
          Firebase.setString(firebaseData, path + "/RFID/ExitStatus", "Card " + String(cardCount) + " -" + " Card doesn't have sufficient balance. Please recharge it");
          // Cho khách xem
          lcd.clear();
          lcd.setCursor(2, 0);
          lcd.print("Card Balance");
          lcd.setCursor(2, 1);
          lcd.print("Insufficient");
          accessDenied();
        }
      }
    }
  } 
  else {
    // Invalid Card
    Serial.println("Invalid Card");
    // Cho bảo vệ xem
    Firebase.setString(firebaseData, path + "/RFID/ExitStatus", "Invalid Card");
    // Cho khách xem
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Invalid Card");
    accessDenied();
  }
} 


// function to setup the wifi with predefined credentials
void setupWifi() {
  // set the wifi to station mode to connect to a access point
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID , WIFI_PASS);

  Serial.println();
  Serial.print("Connecting to " + String(WIFI_SSID));

  // wait till chip is being connected to wifi  (Blocking Mode)
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
  }

  // now it is connected to the access point just print the ip assigned to chip
  Serial.println();
  Serial.print("Connected to " + String(WIFI_SSID) + ", Got IP address : ");
  Serial.println(WiFi.localIP());
}

// function to initiaize the main app
void mainInit()
{
  //
  // MFRC522 Reader is connected via SPI
  // initialize the MFRC522 Card Reader
  //
  SPI.begin();                
  Reader.PCD_Init();     
  lcd.init();
  lcd.backlight();

  // setup the pins (servo + led)
  myServo.attach(SERVO_PIN, 500, 2400);       
  pinMode(RED_LED, OUTPUT);

  // default configurations (servo + led)
  myServo.write(0); 
  digitalWrite(RED_LED, LOW);

  // sync the current time from SNTP Server
  timeClient.begin();

  // Atlast. Connect to Firebase using Host and Authentication key
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}