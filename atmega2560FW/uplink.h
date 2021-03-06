

#ifndef UPLINK_H
#define UPLINK_H

#ifndef F_CPU
#define F_CPU 16000000UL
#endif // F_CPU

#define UPLINK_USART 0

#define MAX_CMD_LENGTH 20
#define MAX_VAR 10

#define RESET()           \
    {                     \
        asm("ldi r30,0"); \
        asm("ldi r31,0"); \
        asm("ijmp");      \
    }

typedef struct
{
    char *cmd;
    void (*funcptr)(char *arg);
} cmdStruct_t;

void uplink_cmd_handler(void);
void parse_cmd(char *cmd);

void cmd_reset(char *arg);
void cmd_help(char *arg);
void cmd_info(char *arg);
void cmd_get_mdb_state(char *arg);
void cmd_start_session(char *arg);
void cmd_approve_vend(char *arg);
void cmd_deny_vend(char *arg);
void cmd_cancel_session(char *arg);
void cmd_io(char *arg);
void vendInvoker(char *arg);

void startTimer();
void stopTimer();
uint8_t getTimerState();

void resetDevice();
void incrementTimerPass();
uint8_t getTimerPass();

void timeHandler(uint8_t v);
#endif // UPLINK_H
