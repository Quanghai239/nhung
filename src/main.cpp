#include <FirebaseESP32.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <time.h>
#include <cstring>
/////////////////
HardwareSerial Serialport(2);
#define BUTTON_PIN 2                
/////////////////
FirebaseData fbdo;
FirebaseData fbdo2;
FirebaseData fbdo5;
FirebaseData fbdo6;
FirebaseData fbdo7;
FirebaseData fbdo8;
///////////
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData firebaseData;
/////////
String paths   = "/BT1";
String paths2  = "/BT2";
String paths5  = "/LB1";
String paths6  = "/LB2";
String paths7  = "/LEDRGB";
String paths8  = "/TIME";
///////////////
// const char ssid[] = "Cà Phê Võng";
// const char pass[] = "";
// const char ssid[] = "GIANG L2";
// const char pass[] = "0393286953";
// const char ssid[] = "Nhà Thông Minh";
// const char pass[] = "0981805945";
// const char ssid[] = "Iphone 6s";
// const char pass[] = "12345678";
const char ssid[] = "NHOM12";
const char pass[] = "01234567";
//////////
////////////////
bool dataButton1 = false; 
bool dataButton2 = false; 
int dataButton7 = 0;
int button1State ;
int button2State; 
unsigned long lastDebounceTime = 0;  // Thời gian lần nhấn cuối cùng
unsigned long debounceDelay = 300; 
volatile bool sendToFirebaseFlag = false; 
volatile bool buttonPressed = false;  // Flag for Button press
bool lastButtonState = LOW;   
bool buttonState = false;
char currentTimeStr[9]; // Để lưu thời gian dạng HH:MM:SS
char currentDateStr[11]; //
unsigned long previousMillis = 0;  // Thời gian trước đó
const long interval = 1000;  
struct Alarm {
    String time;  // Lưu thời gian báo thức dưới dạng chuỗi "HH:MM:SS"
    String note;  // Lưu chuỗi note tương ứng
};
// Tạo mảng để lưu nhiều báo thức và note
Alarm alarms[20]; // Lưu tối đa 20 báo thức với các note khác nhau
int alarmCount = 0;
unsigned long currentMillis = millis(); // Lấy thời gian hiện tại
//////////
void printStreamData(StreamData data) {
    int value;
    Serial.print("Path: " + data.streamPath());

    // Nhận dữ liệu từ Firebase
    if (data.dataType() == "boolean") { // Kiểm tra kiểu dữ liệu là boolean
        bool boolValue = data.boolData(); // Lấy giá trị boolean từ Firebase
        value = boolValue ? 1 : 0;  // Chuyển true thành 1 và false thành 0
        Serial.println("Giá trị nhận được (boolean): " + String(boolValue));
    } else if (data.dataType() == "int") {
        value = data.intData();
        Serial.println("Giá trị nhận được (int): " + String(value));
    } else if (data.dataType() == "string") {
        String valueStr = data.stringData();
        value = valueStr.toInt();  // Chuyển đổi chuỗi sang số nguyên nếu cần
        Serial.println("Giá trị nhận được (string): " + valueStr);
    } else if (data.dataType() == "json") {
        // Lấy dữ liệu JSON từ Firebase
        FirebaseJson jsonData = data.jsonObject();
        // Khai báo biến để lưu giá trị hour, minute và note
        FirebaseJsonData jsonHourData, jsonMinuteData, jsonNoteData;
        int hour = 0;
        int minute = 0;
        String note = "";
        // Kiểm tra và lấy giá trị từ Firebase cho giờ, phút và note
        if (jsonData.get(jsonHourData, "/hour")) {
            hour = jsonHourData.intValue;
            Serial.println("Giờ: " + String(hour));
        }
        if (jsonData.get(jsonMinuteData, "/minute")) {
            minute = jsonMinuteData.intValue;
            Serial.println("Phút: " + String(minute));
        }
        if (jsonData.get(jsonNoteData, "/note")) {
            note = jsonNoteData.stringValue;
            Serial.println("Note: " + note);
        }
        // Định dạng thời gian thành "HH:MM:SS"
        char buffer[9];
        sprintf(buffer, "%02d:%02d:00", hour, minute);
        String alarmTime = String(buffer);
        // Lưu thời gian báo thức và note vào mảng
if (alarmCount < 20) { // Kiểm tra nếu mảng chưa đầy
    alarms[alarmCount].time = alarmTime;
    alarms[alarmCount].note = note;
    alarmCount++;
    Serial.println("Đã lưu báo thức: " + alarmTime + " với note: " + note);

    // Chuyển đổi thời gian thành định dạng "HH,MM,SS"
    String hour = alarmTime.substring(0, 2);
    String minute = alarmTime.substring(3, 5);
    String second = alarmTime.substring(6, 8);

    // Chuyển đổi giá trị của note từ ký tự (a, b, c, d) thành số (1, 2, 3, 4)
    String noteCommand;
    if (note == "a") {
        noteCommand = "1";
    } else if (note == "b") {
        noteCommand = "2";
    } else if (note == "c") {
        noteCommand = "3";
    } else if (note == "d") {
        noteCommand = "4";
    } else {
        noteCommand = "0"; // Trường hợp mặc định nếu không khớp
    }

    // Tạo chuỗi dữ liệu báo thức với định dạng "$HH,MM,SS,note#"
    String command = "$," + hour + "," + minute + "," + second + "," + noteCommand + "#";
    Serialport.print(command);
    Serial.println("Đã gửi lệnh qua STM32: " + command);
} else {
    Serial.println("Danh sách báo thức đã đầy!");
}


    }
        // Tiếp tục xử lý giá trị...
        // Kiểm tra từng path và gửi ký tự tương ứng
        if (data.streamPath() == paths) { // Kiểm tra SW1
            char valueToSend = (value == 1) ? 'a' : 'b';  // Chuyển 1 thành 'a' và 0 thành 'b'
            // Gửi ký tự tương ứng qua STM32
            Serialport.print(valueToSend);
            Serial.println("Gửi ký tự: " + String(valueToSend) + " qua STM32");
        } else if (data.streamPath() == paths2) { // Kiểm tra SW2
            char valueToSend = (value == 1) ? 'c' : 'd';  // Chuyển 1 thành 'c' và 0 thành 'd'
            // Gửi ký tự tương ứng qua STM32
            Serialport.print(valueToSend);
            Serial.println("Gửi ký tự: " + String(valueToSend) + " qua STM32");
        } else { // Đối với các đường dẫn khác
            if (data.dataType() == "int") {
                value = data.intData();
        } else if (data.dataType() == "string") {
                String valueStr = data.stringData();
                value = valueStr.toInt();  // Chuyển đổi chuỗi sang số nguyên nếu cần
        } else {
            Serial.print("Other data type");
            return;
            }
            // Kiểm tra từng path và xử lý giá trị như trước
        if (data.streamPath() == paths5 || data.streamPath() == paths6) { // Kiểm tra MUC SANG DEN 1 và 2
            static int brightness1 = 0, brightness2 = 0;
            if (value >= 0 && value <= 100) { // Chỉ nhận giá trị từ 0 đến 100
                if (data.streamPath() == paths5 && brightness1 != value) { // Chỉ gửi khi có sự thay đổi cho paths5
                    brightness1 = value;
                    Serial.println("Brightness1 (MUC SANG DEN 1): " + String(brightness1));
                } else if (data.streamPath() == paths6 && brightness2 != value) { // Chỉ gửi khi có sự thay đổi cho paths6
                    brightness2 = value;
                    Serial.println("Brightness2 (MUC SANG DEN 2): " + String(brightness2));
                }
                    // Gửi giá trị từ cả MUC SANG DEN 1 và 2 dưới dạng chuỗi "brightness1,brightness2"
                Serialport.print(String(brightness1) + "," + String(brightness2) + "\n");
                Serial.println("Gửi giá trị: " + String(brightness1) + "," + String(brightness2) + " qua STM32");
            } else {
                Serial.println("Invalid brightness value. Expected 0 to 100.");
            }
        } else if (data.streamPath() == paths7) { // Kiểm tra đường dẫn nhận dữ liệu
            dataButton7 = value; // Lưu giá trị nhận được từ Firebase vào biến dataButton7.
                // In giá trị RGB ra Serial Monitor
            Serial.println("RGB Data: " + String(dataButton7)); 
            // Tách thành phần màu đỏ (R), xanh lá cây (G), và xanh dương (B)
            int red = (dataButton7 >> 16) & 0xFF; // Tách thành phần màu đỏ
            int green = (dataButton7 >> 8) & 0xFF; // Tách thành phần màu xanh lá cây
            int blue = dataButton7 & 0xFF; // Tách thành phần màu xanh dương
            Serial.println("R: " + String(red) + ", G: " + String(green) + ", B: " + String(blue));
          
            // Gửi giá trị RGB đến STM32 dưới dạng chuỗi "R,G,B"
            Serialport.print(String(red) + "," + String(green) + "," + String(blue) + "\n");
            Serial.println("Gửi dữ liệu RGB: " + String(red) + "," + String(green) + "," + String(blue) + " qua STM32");
        }
   } 
}
void streamCallback(StreamData data) {
  Serial.println("Stream 1 received data!");
  printStreamData(data);
}
void streamCallback2(StreamData data) {
  Serial.println("Stream 2 received data!");
  printStreamData(data);
}
void streamCallback5(StreamData data) {
  Serial.println("Stream 5 received data!");
  printStreamData(data);
}
void streamCallback6(StreamData data) {
  Serial.println("Stream 6 received data!");
  printStreamData(data);
}
void streamCallback7(StreamData data) {
  Serial.println("Stream 7 received data!");
  printStreamData(data);
}
void streamCallback8(StreamData data) {
  Serial.println("Stream 8 received data!");
  printStreamData(data);
}
void streamTimeoutCallback(bool timeout) {
    if (timeout) {
        Serial.println("Stream timeout, reconnecting...");
        Firebase.beginStream(fbdo, paths);
        Firebase.beginStream(fbdo2, paths2);
        Firebase.beginStream(fbdo5, paths5);
        Firebase.beginStream(fbdo6, paths6);
        Firebase.beginStream(fbdo7, paths7);
        Firebase.beginStream(fbdo8, paths8);
    }
}
void receiveDataFromSTM32() {
    if (Serialport.available()) {
        String dataReceived = Serialport.readStringUntil('\n');
        dataReceived.trim();
        Serial.println(dataReceived);  // In dữ liệu đã nhận
        // Kiểm tra giá trị hợp lệ
        if (dataReceived == "0,0" || dataReceived == "0,1" || dataReceived == "1,0" || dataReceived == "1,1") {
            int commaIndex = dataReceived.indexOf(',');
            button1State = dataReceived.substring(0, commaIndex).toInt();
            button2State = dataReceived.substring(commaIndex + 1).toInt();

            // Chuyển đổi trạng thái nút từ int sang bool
            bool button1Pressed = (button1State == 1);
            bool button2Pressed = (button2State == 1);

            // Cập nhật trạng thái của các nút
            dataButton1 = button1Pressed;
            dataButton2 = button2Pressed;

            sendToFirebaseFlag = true;  // Kích hoạt biến cờ để gửi lên Firebase

            Serial.println("button1Pressed: " + String(button1Pressed) + ", button2Pressed: " + String(button2Pressed));
        } else {
            Serial.println("Giá trị không hợp lệ! Chỉ chấp nhận 0,0; 0,1; 1,0; hoặc 1,1.");
        }
    }
}

