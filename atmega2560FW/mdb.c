
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include "usart.h"
#include "uplink.h"
#include "mdb.h"
#include "txSwitch.h"
#include "vendSessionStates.h"

#define TXSWITCH 0
uint8_t MDB_USART_TX_READY = 1;

uint8_t mdb_state = MDB_INACTIVE;
uint8_t mdb_poll_reply = MDB_REPLY_ACK;
uint8_t mdb_active_cmd = MDB_IDLE;

uint8_t reset_done = FALSE;

extern volatile uint8_t cmd_var[MAX_VAR];

vmcCfg_t vmc = {0, 0, 0, 0};
vmcPrice_t price = {0, 0};

int m = 0;

int statePos()
{

    return m;
}

cdCfg_t cd = {
    0x01,   // Reader CFG (constant)
    0x02,   // Feature Level [1,2,3]   ////feature level of nayax is 2
    0x01CA, // Country Code,,,1458MYR  ISO 4217 Country code
    0x01,   // Scale Factor
    0x00,   // Decimal Places
    0x07,   // max Response Time
    0x0D    // Misc Options/        //org 00
};

Stage3DataTest s3dT = {
    0x00};

Stage3Data s3d = {
    0x09,
    0x4E,
    0x59,
    0x58,
    0x30,
    0x30,
    0x30,
    0x30,
    0x30,
    0x30,
    0x32,
    0x36,
    0x33,
    0x33,
    0x32,
    0x30,
    0x44,
    0x4D,
    0x58,
    0x20,
    0x2D,
    0x20,
    0x32,
    0x30,
    0x31,
    0x31,
    0x20,
    0x20,
    0x01,
    0x00,
    0x1B3

};

// cdCfg_t cd = {
//     0x01,   // Reader CFG (constant)
//     0x15,   // Feature Level [1,2,3]
//     0x01CA, // // Country Code,,,1458MYR  ISO 4217 Country code
//     0x01,   // Scale Factor
//     0x00,   // Decimal Places
//     0x10,   // max Response Time
//     0x00    // Misc Options
//     };

mdbSession_t session = {
    {0, 0},
    {0, 0, 0}};

void mdb_cmd_handler(void)
{

    switch (mdb_active_cmd)
    {

    case MDB_IDLE:
        // Wait for enough data in buffer
        if (buffer_level(MDB_USART, RX) < 2)
            return;

        uint16_t data = recv_mdb(MDB_USART);

        // if modebit is set and command is in command range for cashless device
        if ((data & 0x100) == 0x100 && MDB_RESET <= (data ^ 0x100) && (data ^ 0x100) <= MDB_READER)
        {

            mdb_active_cmd = (data ^ 0x100);

            if (!reset_done && mdb_active_cmd != MDB_RESET)
            {
                mdb_active_cmd = MDB_IDLE;
                send_str_p(UPLINK_USART, PSTR("resetting\n"));
                //flag=0,1
            }
        }
        break;

    case MDB_RESET:
        mdb_reset();
        break;

    case MDB_SETUP:
        mdb_setup();
        break;

    /*case MDB_STAGE3:
            mdb_stage3();
        break;
*/
    case MDB_POLL:
        mdb_poll();
        break;

    case MDB_VEND:
        mdb_vend();
        break;

    case MDB_READER:
        mdb_reader();
        break;
    }
}

void mdb_reset(void)
{
#if TXSWITCH == 1
    txSwitchState(PIN12, 0);
#endif
    // Wait for enough data in buffer to proceed reset
    if (buffer_level(MDB_USART, RX) < 2)
        return;

#if DEBUG == 1
    send_str_p(UPLINK_USART, PSTR("RESET\r\n"));
#endif

    // validate checksum
    if (recv_mdb(MDB_USART) != MDB_RESET)
    {
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;
        send_str_p(UPLINK_USART, PSTR("Error: invalid checksum for [RESET]\r\n"));
#if TXSWITCH == 1
        txSwitchState(PIN12, 0);
#endif
        return;
    }

    // Reset everything
    vmc.feature_level = 0;
    vmc.dispaly_cols = 0;
    vmc.dispaly_rows = 0;
    vmc.dispaly_info = 0;
    price.max = 0;
    price.min = 0;

    // Send ACK
    send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY);
    reset_done = TRUE;
    mdb_state = MDB_INACTIVE; //orignal
    //mdb_state= MDB_ENABLED;///not orignal
    mdb_active_cmd = MDB_IDLE;

    mdb_poll_reply = MDB_REPLY_JUST_RESET;
}

