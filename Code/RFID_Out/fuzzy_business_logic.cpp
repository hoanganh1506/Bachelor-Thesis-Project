#include "main.h"
#include "fuzzy_business_logic.h"

bool fuzzy_business_logic() {
  // Logic: Charge 500 VNĐ per min.

  String str;
  if (Firebase.getString(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/Entry/Time")) {
    str = firebaseData.stringData();
  }
  //tạo một chuỗi str1 mới để lưu giá trị str lại và in ra Serial
  String str1 = str;
  
  // Trích xuất giờ và phút từ chuỗi thời gian vào
  int entryHour = str.substring(0, 2).toInt();//str.toInt là chuyển đổi chuỗi "MM" thành số nguyên, đại diện cho phút
  int entryMinute = str.substring(3, 5).toInt();//trích xuất các ký tự từ vị trí thứ 3 đến vị trí thứ 4 của chuỗi đếm từ số 0 (ở đây là trích xuất MM đó)
  
  // Lấy giờ và phút hiện tại từ thời gian ra
  int exitHour = timeClient.getHours();
  int exitMinute = timeClient.getMinutes();//lấy minute ở thời điểm Exit
  Serial.println("Your Entry Time is : " + str1 + " in HH:MM:SS Format");
  Serial.println("Your Exit  Time is : " + timeClient.getFormattedTime() + " in HH:MM:SS Format"); //lấy thời gian hiện tại in ra
  
  // Tính toán số phút giữa giờ vào và giờ ra
  int totalEntryMinutes = entryHour * 60 + entryMinute;
  int totalExitMinutes = exitHour * 60 + exitMinute;
  int durationMinutes = totalExitMinutes - totalEntryMinutes;
  
  // Xử lý trường hợp thời gian ra nhỏ hơn thời gian vào (qua nửa đêm)
  /*Ví dụ vào 23h50 (1430 phút) và ra lúc 0h10 (10 phút) thì 10 - 1430 = -1420 nên cộng thêm 1440 ra 20 phút là đúng đó*/
  if (durationMinutes < 0) {
    durationMinutes += 24 * 60;
  }

  // Generate the new balance status
  int currentBalance;
  if (Firebase.getInt(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/Balance")) {
    currentBalance = firebaseData.intData();
  }
  int charge = durationMinutes * 500; //tính tiền
  int newBalance = currentBalance - charge; //trừ tiền đi rồi đưa lại lên Firebase
  if(newBalance < 0){
    newBalance = currentBalance; // Giữ nguyên số dư hiện tại
    // Đưa lên firebase
    String chargeMessage = "Card " + String(cardCount) + " -" +" Your charge is " + String(charge) + " VND." + " But your current balance is " + String(currentBalance) + " VND";
    Firebase.setString(firebaseData, path + "/RFID/ExitStatus", chargeMessage);
    // Cho khách xem
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Your Charge Is");
    lcd.setCursor(4, 1);
    lcd.print(charge);
    lcd.setCursor(10, 1);
    lcd.print("VND");
    Serial.println("Your Current Balance is : " + String(currentBalance) + " VNĐ");
    Serial.println("Sur Charge is : " + String(charge) + " VNĐ");
    return false;
  }
  else{
  // Atlast, Update the card balance status
  Firebase.setInt(firebaseData, path + "/RFID/Cards/Card" + String(cardCount) + "/Balance" , newBalance);
  Firebase.setString(firebaseData, path + "/RFID/ExitStatus", "Card " + String(cardCount) + " -" + " Thank you for parking");
  // Để debug
  Serial.println("Thank you for parking");
  // Cho khách xem
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Thank You");
  lcd.setCursor(2, 1);
  lcd.print("For Parking!");
  Serial.println("Your Current Balance is : " + String(currentBalance) + " VNĐ");
  Serial.println("Sur Charge is : " + String(charge) + " VNĐ");
  Serial.println("Your New Balance is : " + String(newBalance) + " VNĐ");
  return true;
  }
}