#include "main.h"
#include "card_control.h"

// function to convert byte array to string
//Chuyển đổi mảng byte thành chuỗi ký tự thập lục phân
static void byteArrayToString(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    //nếu nib1 < 0xA thì buf = '0' + nib1, ngược lại thì buf = 'A' + nib1 - 0xA
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}

// function to get the card UID
//Đọc UID của thẻ RFID và chuyển đổi nó thành chuỗi ký tự thập lục phân, sau đó in ra chuỗi này
bool getCardID() {
  //Kiểm tra xem có thẻ mới nào được đặt trước đầu đọc hay không. Nếu không, trả về false
  if (!Reader.PICC_IsNewCardPresent()) {
    return false;
  }
  //Kiểm tra xem có đọc được UID của thẻ hay không. Nếu không, trả về false
  if (!Reader.PICC_ReadCardSerial()) {
    return false;
  }
  //Mảng ký tự để chứa chuỗi UID
  char str[32]; //thẻ RFID sử dụng UID (Unique Identifier) có độ dài 4 byte (32 bit). Thẻ này là dài 4 byte nên là 32 bit, còn loại 7 byte, 10 byte nữa nên tùy phần cứng mà mình sửa đổi
  //Mảng byte để chứa các byte UID đọc được
  byte readCard[4]; 
  // Get the UID and Convert it into String for further use
  /* 
    Lấy UID và chuyển đổi thành chuỗi:
    for (uint8_t i = 0; i < 4; i++) { readCard[i] = Reader.uid.uidByte[i]; }: Lấy từng byte của UID và lưu vào readCard.
    byteArrayToString(readCard, 4, str);: Chuyển đổi mảng byte readCard thành chuỗi ký tự thập lục phân và lưu vào str.
  */
  for (uint8_t i = 0; i < 4; i++) {
    readCard[i] = Reader.uid.uidByte[i]; //đọc ra dữ liệu dạng byte (số nguyên không dấu 8 bit, giá trị từ 0 đến 255)
    byteArrayToString(readCard, 4, str); 
    strUID = str;
  }

  // Print the card UID for refrence
  Serial.print("The UID of the Scanned Card is : ");     
  Serial.println(strUID); 
  //Nháy led GREEN mỗi lần quẹt để biết là quẹt
  digitalWrite(GREEN_LED,HIGH);
  delay(100);
  digitalWrite(GREEN_LED,LOW);
  Reader.PICC_HaltA();

  return true; //nếu đọc thành công thì hàm trả về true
}

// function to add card to the database
void addCard(String strUID) {
  // Holds index of the card where Null String is present in Database
  unsigned int addCount = 0;

  // Hold total number of cards available in Database
  unsigned int totalCards;
  if(Firebase.getInt(firebaseData, path + "/RFID/TotalCards")){
    totalCards = firebaseData.intData();
  }
  /* Check if Card with UID exists */
  String cardUID;
  for (unsigned int i=1; i<=totalCards; i++) {
    if (Firebase.getString(firebaseData, path + "/RFID/Cards/Card" + String(i) + "/UID")) {
      cardUID = firebaseData.stringData();
    }
    if(cardUID == "Null") {
      addCount=i;
      break;
    }
  }

  if(addCount > 0) {  
    // Use the unused card from the database and make the entry
    Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(addCount) + "/UID" , strUID);
    Firebase.setInt(firebaseData, path + "/RFID/Cards/Card" + String(addCount) + "/Balance" , 0);
  }
  else {
    //Add a new card to the database using card template
    if (Firebase.getJSON(firebaseData, "/RFID/CardTemplate")) {
      if (firebaseData.dataType() == "json") {
        Firebase.setJSON(firebaseData, path + ("/RFID/Cards/Card" + String(totalCards+1)), firebaseData.jsonObject());
      }
    } 
    Firebase.setString(firebaseData, path + ("/RFID/Cards/Card" + String(totalCards+1) + "/UID") , strUID);
    Firebase.setInt(firebaseData, path + "/RFID/TotalCards" , totalCards+1);                       
  }
}

// function to remove card from the database
void removeCard(String strUID) {
  // Push the default values in the database
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/UID" , "Null");
  Firebase.setInt(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/Balance" , 0);
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/CardInformation/CardType", "Guest");      
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/CardInformation/Plate", "Null"); 
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/CardInformation/BrandOfVehicle", "Null");
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/CardInformation/TypeOfVehicle", "Null");     
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/Entry/Date","YYYY-MM-DD");                                                       
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/Entry/Time","HH:MM:SS");
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/Exit/Date","YYYY-MM-DD");
  Firebase.setString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/Exit/Time","HH:MM:SS");
  Firebase.setBool(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/IsVehicleIn", false);
}

/* function to check if the card is Master card */
bool isMaster(String strUID) {
  if (Firebase.getString(firebaseData, path + "/RFID/MasterCardUID")) {
    String masterCardUID = firebaseData.stringData();
    if (strUID == masterCardUID){
      return true;
    }else{
      return false;
    }
  } 
  else{
      Serial.println("Lỗi khi lấy MasterCardUID từ Firebase: " + firebaseData.errorReason());
      return false;
  }
}

/* function to check if the card is there in database */
bool isCardFind(String strUID) {
  unsigned int totalCards;
  if(Firebase.getInt(firebaseData, path + "/RFID/TotalCards")){
    totalCards = firebaseData.intData();
  }
  for (unsigned int i=1; i<=totalCards; i++) {
    if (Firebase.getString(firebaseData, path + "/RFID/Cards/Card" + String(i) + "/UID")) {
      String cardUID = firebaseData.stringData();
      if(strUID == cardUID) {
        // Set the counter to the position where card is find
        cardCount=i;
        return true;
      }
    }
  }

  return false;
}