void mdb_setup(void)
{

    static uint16_t checksum = MDB_SETUP;
    static uint8_t state = 0;
    uint8_t data[6] = {0, 0, 0, 0, 0, 0};
    uint8_t index = 0;

    // txSwitchState(PIN12,0);

    if (state < 2)
    {
        // Wait for enough data in buffer
        if (buffer_level(MDB_USART, RX) < 12)
            return;

        // fetch the data from buffer
        for (index = 0; index < 6; index++)
        {
            data[index] = (uint8_t)recv_mdb(MDB_USART);
        }

        // calculate checksum

        checksum += data[0] + data[1] + data[2] + data[3] + data[4];

        checksum = checksum & 0xFF;
        // validate checksum
        char abc[40];
        sprintf(abc, "data[5]: %#08x ;;checksum-calc: %#08x\r\n", data[5], checksum);
        send_str(UPLINK_USART, abc);
        if (checksum != data[5])
        {
            state = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            checksum = MDB_SETUP; //MDB_SETUP;

            send_str_p(UPLINK_USART, PSTR("Error: invalid checksum [SETUP]\r\n"));
#if TXSWITCH == 1
            txSwitchState(PIN12, 0);
#endif
            return;
        }

        state = data[0];
    }

    // Switch setup state
    switch (state)
    {

    // Stage 1 - config Data
    case 0:

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("SETUP STAGE 1\r\n"));
#endif

        // store VMC configuration data
        vmc.feature_level = data[1];
        vmc.dispaly_cols = data[2];
        vmc.dispaly_rows = data[3];
        vmc.dispaly_info = data[4];

        // calculate checksum for own configuration
        checksum = ((cd.reader_cfg +
                     cd.feature_level +
                     (cd.country_code >> 8) +
                     (cd.country_code & 0xFF) +
                     cd.scale_factor +
                     cd.decimal_places +
                     cd.max_resp_time +
                     cd.misc_options) &
                    0xFF) |
                   0x100;

        // Send own config data
        send_mdb(MDB_USART, cd.reader_cfg, MDB_USART_TX_READY);
        send_mdb(MDB_USART, cd.feature_level, MDB_USART_TX_READY);
        send_mdb(MDB_USART, (cd.country_code >> 8), MDB_USART_TX_READY);
        send_mdb(MDB_USART, (cd.country_code & 0xFF), MDB_USART_TX_READY);
        send_mdb(MDB_USART, cd.scale_factor, MDB_USART_TX_READY);
        send_mdb(MDB_USART, cd.decimal_places, MDB_USART_TX_READY);
        send_mdb(MDB_USART, cd.max_resp_time, MDB_USART_TX_READY);
        send_mdb(MDB_USART, cd.misc_options, MDB_USART_TX_READY);
        send_mdb(MDB_USART, checksum, MDB_USART_TX_READY);

        state = 2;

        // reset checksum for next stage
        checksum = MDB_SETUP; //MDB_SETUP;
        return;

        break;

    // Stage 2 - price data
    case 1:

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("SETUP STAGE 2\r\n"));
#endif
        //mdb_state = MDB_ENABLED;///not in orignal
        // store VMC price data
        price.max = (data[1] << 8) | data[2];
        price.min = (data[3] << 8) | data[4];

        // send ACK
        send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY); /////*uncomment it

        // Set MDB State
        ////Orignal MDB_DISABLED
        mdb_state = MDB_ENABLED; ////////////try changing to ENABLED////orignal disabled
        //reset_done=TRUE;//////this line is not in orignal code
        state = 0; //orignal 0

        checksum = MDB_SETUP; //MDB_SETUP; //orginal is 17 in decimal
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;
        //mdb_poll_reply = MDB_REPLY_DISPLAY_REQ;

        //mdb_state = MDB_ENABLED;////not in orignal
        char abc2[30];
        sprintf(abc2, "stage2checksum: %#08x mdb_poll_reply : %#08x\r\n", checksum, mdb_poll_reply);
        send_str(UPLINK_USART, abc2);

        m = 1;
        mdb_stage3();

        return;
        break;

    // ACK from VMC for MateDealer cfg data
    case 2:
        // Wait for enough data in buffer
        if (buffer_level(MDB_USART, RX) < 2)
            return;

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("SETUP WAIT FOR ACK\r\n"));
//txSwitchState(PIN12,0);
#if TXSWITCH == 1
        txSwitchState(PIN12, 0); ////FLAG0