void IRAM_ATTR handleButtonPress() {
    buttonPressed = true;  // Set flag when button is pressed
}
/////////////////
void setup(){
    Serialport.begin(115200, SERIAL_8N1, 16, 17);
    Serial.begin(9600); 
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    { 
        Serial.print(".");

    } 
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
  ///////////////////////
    config.host = "iot-arduino-40f94-default-rtdb.asia-southeast1.firebasedatabase.app";
    config.api_key = "AIzaSyDglobI51-CImZeGcvW3uca3j399N1ArRE";
    auth.user.email = "nhom12@gmail.com";
    auth.user.password = "1234567";
    Firebase.reconnectWiFi(true);
    Firebase.begin(&config, &auth);
    Firebase.beginStream(fbdo, paths);
    Firebase.beginStream(fbdo2, paths2);
    Firebase.beginStream(fbdo5, paths5);
    Firebase.beginStream(fbdo6, paths6);
    Firebase.beginStream(fbdo7, paths7);
    Firebase.beginStream(fbdo8, paths8);
    if(Firebase.ready()){
    Serial.println("Connected with FIREBASE!");
    }else{
    Serial.println("Failed to connect with FIREBASE!");
    }
    Firebase.setStreamCallback(fbdo, streamCallback, streamTimeoutCallback);
    Firebase.setStreamCallback(fbdo2, streamCallback2, streamTimeoutCallback);
    Firebase.setStreamCallback(fbdo5, streamCallback5, streamTimeoutCallback);
    Firebase.setStreamCallback(fbdo6, streamCallback6, streamTimeoutCallback);
    Firebase.setStreamCallback(fbdo7, streamCallback7, streamTimeoutCallback);
    Firebase.setStreamCallback(fbdo8, streamCallback8, streamTimeoutCallback);
    attachInterrupt(digitalPinToInterrupt(2), receiveDataFromSTM32, CHANGE);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, CHANGE);
    configTime(25200, 0, "pool.ntp.org", "time.nist.gov");
}
  ///////////////
