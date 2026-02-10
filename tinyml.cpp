#include <cstdint>
#include <stdio.h>
#include "tinyml.h"
#include "model.h" // Certifique-se que o arquivo gerado pelo Colab se chama model.h

// --- BIBLIOTECAS MODERNAS DO TENSORFLOW ---
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Variáveis Globais
const int kTensorArenaSize = 10 * 1024; // Aumentado para 10kb por segurança
uint8_t tensor_arena[kTensorArenaSize] __attribute__((aligned(16)));

tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

extern "C" int iniciar_modelo(void) {
    const tflite::Model* model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        printf("Erro: Schema Version incorreto.\n");
        return -1;
    }

    // --- ADAPTADO PARA O MODELO DE CORAÇÃO ---
    static tflite::MicroMutableOpResolver<10> resolver;
    
    resolver.AddFullyConnected(); // Camadas densas
    resolver.AddRelu();           // Ativação intermediária
    
    // MUDANÇA 1: Usar LOGISTIC (Sigmoid) em vez de Softmax
    resolver.AddLogistic();       // O modelo do Colab usou activation='sigmoid'
    
    resolver.AddQuantize();       // Suporte a quantização
    resolver.AddDequantize(); 
    resolver.AddReshape();        
    
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        printf("Erro: Falha ao alocar tensores.\n");
        return -2;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
    
    // Debug inicial
    printf("\n[DEBUG] Modelo carregado.\n");
    printf("[DEBUG] Entrada: %d (Esperado: 15)\n", input->dims->data[1]);
    printf("[DEBUG] Saida:   %d (Esperado: 1)\n", output->dims->data[1]);

    return 0;
}

extern "C" float fazer_predicao(float* dados_entrada) {
    if (interpreter == nullptr) return 0.0f;

    // Copia dados para o tensor de entrada
    // O modelo Heart espera 15 entradas (verifica dinamicamente)
    int tamanho_esperado = input->dims->data[1]; 
    for (int i = 0; i < tamanho_esperado; i++) {
        input->data.f[i] = dados_entrada[i];
    }

    if (interpreter->Invoke() != kTfLiteOk) {
        printf("Erro na inferencia.\n");
        return -1.0f;
    }

    // MUDANÇA 2: O modelo de coração tem apenas 1 saída (Probabilidade)
    // O de diabetes tinha 2 [Neg, Pos]. O Heart é direto [Prob].
    float probabilidade = output->data.f[0]; 
    
    return probabilidade;
}