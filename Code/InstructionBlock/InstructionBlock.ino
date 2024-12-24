#include <WiFi.h>
#include <FirebaseESP32.h>
#include <TFT_eSPI.h>
#include <SPI.h>
/*
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
VCC và LED nối vào 3V3
*/
//do x x y = 240 x 320 nên khi draw pixel thì 120 x 160 sẽ là chính giữa
TFT_eSPI tft = TFT_eSPI();

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""
String prevAreaStatus = ""; // Biến để lưu trữ trạng thái trước đó của khu vực
String areaStatus = "";
FirebaseData firebaseData; //Declare the Firebase Data object in the global scope
FirebaseAuth auth; //Define the FirebaseAuth data for authentication data
FirebaseConfig config; //Define the FirebaseConfig data for config data
String path = "/";
unsigned long t1 = 0; 
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  //Kết nối Wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  //Kết nối Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true); //khi thiết bị bị ngắt kết nối, thì dòng này giúp firebase tự động kết nối wifi với esp
  delay(1000);
  // Đăng ký callback
  if (Firebase.beginStream(firebaseData, path + "/Instruction")) {
    Firebase.setStreamCallback(firebaseData, streamCallback, streamTimeoutCallback);
  } else {
    Serial.println("Không thể bắt đầu stream, lý do: " + firebaseData.errorReason());
  }

  //Chữ hướng dẫn đỗ
  tft.setTextColor(TFT_DARKCYAN);
  tft.drawString("Parking Instruction", 17, 0, 4);
  //Vẽ barie
  tft.drawLine(80,288,160,256,TFT_BLUE); //Tọa độ điểm đầu (0,0) - cuối (120,160) và màu
  tft.fillRect(0,288,80,32,TFT_BLUE); //tọa độ điểm vẽ, chiều dài tính theo Ox và chiều cao tính theo Oy, màu
  tft.fillRect(160,288,80,32,TFT_BLUE);
  tft.drawString("You are here",85,304);
  //Vẽ các zone
  tft.fillRect(80,40,80,40,TFT_GREENYELLOW);
  tft.setTextColor(TFT_BLACK);
  tft.drawString("Zone A",103,58);

  tft.fillRect(0,100,40,80,TFT_GREENYELLOW);
  tft.setTextColor(TFT_BLACK);
  tft.drawString("Zone B", 3 , 136);

  tft.fillRect(200,100,40,80,TFT_GREENYELLOW);
  tft.setTextColor(TFT_BLACK);
  tft.drawString("Zone C", 203, 136); 
}

void loop() {
}

// Hàm callback khi có dữ liệu thay đổi
void streamCallback(StreamData data) {
  if (data.dataType() == "string") {
    areaStatus = data.stringData();

    // Kiểm tra nếu trạng thái đã thay đổi
    if (areaStatus != prevAreaStatus) {
      //Xóa các hình vẽ
      tft.fillRect(50, 90, 145, 110, TFT_BLACK);
      //Xóa chữ
      tft.fillRect(0,210,240,40,TFT_BLACK);

      // Hiển thị hướng dẫn mới
      if(areaStatus == "ZoneA"){
        /* Instruction to Zone A */
        Serial.println(areaStatus);
        Serial.println("Instruction to Zone A");
        tft.setTextColor(TFT_GOLD);
        tft.drawString("Go straight ahead 30m",48,210,2);
        tft.fillRect(110,120,20,80,TFT_RED);
        tft.fillTriangle(120,90,100,120,140,120,TFT_RED);
      } else if(areaStatus == "ZoneB"){
        Serial.println(areaStatus);
        Serial.println("Instruction to Zone B");
        /* Instruction to Zone B */
        tft.setTextColor(TFT_GOLD);
        tft.drawString("Go straight ahead 15m",50,210,2);
        tft.drawString("Turn left 10m",78,228,2);
        tft.fillRect(110,140,20,60,TFT_RED);
        tft.fillRect(80,130,50,20,TFT_RED);
        tft.fillTriangle(50,140,80,120,80,160,TFT_RED);
      } else if(areaStatus == "ZoneC"){
        Serial.println(areaStatus);
        Serial.println("Instruction to Zone C");
        /* Instruction to Zone C */
        tft.setTextColor(TFT_GOLD);
        tft.drawString("Go straight ahead 15m",50,210,2);
        tft.drawString("Turn right 10m",74,228,2);
        tft.fillRect(110,140,20,60,TFT_RED);
        tft.fillRect(110,130,50,20,TFT_RED);
        tft.fillTriangle(190,140,160,120,160,160,TFT_RED);
      } else if(areaStatus == "Remove"){
        Serial.println(areaStatus);
        Serial.println("Xoa man hinh hien thi");
        tft.setTextColor(TFT_GOLD);
        tft.drawString("Please choose the parking area",21,210,2);
      }
      // Lưu trữ trạng thái hiện tại để so sánh với lần lặp tiếp theo
      prevAreaStatus = areaStatus;
    }
  }
}

// Hàm callback khi stream bị ngắt
void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream timeout, đang cố gắng kết nối lại...");
    if (!firebaseData.httpConnected()) {
      Serial.println("Đã mất kết nối tới Firebase, đang kết nối lại...");
    }
  }
}