#endif
            //MDB_USART_TX_READY=0;
#endif

        // Check if VMC sent ACK
        data[0] = recv_mdb(MDB_USART);

        /*
             * The following check if VMC answers with ACK to the Setup data we send is not as in the MDB Spec defined.
             * The Sanden Vendo VDI 100-5 send the setup request twice, and ACK with 0x000 first time 
             * (as in the spec!) and 0x001 the second time !? 
             */
        ///if(data[0] != 0x000 && data[0] != 0x001) { //org

        if (data[0] != 0x000 && data[0] != 0x001)
        {
            state = 0; ////0 in org
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            send_str_p(UPLINK_USART, PSTR("Error: no ACK received on [SETUP]"));
            return;
        }

        state = 0; ////0 in org
        mdb_active_cmd = MDB_IDLE;

        mdb_poll_reply = MDB_REPLY_ACK;

        return;
        break;

    // Unknown Subcommand from VMC
    default:
        send_str_p(UPLINK_USART, PSTR("Error: unknown subcommand [SETUP]\r\n"));
        state = 0;

        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;
        return;
        break;
    }
}

void mdb_stage3(void)
{
    uint8_t data2[128] = {0};
    uint8_t index = 0;

    send_str_p(UPLINK_USART, PSTR("IN STAGE3\r\n"));

    //if(buffer_level(MDB_USART,RX) < 12) return;
    while (buffer_level(MDB_USART, RX) <= 62)
    {
        continue;
    }
    send_str_p(UPLINK_USART, PSTR("02 IN STAGE3\r\n"));
    // fetch the data from buffer
    for (index = 0; index < 31; index++)
    {
        data2[index] = (uint8_t)recv_mdb(MDB_USART);
    }

    //sprintf(abc, "STAGE 3 data[0]=%#08x   ,, data[1]=%#08x ,, data[2]=%#08x\r\n",data2[0],data2[1],data2[2]);
    send_str_p(0, PSTR("Sending up stage 3 data\r\n"));

    send_mdb(MDB_USART, s3d.d0, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d1, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d2, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d3, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d4, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d5, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d6, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d7, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d8, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d9, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d10, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d11, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d12, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d13, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d14, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d15, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d16, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d17, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d18, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d19, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d20, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d21, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d22, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d23, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d24, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d25, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d26, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d27, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d28, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d29, MDB_USART_TX_READY);
    send_mdb(MDB_USART, s3d.d30, MDB_USART_TX_READY);

    char abc[80];
    for (int i = 0; i < 32; i++)
    {
        sprintf(abc, "STAGE 3 DATA[%d] = %#08x", i, data2[i]);
        send_str(UPLINK_USART, abc);
    }

    //   mdb_active_cmd = MDB_IDLE;
    // mdb_poll_reply = MDB_REPLY_ACK;

    return;
}
void mdb_poll(void)
{

    static uint8_t state = 0;
    uint16_t checksum = 0;

    if (state == 0)
    {
        // Wait for enough data in buffer
        if (buffer_level(MDB_USART, RX) < 2)
            return;

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("POLL\r\n"));
#endif

        // validate checksum
        if (recv_mdb(MDB_USART) != MDB_POLL)
        {
            //    char ba[30];
            //  sprintf(ba,"recv_mdb  %d\r\n",recv_mdb(MDB_USART));
            //            send_str(UPLINK_USART,ba);
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            state = 0;

            send_str_p(UPLINK_USART, PSTR("Error: Invalid checksum [Poll]\r\n"));
            return;
        }

        state = 1;
    }

    switch (mdb_poll_reply)
    {

    case MDB_REPLY_ACK:
        // send ACK

        send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY); ////*uncomment it

        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;

        //sprintf(buf,"checksum-total-recv:      0x%08x\r\n", checksum);
        //#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("ACK00\r\n"));
        //#endif
        txSwitchState(PIN12, 1);
