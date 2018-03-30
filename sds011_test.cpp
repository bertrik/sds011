// simple test code to verify creation of tx command and parsing of rx response 

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "sds011_esp/sds011.cpp"

static bool assertEquals(int expected, int actual, const char *field)
{
    if (actual != expected) {
        fprintf(stderr, "Assertion failure: field '%s' expected %d, got %d\n", field, expected, actual);
        return false;
    }
    return true;
}

static bool test_rx(void)
{
    uint8_t frame[] = {
        // header, cmd
        0xAA, 0xC0,
        // PM2.5
        0xD4, 0x04,
        // PM10
        0x3A, 0x0A,
        // ID
        0xA1, 0x60,
        // checksum
        0x1D,
        // tail
        0xAB
    };

    // send frame data
    bool ok;
    SdsInit();
    for (size_t i = 0; i < sizeof(frame); i++) {
        ok = SdsProcess(frame[i]);
    }
    if (!ok) {
        fprintf(stderr, "expected successful frame!\n");
        return false;
    }
    
    // parse
    sds_meas_t meas;
    SdsParse(&meas);
    ok = assertEquals(1236, meas.pm2_5, "PM2.5");
    ok = ok && assertEquals(2618, meas.pm10, "PM10");
    ok = ok && assertEquals(0x60A1, meas.id, "ID");

    return ok;    
}

#if 0
static bool test_tx(void)
{
    int res;
    uint8_t txbuf[16];

    // verify too small buffer
    res = SdsCreateCmd(txbuf, 6, 0xE0, 0);
    if (res > 0) {
        fprintf(stderr, "expected failure for too small buffer!");
        return false;
    }

    // verify valid message
    res = SdsCreateCmd(txbuf, sizeof(txbuf), 0xE1, 0x1234);
    assertEquals(7, res, "sizeof(txbuf)");

    const uint8_t expected[] = {0x42, 0x4d, 0xE1, 0x12, 0x34, 0x01, 0xB6};
    return (memcmp(expected, txbuf, sizeof(expected)) == 0);
}
#endif

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    bool b;

    printf("test_rx ...");
    b = test_rx();
    printf("%s\n", b ? "PASS" : "FAIL");
#if 0
    printf("test_tx ...");
    b = test_tx();
    printf("%s\n", b ? "PASS" : "FAIL");
#endif
    return 0;
}

