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
    ok = assertEquals(123, (int)meas.pm2_5, "PM2.5");
    ok = ok && assertEquals(261, (int)meas.pm10, "PM10");
    ok = ok && assertEquals(0x60A1, meas.id, "ID");

    return ok;    
}

static bool test_tx(void)
{
    int res;
    uint8_t txbuf[32];

    // query data command
    uint8_t cmd_data[15] = {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};

    // verify valid message
    res = SdsCreateCmd(txbuf, sizeof(txbuf), 0xB4, cmd_data, sizeof(cmd_data));
    assertEquals(19, res, "sizeof(txbuf)");

    const uint8_t expected[] = {0xAA, 0xB4, 0x04, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0x02, 0xAB};
    return (memcmp(expected, txbuf, sizeof(expected)) == 0);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    bool b;

    printf("test_rx ...");
    b = test_rx();
    printf("%s\n", b ? "PASS" : "FAIL");
    printf("test_tx ...");
    b = test_tx();
    printf("%s\n", b ? "PASS" : "FAIL");
    return 0;
}

