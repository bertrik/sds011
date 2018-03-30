#include <stdbool.h>
#include <stdint.h>

#include "sds011.h"

// magic header bytes
#define MAGIC1 0xAA
#define MAGIC2 0xAB

// parsing state
typedef enum {
    HEAD = 0,
    COMMAND,
    DATA,
    CHECK,
    TAIL
} EState;

typedef struct {
    EState  state;
    uint8_t cmd;
    uint8_t buf[32];
    int     size;
    int     idx, len;
    uint16_t chk, sum;
} TState;

static TState state;

/**
    Initializes the measurement data state machine.
 */
void SdsInit(void)
{
    state.state = HEAD;
    state.size = sizeof(state.buf);
    state.idx = state.len = 0;
    state.chk = state.sum = 0;
}

/**
    Processes one byte in the measurement data state machine.
    @param[in] b the byte
    @return true if a full message was received
 */
bool SdsProcess(uint8_t b)
{
    switch (state.state) {
    // wait for HEAD byte
    case HEAD:
        state.sum = b;
        if (b == MAGIC1) {
            state.state = COMMAND;
        }
        break;
    // wait for BEGIN2 byte
    case COMMAND:
        state.sum += b;
        state.cmd = b;
        state.idx = 0;
        state.len = 6;
        state.state = DATA;
        break;
    // store data
    case DATA:
        state.sum += b;
        if (state.idx < state.len) {
            state.buf[state.idx++] = b;
        }
        if (state.idx == state.len) {
            state.state = CHECK;
        }
        break;
    // store checksum
    case CHECK:
        state.chk = b;
        state.state = TAIL;
        break;
    case TAIL:
        state.state = HEAD;
        return (b == MAGIC2);
    default:
        state.state = HEAD;
        break;
    }
    return false;
}

static uint16_t get(const uint8_t *buf, int idx)
{
    uint16_t word;
    word = buf[idx++];
    word += buf[idx++] << 8;
    return word;
}

/**
    Parses a complete measurement data frame into a structure.
    @param[out] meas the parsed measurement data
 */
void SdsParse(sds_meas_t *meas)
{
    meas->pm2_5 = get(state.buf, 0);
    meas->pm10 = get(state.buf, 2);
    meas->id = get(state.buf, 4);
}

/**
    Creates a command buffer to send.
    @param[in] buf the command buffer
    @param[in] size the size of the command buffer, should be at least 7 bytes
    @param[in] cmd the command byte
    @param[in] data the data field
    @return the length of the command buffer, or 0 if the size was too small
*/
int SdsCreateCmd(uint8_t *buf, int size, uint8_t cmd, uint16_t data)
{
    if (size < 7) {
        return 0;
    }

    int idx = 0;
    buf[idx++] = MAGIC1;
    buf[idx++] = cmd;
    buf[idx++] = (data >> 8) & 0xFF;
    buf[idx++] = (data >> 0) & 0xFF;
    int sum = 0;
    for (int i = 0; i < idx; i++) {
        sum += buf[i];
    }
    buf[idx++] = (sum  >> 8) & 0xFF;
    buf[idx++] = (sum  >> 0) & 0xFF;
    return idx;
}