#if TXSWITCH == 1
        txSwitchState(PIN12, 1); ////this should be one
#endif
        //MDB_USART_TX_READY=0;
        state = 0;
        break;

    case MDB_REPLY_JUST_RESET:
        // send JUST RESET
        if (state == 1)
        {
            send_mdb(MDB_USART, 0x000, MDB_USART_TX_READY);
            send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY);
            send_str_p(UPLINK_USART, PSTR("JustREset00\r\n"));

            // {asm("ldi r30,0"); asm("ldi r31,0"); asm("ijmp");}
#if TXSWITCH == 1
            txSwitchState(PIN12, 0);
#endif
            state = 2;
            //return; //no return in orignal
        }

        // wait for the ACK
        else if (state == 2)
        {
            // wait for enough data in Buffer
            if (buffer_level(MDB_USART, RX) < 2)
                return;
            // check if VMC sent ACK
            if (recv_mdb(MDB_USART) != 0x000)
            {
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                state = 0;
                send_str_p(UPLINK_USART, PSTR("Error: no ACK received on [JUST RESET]\r\n"));
                return;
            }

            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            state = 0;
            return;
        }
        break;

    case MDB_REPLY_READER_CFG:
        // not yet implemented

        break;

    case MDB_REPLY_DISPLAY_REQ:
        // not yet implemented
        send_mdb(MDB_USART, 0x002, MDB_USART_TX_READY);
        send_mdb(MDB_USART, 0x032, MDB_USART_TX_READY);
        //sending iota
        send_mdb(MDB_USART, 0x049, MDB_USART_TX_READY);
        send_mdb(MDB_USART, 0x04F, MDB_USART_TX_READY);
        send_mdb(MDB_USART, 0x054, MDB_USART_TX_READY);
        send_mdb(MDB_USART, 0x041, MDB_USART_TX_READY);

        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;

        state = 0;
        break;

    case MDB_REPLY_BEGIN_SESSION:
        //added newly
        if (session.start.flag && state == 1)
        {

            send_mdb(MDB_USART, 0x003, MDB_USART_TX_READY);
            send_mdb(MDB_USART, (session.start.funds >> 8), MDB_USART_TX_READY);
            send_mdb(MDB_USART, (session.start.funds & 0xFF), MDB_USART_TX_READY);
            checksum = 0x003 + (session.start.funds >> 8) + (session.start.funds & 0xFF);
            checksum = (checksum & 0xFF) | 0x100;
            send_mdb(MDB_USART, checksum, MDB_USART_TX_READY);
            send_str_p(UPLINK_USART, PSTR("BeginSession00\r\n"));
            state = 2;

            //                play(1);
        }

        else if (session.start.flag && state == 2)
        {
            // wait for enough data in Buffer
            if (buffer_level(MDB_USART, RX) < 2)
                return;
            //   play(2);
            // check if VMC sent ACK
            if (recv_mdb(MDB_USART) != 0x000)
            {
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                session.start.flag = 0;
                session.start.funds = 0;
                state = 0;
                send_str_p(UPLINK_USART, PSTR("Error: no ACK received on [START SESSION]\r\n"));
                return;
            }
            session.start.flag = 0;
            session.start.funds = 0;
            mdb_state = MDB_SESSION_IDLE;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            state = 0;
            return;
        }
        break;

    case MDB_REPLY_SESSION_CANCEL_REQ:
        if (state == 1)
        {
            send_mdb(MDB_USART, 0x004, MDB_USART_TX_READY);
            send_mdb(MDB_USART, 0x104, MDB_USART_TX_READY);
            send_str_p(UPLINK_USART, PSTR("SessionCancelled00\r\n"));
            state = 2;
            // play(4);
        }
        else if (state == 2)
        {
            // wait for enough data in Buffer
            //play(4);
            if (buffer_level(MDB_USART, RX) < 2)
                return;
            // check if VMC sent ACK
            if (recv_mdb(MDB_USART) != 0x000)
            {
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                session.start.flag = 0;
                session.start.funds = 0;
                state = 0;
                send_str_p(UPLINK_USART, PSTR("Error: no ACK received on [SESSION CANCEL REQ]\r\n"));
                return;
            }
            session.start.flag = 0;
            session.start.funds = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            state = 0;
            return;
        }
        break;

    case MDB_REPLY_VEND_APPROVED:
        if (session.result.vend_approved && state == 1)
        {
            send_mdb(MDB_USART, 0x005, MDB_USART_TX_READY);
            send_mdb(MDB_USART, (session.result.vend_amount >> 8), MDB_USART_TX_READY);
            send_mdb(MDB_USART, (session.result.vend_amount & 0xFF), MDB_USART_TX_READY);
            checksum = 0x005 + (session.result.vend_amount >> 8) + (session.result.vend_amount & 0xFF);
            checksum = (checksum & 0xFF) | 0x100;
            send_mdb(MDB_USART, checksum, MDB_USART_TX_READY);
            state = 2;
            // play(5);
        }
        else if (session.result.vend_approved && state == 2)
        {
            //play(5);
            // wait for enough data in Buffer
            if (buffer_level(MDB_USART, RX) < 2)
                return;
            // check if VMC sent ACK
            if (recv_mdb(MDB_USART) != 0x000)
            {
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                session.result.vend_approved = 0;
                session.result.vend_amount = 0;
                state = 0;
                send_str_p(UPLINK_USART, PSTR("Error: no ACK received on [VEND APPROVE]\r\n"));
                return;
            }
            session.result.vend_approved = 0;
            session.result.vend_amount = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            state = 0;
            return;
        }
        break;

    case MDB_REPLY_VEND_DENIED:
        if (session.result.vend_denied && state == 1)
        {
            send_mdb(MDB_USART, 0x006, MDB_USART_TX_READY);
            send_mdb(MDB_USART, 0x106, MDB_USART_TX_READY);
            state = 2;
            //                play(4);
        }
        else if (session.result.vend_denied && state == 2)
        {
            // wait for enough data in Buffer
            // play(4);
            if (buffer_level(MDB_USART, RX) < 2)
                return;
            // check if VMC sent ACK
            if (recv_mdb(MDB_USART) != 0x000)
            {
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                session.start.flag = 0;
                session.start.funds = 0;
                session.result.vend_denied = 0;
                state = 0;
                send_str_p(UPLINK_USART, PSTR("Error: no ACK received on [VEND DENY]\r\n"));
                return;
            }
            session.start.flag = 0;
            session.start.funds = 0;
            session.result.vend_denied = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            state = 0;
            return;
        }
        break;

    case MDB_REPLY_END_SESSION:
        if (state == 1)
        {
            send_mdb(MDB_USART, 0x007, MDB_USART_TX_READY);
            send_mdb(MDB_USART, 0x107, MDB_USART_TX_READY);
            send_str_p(UPLINK_USART, PSTR("EndSession00\r\n"));
            state = 2;
        }
        else if (state == 2)
        {
            // wait for enough data in Buffer
            if (buffer_level(MDB_USART, RX) < 2)
                return;
            // check if VMC sent ACK
            if (recv_mdb(MDB_USART) != 0x000)
            {
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                state = 0;
                send_str_p(UPLINK_USART, PSTR("Error: no ACK received on [END SESSION]\r\n"));
                return;
            }
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            state = 0;
            return;
        }
        break;

    case MDB_REPLY_CANCELED:
        if (state == 1)
        {
            send_mdb(MDB_USART, 0x008, MDB_USART_TX_READY);
            send_mdb(MDB_USART, 0x108, MDB_USART_TX_READY);
            state = 2;
            //play(4);
        }
        else if (state == 2)
        {
            // wait for enough data in Buffer
            if (buffer_level(MDB_USART, RX) < 2)
                return;
            // check if VMC sent ACK
            if (recv_mdb(MDB_USART) != 0x000)
            {
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                state = 0;
                send_str_p(UPLINK_USART, PSTR("Error: no ACK received on [REPLY CANCELED]\r\n"));
                return;
            }
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            state = 0;
            return;
        }
        break;

    case MDB_REPLY_PERIPHERIAL_ID:

        break;

    case MDB_REPLY_ERROR:

        break;

    case MDB_REPLY_CMD_OUT_SEQUENCE:

        break;
    }
}

