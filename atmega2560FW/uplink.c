

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <inttypes.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "mdb.h"
#include "uplink.h"
#include "txSwitch.h"
#include "vendSessionStates.h"
#include <util/delay.h>
//#include <util/delay.h>

uint8_t timerVal = 0;
uint8_t timerPassV = 0;
uint8_t processComplete = 0;
uint8_t paymentMethod = 0; //0==easytickets; 1==jazzcash; 2==simsim

char *vendValue;
cmdStruct_t CMD_LIST[] = {
    {"reset", cmd_reset},
    {"help", cmd_help},
    {"info", cmd_info},
    {"mdb-state", cmd_get_mdb_state},
    {"start-session", cmd_start_session},
    {"approve-vend", cmd_approve_vend},
    {"deny-vend", cmd_deny_vend},
    {"cancel-session", cmd_cancel_session},
    {"io", cmd_io},
    {"invokevend", vendInvoker},
    /*
    {"read",cmd_read},
    {"write",cmd_write},*/
    {NULL, NULL}};

extern uint8_t mdb_state;
extern uint8_t mdb_poll_reply;
extern vmcCfg_t vmc;
extern vmcPrice_t price;
extern mdbSession_t session;

char buff[20];

//uint8_t eeByte EEMEM;

void uplink_cmd_handler(void)
{

    static char cmd[20];
    static uint8_t index = 0;

    // No data received, return
    if (buffer_level(UPLINK_USART, RX) < 1)
        return;

    // flush cmd buffer if cmd is out of a valid length
    if (index == MAX_CMD_LENGTH)
    {
        index = 0;
    }

    // append char to cmd
    recv_char(UPLINK_USART, &cmd[index]);

    switch (cmd[index])
    {
    case '\r':
        //carriage return received, replace with stringtermination and parse
        send_str(UPLINK_USART, "\r\n");
        cmd[index] = '\0';
        parse_cmd(cmd);
        index = 0;
        break;
    case '\n':
        // do nothing, but avoid index from incrementing
        break;
    case '\b':
        // backspace, remove last received char
        index--;
        send_char(UPLINK_USART, '\b');
        break;
        // char is part of an ESC sequence
    case 0x1B:
    case 0x5B:
        index++;
        break;
        // each other if the last two char was not part of an ESC sequence
    default:
        if (cmd[index - 1] == 0x5B && cmd[index - 2] == 0x1B)
        {
            index = index - 2;
        }
        else
        {
            send_char(UPLINK_USART, cmd[index]);
            index++;
        }
    }
}

void parse_cmd(char *cmd)
{

    char *tmp;
    uint8_t index = 0;

    // seperate command from arguments
    tmp = strsep(&cmd, " ");
    //send_str_p(UPLINK_USART,PSTR("PARSE_CMD00\r\n"));
    // search in command list for the command
    while (strcasecmp(CMD_LIST[index].cmd, tmp))
    {
        if (CMD_LIST[index + 1].cmd == NULL)
        {
            send_str_p(UPLINK_USART, PSTR("Error: Unknown command\r\n"));
            return;
        }
        index++;
    }

    // run the command
    CMD_LIST[index].funcptr(cmd);
    return;
}

void cmd_reset(char *arg)
{
    send_str_p(UPLINK_USART, PSTR("@RESETTING DEVICE*\r\n"));
    RESET();
}

