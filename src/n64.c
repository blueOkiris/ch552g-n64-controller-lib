// Implement functionality for acting as an N64 controller

#include <stdbool.h>
#include <stdint.h>
#include <debug.h>
#include <n64.h>

// 3 bytes of status report that we send as the controller
typedef union {
    uint8_t raw8[3];
    struct {
        /*
         * Device information:
         *
         * 15 - 0 for Wired Controller, 1 for Wireless Controller
         * 14 - 0 for Wired Receive, 1 for Wireless Receive
         * 13 - 0 for Rumble, 1 for None
         * 12 - Controller type b1 (always 0)
         * 11 - Controller type b2, 1 - GCN, 0 - N64
         * 10 - Wireless Type - 0 for IF, 1 for RF
         *  9 - Wireless state - 0 for Variable, 1 for Fixed
         *  8 - 0 for Non-standard controller, 1 for Dolphin Standard Controller
         *  7
         *  6
         *  5 - Wireless Origin (0 for invalid, 1 for valid)
         *  4 - Wireless Fix Id (0 for not, 1 for fixed)
         *  3 - Wireless Type - 0 for normal, 1 for non-controller?
         *  2 - Wireless Type - 0 for normal, 1 for lite controller
         *  1 - Wireless Type - Other?
         *  0 - Wireless Type - Other?
         */
        union {
            uint16_t dev;
            struct {
                uint8_t wired: 1;
                uint8_t wired_recv: 1;
                uint8_t ctl_type: 2;
                uint8_t wireless_type: 1;
                uint8_t wireless_state: 1;
                uint8_t console_type: 1;
                uint8_t zero: 2;
                uint8_t wireless_origin: 1;
                uint8_t wireless_fixed_id: 1;
                uint8_t wireless_type_other: 5;
            };
        };

        // Controller Status
        union {
            uint8_t status;
            struct {
                uint8_t status0: 3;
                uint8_t rumble: 1;
                uint8_t status1: 4;
            };
        };
    };
} n64_status_t;

typedef enum {
    RET_NONE = 0,
    RET_ID,
    RET_STATUS,
    RET_READ_EXP_BUS,
    RET_WRITE_EXP_BUS
} n64_ret_t;

static const n64_status_t DEF_STATUS = { { 0x05, 0x00, 0x02 } };
static const uint8_t N64_PIN_BIT = 1 << N64_DATA_PIN;

static void n64_send(const uint8_t *const ref_buff, uint8_t len) {
    // Set pin to output, default high
    N64_DATA_PORT_PU |= N64_PIN_BIT;
    N64_DATA_PORT_OC &= ~N64_PIN_BIT;
    N64_DATA_PORT |= N64_PIN_BIT;

    /*
     * Send data:
     * - 3us Low, 1us High -> 0
     * - 1us Low, 3us High -> 1
     */
    for (uint8_t i = 0; i < len; i++) {
        const uint8_t data = ref_buff[i];
        for (int j = 7; j >= 0; j--) {
            N64_DATA_PORT &= ~N64_PIN_BIT;
            mDelayuS(((data >> j) & 0x01) ? 1 : 3);
            N64_DATA_PORT |= N64_PIN_BIT;
            mDelayuS(((data >> j) & 0x01) ? 3 : 1);
        }
    }

    // Stop bit - 2us Low, High
    N64_DATA_PORT &= ~N64_PIN_BIT;
    mDelayuS(2);
    N64_DATA_PORT |= N64_PIN_BIT;
}

static uint8_t n64_get(uint8_t *ref_buff, const uint8_t len) {
    uint8_t recvd_bytes = 0;

    // Set pin to input pullup
    N64_DATA_PORT_PU &= ~N64_PIN_BIT;
    N64_DATA_PORT_OC |= N64_PIN_BIT;

    for (uint8_t i = 0; i < len; i++) {
        while (!(N64_DATA_PORT & N64_PIN_BIT)); // Wait for high
        for (uint8_t j = 0; j < 8; j++) {
            while (N64_DATA_PORT & N64_PIN_BIT); // Wait for low
            mDelayuS(1);
            if (!(N64_DATA_PORT & N64_PIN_BIT)) {
                // If High after 1us, 1 bit
                ref_buff[j] |= 0x01 << j;
            } else {
                // Otherwise it's a 0 bit
                ref_buff[i] &= ~(0x01 << j);
            }
        }
    }

    return recvd_bytes;
}

static n64_ret_t n64_respond(
        const n64_status_t *const ref_status, const n64_report_t *const ref_report) {
    uint8_t cmd[3] = { 0 };
    uint8_t recvd_bytes = n64_get(cmd, sizeof(cmd));
    if (recvd_bytes == 1 && cmd[0] == 0x00) {
        // Identify
        n64_send(ref_status->raw8, sizeof(n64_status_t));
        return RET_ID;
    } else if (recvd_bytes == 1 && cmd[0] == 0x01) {
        // Poll
        n64_send(ref_report->raw8, sizeof(n64_report_t));
        return RET_STATUS;
    } else {
        return RET_NONE;
    }
}

bool n64_write(const n64_report_t *const ref_report) {
    n64_ret_t ret = n64_respond(&DEF_STATUS, ref_report);

    // Init
    if (ret == 1) {
        // Try to answer a following read request
        ret = n64_respond(&DEF_STATUS, ref_report);
    }

    return false;
}

