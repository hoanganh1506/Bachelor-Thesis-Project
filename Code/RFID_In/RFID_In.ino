#include <SPI.h>
#include <Servo.h>
#include <MFRC522.h>
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
bool updateMode  = false;      // Boolean variable to switch between Update Mode & Regular Mode
String strUID;                 // Holds Card UID in String format 

// UDP Client Instance for NTP Server
WiFiUDP udpClient;

// NTP Client Instance for connecting to NTP Server
NTPClient timeClient(udpClient, "pool.ntp.org", UTC_OFFSET_VIETNAM_IN_SEC);

// Servo Motor Instance
Servo myServo;                       

// MFRC522 Reader Instance
MFRC522 Reader(MFRC522_SLAVE_SELECT_PIN,MFRC522_RESET_PIN);

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
  cycleLed(2, 200);

  Serial.println("|************ IoT Smart Park System ************|");
  Serial.println();

  // Desciption of our Smart Park System 
  Serial.println("It works in two modes i.e (Regular Mode[Blue LED On] and Update Mode[Blue LED Off])");
  Serial.println("1) Regular Mode : If Card is Valid then Grant the Entry or Exit else deny the Access");
  Serial.println("2) Update Mode  : ADD the Unknown Cards to Database and remove the known Cards from Database"); 
  Serial.println("Default Mode is Regular Mode. If you want to go to Update Mode then just scan the Master Card");
  Serial.println("If you want to go back to the Regular Mode then just scan the Master Card again"); 
  Serial.println("Thank You, Happy Usage..."); 
  Serial.println();

  // Check if Master Card is defined of not (Do not further if Master is not defined)   
  bool isMasterDefined;
  if(Firebase.getBool(firebaseData, "/RFID/IsMasterDefined")) {
    // Nếu tồn tại trường "IsMasterDefined" thì lấy giá trị lưu vào isMasterDefined
    isMasterDefined = firebaseData.boolData();
  }else{
    // Nếu trường không tồn tại, đặt giá trị mặc định cho isMasterDefined là false
    isMasterDefined = false;
  }
  // Kiểm tra giá trị của isMasterDefined
  if (!isMasterDefined) {
  // if(!Firebase.getBool(firebaseData, path + "/IsMasterDefined")) {
    Serial.println("There is no Master");
    Serial.println("Please Scan a Card to define Master Card...");
    
    do {       
      readSuccess = getCardID();

      // Visualize the no master status by blinking Blue LED super quickly      
      digitalWrite(BLUE_LED, HIGH);  
      delay(250);
      digitalWrite(BLUE_LED, LOW);
      delay(250);

    } while(!readSuccess); // Block till Card read  

    // Card readed successfully, So make it Master Card and push all the neccessary data  
    // Basically Creating the card template over firebase for further cards entry and exit                                                   
    Firebase.setInt(firebaseData, path + "/RFID/TotalCards", 0);                   // TotalCards to 0
    Firebase.setString(firebaseData, path + "/RFID/MasterCardUID", strUID);        // MasterCardUID to Scanned Card UID
    Firebase.setBool(firebaseData, path + "/RFID/IsMasterDefined", true);          // IsMasterDefined to True
    // Để mở cửa vào và ra bằng nút nhấn
    Firebase.setBool(firebaseData, path + "/RFID/OpenInRequest", false);
    Firebase.setBool(firebaseData, path + "/RFID/OpenOutRequest", false);
    // Card Template
    Firebase.setInt(firebaseData, path + "/RFID/CardTemplate/Balance", 0);        
    Firebase.setString(firebaseData, path + "/RFID/CardTemplate/UID", "XXXXXXXX");
    Firebase.setString(firebaseData, path + "/RFID/CardTemplate/CardInformation/CardType", "Guest");      
    Firebase.setString(firebaseData, path + "/RFID/CardTemplate/CardInformation/Plate", "Null"); 
    Firebase.setString(firebaseData, path + "/RFID/CardTemplate/CardInformation/BrandOfVehicle", "Null");
    Firebase.setString(firebaseData, path + "/RFID/CardTemplate/CardInformation/TypeOfVehicle", "Null");                                  
    Firebase.setString(firebaseData, path + "/RFID/CardTemplate/Entry/Date", "YYYY-MM-DD");                                                       
    Firebase.setString(firebaseData, path + "/RFID/CardTemplate/Entry/Time", "HH:MM:SS");
    Firebase.setString(firebaseData, path + "/RFID/CardTemplate/Exit/Date", "YYYY-MM-DD");
    Firebase.setString(firebaseData, path + "/RFID/CardTemplate/Exit/Time", "HH:MM:SS");
    Firebase.setBool(firebaseData, path + "/RFID/CardTemplate/IsVehicleIn", false);
        
    Serial.println("Welcome Master");
    Serial.println("Initialization Completed !");
    Serial.println();
    
    // Visualize the Initialization by cycle the LED's Blinks
    cycleLed(3, 200);
  }
  else{
    cycleLed(3, 200);
    Serial.println("Welcome Master");
    Serial.println("Please swipe your card to enter update mode, Master!");
    Serial.println();
  }
}

