/*
 * API for acting as an N64 Controller
 *
 * Based on the N64 and GameCube controller input/output library called "Nintendo" by Nico Hood
 * This repo is in fact a fork, although the code will largely be entirely different by the end,
 * but credit is due!
 *
 * NOTE: This contains time-sensitive code. Pause interrupts before using if interrupts are enabled
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Before including, define yourself these values if you want different than default
 *
 * You need all of them:
 * - Port PU
 * - Port OC
 * - Port
 * - Pin
 */
#ifndef N64_DATA_PIN

#define N64_DATA_PORT_PU    P1_DIR_PU
#define N64_DATA_PORT_OC    P1_MOD_OC
#define N64_DATA_PORT       P1
#define N64_DATA_PIN        6

#endif

// 4 byte data report to send as the controller
typedef union {
    uint8_t raw8[4];
    uint16_t raw16[2];
    uint32_t raw32;
    
    struct {
        uint8_t d_pad: 4;
        uint8_t btns0: 4;
        uint8_t c_pad: 4;
        uint8_t btns1: 4;
        uint16_t axes;
    };

    struct {
        // Byte 1
        uint8_t d_pad_right: 1;
        uint8_t d_pad_left: 1;
        uint8_t d_pad_down: 1;
        uint8_t d_pad_up: 1;
        uint8_t start: 1;
        uint8_t z: 1;
        uint8_t b: 1;
        uint8_t a: 1;

        // Byte 2
        uint8_t c_right: 1;
        uint8_t c_left: 1;
        uint8_t c_down: 1;
        uint8_t c_up: 1;
        uint8_t r: 1;
        uint8_t l: 1;
        uint8_t low1: 1;
        uint8_t low0: 1;

        // 3rd-4th data byte
        int8_t x_axis;
        int8_t y_axis;
    };
} n64_report_t;

bool n64_write(const n64_report_t *const ref_report);

