
 
#ifndef MDB_H
#define MDB_H

#ifndef F_CPU
#define F_CPU       16000000UL
#endif

#ifndef TRUE
#define TRUE        1
#endif

#ifndef FALSE
#define FALSE       0
#endif

#define DEBUG       0

#define MDB_USART   1



int statePos();


//---------------------------------------------------------------------------
//  MDB STATES
//---------------------------------------------------------------------------
enum MDB_STATES {MDB_INACTIVE,MDB_DISABLED,MDB_ENABLED,MDB_SESSION_IDLE,MDB_VENDING,MDB_REVALUE,MDB_NEGATIVE_VEND};

//---------------------------------------------------------------------------
//  MDB CMDS
//---------------------------------------------------------------------------
enum MDB_CMD {MDB_IDLE,MDB_RESET = 0x10,MDB_SETUP,MDB_POLL,MDB_VEND,MDB_READER, MDB_STAGE3 = 0x17};

//---------------------------------------------------------------------------
//  POLL REPLYS
//---------------------------------------------------------------------------
enum POLL_REPLY {	MDB_REPLY_ACK, MDB_REPLY_JUST_RESET, MDB_REPLY_READER_CFG, MDB_REPLY_DISPLAY_REQ, MDB_REPLY_BEGIN_SESSION,
                MDB_REPLY_SESSION_CANCEL_REQ, MDB_REPLY_VEND_APPROVED, MDB_REPLY_VEND_DENIED, MDB_REPLY_END_SESSION,
                MDB_REPLY_CANCELED, MDB_REPLY_PERIPHERIAL_ID, MDB_REPLY_ERROR, MDB_REPLY_CMD_OUT_SEQUENCE 
                };
                
typedef struct {
    uint8_t feature_level;
    uint8_t dispaly_cols;
    uint8_t dispaly_rows;
    uint8_t dispaly_info;
} vmcCfg_t;

typedef struct {
    uint8_t  reader_cfg;
    uint8_t  feature_level;
    uint16_t country_code;
    uint8_t  scale_factor;
    uint8_t  decimal_places;
    uint8_t  max_resp_time;
    uint8_t  misc_options;
} cdCfg_t;

typedef struct {
    uint8_t  d0;
    uint8_t  d1;
    uint8_t  d2;
    uint8_t  d3;
    uint8_t  d4;
    uint8_t  d5;
    uint8_t  d6;
    uint8_t  d7;
    uint8_t  d8;
    uint8_t  d9;
    uint8_t  d10;
    uint8_t  d11;
    uint8_t  d12;
    uint8_t  d13;
    uint8_t  d14;
    uint8_t  d15;
    uint8_t  d16;
    uint8_t  d17;
    uint8_t  d18;
    uint8_t  d19;
    uint8_t  d20;
    uint8_t  d21;
    uint8_t  d22;
    uint8_t  d23;
    uint8_t  d24;
    uint8_t  d25;
    uint8_t  d26;
    uint8_t  d27;
    uint8_t  d28;
    uint8_t  d29;
    uint16_t  d30;
    
    

} Stage3Data;

typedef struct {
    uint8_t  d0;

}Stage3DataTest;



typedef struct {
    uint16_t max;
    uint16_t min;
} vmcPrice_t;


typedef struct {
    uint8_t flag;
    uint16_t funds;
} start_t;

typedef struct {
    uint8_t vend_approved;
    uint8_t vend_denied;
    uint16_t vend_amount;
} result_t;


typedef struct {
    start_t start;
    result_t result;
} mdbSession_t;

//uint8_t mdb_state;

void mdb_cmd_handler(void);
void mdb_reset(void);
void mdb_setup(void);
void mdb_stage3(void);
int statePos();
void mdb_poll(void);
void mdb_vend(void);
void mdb_reader(void);

#endif // MDB_H
