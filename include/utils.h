#ifndef UTILS_H
#define UTILS_H

// --- INCLUDES ---
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Includes do Pico
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"

// Includes das Bibliotecas e FreeRTOS
#include "ssd1306/ssd1306.h"
#include "ws2818b.pio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Configuração dos Pinos
#define LED_PIN         7
#define LED_COUNT       25
#define BUZZER1_PIN     21
#define BUTTON_B_PIN    6

// Parâmetros do Jogo
#define REACTION_TIMEOUT_MS 2000
#define MIN_WAIT_MS         2000
#define MAX_WAIT_MS         7000

// Notas para distração sonora
#define NOTE_C4 262
#define NOTE_G4 392
#define NOTE_E4 330

// Estrutura para um pixel RGB
typedef struct {
    uint8_t G, R, B;
} npLED_t;

// Variáveis globais
extern ssd1306_t oled;
extern PIO np_pio;
extern uint sm;
extern float brightness_factor;

// Prótotipo das funções
void npInit(uint pin);
uint8_t reverse_bits(uint8_t num);
void npSetLED(const uint index,  uint8_t r,  uint8_t g,  uint8_t b);
void npWrite();
void npClear();
void setup_buzzer(uint gpio);
void play_note(uint gpio, int freq, int duration);
void setup_display();
void mostrar_mensagem(const char *str, int x, int y, bool clear_first);

// Tarefas do FreeRTOS
void game_logic_task(void *pvParameters);
void input_task(void *pvParameters);
void visual_distraction_task(void *pvParameters);
void audio_distraction_task(void *pvParameters);

#endif