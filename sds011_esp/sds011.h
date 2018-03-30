#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t pm2_5;
    uint16_t pm10;
    uint16_t id;
} sds_meas_t;

void SdsInit(void);
bool SdsProcess(uint8_t b);
void SdsParse(sds_meas_t *meas);
int SdsCreateCmd(uint8_t *buf, int size, uint8_t cmd, uint16_t data);