void loop() {
    // Kiểm tra bộ nhớ
    receiveDataFromSTM32();  // Nhận dữ liệu từ STM32
    if (sendToFirebaseFlag) {
        // Gửi dữ liệu lên Firebase
        if (Firebase.setInt(firebaseData, "/BT1", dataButton1)) {
            Serial.println("Cập nhật BT1 thành công");
        } else {
            Serial.println("Cập nhật BT1 thất bại");
        }

        if (Firebase.setInt(firebaseData, "/BT2", dataButton2)) {
            Serial.println("Cập nhật BT2 thành công");
        } else {
            Serial.println("Cập nhật BT2 thất bại");
        }
        // Đặt lại biến cờ sau khi gửi xong
        sendToFirebaseFlag = false;
    }
    if (buttonPressed) {
        bool currentButtonState = digitalRead(BUTTON_PIN);
        // Kiểm tra xem trạng thái nút có thay đổi và đã qua thời gian debounce hay chưa
        if (currentButtonState != lastButtonState && (millis() - lastDebounceTime) > debounceDelay) {
            if (currentButtonState == HIGH) {
                Serialport.write('f');  // Gửi 'e' đến STM32 khi nút được nhấn (HIGH)
                Serialport.flush();      // Đảm bảo dữ liệu được gửi ngay lập tức
                Serial.println("Sent: f"); // In thông báo xác nhận
            } else {
                Serialport.write('e');  // Gửi 'f' đến STM32 khi nút được thả (LOW)
                Serialport.flush();      // Đảm bảo dữ liệu được gửi ngay lập tức
                Serial.println("Sent: e"); // In thông báo xác nhận
            }
            lastButtonState = currentButtonState;  // Cập nhật trạng thái nút lần cuối
            lastDebounceTime = millis();  // Cập nhật thời gian debounce
        }
        buttonPressed = false;  // Reset flag
    }
    // Lấy thời gian hiện tại
    unsigned long currentMillis = millis(); // Lấy thời gian hiện tại
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis; // Lưu thời gian hiện tại
        struct tm timeinfo;
        // Lấy thời gian
        if (!getLocalTime(&timeinfo)) {
            Serial.println("Không thể lấy thời gian");
            return;
        }
        // Định dạng thời gian thành "HH:MM:SS"
        snprintf(currentTimeStr, sizeof(currentTimeStr), "%02d:%02d:%02d", 
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        // Định dạng ngày thành "DD/MM/YYYY"
        snprintf(currentDateStr, sizeof(currentDateStr), "%02d/%02d/%04d",
                 timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900); // Cộng thêm 1900 cho năm
        // In ra thời gian và ngày tháng
        Serial.print("Thời gian hiện tại: ");
        Serial.println(currentTimeStr);
        Serial.print("Ngày hiện tại: ");
        Serial.println(currentDateStr);
        // Kiểm tra và kích hoạt báo thức
       for (int i = 0; i < alarmCount; i++) {
            if (alarms[i].time == String(currentTimeStr)) {
                // Báo thức kích hoạt
                Serial.println("Báo thức!!! Thời gian khớp với: " + alarms[i].time);
                // Gửi chuỗi `note` qua STM32
                Serialport.print(alarms[i].note);
                Serial.println("Gửi note qua STM32: " + alarms[i].note);
                if (alarms[i].note == "a") {
                    Firebase.setBool(firebaseData, "/BT1", true);
                    Serial.println("Gửi /BT1 = true lên Firebase");
                    Serialport.print('g');  // Gửi 'g' khi note là 'a'
                    Serial.println("Gửi ký tự 'g' qua STM32");
                } else if (alarms[i].note == "b") {
                    Firebase.setBool(firebaseData, "/BT1", false);
                    Serial.println("Gửi /BT1 = false lên Firebase");
                    Serialport.print('h');  // Gửi 'h' khi note là 'b'
                    Serial.println("Gửi ký tự 'h' qua STM32");
                } else if (alarms[i].note == "c") {
                    Firebase.setBool(firebaseData, "/BT2", true);
                    Serial.println("Gửi /BT2 = true lên Firebase");
                    Serialport.print('i');  // Gửi 'i' khi note là 'c'
                    Serial.println("Gửi ký tự 'i' qua STM32");
                } else if (alarms[i].note == "d") {
                    Firebase.setBool(firebaseData, "/BT2", false);
                    Serial.println("Gửi /BT2 = false lên Firebase");
                    Serialport.print('j');  // Gửi 'j' khi note là 'd'
                    Serial.println("Gửi ký tự 'j' qua STM32");
                }
                for (int j = i; j < alarmCount - 1; j++) {
                    alarms[j] = alarms[j + 1];  // Dịch chuyển phần tử lên
                }
                alarmCount--; // Giảm số lượng báo thức
                i--; // Giảm chỉ số để không bỏ qua phần tử tiếp theo
            
            }
        }
    }
}
