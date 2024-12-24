#include <WiFi.h>
#include <FirebaseESP32.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <SimpleTimer.h>

#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""           
#define WIFI_SSID ""                                            
#define WIFI_PASSWORD ""  

// Firebase
FirebaseData firebaseData; //Declare the Firebase Data object in the global scope
FirebaseAuth auth; //Define the FirebaseAuth data for authentication data
FirebaseConfig config; //Define the FirebaseConfig data for config data
String path = "/";

// LCD1602 
/* SDA - GPIO21 - SCL - GPIO22 */
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

// DHT11 sensor
#define DHTPIN 23
DHT dht(DHTPIN, DHT11); 
#define buzzDHT11 17
// MQ2 sensor
#define MQ2 32
#define buzzMQ2 4
// Flame Sensor
#define Flame 33
#define buzzFlame 27
// IR Sensor
#define ZoneA_Slot1 18
#define ZoneA_Slot2 19
#define ZoneB_Slot1 25
#define ZoneB_Slot2 26
#define ZoneC_Slot1 13
#define ZoneC_Slot2 14

int A_1 = 0, A_2 = 0, B_1 = 0, B_2 = 0, C_1 = 0, C_2 = 0;
int prev_zoneA = -1, prev_zoneB = -1, prev_zoneC = -1;
int zoneA = A_1 + A_2, zoneB = B_1 + B_2, zoneC = C_1 + C_2;

SimpleTimer timer;

// Hành động khi vượt ngưỡng trong 3s
unsigned long thresholdTimeMQ2 = 3000;
unsigned long startTimeMQ2 = 0;
bool timerStartedMQ2 = false;

unsigned long thresholdTimeFlame = 3000;
unsigned long startTimeFlame = 0;
bool timerStartedFlame = false;

unsigned long thresholdTimeDHT11 = 3000;
unsigned long startTimeDHT11 = 0;
bool timerStartedDHT11 = false;

