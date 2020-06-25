#include <Adafruit_Sensor.h>
#include <DHT.h>

#include <WiFiUdp.h>
#include <WifiLocation.h>
#include <NTPClient.h>

#include <ESP8266WiFi.h>

#include <Firebase.h>
#include <FirebaseArduino.h>
#include <FirebaseCloudMessaging.h>
#include <string>
//
//#define WIFI_SSID "AndroidAP61f8"
//#define WIFI_PASSWORD "arbe3354"

#define WIFI_SSID "Huynhptr"
#define WIFI_PASSWORD "62626262"
#define FIREBASE_HOST "controlarduino-6c362.firebaseio.com"
#define FIREBASE_AUTH "IUYVzuJnTydTT4TRwptOuP2QJ5Eyzzrzcffi0Mzr"
#define R1 8200
#define Vin 5
#define LED_PIN1 15// là chân D6
#define LED_PIN2 0// chân D7
#define M 100000

 
//String pos="20114:106163";
String pos="21032:105796";
const int quangtro=A0;
const int DHTTYPE = DHT11;
const int DHTPIN = 2; //chân D9
unsigned long time_collection=900000;
byte mac[6];
unsigned long cycle1 = 0;
unsigned long cycle2 = 0;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(9600);
  delay(2000);
  Serial.println('\n');
  wifiConnect();
  timeClient.begin();
  timeClient.setTimeOffset(25200);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  setStatusPos();
  delay(2000);
  Firebase.stream("/status/" + pos);
  pinMode(quangtro,INPUT);
  pinMode(LED_PIN1,OUTPUT);
  pinMode(LED_PIN2,OUTPUT);
  dht.begin();
  WiFi.macAddress(mac);
  delay(2000);
}

void loop()
{  
  /**********************************************************************************/
  unsigned long curr=(unsigned long)millis();
  if (curr - cycle1 > time_collection){
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    timeClient.update();
    String formattedDate = timeClient.getFormattedDate();
    /***********************************/
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float ADC=analogRead(quangtro);
//    Serial.println(Vin);
//    float Vout = ADC * (Vin / 1024.0);
//    Serial.println(Vout);
//    float RLDR = (R1 * (Vin - Vout))/Vout;
    if (isnan(t) || isnan(h)) {
        Serial.println("Failed to read from DHT sensor!");
        if (isnan(t)) t = M;
        if (isnan(h)) h = M;
    }
    root["mac_add"]=binToString(mac,6);
    root["temp"]=t;
    root["lux"]=ADC;
    root["humidity"]=h;
    /*************************************/
    String jsonStr = "";
    root.printTo(jsonStr);
    String path="storage/" + pos + "/" + formattedDate;
    Firebase.set(path, root);
    if (Firebase.success())
    {
      Serial.println("SET JSON --------------------");
      Serial.println("PASSED");
      Serial.println();
    }else {
      Serial.println("SET DATA FAILED");
      Serial.println("------------------------------------");
      Serial.println();
    }
    /***********************************/
    if (Firebase.failed()) {
      Serial.println(Firebase.error());
      Serial.print("fail!");
    }
    cycle1 = curr;
  }// kết thúc chu kì gửi dữ liệu
  else if (curr-cycle2>51){
    if (Firebase.failed()) {
      Serial.println("streaming error");
      Serial.println(Firebase.error());
      Firebase.stream("/status/" + pos);
      return;
    }
    if (Firebase.available()) {
     FirebaseObject event = Firebase.readEvent();
     String eventType = event.getString("type");
     if (eventType=="put"){
       Serial.print("data: ");
       String data=event.getString("data");
       Serial.println(data);
       DynamicJsonBuffer jsonBuffer;
       JsonObject& r = jsonBuffer.parseObject(data);
       String path = r.get<String>("path");
       if (path=="/"){
        JsonObject& conf=jsonBuffer.parseObject(r.get<String>("data"));
        analogWrite(LED_PIN1,int(conf.get<float>("LED1")));
        analogWrite(LED_PIN2,int(conf.get<float>("LED2")));
        time_collection=conf.get<int>("TIME_COLLECTION");
        time_collection=conf.get<int>("TIME_COLLECTION");
       }else if (path=="/LED1"){
        analogWrite(LED_PIN1,int(r.get<float>("data")));
       }else if (path=="/LED2"){
        analogWrite(LED_PIN2,int(r.get<float>("data")));
       }else if (path==""){
        time_collection=r.get<int>("data");
       }
     }
    }
    cycle2 = curr;
  }
  if(WiFi.status() != WL_CONNECTED){
    wifiConnect();
  }
}

//-----------------------------------------end loop---------------------
String binToString(byte *inputData, int dataLength) {
  char asciiString[dataLength*2 +1];   // 2 characters per byte plus a null at the end.
 
  for (int i = 0; i<dataLength; i++) {
    sprintf(asciiString+2*i,"%02X",*(inputData+i));
  }
 
  asciiString[dataLength*2] = 0; // in theory redundant, the last sprintf should have done this but just to be sure...
  return String(asciiString);
}

//*****************************************************************************
void wifiConnect()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID); Serial.println(" ...");

  int teller = 0;
  while (WiFi.status() != WL_CONNECTED)
  {                                       // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++teller); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
}
//*************************************************************************************
void setStatusPos(){
  if (Firebase.get("/status/" + pos).isNullString()){
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["LED1"]=0;
    root["LED2"]=0;
    root["TIME_COLLECTION"]=time_collection;
    Firebase.set("/status/" + pos, root);
    if (Firebase.success())
    {
      Serial.println("SET JSON STATUS");
      Serial.println("PASSED");
      Serial.println();
    }else {
      Serial.println("SET STATUS FAILED");
      Serial.println("------------------------------------");
      Serial.println();
    }
    /***********************************/
    if (Firebase.failed()) {
      Serial.println(Firebase.error());
      Serial.print("set status fail!");
    }
  }
}
