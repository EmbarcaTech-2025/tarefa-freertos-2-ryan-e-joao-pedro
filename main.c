#include "include/utils.h" // Importações e constantes definidas

ssd1306_t oled;
npLED_t leds[LED_COUNT];
PIO np_pio;
uint sm;
float brightness_factor = 0.05; // Fator de brilho do seu código

const uint8_t GO_PATTERN[5] = {0b10001, 0b01010, 0b00100, 0b01010, 0b10001};
const uint8_t DISTRACTION_PATTERN_O[5] = {0b01110, 0b10001, 0b10001, 0b10001, 0b01110};
const uint8_t DISTRACTION_PATTERN_SQUARE[5] = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
const uint8_t DISTRACTION_PATTERN_HEART[5] = {0b00100, 0b01110, 0b11111, 0b11111, 0b01010};

SemaphoreHandle_t g_criticalMomentMutex;
SemaphoreHandle_t g_reactionSignalSemaphore;

// Funções da Matriz de LED
uint8_t reverse_bits(uint8_t num) {
    uint8_t reversed = 0;
    for (int i = 0; i < 8; i++) {
        reversed <<= 1;
        reversed |= (num & 1);
        num >>= 1;
    }
    return reversed;
}

void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
}

void npSetLED(const uint index,  uint8_t r,  uint8_t g,  uint8_t b) {
    leds[index].R = reverse_bits((r * brightness_factor));
    leds[index].G = reverse_bits((g * brightness_factor));
    leds[index].B = reverse_bits((b * brightness_factor));
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

// Nova função helper para desenhar nossos padrões
void display_pattern(const uint8_t pattern[], uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int led_index = y * 5 + x;
            if ((pattern[y] >> (4 - x)) & 1) {
                npSetLED(led_index, r, g, b);
            } else {
                npSetLED(led_index, 0, 0, 0);
            }
        }
    }
    npWrite(); // Envia os dados para a matriz
}

// Funções do Buzzer
void setup_buzzer(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice, 12500);
    pwm_set_enabled(slice, true);
}

void play_note(uint gpio, int freq, int duration) {
    if (freq == 0) {
        pwm_set_gpio_level(gpio, 0);
        return;
    }
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint32_t wrap = 12500000 / freq;
    pwm_set_wrap(slice, wrap);
    pwm_set_gpio_level(gpio, wrap/10); // duty cycle
    vTaskDelay(pdMS_TO_TICKS(duration));
    pwm_set_gpio_level(gpio, 0);
}

// Setup do display OLED
void setup_display() {
    i2c_init(i2c1, 400000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    oled.external_vcc = false;
    ssd1306_init(&oled, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&oled);
}

// Função para facilitar o desenho no OLED

void oled_draw_text(const char *text, int x, int y, bool clear_first) {
    if (clear_first) {
        ssd1306_clear(&oled);
    }
    ssd1306_draw_string(&oled, x, y, 1, (char*)text);
    ssd1306_show(&oled);
}

// Gera um inteiro aleatório num intervalo

int generate_random_int(int min, int max) {
    return min + (rand() % (max - min + 1));
}


// Tasks RTOS

void game_logic_task(void *pvParameters) {
    TickType_t startTime;
    uint32_t reactionTimeMs;
    char buffer[20];

    oled_draw_text("Jogo de Reflexo", 5, 10, true);
    oled_draw_text("Aperte B no 'X'", 5, 25, false);
    vTaskDelay(pdMS_TO_TICKS(3000));

    for (;;) {
        oled_draw_text("Prepare-se...", 15, 40, true);
        vTaskDelay(pdMS_TO_TICKS(generate_random_int(MIN_WAIT_MS, MAX_WAIT_MS)));
        
        xSemaphoreTake(g_criticalMomentMutex, portMAX_DELAY);
        display_pattern(GO_PATTERN, 0x00FF00); // Verde
        startTime = xTaskGetTickCount();

        if (xSemaphoreTake(g_reactionSignalSemaphore, pdMS_TO_TICKS(REACTION_TIMEOUT_MS)) == pdTRUE) {
            reactionTimeMs = (xTaskGetTickCount() - startTime) * portTICK_PERIOD_MS;
            sprintf(buffer, "Tempo: %lu ms", reactionTimeMs);
            oled_draw_text(buffer, 5, 40, true);
        } else {
            oled_draw_text("Lento demais!", 5, 40, true);
            display_pattern(GO_PATTERN, 0xFF0000); // Vermelho
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        xSemaphoreGive(g_criticalMomentMutex);
        npClear();
        npWrite();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void input_task(void *pvParameters) {
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
    for (;;) {
        if (gpio_get(BUTTON_B_PIN) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            xSemaphoreGive(g_reactionSignalSemaphore);
            while(gpio_get(BUTTON_B_PIN) == 0) {
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void visual_distraction_task(void *pvParameters) {
    const uint8_t* patterns[] = {DISTRACTION_PATTERN_O, DISTRACTION_PATTERN_SQUARE, DISTRACTION_PATTERN_HEART};
    uint32_t colors[] = {0x0000FF, 0xFFFF00, 0xFF05F0}; // Azul, Amarelo

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(generate_random_int(400, 1500)));
        if (xSemaphoreTake(g_criticalMomentMutex, 0) == pdTRUE) {
            int p_idx = generate_random_int(0, 2);
            int c_idx = generate_random_int(0, 2);
            display_pattern(patterns[p_idx], colors[c_idx]);
            vTaskDelay(pdMS_TO_TICKS(250));
            npClear();
            npWrite();
            xSemaphoreGive(g_criticalMomentMutex);
        }
    }
}

void audio_distraction_task(void *pvParameters) {
    int notes[] = {NOTE_C4, NOTE_G4, NOTE_E4};
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(generate_random_int(500, 2000)));
        int note_idx = generate_random_int(0, 2);
        play_note(BUZZER1_PIN, notes[note_idx], 100);
    }
}


// Função Principal
int main() {
    stdio_init_all();
    
    // Inicialização do Hardware
    setup_display();
    setup_buzzer(BUZZER1_PIN);
    npInit(LED_PIN);
    srand(get_absolute_time()); // semente do gerador aleatório

    // Criação dos Recursos RTOS
    g_criticalMomentMutex = xSemaphoreCreateMutex();
    g_reactionSignalSemaphore = xSemaphoreCreateBinary();

    if (g_criticalMomentMutex == NULL || g_reactionSignalSemaphore == NULL) {
        oled_draw_text("Erro RTOS!", 0, 0, true);
        while(1);
    }

    // Criação das Tarefas
    xTaskCreate(game_logic_task, "GameLogic", 512, NULL, 2, NULL);
    xTaskCreate(input_task, "Input", 256, NULL, 3, NULL);
    xTaskCreate(visual_distraction_task, "VisualDistraction", 512, NULL, 1, NULL);
    xTaskCreate(audio_distraction_task, "AudioDistraction", 256, NULL, 1, NULL);

    // Inicia o escalonador de eventos do FreeRTOS
    vTaskStartScheduler();

    while(1);
    return 0;
}