void readIRSensor(){
  A_1 = 0, A_2 = 0, B_1 = 0, B_2 = 0, C_1 = 0, C_2 = 0;
  //Đọc cảm biến giá trị bằng 1 thì là A = 0 do không có vật cản, bằng 0 thì A = 1 tức là có vật cản
  if(digitalRead(ZoneA_Slot1) == 0) {A_1=1;}
  if(digitalRead(ZoneA_Slot2) == 0) {A_2=1;}
  if(digitalRead(ZoneB_Slot1) == 0) {B_1=1;}
  if(digitalRead(ZoneB_Slot2) == 0) {B_2=1;}
  if(digitalRead(ZoneC_Slot1) == 0) {C_1=1;}
  if(digitalRead(ZoneC_Slot2) == 0) {C_2=1;}  
}
void displayLCDandSendAllDataToFirebase(){
  unsigned long startSend1 = millis();
  readIRSensor();
  /*Zone A*/
  int zoneA = A_1 + A_2;
  if(zoneA != prev_zoneA){
    prev_zoneA = zoneA;
    lcd.setCursor(9, 1);
    lcd.print(2 - zoneA);
    Serial.print("Zone A: ");
    Serial.println(2 - zoneA);
    Firebase.setInt(firebaseData, path + "/Monitor/Parking Status/ZoneA", 2 - zoneA);
  }
  /*Zone B*/
  int zoneB = B_1 + B_2;
  if(zoneB != prev_zoneB){
    prev_zoneB = zoneB;
    lcd.setCursor(11, 1);
    lcd.print(2 - zoneB);
    Serial.print("Zone B: ");
    Serial.println(2 - zoneB);
    Firebase.setInt(firebaseData, path + "/Monitor/Parking Status/ZoneB", 2 - zoneB);
  }
  /*Zone C*/
  int zoneC = C_1 + C_2;
  if(zoneC != prev_zoneC){
    prev_zoneC = zoneC;
    lcd.setCursor(13, 1);
    lcd.print(2 - zoneC);
    Serial.print("Zone C: ");
    Serial.println(2 - zoneC);
    Firebase.setInt(firebaseData, path + "/Monitor/Parking Status/ZoneC", 2 - zoneC);
  }
  // Gửi dữ liệu cảm biến lên Firebase
  int MQ2Sensor = analogRead(MQ2);
  int MQ2Percent = map(MQ2Sensor, 0, 4095, 0, 100);
  int flameSensor = analogRead(Flame);
  int flamePercent = map(flameSensor, 0, 4095, 100, 0);
  float temp = dht.readTemperature();
  unsigned long startSend2 = millis();
  Firebase.setInt(firebaseData, "/Monitor/MQ2Sensor/Value", MQ2Percent);
  Firebase.setInt(firebaseData, "/Monitor/FlameSensor/Value", flamePercent);
  Firebase.setFloat(firebaseData, "/Monitor/DHT11/Temperature", temp);
  unsigned long endSend = millis();
  Serial.print("Time to send data cả hàm to Firebase: ");
  Serial.println(endSend - startSend1);
  Serial.print("Time to send data 3 cảm biến to Firebase: ");
  Serial.println(endSend - startSend2);
  Serial.println("Data sent to Firebase");
}
void checkSensors(){
  int MQ2Sensor = analogRead(MQ2);
  int MQ2Percent = map(MQ2Sensor, 0, 4095, 0, 100);
  int flameSensor = analogRead(Flame);
  int flamePercent = map(flameSensor, 0, 4095, 100, 0);
  float temp = dht.readTemperature();
  Serial.print("MQ2 Value Lần 1: ");
  Serial.println(MQ2Percent);
  Serial.print("Flame Value Lần 1: ");
  Serial.println(flamePercent);
  Serial.print("Temperature Lần 1: ");
  Serial.println(temp);
  Serial.println();
  if(MQ2Percent > 55){
    if (!timerStartedMQ2) {
      // Start the timer
      startTimeMQ2 = millis();
      Serial.print("startTimeMQ2 lần đầu: "); Serial.println(startTimeMQ2);
      timerStartedMQ2 = true;
    } else {
    // Check if 3 seconds have passed
      if (millis() - startTimeMQ2 >= thresholdTimeMQ2) {
        Serial.print("startTimeMQ2 lần hai (để xem có giống lần một không): "); Serial.println(startTimeMQ2);
        Serial.print("Giá trị thời gian khi bật còi: ");
        Serial.println(millis() - startTimeMQ2);
        Serial.println("Fire detected in Zone C for 3 seconds");
        // Firebase.setInt(firebaseData, "/Monitor/IsFlame/ZoneC", 1);
        digitalWrite(buzzMQ2, HIGH); // Buzzer on
        Serial.print("MQ2 Value Detected: ");
        Serial.println(MQ2Percent);
        Serial.println();
      } else {
        Serial.print("startTimeMQ2 lần ba (để xem có giống lần một không): "); Serial.println(startTimeMQ2);
        Serial.print("Gía trị thời gian đang đếm: ");
        Serial.println(millis() - startTimeMQ2);
        Serial.println("Fire detected in zone C, timer running");
        Serial.print("MQ2 Value Timer Running: ");
        Serial.println(MQ2Percent);
        Serial.println();
      }
    }
  } else {
    // Reset the timer if the threshold is not met
    timerStartedMQ2 = false;
    Serial.println("No Fire in Zone C");
    // Firebase.setInt(firebaseData, "/Monitor/IsFlame/ZoneC", 0);
    digitalWrite(buzzMQ2, LOW); // Buzzer off
    Serial.print("MQ2 Value No Fire: ");
    Serial.println(MQ2Percent);
    Serial.println();
  }

  if(flamePercent > 55){
    if (!timerStartedFlame) {
      // Start the timer
      startTimeFlame = millis();
      Serial.print("startTimeFlame lần đầu: "); Serial.println(startTimeFlame);
      timerStartedFlame = true;
    } else {
    // Check if 3 seconds have passed
      if (millis() - startTimeFlame >= thresholdTimeFlame) {
        Serial.print("startTimeFlame lần hai (để xem có giống lần một không): "); Serial.println(startTimeFlame);
        Serial.print("Giá trị thời gian khi bật còi: ");
        Serial.println(millis() - startTimeFlame);
        Serial.println("Fire detected in Zone B for 3 seconds");
        // Firebase.setInt(firebaseData, "/Monitor/IsFlame/ZoneB", 1);
        digitalWrite(buzzFlame, HIGH); // Buzzer on
        Serial.print("Flame Sensor Detected: ");
        Serial.println(flamePercent);
        Serial.println();
      } else {
        Serial.print("startTimeFlame lần ba (để xem có giống lần một không): "); Serial.println(startTimeFlame);
        Serial.print("Gía trị thời gian đang đếm: ");
        Serial.println(millis() - startTimeFlame);
        Serial.println("Fire detected in zone B, timer running");
        Serial.print("Flame Sensor Timer Running: ");
        Serial.println(flamePercent);
        Serial.println();
      }
    }
  } else {
    // Reset the timer if the threshold is not met
    timerStartedFlame = false;
    Serial.println("No Fire in Zone B");
    // Firebase.setInt(firebaseData, "/Monitor/IsFlame/ZoneB", 0);
    digitalWrite(buzzFlame, LOW); // Buzzer off
    Serial.print("Flame Sensor No Fire: ");
    Serial.println(flamePercent);
    Serial.println();
  }
  // DHT11
  if(temp > 40){
    if (!timerStartedDHT11) {
      // Start the timer
      startTimeDHT11 = millis();
      Serial.print("startTimeDHT11 lần đầu: "); Serial.println(startTimeDHT11);
      timerStartedDHT11 = true;
    } else {
    // Check if 3 seconds have passed
      if (millis() - startTimeDHT11 >= thresholdTimeDHT11) {
        Serial.print("startTimeDHT11 lần hai (để xem có giống lần một không): "); Serial.println(startTimeDHT11);
        Serial.print("Giá trị thời gian khi bật còi: ");
        Serial.println(millis() - startTimeDHT11);
        Serial.println("Fire detected in Zone A for 3 seconds");
        // Firebase.setInt(firebaseData, "/Monitor/IsFlame/ZoneA", 1);
        digitalWrite(buzzDHT11, HIGH); // Buzzer on
        Serial.print("Temperature Detected: ");
        Serial.print(temp);
        Serial.println("*C ");
        Serial.println();
      } else {
        Serial.print("startTimeDHT11 lần ba (để xem có giống lần một không): "); Serial.println(startTimeDHT11);
        Serial.print("Gía trị thời gian đang đếm: ");
        Serial.println(millis() - startTimeDHT11);
        Serial.println("Fire detected in zone A, timer running");
        Serial.print("Temperature Timer Running: ");
        Serial.print(temp);
        Serial.println("*C ");
        Serial.println();
      }
    }
  } else {
    // Reset the timer if the threshold is not met
    timerStartedDHT11 = false;
    Serial.println("No Fire in Zone A");
    // Firebase.setInt(firebaseData, "/Monitor/IsFlame/ZoneA", 0);
    digitalWrite(buzzDHT11, LOW); // Buzzer off
    Serial.print("Temperature No Fire: ");
    Serial.print(temp);
    Serial.println("*C ");
    Serial.println();
  }
}
void setup(){
  Serial.begin(115200);
  dht.begin();
  lcd.init();
  lcd.backlight();

  //Kết nối Wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP Address is : ");
  Serial.println(WiFi.localIP()); //in local IP address
  Serial.println();

  //Kết nối Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true); //khi thiết bị ngắt kết nối thì hàm này giúp Firebase tự động kết nối Wifi với ESP32

  /* Khởi tạo */
  //Khởi tạo cho IR Sensor
  pinMode(ZoneA_Slot1, INPUT);
  pinMode(ZoneA_Slot2, INPUT);
  pinMode(ZoneB_Slot1, INPUT);
  pinMode(ZoneB_Slot2, INPUT);
  pinMode(ZoneC_Slot1, INPUT);
  pinMode(ZoneC_Slot2, INPUT);
  //Khởi tạo cho DHT11
  pinMode(buzzDHT11, OUTPUT); 
  //Khởi tạo cho MQ2 Sensor
  pinMode(MQ2, INPUT);
  pinMode(buzzMQ2, OUTPUT); 
  //Khởi tạo cho Flame Sensor
  pinMode(Flame, INPUT);
  pinMode(buzzFlame, OUTPUT); 

  // In chữ Zone và các ký tự A, B, C lên màn hình LCD
  lcd.setCursor(0, 0);
  lcd.print("Zone");
  lcd.setCursor(9, 0);
  lcd.print("A B C");
  lcd.setCursor(0, 1);
  lcd.print("Con so o");
  lcd.setCursor(9, 1);
  lcd.print("2");
  lcd.setCursor(11, 1);
  lcd.print("2");
  lcd.setCursor(13, 1);
  lcd.print("2");
  timer.setInterval(2000, displayLCDandSendAllDataToFirebase);
  timer.setInterval(100, checkSensors);
}

void loop(){
  timer.run(); 
}