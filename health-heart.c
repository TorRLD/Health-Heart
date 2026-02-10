#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "tinyml.h"
#include "lib/ssd1306.h"

// --- CONFIGURAÇÃO I2C ---
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// --- CONFIGURAÇÃO BOTÃO E LED ---
#define BUTTON_A_PIN 5  // Botão A da BitDogLab
const uint LED_PIN = 13;       
const uint JOYSTICK_X_PIN = 26; 
const uint JOYSTICK_Y_PIN = 27; 

// --- DEFINIÇÕES DO DISPLAY ---
#ifndef SSD1306_WIDTH
 #define SSD1306_WIDTH 128
#endif
#ifndef SSD1306_HEIGHT
 #define SSD1306_HEIGHT 64
#endif

// --- CONSTANTES DE NORMALIZAÇÃO ---
const int IDX_MALE = 0;           const float MEAN_MALE = 0.4411f;          const float STD_MALE = 0.4965f;
const int IDX_AGE = 1;            const float MEAN_AGE = 49.7831f;          const float STD_AGE = 8.6077f;
const int IDX_EDUCATION = 2;      const float MEAN_EDUCATION = 1.9715f;     const float STD_EDUCATION = 1.0137f;
const int IDX_CURRENTSMOKER = 3;  const float MEAN_CURRENTSMOKER = 0.4909f; const float STD_CURRENTSMOKER = 0.4999f;
const int IDX_CIGSPERDAY = 4;     const float MEAN_CIGSPERDAY = 9.0201f;    const float STD_CIGSPERDAY = 11.8583f;
const int IDX_BPMEDS = 5;         const float MEAN_BPMEDS = 0.0350f;        const float STD_BPMEDS = 0.1837f;
const int IDX_PREVALENTSTROKE = 6;const float MEAN_PREVALENTSTROKE = 0.0068f;const float STD_PREVALENTSTROKE = 0.0824f;
const int IDX_PREVALENTHYP = 7;   const float MEAN_PREVALENTHYP = 0.3245f;  const float STD_PREVALENTHYP = 0.4682f;
const int IDX_DIABETES = 8;       const float MEAN_DIABETES = 0.0262f;      const float STD_DIABETES = 0.1598f;
const int IDX_TOTCHOL = 9;        const float MEAN_TOTCHOL = 237.1049f;     const float STD_TOTCHOL = 43.9122f;
const int IDX_SYSBP = 10;         const float MEAN_SYSBP = 132.9008f;       const float STD_SYSBP = 22.3815f;
const int IDX_DIABP = 11;         const float MEAN_DIABP = 83.1049f;        const float STD_DIABP = 11.9385f;
const int IDX_BMI = 12;           const float MEAN_BMI = 25.8282f;          const float STD_BMI = 4.1715f;
const int IDX_HEARTRATE = 13;     const float MEAN_HEARTRATE = 75.8248f;    const float STD_HEARTRATE = 12.0982f;
const int IDX_GLUCOSE = 14;       const float MEAN_GLUCOSE = 81.8480f;      const float STD_GLUCOSE = 24.4452f;

#define NUM_ENTRADAS 15

float perfil[NUM_ENTRADAS]; 
float dados_para_ia[NUM_ENTRADAS];

// --- FUNÇÕES AUXILIARES ---