void loop() {
  //
  // Main loop first will wait for any card to be scanned
  // Then based on card (Master Card or Normal Card) it will enter into desired mode 
  // Then based on mode the code will do the needfull :)
  //

  do {
    readSuccess = getCardID();
    checkAndOpenInDoor();
    delay(200);
  }while(!readSuccess); // Block till Card read

  //
  // Update Mode : You can manage the cards.
  // Regular Mode : You can mark the entry/exit in the parikng.
  //

  if(updateMode) {     
    // Visualize Update Mode by Turn OFF Blue LED
    digitalWrite(BLUE_LED, LOW);

    if(isMaster(strUID)) {
      // Cho admin biết
      Firebase.setString(firebaseData, path + "/RFID/UpdateModeStatus", "Exiting Update Mode");   
      // Exit the Update Mode if Master Card is Scanned again                  
      Serial.println("Hey Master!");
      Serial.println("Exiting Update Mode...");
      Serial.println();
      updateMode = false;
    }
    else {
      if(isCardFind(strUID)) {
        // If scanned card is there in database then remove it           
        Serial.println("I know this card , removing...");

        // Visualize it by making Red LED On till the Card is removed from database  
        digitalWrite(RED_LED, HIGH);   
        removeCard(strUID);
        Serial.println("Card Removed");
        Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
        Serial.println("Scan Master Card again to exit Update mode");
        Serial.println();
        digitalWrite(RED_LED, LOW);
      }
      else {
        // If scanned card is not in the databse then add it     
        Serial.println("I don't know this card , adding..."); 

        // Visualize it by making Green LED On till the card is added to database                     
        digitalWrite(GREEN_LED, HIGH);              
        addCard(strUID);
        Serial.println("Card Added");
        Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
        Serial.println("Scan Master Card again to exit Update mode");
        Serial.println();
        digitalWrite(GREEN_LED, LOW);
      }
    }
  }
  else {
    // Visualize Regular Mode by Turn ON Blue LED        
    digitalWrite(BLUE_LED,  HIGH);

    if(isMaster(strUID)) {
      Firebase.setString(firebaseData, path + "/RFID/UpdateModeStatus", "Entering Update Mode"); 
      // If scanned card is Master Card, Step into Update Mode 
      Serial.println("Hey Master!");
      Serial.println("Entering Update Mode...");          
      Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
      Serial.println("Scan Master card again to exit Update Mode");
      Serial.println();
      updateMode = true;
    }
    else {
      // If scanned card exists then check for the balance and grant the extry     
      if(isCardFind(strUID)==true) {

        //
        // Allow only if card have some threshhold (Above 500 VND)
        // We will let the card be in negative if in case the person spends more time.
        //
        String cardType;
        bool isVehicleIn;
        if (Firebase.getString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/CardInformation/CardType")) {
          cardType = firebaseData.stringData();
        }
        if (cardType == "Guest") {
          if(Firebase.getBool(firebaseData, path + ("/RFID/Cards/Card" + String(cardCount) + "/IsVehicleIn"))){
            isVehicleIn = firebaseData.boolData();
            if(!isVehicleIn){
              // Welcome! Enjoy your parking
              Serial.println("Welcome! Enjoy your parking");
              // Cho bảo vệ xem
              Firebase.setString(firebaseData, path + "/RFID/EntryStatus", "Card " + String(cardCount) + " -" + " Welcome. Enjoy your parking");
              grantEntry();
            }
            else{
              // Card has not exited
              Serial.println("Card has not exited");
              // Cho bảo vệ xem
              Firebase.setString(firebaseData, path + "/RFID/EntryStatus","Card " + String(cardCount) + " -" + " Card has not exited");
              // Cho khách xem
              digitalWrite(RED_LED, HIGH);
              delay(1000);
              digitalWrite(RED_LED, LOW);
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
                if(!isVehicleIn){
                  // Welcome! Enjoy your parking
                  Serial.println("Welcome! Enjoy your parking");
                  // Cho bảo vệ xem
                  Firebase.setString(firebaseData, path + "/RFID/EntryStatus", "Card " + String(cardCount) + " -" + " Welcome. Enjoy your parking");
                  grantEntry();
                }
                else{
                  // Card has not exited
                  Serial.println("Card has not exited");
                  // Cho bảo vệ xem
                  Firebase.setString(firebaseData, path + "/RFID/EntryStatus", "Card " + String(cardCount) + " -" + " Card has not exited");
                  // Cho khách xem
                  digitalWrite(RED_LED, HIGH);
                  delay(1000);
                  digitalWrite(RED_LED, LOW);
                }
              }
            }
            else {
            // Card doesn't have sufficient Balance
            Serial.println("Card doesn't have sufficient balance. Please recharge it.");
            // Cho bảo vệ xem
            Firebase.setString(firebaseData, path + "/RFID/EntryStatus", "Card " + String(cardCount) + " -" + " Card doesn't have sufficient balance. Please recharge it");
            // Cho khách xem
            accessDenied();
            }
          }
        }
      } 
      else {
          // Card doesn't exists
          Serial.println("Card doesn't exists. Please add it");
          // Cho bảo vệ xem
          Firebase.setString(firebaseData, path + "/RFID/EntryStatus", "Card doesn't exists. Please add it");
          // Cho khách xem
          accessDenied();
      }
    }
  } 
}

