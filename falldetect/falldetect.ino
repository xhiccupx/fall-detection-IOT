#include <Wire.h>
#include <ESP8266WiFi.h>
const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
float ax=0, ay=0, az=0, gx=0, gy=0, gz=0;
boolean fall = false; //stores if a fall has occurred
boolean LFTacc=false; //stores if first trigger (lower threshold) has occurred
boolean UFTacc=false; //stores if second trigger (upper threshold) has occurred
boolean UFTgyro=false; //stores if third trigger (orientation change) has occurred
byte trigger1count=0; //stores the counts past since trigger 1 was set true
byte trigger2count=0; //stores the counts past since trigger 2 was set true
byte trigger3count=0; //stores the counts past since trigger 3 was set true
int angleChange=0;
float CVacc=16384.00; //values ​​for the calibration of an accelerometer
float CVgyro=131.07; //values ​​for the calibration of the gyroscope
// WiFi network info.
const char *ssid ="amazon"; // Wi-Fi Name
const char *pass = "jefbezos171.6B"; // Wi-Fi Password
void send_event(const char *event);
const char *host = "maker.ifttt.com";
const char *privateKey = "bGFMBXIHS7wlWDwvdiG7lZ";
void setup(){
 Serial.begin(115200);
 Wire.begin();
 Wire.beginTransmission(MPU_addr);
 Wire.write(0x6B);  // PWR_MGMT_1 register
 Wire.write(0);     // set to zero (wakes up the MPU-6050)
 Wire.endTransmission(true);
 Serial.println("Wrote to IMU");
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");              // print ... till not connected
  }
  Serial.println("");
  Serial.println("WiFi connected");
 }
void loop(){
 mpu_read();
 ax = (AcX-2050)/CVacc;//2050, 77, 1947 are the values ​​for the calibration of an accelerometer. 
 ay = (AcY-77)/CVacc;
 az = (AcZ-1947)/CVacc;
 gx = (GyX+270)/CVgyro;//270, 351, 136 are the values ​​for the calibration of the gyroscope. 
 gy = (GyY-351)/CVgyro;
 gz = (GyZ+136)/CVgyro;
 // calculating Amplitute vactor for 3 axis
 float Raw_Amp = pow(pow(ax,2)+pow(ay,2)+pow(az,2),0.5);
 int Amp = Raw_Amp * 10;  // multiplying by 10 because values are between 0 to 1
 Serial.println(Amp);
 if (Amp<=2 && UFTacc==false){ //if device breaks LFTacc (Lower Fall Threshold of accelerometer)
   LFTacc=true;
   Serial.println("LFTacc broke:1st step activated");
   }
 if (LFTacc==true){
   trigger1count++;
   if (Amp>=12){ //if device breaks UFTacc (Upper Fall Threshold of accelerometer)
     UFTacc=true;
     Serial.println("UFTacc broke:2nd step activated");
     LFTacc=false; trigger1count=0;
     }
 }
 if (UFTacc==true){
   trigger2count++;
   angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5); Serial.println(angleChange);
   if (angleChange>=30 && angleChange<=400){ //if orientation changes by between 80-100 degrees
     UFTgyro=true; UFTacc=false; trigger2count=0;
     Serial.println(angleChange);
     Serial.println("UFTgyro broke:3rd step activated");
       }
   }
 if (UFTgyro==true){
    trigger3count++;
    if (trigger3count>=10){ 
       angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5);
       Serial.println(angleChange); 
       if ((angleChange>=0) && (angleChange<=10)){ //if orientation changes remains between 0-10 degrees
           fall=true; UFTgyro=false; trigger3count=0;
           Serial.println(angleChange);
             }
       else{ //if user regained normal orientation
          UFTgyro=false; trigger3count=0;
          Serial.println("3rd step deactivated");
       }
     }
  }
 if (fall==true){ //if fall has been detected
   Serial.println("FALL HAS BEEN DETECTED");
   send_event("fall_detect"); 
   fall=false;
   }
 if (trigger2count>=6){ //allow 0.5s delay for orientation change
   UFTacc=false; trigger2count=0;
   Serial.println("2nd step deactivated");
   }
 if (trigger1count>=6){ //allow 0.5s delay for AM to break upper threshold
   LFTacc=false; trigger1count=0;
   Serial.println("1st step deactivated");
   }
  delay(100);
   }
void mpu_read(){
 Wire.beginTransmission(MPU_addr);
 Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
 Wire.endTransmission(false);
 Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
 AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
 AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
 AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
 Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
 GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
 GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
 GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
 }
void send_event(const char *event)
{
  Serial.print("Connecting to "); 
  Serial.println(host);
    // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection failed");
    return;
  }
    // We now create a URI for the request
  String url = "/trigger/";
  url += event;
  url += "/with/key/";
  url += privateKey;
  Serial.print("Requesting URL: ");
  Serial.println(url);
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  while(client.connected())
  {
    if(client.available())
    {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    } else {
      // No data yet, wait a bit
      delay(50);
    };
  }
  Serial.println();
  Serial.println("closing connection");
  client.stop();
}