void cmd_io(char *arg)
{
    //uint16_t sizeStr=strlen(arg);

    uint16_t argV = atoi(arg);
    uint16_t pinNumber = atoi(arg) / 100;
    uint16_t pinState = atoi(arg) % 100;

    /*
   working snippet
    playerBegin();
    volume(30);
     play(1);*/

    ///format io 1200 to turn 12th pin off
    ///////   io 1201 to turn 12th pin on

    /* if(argV==0){
   saveVendState(VEND_DEFAULT);    
       
   }
   else if(argV==1){
       saveVendState(VEND_SUCCESS_FLAG);
       
   }
   else if(argV==2){
       saveVendState(VEND_FAILURE_FLAG);
   } */
    if (pinNumber == 12)
    {
        if (pinState == 0)
        {
            send_str_p(UPLINK_USART, PSTR("@IO: PIN12 OFF*\r\n"));

            PORTB = PIN12_OFF;
        }
        else if (pinState == 1)
        {
            send_str_p(UPLINK_USART, PSTR("@PO: PIN12 ON*\r\n"));

            PORTB = PIN12_ON;
        }
        else if (pinState == 2)
        {
            send_str_p(UPLINK_USART, PSTR("@IO: PLAY3*\r\n"));
        }
        else if (pinState == 3)
        {
            send_str_p(UPLINK_USART, PSTR("@IO: PLAY4*\r\n"));
        }
    }
}
void startTimer()
{
    timerVal = 1;
}
void stopTimer()
{
    timerVal = 0;

    timerPassV = 0;
    vendValue = "0";
}
uint8_t getTimerState()
{
    return timerVal;
}
void incrementTimerPass()
{

    timerPassV = timerPassV + 1;
}
uint8_t getTimerPass()
{

    return timerPassV;
}
void liveVendMonitor()
{
    //not needed yet
}
void resetDevice()
{
    stopTimer();
    RESET();
}

void timeHandler(uint8_t v)
{

    if (v == 0)
    {

        // First Time Play the payment accepted from easytickets
        // And Start Session
        if (paymentMethod == 1)
        {
            //payment accepted
        }
        else if (paymentMethod == 2)
        {
            //payment accepted
        }
        else if (paymentMethod == 3)
        {
            //payment accepted
        }
        cmd_start_session(vendValue);
    }

    if (v == 3)
    {
        // At this State we Assume that the user has already made selection
        // Before the Payment
        cmd_approve_vend(vendValue);
    }

    if (v == 9)
    {

        // Lets Check the State
        if (vendStateT == 1)
        {

            //Success Case

            send_str_p(UPLINK_USART, PSTR("@vend-success*\r\n"));
        }
        else
        {

            // Failed Case So play Please Select the Column
            // And Wait for 20 Sec again
        }
    }

    if (v == 12)
    {
        if (vendStateT == 1)
        {
            resetDevice();
        }
    }

    ////////////////////////////////////////Case2//////////////////////////////////

    if (v == 15)
    {
        //please wait
    }

    if (v == 19)
    {
        //please wait
    }

    if (v == 23)
    {
        //please wait
    }

    if (v == 27)
    {

        // At this point we assume that the user has made some Selection
        // So we just Appove the vend and see in v==9 wether the vend is
        // success or not

        cmd_approve_vend(vendValue);
    }

    if (v == 31)
    {
        //please wait
    }

    if (v == 33)
    {

        // lets check the state

        if (vendStateT == 1)
        {

            // Success Case

            //success
            send_str_p(UPLINK_USART, PSTR("@vend-success*\r\n"));
            //_delay_ms(3000);
            //resetDevice();
        }
        else
        {

            // Denied case

            //denied
            send_str_p(UPLINK_USART, PSTR("@vend-failed*\r\n"));
            //_delay_ms(3000);
            //resetDevice();
        }
    }

    if (v == 38)
    {
        resetDevice();
    }

    if (v == 42)
    {
        resetDevice(); //second reset try if first try didn't worked
    }

    incrementTimerPass();
}

