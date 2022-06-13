

#include <WiFi.h>
#include <PubSubClient.h>

#include "constsV.h"
#include "SStack.h"
#include "MDBHandler.h"
#include "MQTTHandler.h"

#define BUILTIN_LED 2
// Update these with values suitable for your network.
const char* ssid = "YOUR WIFI NAME";
const char* password = "YOUR WIFI PASSWORD";


const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String payloadV="";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadV=payloadV+String((char)payload[i]);
  }
  Serial.println();
  if(String(topic)==String(resetMDBT)){
    resetMDB();//reset MDB devices
  }
  if(String(topic)==String(MDBState)){
    mdbState();
    String res="Commiting MDBState";
    publishValues(String(responseV),res);
  }
  if(String(topic)==String(MDBInfo)){
    mdbState();
    String res="Commiting MDBInfo";
    publishValues(String(responseV),res);
  }

  if(String(topic)==String(invokeVendT)){
    
    String res="Invoking vend with ammount " + payloadV;
    vendInvoker(payloadV, "0","1");//invoke vend with ammount
    publishValues(String(responseV),res);
  }

  if(String(topic)==String(startSessionT)){
    
    String res="Starting session with ammount " + payloadV;
    startSession(payloadV);
    publishValues(String(responseV),res);
  }
  if(String(topic)==String(approveVendT)){
    
    String res="Approving session with ammount " + payloadV;
    approveVend(payloadV);
    publishValues(String(responseV),res);
    
  }
  if(String(topic)==String(cancelSessionT)){
    
    String res="Cancelling session";
    cancelSession();
    publishValues(String(responseV),res);
  }
  
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void publishValues(String topic, String msg){
      client.publish(topic.c_str(), msg.c_str()); 
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      MQTTstatus=1;
      // Once connected, publish an announcement...
      
      // ... and resubscribe
      client.subscribe(resetMDBT);
      client.subscribe(MDBState);
      client.subscribe(MDBInfo);
      client.subscribe(invokeVendT);
      client.subscribe(cancelSessionT);
      client.subscribe(startSessionT);
      client.subscribe(approveVendT);
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  //setupADC();
  setupMDB();
  Serial.print("WiFi ST Mac: ");
  MAC=String(WiFi.macAddress());
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  checkResponse();
  String gR=getResponse();
  if(gR!=String("null")){
    
    String res=gR;//"@IO: HIGH*";
    publishValues(String(responseV),res);
    if(gR.indexOf("vend-success")>=0){
      
       Serial.println("Vend Successful");
       
    }
    if(gR.indexOf("vend-failed")>=0){

      Serial.println("Vend Failed");
      
    }

  }


  unsigned long now = millis();
  if (now - lastMsg > 2000) {//data update frequency == 2000mSec or 2sec
    lastMsg = now;

    // put normal processing code here
    
  }
}