float random_float(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

float random_bool() { return (rand() % 2) ? 1.0f : 0.0f; }

// Gera o paciente base
void gerar_paciente_aleatorio() {
    perfil[IDX_MALE] = random_bool();
    perfil[IDX_AGE] = random_float(30.0f, 75.0f);
    perfil[IDX_EDUCATION] = (float)(rand() % 4 + 1);
    perfil[IDX_CURRENTSMOKER] = (rand() % 100 < 30) ? 1.0f : 0.0f;
    perfil[IDX_CIGSPERDAY] = (perfil[IDX_CURRENTSMOKER] > 0.5f) ? random_float(5.0f, 40.0f) : 0.0f;
    perfil[IDX_BPMEDS] = (rand() % 100 < 15) ? 1.0f : 0.0f;
    perfil[IDX_PREVALENTSTROKE] = (rand() % 100 < 2) ? 1.0f : 0.0f;
    perfil[IDX_PREVALENTHYP] = (rand() % 100 < 25) ? 1.0f : 0.0f;
    perfil[IDX_DIABETES] = (rand() % 100 < 10) ? 1.0f : 0.0f;
    perfil[IDX_TOTCHOL] = random_float(150.0f, 300.0f);
    perfil[IDX_BMI] = random_float(18.5f, 35.0f);
    perfil[IDX_HEARTRATE] = random_float(55.0f, 100.0f);
    
    // Valores iniciais sensores
    perfil[IDX_SYSBP] = 120.0f;
    perfil[IDX_GLUCOSE] = 80.0f;
}

// Imprime a ficha no Serial
void imprimir_ficha_paciente() {
    printf("\n");
    printf("============================================================\n");
    printf("               PRONTUARIO DO PACIENTE (SIMULADO)            \n");
    printf("============================================================\n");
    printf("| DADO                 | VALOR                             |\n");
    printf("|----------------------|-----------------------------------|\n");
    printf("| Sexo                 | %s\n", (perfil[IDX_MALE] > 0.5) ? "Masculino" : "Feminino");
    printf("| Idade                | %.0f anos\n", perfil[IDX_AGE]);
    printf("| Escolaridade (1-4)   | %.0f\n", perfil[IDX_EDUCATION]);
    printf("| Fumante              | %s (%.0f cig/dia)\n", (perfil[IDX_CURRENTSMOKER]>0.5)?"Sim":"Nao", perfil[IDX_CIGSPERDAY]);
    printf("| Colesterol Total     | %.0f mg/dL\n", perfil[IDX_TOTCHOL]);
    printf("| IMC                  | %.1f\n", perfil[IDX_BMI]);
    printf("| Batimentos           | %.0f bpm\n", perfil[IDX_HEARTRATE]);
    printf("| Diabetes (Hist.)     | %s\n", (perfil[IDX_DIABETES]>0.5)?"Sim":"Nao");
    printf("| Hipertensao (Hist.)  | %s\n", (perfil[IDX_PREVALENTHYP]>0.5)?"Sim":"Nao");
    printf("| AVC (Historico)      | %s\n", (perfil[IDX_PREVALENTSTROKE]>0.5)?"Sim":"Nao");
    printf("============================================================\n");
    printf(">>> PACIENTE CARREGADO. Ajuste Pressao/Glicose no Joystick.\n\n");
}

float map_val(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float normalizar(float valor, float media, float std_dev) {
    return (valor - media) / std_dev;
}

void imprimir_cabecalho_monitor() {
    printf("\n+----------------------------------------------------------------------+\n");
    printf("| PRESSAO (mmHg) | GLICOSE (mg/dL) | RISCO | STATUS    | PERFIL BASE   |\n");
    printf("+----------------------------------------------------------------------+\n");
}

void imprimir_barra_serial(float porcentagem) {
    printf("[");
    int barras = (int)(porcentagem / 5.0f);
    if(barras > 10) barras = 10;
    for(int i=0; i<10; i++) (i < barras) ? printf("#") : printf("-");
    printf("]");
}

int main() {
    stdio_init_all();
    
    // I2C OLED
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;
    ssd1306_init(&ssd, SSD1306_WIDTH, SSD1306_HEIGHT, false, 0x3C, i2c1);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "CARREGANDO...", 20, 30);
    ssd1306_send_data(&ssd);

    // Hardware
    gpio_init(LED_PIN); gpio_set_dir(LED_PIN, GPIO_OUT);
    adc_init(); 
    adc_gpio_init(JOYSTICK_X_PIN); 
    adc_gpio_init(JOYSTICK_Y_PIN);
    adc_select_input(2); srand(adc_read()); 

    // --- CONFIGURAÇÃO DO BOTÃO A ---
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN); // Importante: Botão aterra o pino

    sleep_ms(2000);

    if (iniciar_modelo() != 0) {
        printf("ERRO CRITICO TFLITE\n");
        while(1) tight_loop_contents();
    }

    // Gera primeiro paciente
    gerar_paciente_aleatorio();
    imprimir_ficha_paciente();
    
    char perfil_str[20];
    sprintf(perfil_str, "%c/%.0fa", (perfil[IDX_MALE]>0.5)?'M':'F', perfil[IDX_AGE]);

    imprimir_cabecalho_monitor();
    
    char buf[32];
    int count = 0;

    while (true) {
        // --- NOVO: VERIFICAÇÃO DO BOTÃO A ---
        if (!gpio_get(BUTTON_A_PIN)) { // Botão pressionado (nível lógico 0)
            // 1. Feedback Visual OLED
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "TROCANDO", 30, 20);
            ssd1306_draw_string(&ssd, "PACIENTE...", 25, 35);
            ssd1306_send_data(&ssd);
            
            // 2. Gera novos dados
            gerar_paciente_aleatorio();
            
            // 3. Atualiza Serial e Strings
            imprimir_ficha_paciente();
            sprintf(perfil_str, "%c/%.0fa", (perfil[IDX_MALE]>0.5)?'M':'F', perfil[IDX_AGE]);
            imprimir_cabecalho_monitor();
            count = 0; // Reseta contador do cabeçalho

            // 4. Delay para não trocar 50 vezes num clique (Debounce)
            sleep_ms(1000); 
        }

        // Leitura Sensores
        adc_select_input(0); uint16_t adc_x = adc_read();
        adc_select_input(1); uint16_t adc_y = adc_read();

        float sys_bp = map_val(adc_x, 0, 4095, 90.0f, 220.0f);
        float glicose = map_val(adc_y, 0, 4095, 50.0f, 350.0f);
        float dia_bp = (sys_bp * 0.65f);

        perfil[IDX_SYSBP] = sys_bp;
        perfil[IDX_DIABP] = dia_bp;
        perfil[IDX_GLUCOSE] = glicose;

        // Normalização
        dados_para_ia[IDX_MALE] = normalizar(perfil[IDX_MALE], MEAN_MALE, STD_MALE);
        dados_para_ia[IDX_AGE] = normalizar(perfil[IDX_AGE], MEAN_AGE, STD_AGE);
        dados_para_ia[IDX_EDUCATION] = normalizar(perfil[IDX_EDUCATION], MEAN_EDUCATION, STD_EDUCATION);
        dados_para_ia[IDX_CURRENTSMOKER] = normalizar(perfil[IDX_CURRENTSMOKER], MEAN_CURRENTSMOKER, STD_CURRENTSMOKER);
        dados_para_ia[IDX_CIGSPERDAY] = normalizar(perfil[IDX_CIGSPERDAY], MEAN_CIGSPERDAY, STD_CIGSPERDAY);
        dados_para_ia[IDX_BPMEDS] = normalizar(perfil[IDX_BPMEDS], MEAN_BPMEDS, STD_BPMEDS);
        dados_para_ia[IDX_PREVALENTSTROKE] = normalizar(perfil[IDX_PREVALENTSTROKE], MEAN_PREVALENTSTROKE, STD_PREVALENTSTROKE);
        dados_para_ia[IDX_PREVALENTHYP] = normalizar(perfil[IDX_PREVALENTHYP], MEAN_PREVALENTHYP, STD_PREVALENTHYP);
        dados_para_ia[IDX_DIABETES] = normalizar(perfil[IDX_DIABETES], MEAN_DIABETES, STD_DIABETES);
        dados_para_ia[IDX_TOTCHOL] = normalizar(perfil[IDX_TOTCHOL], MEAN_TOTCHOL, STD_TOTCHOL);
        dados_para_ia[IDX_SYSBP] = normalizar(perfil[IDX_SYSBP], MEAN_SYSBP, STD_SYSBP);
        dados_para_ia[IDX_DIABP] = normalizar(perfil[IDX_DIABP], MEAN_DIABP, STD_DIABP);
        dados_para_ia[IDX_BMI] = normalizar(perfil[IDX_BMI], MEAN_BMI, STD_BMI);
        dados_para_ia[IDX_HEARTRATE] = normalizar(perfil[IDX_HEARTRATE], MEAN_HEARTRATE, STD_HEARTRATE);
        dados_para_ia[IDX_GLUCOSE] = normalizar(perfil[IDX_GLUCOSE], MEAN_GLUCOSE, STD_GLUCOSE);

        // Inferência
        float risco = fazer_predicao(dados_para_ia);
        float risco_percent = risco * 100.0f;

        // OLED
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "MONITOR IA", 30, 0);
        ssd1306_rect(&ssd, 9, 0, 128, 2, true, true); 

        sprintf(buf, "PA : %3.0f/%3.0f", sys_bp, dia_bp);
        ssd1306_draw_string(&ssd, buf, 0, 14);
        
        sprintf(buf, "GLI: %3.0f mg/dL", glicose);
        ssd1306_draw_string(&ssd, buf, 0, 24);

        // Rodapé com info do paciente
        char rodape[20];
        sprintf(rodape, "%s %.0f Anos", (perfil[IDX_MALE]>0.5)?"M":"F", perfil[IDX_AGE]);
        ssd1306_draw_string(&ssd, rodape, 0, 54);

        // Barra Risco Vertical
        int altura = (int)(risco * 40);
        if (altura > 40) altura = 40;
        ssd1306_rect(&ssd, 12, 110, 8, 42, true, false); // Moldura
        ssd1306_rect(&ssd, 12 + (42-altura), 110, 8, altura, true, true); // Fill
        
        sprintf(buf, "%.0f%%", risco_percent);
        ssd1306_draw_string(&ssd, buf, 90, 35);

        ssd1306_send_data(&ssd);
        
        // Serial
        char status[10];
        if (risco_percent < 10) sprintf(status, "OK");
        else if (risco_percent < 30) sprintf(status, "ATENCAO");
        else sprintf(status, "PERIGO");

        printf("| %3.0f/%3.0f mmHg | %3.0f mg/dL    | %4.1f%% | %-9s | %-12s |\n", 
               sys_bp, dia_bp, glicose, risco_percent, status, perfil_str);

        if (++count > 15) {
            imprimir_cabecalho_monitor();
            count = 0;
        }
        
        gpio_put(LED_PIN, risco > 0.5f ? 1 : 0);
        sleep_ms(250); 
    }
    return 0;
}