/* void timeHandler(uint8_t v){
    //  char buf[40];
    
    
    if(v==0){
        //SoundPlayerWakeUp();
    
    
    play(1);//payment-accepted-from1582628071.mp3
    
    cmd_start_session(vendValue);
   
  
    //send_str_p(UPLINK_USART, PSTR("###VENDINVOKED-R RUNNING###\r\n"));
    }
    if(v==1){
        cmd_approve_vend(vendValue);   

    }

     if(v==2){
        if(vendStateT==1){
             play(3);//vend-successful-please-collect-your1582628159.mp3
             //_delay_ms(3000);
            //clearLastVendState();
            //success
           // stopTimer;
            send_str_p(UPLINK_USART, PSTR("###VENDINVOKED-S SUCCESS###\r\n"));
            //RESET();
            //return;

        }
        else{
            //play(2);//please select
            //_delay_ms(3000);
            //RESET();
        }
       
        
    }
    if(v==4){
        if(vendStateT==1){
            stopTimer();
            RESET();
        }
    }

     if(v==6){
        //v=7 is 20 seconds
         if(mdb_state==MDB_SESSION_IDLE || mdb_state==MDB_VENDING ){

             cmd_approve_vend(vendValue);
             
             
         }
         

    }
     if(v==7){

        if(vendStateT==1){
            // play(2);
            play(3);
            //clearLastVendState();
       /// cmd_approve_vend(vendValue);
        stopTimer();
        send_str_p(UPLINK_USART, PSTR("###VENDINVOKED-S SUCCESS###\r\n"));
        //RESET();
      //  return;
        }
        else{
                //reversal
               play(4);//vend-denied-payment-will1582628200.mp3
                cmd_deny_vend(vendValue);
              //  clearLastVendState();


                //stopTimer();

                send_str_p(UPLINK_USART, PSTR("###VENDINVOKED-R REVERSAL###\r\n"));
                
                //_delay_ms(4000);
                //RESET();
                //return;
        }
        

        
    }
    if(v==8){
        stopTimer();
        RESET();

        return;
    }
    
    
    
    
   
    incrementTimerPass();
    

    
    
    
} */
void vendInvoker(char *arg)
{
    uint16_t argV = 0; //atoi(arg);

    char delim[] = " ";
    char *ptr = strtok(arg, delim);

    int a = 0; //vend value
    int b = 0; //transaction id
    int c = 0; //payment method
               //   char* vendVal;
    char buf[40];

    for (int i = 0; i <= 2; i++)
    {
        if (i == 0)
        {
            vendValue = ptr;
            a = atoi(ptr);
            argV = a;
        }
        else if (i == 1)
        {
            ptr = strtok(NULL, delim);
            b = atoi(ptr);
        }
        else if (i == 2)
        {
            ptr = strtok(NULL, delim);
            c = atoi(ptr);
            paymentMethod = c;
        }
    }
    sprintf(buf, "a==%d b==%d c==%d \r\n", a, b, c);
    send_str(UPLINK_USART, buf);

    // vendValue=arg;

    char buf00[40];
    sprintf(buf00, "LAST STATE=%d\r\n", getLastVendState());
    send_str(UPLINK_USART, buf00);

    startTimer();

    //char buf[40];
    sprintf(buf, "VendInvoked with Ammount:  %d\r\n", argV);
    send_str(UPLINK_USART, buf);
}

void cmd_help(char *arg)
{
    send_str_p(UPLINK_USART, PSTR("-----------------------------------------------\r\n"));
    send_str_p(UPLINK_USART, PSTR("reset:\r\n   reset the Arduino\r\n"));
    send_str_p(UPLINK_USART, PSTR("info:\r\n   shows the VMC infos transfered during the setup process\r\n"));
    send_str_p(UPLINK_USART, PSTR("mdb-state:\r\n   displays the current MDB state.\r\n"));
    send_str_p(UPLINK_USART, PSTR("start-session <funds>:\r\n   starts a session with <funds> Euro Cents.\r\n"));
    send_str_p(UPLINK_USART, PSTR("approve-vend <vend-amount>:\r\n   approves a vend request with <vend-amount> Euro Cents.\r\n"));
    send_str_p(UPLINK_USART, PSTR("deny-vend:\r\n   denies a vend request.\r\n"));
    send_str_p(UPLINK_USART, PSTR("-----------------------------------------------\r\n"));
}