void mdb_vend(void)
{

    static uint8_t data[6] = {0, 0, 0, 0, 0, 0};
    static uint8_t state = 0;
    uint8_t checksum = MDB_VEND;
    char buffer[40];
    send_str_p(UPLINK_USART, PSTR("MDB-VEND00\r\n"));
    // wait for the subcommand
    if (state == 0)
    {
        // wait for enough data in buffer
        if (buffer_level(MDB_USART, RX) < 2)
            return;

        // fetch the subommand from Buffer
        data[0] = recv_mdb(MDB_USART);
        state = 1;
    }

    // switch through subcommands
    switch (data[0])
    {
    // vend request
    case 0:
        // wait for enough data in buffer
        if (buffer_level(MDB_USART, RX) < 10)
            return;

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("VEND REQUEST\r\n"));
#endif

        // fetch the data from buffer
        for (uint8_t i = 1; i < 6; i++)
        {
            data[i] = (uint8_t)recv_mdb(MDB_USART);
        }

        // calculate checksum
        checksum += data[0] + data[1] + data[2] + data[3] + data[4];
        checksum &= 0xFF;

        // validate checksum
        if (checksum != data[5])
        {
            state = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            checksum = MDB_VEND;
            send_str_p(UPLINK_USART, PSTR("Error: invalid checksum [VEND]\r\n"));
            return;
        }

        sprintf(buffer, "@vend-request %d;%d;%d;%d;%d*\r\n", data[1], data[2], data[3], data[4], data[5]);
        send_str(UPLINK_USART, buffer);
        send_str(UPLINK_USART, buffer);

        // send ACK
        send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY);
        state = 0;
        mdb_state = MDB_VENDING;
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;
        return;
        break;

    // vend cancel
    case 1:
        // wait for enough data in buffer
        if (buffer_level(MDB_USART, RX) < 2)
            return;

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("VEND Cancel\r\n"));
#endif

        //play(1);

        // fetch the data from buffer
        data[1] = (uint8_t)recv_mdb(MDB_USART);

        // calculate checksum
        checksum += data[0];
        checksum &= 0xFF;

        vendStateT = 2; //newly added
        //saveVendState(vendStateT);

        // validate checksum
        if (checksum != data[1])
        {
            state = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            checksum = MDB_VEND;
            send_str_p(UPLINK_USART, PSTR("Error: invalid checksum [VEND]\r\n"));
            return;
        }

        send_str_p(UPLINK_USART, PSTR("vend-cancel\r\n"));

        // send ACK
        send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY);
        state = 0;
        mdb_state = MDB_SESSION_IDLE;
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_VEND_DENIED;
        RESET(); //NOT IN ORG
        return;
        break;

    // vend success
    case 2:
        // wait for enough data in buffer
        if (buffer_level(MDB_USART, RX) < 6)
            return;
        vendStateT = 1;
        /*  vendStateT=1;//newly added 
            //saveVendState(vendStateT); */
        // play(3);
