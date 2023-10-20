#include <Arduino.h>
#include "pin_manager.h"
#include <LoRa.h>
#include <SPI.h>
#include <FirebaseESP32.h>

//CREDENCIALES E INICIALIZACION

const char* ssid = "EduardoCB";
const char* password = "123456780";

// Credenciales Firebase
#define FIREBASE_HOST "database-29ad3-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyBIJ2KdgXFT5tqaaH0ebmG2qYDoDeuKxAo"

FirebaseData firebaseData;




const int csPin = 14;          // LoRa radio chip select
const int resetPin = 15;       // LoRa radio reset
const int irqPin = 22;         // change for your board; must be a hardware interrupt pin

String outgoing;              // outgoing message

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBC;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 4000;          // interval between sends


float Text;
float Hext;  
float Tint;    
float Hint;
float Vviento; 

int AutomaticoAnterior=2, EncenderAnterior=2;



void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }


  Text    = incoming.substring(incoming.indexOf('T')+4).toFloat();
  Hext    = incoming.substring(incoming.indexOf('H')+4).toFloat();
  Tint    = incoming.substring(incoming.indexOf('t')+4).toFloat();
  Hint    = incoming.substring(incoming.indexOf('h')+4).toFloat();
  Vviento = incoming.substring(incoming.indexOf('V')+4).toFloat();

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }



  // if message is for this device, or broadcast, print details:
  Serial.print("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  //Serial.println(Text);
  //Serial.println(Hext);
  //Serial.println(Tint);
  //Serial.println(Hint);
  //Serial.println(Vviento);

  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}




void setup() {
  Serial.begin(9600);                   // initialize serial
  while (!Serial);

  Serial.println("LoRa Duplex");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");



  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  // Espera a que Firebase se conecte
  while (Firebase.ready() != 1) {
    delay(100);
  }
}


int readAutomFirebase(){
  bool aux = Firebase.getBool(firebaseData, "vdetonantes/selector");
  if (firebaseData.boolData()) {
    return 1;
  } else {
    return 0;
  }
}

int readEncenFirebase(int autom){
  if(autom == 1){
    return 0;
  }
  else{
    bool aux = Firebase.getBool(firebaseData, "vdetonantes/encender");
      if (firebaseData.boolData()) {
        return 1;
      } else {
        return 0;
      }
  }
}


void enviarDatosFirebase() {
  
  Firebase.setFloat(firebaseData,"vdetonantes/Text", Text);
  Firebase.setFloat(firebaseData,"vdetonantes/Hext", Hext);
  Firebase.setFloat(firebaseData,"vdetonantes/Tint", Tint);
  Firebase.setFloat(firebaseData,"vdetonantes/Hint", Hint);
  Firebase.setFloat(firebaseData,"vdetonantes/viento", Vviento);
  
}


void decideandwriteMessage(int automa, int encend){

  if(automa!=AutomaticoAnterior || encend!=EncenderAnterior){
    char buff[30];
    sprintf(buff, "AUTO : %d | Encendido : %d",automa, encend);
    String message = String(buff);
    sendMessage(message);
    Serial.print("Sending " + message);
    Serial.println("  with messageid: "+msgCount);
    AutomaticoAnterior = automa;
    EncenderAnterior = encend;
  }
  
}




void loop() {
  if (millis() - lastSendTime > interval) {
    int autom = readAutomFirebase();
    int encen = readEncenFirebase(autom);
    decideandwriteMessage(autom, encen);
    enviarDatosFirebase();
    lastSendTime = millis();            // timestamp the message
    //interval = random(2000) + 1000;    // 2-3 seconds
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}