void cmd_info(char *arg)
{
    if (mdb_state >= MDB_ENABLED)
    {
        char buffer[40];
        send_str_p(UPLINK_USART, PSTR("@-----------------------------------------------\r\n"));
        send_str_p(UPLINK_USART, PSTR("## VMC configuration data ##\r\n"));
        sprintf(buffer, "VMC feature level:       %d\r\n", vmc.feature_level);
        send_str(UPLINK_USART, buffer);
        sprintf(buffer, "VMC display columns:     %d\r\n", vmc.dispaly_cols);
        send_str(UPLINK_USART, buffer);
        sprintf(buffer, "VMC display rows:        %d\r\n", vmc.dispaly_rows);
        send_str(UPLINK_USART, buffer);
        sprintf(buffer, "VMC display info:        %d\r\n", vmc.dispaly_info);
        send_str(UPLINK_USART, buffer);
        send_str_p(UPLINK_USART, PSTR("##    VMC price range     ##\r\n"));
        sprintf(buffer, "Maximum price:           %d\r\n", price.max);
        send_str(UPLINK_USART, buffer);
        sprintf(buffer, "Minimum price:           %d\r\n", price.min);
        send_str(UPLINK_USART, buffer);
        send_str_p(UPLINK_USART, PSTR("-----------------------------------------------*\r\n"));
    }
    else
    {
        send_str_p(UPLINK_USART, PSTR("Error: Setup not yet completed!\r\n"));
    }
}

void cmd_get_mdb_state(char *arg)
{
    char buf[40];
    switch (mdb_state)
    {
        sprintf(buf, "@MDB_STATE=%d* \r\n", mdb_state);
        send_str(UPLINK_USART, buf);
        send_str(UPLINK_USART, buf);
    case MDB_INACTIVE:
        send_str_p(UPLINK_USART, PSTR("@State: INACTIVE*\r\n"));
        break;
    case MDB_DISABLED:
        send_str_p(UPLINK_USART, PSTR("@State: DISABLED*\r\n"));
        break;
    case MDB_ENABLED:
        send_str_p(UPLINK_USART, PSTR("@State: ENABLED*\r\n"));
        break;
    case MDB_SESSION_IDLE:
        send_str_p(UPLINK_USART, PSTR("@State: SESSION IDLE*\r\n"));
        break;
    case MDB_VENDING:
        send_str_p(UPLINK_USART, PSTR("@State: VEND*\r\n"));
        break;
    case MDB_REVALUE:
        send_str_p(UPLINK_USART, PSTR("@State: REVALUE*\r\n"));
        break;
    case MDB_NEGATIVE_VEND:
        send_str_p(UPLINK_USART, PSTR("@State: NEGATIVE VEND*\r\n"));
        break;
    }
}

void cmd_start_session(char *arg)
{
    if (mdb_state == MDB_ENABLED)
    {
        if (session.start.flag == 0)
        {
            session.start.flag = 1;
            //          char abc[40];
            //            sprintf(abc,"cmd-session-start-000-%c\r\n",arg);
            send_str_p(UPLINK_USART, PSTR("cmd start session00"));
            session.start.funds = atoi(arg);
            mdb_poll_reply = MDB_REPLY_BEGIN_SESSION;
        }
        else
        {
            send_str_p(UPLINK_USART, PSTR("Error: Session is already running\r\n"));
        }
    }
    else
    {
        send_str_p(UPLINK_USART, PSTR("Error: MateDealer not ready for a session\r\n"));
    }
}

void cmd_approve_vend(char *arg)
{
    if (mdb_state == MDB_VENDING)
    {
        session.result.vend_approved = 1;
        session.result.vend_amount = atoi(arg);
        char ac[30];
        sprintf(ac, "cmd-approved %d", atoi(arg));
        send_str(UPLINK_USART, ac);
        mdb_poll_reply = MDB_REPLY_VEND_APPROVED;
    }
    else
    {
        send_str_p(UPLINK_USART, PSTR("Error: MateDealer is not in a suitable state to approve a vend\r\n"));
    }
}

void cmd_deny_vend(char *arg)
{
    if (mdb_state == MDB_VENDING)
    {
        session.result.vend_denied = 1;
        mdb_poll_reply = MDB_REPLY_VEND_DENIED;
    }
    else
    {
        send_str_p(UPLINK_USART, PSTR("Error: MateDealer is not in a suitable state to deny a vend\r\n"));
    }
}

void cmd_cancel_session(char *arg)
{
    if (mdb_state == MDB_SESSION_IDLE)
    {
        mdb_poll_reply = MDB_REPLY_SESSION_CANCEL_REQ;
    }
    else
    {
        send_str_p(UPLINK_USART, PSTR("Error: MateDealer is not in a suitable state to cancel a session\r\n"));
    }
}