#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("VEND SUCCESS\r\n"));
#endif

        // fetch the data from buffer
        for (uint8_t i = 1; i < 4; i++)
        {
            data[i] = (uint8_t)recv_mdb(MDB_USART);
        }

        // calculate checksum
        checksum += data[0] + data[1] + data[2];
        checksum &= 0xFF;

        // validate checksum
        if (checksum != data[3])
        {
            state = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            checksum = MDB_VEND;
            send_str_p(UPLINK_USART, PSTR("Error: invalid checksum [VEND]\r\n"));
            return;
        }

        vendStateT = 1; //newly added
        ////saveVendState(vendStateT);

        sprintf(buffer, "vend-success %d\r\n", (data[1] + data[2]));
        send_str(0, buffer);

        // send ACK
        send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY);
        state = 0;
        mdb_state = MDB_SESSION_IDLE;
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;

        //RESET();///////////////////++Added
        vendStateT = 1;

        return;
        break;

    // vend failure
    case 3:
        // wait for enough data in buffer
        if (buffer_level(MDB_USART, RX) < 2)
            return;

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("VEND FAILURE\r\n"));
#endif

        // play(4);

        // fetch the data from buffer
        data[1] = (uint8_t)recv_mdb(MDB_USART);

        // calculate checksum
        checksum += data[0];
        checksum &= 0xFF;

        // validate checksum
        if (checksum != data[1])
        {
            state = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            checksum = MDB_VEND;
            send_str_p(UPLINK_USART, PSTR("Error: invalid checksum [VEND]\r\n"));
            return;
        }

        send_str_p(UPLINK_USART, PSTR("vend-failure\r\n"));

        vendStateT = 2;
        //saveVendState(vendStateT);

        // send ACK
        send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY);
        state = 0;
        mdb_state = MDB_ENABLED;
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;
        RESET(); //NOT IN ORG
        return;
        break;

    // session complete
    case 4:
        // wait for enough data in buffer
        if (buffer_level(MDB_USART, RX) < 2)
            return;

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("VEND SESSION COMPLETE\r\n"));
#endif

        vendStateT = 1;
        //saveVendState(vendStateT);

        // fetch the data from buffer
        data[1] = (uint8_t)recv_mdb(MDB_USART);

        // calculate checksum
        checksum += data[0];
        checksum &= 0xFF;

        // validate checksum
        if (checksum != data[1])
        {
            state = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            checksum = MDB_VEND;
            send_str_p(UPLINK_USART, PSTR("Error: invalid checksum [VEND]\r\n"));
            return;
        }

        send_str_p(UPLINK_USART, PSTR("session-complete\r\n"));
        vendStateT = 1;
        //saveVendState(vendStateT);
        // send ACK
        send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY);
        state = 0;
        mdb_state = MDB_ENABLED;
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK; ///orignal
                                        ////mdb_poll_reply = MDB_REPLY_END_SESSION;

        // RESET();
        vendStateT = 1;
        return;
        break;
    }
}

