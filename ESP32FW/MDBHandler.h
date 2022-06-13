
void setupMDB();
void sendV(String val);
String getResponse();
int startSession(int price);//simulate the cashless vending with arg price
                        //and return 1==success and 0==failure 
void checkResponse();
void mdbInfo();
String getResponse();
void approveVend(String price);
void cancelSession();
void mdbState();
void vendInvoker(String price);

String currentState="";
String rawData="";
uint8_t serialListnerFlag=0;
void resetMDB();


char receivedChar;
char startC='@';
char endC='*';
uint8_t dataIncoming=0;
uint8_t dataAvailable=0;

String dataD="";
void setupMDB(){

  Serial.begin(38400);
  //pinMode(4,OUTPUT);
  //digitalWrite(4,HIGH);

 
}
void sendV(String val){
  //send value to serial output with \r\n appended
  Serial.println(val);
}
void checkResponse(){
//getResponse and set currentState=Response
  if(Serial.available()){
     receivedChar = Serial.read();
     if(receivedChar==startC){
      dataIncoming=1;
      receivedChar = Serial.read();//to skip the initail '@'
     }
     else if(receivedChar==endC){
      dataIncoming=0;
      dataAvailable=1;

     }
     if(dataIncoming==1){
      if(receivedChar!='\r' or receivedChar!='\n' or receivedChar!='@'){
        dataD=dataD+String(receivedChar);
      }
     }
  }
  if(dataD!=""){
  //dataAvailable=1;
    //Serial.println(dataD);
//    if(dataD.indexOf(String("IO:"))){
//     digitalWrite(2,HIGH); 
//    }
//    if(dataD.indexOf(String("PO:"))){
//     digitalWrite(2,LOW); 
//    }
  }
}
String getResponse(){
//getResponse and set currentState=Response
  String localData="";
  if(dataAvailable==1){
    localData=dataD;
    dataAvailable=0;
    dataD="";
    return localData;
  }
  else{
    return String("null");
  }
  
}
void mdbState(){
  sendV("mdb-state");
}
void cancelSession(){
  sendV("cancel-session");
}
void approveVend(String price){
  String vendS="approve-vend "+(price);
  sendV(vendS);

}
void mdbInfo(){
  sendV("info");
}
int startSession(String price){
  String vendS="start-session "+ (price);
  sendV(vendS);
  
}

void vendInvoker(String price, String id, String paymentM){
    String vendS="invokevend "+ (price)+String(" ")+id+String(" ")+paymentM;
    sendV(vendS);
}

void resetMDB(){
  String vendS="reset";
    sendV(vendS);
   // digitalWrite(4,LOW);
    //vTaskDelay(100);
    //digitalWrite(4,HIGH);
}
 
