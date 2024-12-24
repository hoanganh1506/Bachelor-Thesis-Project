#include "main.h"
#include "fuzzy_business_logic.h"
#include "user_access.h"                  

// function to get the date from epoch time
static String getDate() {
  unsigned long epochTime = timeClient.getEpochTime();
  time_t epochTime_t = static_cast<time_t>(epochTime); // Chuyển đổi epochTime sang kiểu time_t                   
  struct tm *ptm = localtime(&epochTime_t);                  
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;
  String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  return currentDate;
}

// function to open the door lock
static void openDoorLock() {                        
  Serial.println("Access Granted");   

  Serial.println("Door Opening...");
  for(int angle = 0; angle < 90; angle++) {                
    myServo.write(angle);
    delay(15);
  }

  Serial.println("Door Opened");
  Serial.println("You may pass now");

  // Keep the door open for 3 seconds 
  delay(3000);

  Serial.println("Door Locking...");
  for(int angle = 90; angle > 0; angle--) {
    myServo.write(angle);
    delay(15);
  }

  Serial.println("Door Locked");
}

// function to grant the exit
void grantExit() {
  // Update the time client to fetch current date and time from NTP Server
  timeClient.update();

  // Mark the exit in the database
  Firebase.setString(firebaseData, path + ("/RFID/Cards/Card" + String(cardCount) + "/Exit/Date") , getDate());
  Firebase.setString(firebaseData, path + ("/RFID/Cards/Card" + String(cardCount) + "/Exit/Time") , timeClient.getFormattedTime());
  Firebase.setBool(firebaseData, path + ("/RFID/Cards/Card" + String(cardCount) + "/IsVehicleIn") , false);
  
  // fuzzy business logic to charge some ammount :)
  String cardType;
  if(Firebase.getString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/CardInformation/CardType")) {
    cardType = firebaseData.stringData();
  }
  if (cardType == "Member") {
    bool canExit = fuzzy_business_logic();
    if(canExit){
      Serial.println("Thank you for parking your vehicle!");
      Serial.println();
      openDoorLock();
    }
    else{
      Serial.println("Insufficient balance. Please top up your account.");
    }
  }
  if (cardType == "Guest") {
    Serial.println("Thank you for parking your vehicle!");
    Serial.println();
    // open the door lock
    openDoorLock();
  }
}
void checkAndOpenOutDoor() {
  bool openOutDoor;
  if (Firebase.getBool(firebaseData, path + "/RFID/OpenOutRequest")) {
    openOutDoor = firebaseData.boolData();
  }

  if (openOutDoor) {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Thanks You");
    lcd.setCursor(2, 1);
    lcd.print("For Parking!");
    openDoorLock();
    Firebase.setBool(firebaseData, path + "/RFID/OpenOutRequest", false); // Reset flag after opening the door
    Serial.println("Door opened by remote request.");
  }
}
// function to deny the access
void accessDenied() {
  // Indicate the status by Red LED ON
  digitalWrite(RED_LED, HIGH);
  Serial.println("Access Denied");
  Serial.println("You can't go ahead");
  Serial.println("Either your card is not valid or you don't have sufficient balance in card");
  Serial.println("Please contact customer care");
  delay(1000);
  Serial.println();
  digitalWrite(RED_LED, LOW);
}