void mdb_reader(void)
{

    uint8_t data[2] = {0, 0};
    uint8_t index = 0;

    // wait for enough data in buffer
    if (buffer_level(MDB_USART, RX) < 4)
        return;

    // fetch the data from buffer
    for (index = 0; index < 2; index++)
    {
        data[index] = recv_mdb(MDB_USART);
    }

    // switch through subcommands
    switch (data[0])
    {
    // reader disable
    case 0:
        if (data[1] != 0x14)
        {
            send_str_p(UPLINK_USART, PSTR("Error: checksum error [READER]\r\n"));
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;
        }

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("READER DISABLE\r\n"));
#endif

        // send ACK
        send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY);
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;
        mdb_state = MDB_DISABLED;
        break;

    // reader enable
    case 1:
        if (data[1] != 0x15)
        {
            send_str_p(UPLINK_USART, PSTR("Error: checksum error [READER]\r\n"));
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;
        }

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("READER ENABLE\r\n"));
#endif
        txSwitchState(PIN12, 1);
        // send ACK
        send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY);
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;
        mdb_state = MDB_ENABLED;
        break;

    // reader cancel
    case 2:
        if (data[1] != 0x16)
        {
            send_str_p(UPLINK_USART, PSTR("Error: checksum error [READER]\r\n"));
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;
        }

#if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("READER CANCEL\r\n"));
#endif

        // send ACK
        send_mdb(MDB_USART, 0x100, MDB_USART_TX_READY);
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_CANCELED;
        mdb_state = MDB_ENABLED;
        break;

    // unknown subcommand
    default:
        send_str_p(UPLINK_USART, PSTR("Error: unknown subcommand [READER]\r\n"));
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;
        return;
        break;
    }
}