// function to cycle the leds
void cycleLed(int count, int delay_ms) {   
  for(int i=0; i<count; i++) {
    // Red LED On for 200 ms
    digitalWrite(GREEN_LED, LOW );   
    digitalWrite(RED_LED,   HIGH); 
    digitalWrite(BLUE_LED,  LOW );   
    delay(delay_ms);

    // Green LED On for 200 ms                             
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED,   LOW );   
    digitalWrite(BLUE_LED,  LOW );   
    delay(delay_ms);

    // Blue LED On for 200 ms
    digitalWrite(GREEN_LED, LOW );                              
    digitalWrite(RED_LED,   LOW );  
    digitalWrite(BLUE_LED,  HIGH);  
    delay(delay_ms);
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

  // setup the pins (servo + led)
  myServo.attach(SERVO_PIN, 500, 2400);   
  pinMode(GREEN_LED, OUTPUT);    
  pinMode(RED_LED ,  OUTPUT);
  pinMode(BLUE_LED,  OUTPUT);

  // default configurations (servo + led)
  myServo.write(0); 
  digitalWrite(GREEN_LED, LOW);  
  digitalWrite(RED_LED,  LOW);
  digitalWrite(BLUE_LED,  LOW); 

  // sync the current time from SNTP Server
  timeClient.begin();

  // Atlast. Connect to Firebase using Host and Authentication key
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}