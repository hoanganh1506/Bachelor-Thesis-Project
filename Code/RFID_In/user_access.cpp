#include "main.h"
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
  // Indicate the status by Green LED ON
  digitalWrite(GREEN_LED, HIGH);                          
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
  digitalWrite(GREEN_LED, LOW);
}

// function to grant the entry
void grantEntry() {
  // Update the time client to fetch current date and time from NTP Server
  timeClient.update();
  
  // Mark the entry in the database
  Firebase.setString(firebaseData, path + ("/RFID/Cards/Card" + String(cardCount) + "/Entry/Date") , getDate());
  Firebase.setString(firebaseData, path + ("/RFID/Cards/Card" + String(cardCount) + "/Entry/Time") , timeClient.getFormattedTime());
  // Khi vào thì biến bên Exit thành dạng như Template tất
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/Exit/Date" , "YYYY-MM-DD");
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/Exit/Time" , "HH:MM:SS");
  Firebase.setBool(firebaseData, path + ("/RFID/Cards/Card" + String(cardCount) + "/IsVehicleIn") , true);

  Serial.println("Please park your vehicle inside!");
  Serial.println();  

  // open the door lock
  openDoorLock();  
}

void checkAndOpenInDoor() {
  bool openInDoor;
  if (Firebase.getBool(firebaseData, path + "/RFID/OpenInRequest")) {
    openInDoor = firebaseData.boolData();
  }

  if (openInDoor) {
    cycleLed(1, 200);
    openDoorLock();
    Firebase.setBool(firebaseData, path + "/RFID/OpenInRequest", false); // Reset flag after opening the door
    Serial.println("Door opened by remote request.");
  }
}
// function to deny the access
void accessDenied() {
  // Indicate the status by Red LED ON
  digitalWrite(RED_LED, HIGH);

  Serial.println("Access Denied");
  Serial.println("You can't go ahead");
  Serial.println("Either you card is not valid or you don't have sufficient balance in card");
  Serial.println("Please contact customer care");
  delay(1000);

  Serial.println();
  digitalWrite(RED_LED, LOW);
}