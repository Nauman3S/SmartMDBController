#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

uint8_t MQTTstatus=0;
uint8_t mqttStatus(){
 return MQTTstatus; 
}

//broker used: broker.hivemq.com
const char* resetMDBT="mdb/reset";//send anything to this topic and it will reset the MDB bus
const char* MDBState="mdb/state";//send anything to it
const char* MDBInfo="mdb/info";//send anything to it
const char* responseV="mdb/response";//subscribe to this topic to get the info

const char* invokeVendT="mdb/invoke";//master invoke function to start vend with ammount; send an ammount to it


const char* cancelSessionT="mdb/cancel";//send anything to it to cancel
const char* startSessionT="mdb/startSession";//send an amount to it to start a session
const char* approveVendT="mdb/approveVend";//send the same ammount to approve the vending session
