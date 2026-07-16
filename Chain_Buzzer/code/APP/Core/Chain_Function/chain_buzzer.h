/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _CHAIN_BUZZER_H_
#define _CHAIN_BUZZER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tim.h"
#include "base_function.h"
#include <stdbool.h>

#define TIM_CLK_FREQ        (64000000UL)
#define BUZZER_FREQ_MIN     (100)
#define BUZZER_FREQ_MAX     (10000)
#define BUZZER_DUTY_MAX     (100)
#define BUZZER_DUTY_DEFAULT (50)
#define BUZZER_FREQ_DEFAULT (2700)
#define NOTE_COUNT          (62)

typedef enum {
    BUZZER_MODE_AUTO_PLAY   = 0x00,
    BUZZER_MODE_MANUAL_PLAY = 0x01,
    BUZZER_MODE_NOTE_PLAY   = 0x02,
} buzzer_mode_t;

typedef enum {
    BUZZER_STATUS_OFF = 0x00,
    BUZZER_STATUS_ON  = 0x01,
} buzzer_status_t;

typedef enum {
    CHAIN_BUZZER_SET_MODE      = 0x30,
    CHAIN_BUZZER_GET_MODE      = 0x31,
    CHAIN_BUZZER_SET_AUTO_PLAY = 0x32,
    CHAIN_BUZZER_SET_FREQ      = 0x33,
    CHAIN_BUZZER_GET_FREQ      = 0x34,
    CHAIN_BUZZER_SET_DUTY      = 0x35,
    CHAIN_BUZZER_GET_DUTY      = 0x36,
    CHAIN_BUZZER_SET_STATUS    = 0x37,
    CHAIN_BUZZER_GET_STATUS    = 0x38,
    CHAIN_BUZZER_SET_NOTE_PLAY = 0x39,
} chain_buzzer_cmd_t;

// ==================== Special Notes ====================
#define NOTE_REST (0)  // Rest (silence)

// ==================== Octave 3 ====================
#define NOTE_C3  (131)
#define NOTE_CS3 (139)
#define NOTE_D3  (147)
#define NOTE_DS3 (156)
#define NOTE_E3  (165)
#define NOTE_F3  (175)
#define NOTE_FS3 (185)
#define NOTE_G3  (196)
#define NOTE_GS3 (208)
#define NOTE_A3  (220)
#define NOTE_AS3 (233)
#define NOTE_B3  (247)

// ==================== Octave 4 (Middle Range) ====================
#define NOTE_C4  (262)  // Middle C
#define NOTE_CS4 (277)
#define NOTE_D4  (294)
#define NOTE_DS4 (311)
#define NOTE_E4  (330)
#define NOTE_F4  (349)
#define NOTE_FS4 (370)
#define NOTE_G4  (392)
#define NOTE_GS4 (415)
#define NOTE_A4  (440)  // Standard pitch A4 = 440Hz
#define NOTE_AS4 (466)
#define NOTE_B4  (494)

// ==================== Octave 5 ====================
#define NOTE_C5  (523)
#define NOTE_CS5 (554)
#define NOTE_D5  (587)
#define NOTE_DS5 (622)
#define NOTE_E5  (659)
#define NOTE_F5  (698)
#define NOTE_FS5 (740)
#define NOTE_G5  (784)
#define NOTE_GS5 (831)
#define NOTE_A5  (880)
#define NOTE_AS5 (932)
#define NOTE_B5  (988)

// ==================== Octave 6 ====================
#define NOTE_C6  (1047)
#define NOTE_CS6 (1109)
#define NOTE_D6  (1175)
#define NOTE_DS6 (1245)
#define NOTE_E6  (1319)
#define NOTE_F6  (1397)
#define NOTE_FS6 (1480)
#define NOTE_G6  (1568)
#define NOTE_GS6 (1661)
#define NOTE_A6  (1760)
#define NOTE_AS6 (1865)
#define NOTE_B6  (1976)

// ==================== Octave 7 ====================
#define NOTE_C7  (2093)
#define NOTE_CS7 (2217)
#define NOTE_D7  (2349)
#define NOTE_DS7 (2489)
#define NOTE_E7  (2637)
#define NOTE_F7  (2794)
#define NOTE_FS7 (2960)
#define NOTE_G7  (3136)
#define NOTE_GS7 (3322)
#define NOTE_A7  (3520)
#define NOTE_AS7 (3729)
#define NOTE_B7  (3951)

// ==================== Octave 8 (Partial) ====================
#define NOTE_C8 (4186)  // Highest note on piano

void buzzer_init(void);
void buzzer_set_mode(buzzer_mode_t mode);
void buzzer_set_freq(uint16_t freq);
void buzzer_set_duty(uint8_t duty);
void buzzer_set_turn_on(buzzer_status_t on);
void buzzer_set_play(uint16_t freq, uint8_t duty, uint16_t duration);
void buzzer_update(void);
void buzzer_stop(void);
void chain_buzzer_set_mode(uint8_t *buffer, uint8_t size);
void chain_buzzer_get_mode(void);
void chain_buzzer_set_auto_play(uint8_t *buffer, uint8_t size);
void chain_buzzer_set_freq(uint8_t *buffer, uint8_t size);
void chain_buzzer_get_freq(void);
void chain_buzzer_set_duty(uint8_t *buffer, uint8_t size);
void chain_buzzer_get_duty(void);
void chain_buzzer_set_status(uint8_t *buffer, uint8_t size);
void chain_buzzer_get_status(void);
void chain_buzzer_set_note_play(uint8_t *buffer, uint8_t size);

#ifdef __cplusplus
}
#endif
#endif /* _CHAIN_BUZZER_H_ */
