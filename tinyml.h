#ifndef TINYML_H
#define TINYML_H

#ifdef __cplusplus
extern "C" {
#endif

// Declaração das funções
int iniciar_modelo(void);
float fazer_predicao(float* dados_entrada);

#ifdef __cplusplus
}
#endif

#endif // TINYML_H