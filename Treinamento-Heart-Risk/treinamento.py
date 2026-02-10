import pandas as pd
import numpy as np
import tensorflow as tf
from sklearn.preprocessing import StandardScaler
import os

# ==========================================
# 1. CARREGAMENTO E LIMPEZA DE DADOS
# ==========================================
print("--- 1. Processando Dados ---")
# Certifique-se de que o arquivo 'framingham - pre processado.csv' está na pasta do Colab
df = pd.read_csv('framingham - pre processado.csv', sep=';')

# LIMPEZA: Removemos valores impossíveis (erros de digitação no dataset original)
# Ex: Pressão diastólica acima de 150 ou sistólica acima de 300 são descartadas
df_clean = df[(df['sysBP'] <= 300) & (df['diaBP'] <= 150)].dropna()

print(f"Dataset Original: {len(df)} pacientes")
print(f"Dataset Limpo:    {len(df_clean)} pacientes (Erros removidos)")

# Separar Features (X) e Alvo (y)
X = df_clean.drop('TenYearCHD', axis=1)
y = df_clean['TenYearCHD']
col_names = list(X.columns)

# ==========================================
# 2. NORMALIZAÇÃO
# ==========================================
# A rede neural precisa de dados normalizados (Média 0, Desvio 1)
scaler = StandardScaler()
X_scaled = scaler.fit_transform(X)

# Guardamos essas médias para usar no Arduino/Pico depois
means = scaler.mean_
scales = scaler.scale_

# ==========================================
# 3. TREINAMENTO DO MODELO
# ==========================================
print("\n--- 2. Treinando Rede Neural ---")
tf.random.set_seed(42) # Para resultados reproduzíveis

model = tf.keras.Sequential([
    tf.keras.layers.Dense(16, activation='relu', input_shape=(15,)),
    tf.keras.layers.Dense(8, activation='relu'),
    tf.keras.layers.Dense(1, activation='sigmoid') # Saída entre 0 e 1 (Risco)
])

model.compile(optimizer='adam', loss='binary_crossentropy', metrics=['accuracy'])
model.fit(X_scaled, y, epochs=50, batch_size=16, verbose=0)
loss, acc = model.evaluate(X_scaled, y, verbose=0)
print(f"Acurácia do Modelo: {acc*100:.2f}%")

# ==========================================
# 4. CONVERSÃO PARA TENSORFLOW LITE
# ==========================================
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

# ==========================================
# 5. GERAR ARQUIVO .H (HEADER)
# ==========================================
def generate_header(hex_data):
    c_str = '#ifndef MODEL_H\n'
    c_str += '#define MODEL_H\n\n'
    c_str += '// Modelo treinado no Google Colab\n'
    c_str += f'const unsigned int model_data_len = {len(hex_data)};\n'
    c_str += 'const unsigned char model_data[] = {\n'

    for i, val in enumerate(hex_data):
        if i % 12 == 0:
            c_str += '  '
        c_str += f'0x{val:02x}, '
        if (i + 1) % 12 == 0:
            c_str += '\n'

    c_str += '\n};\n\n'
    c_str += '#endif // MODEL_H'
    return c_str

header_content = generate_header(tflite_model)

# Salva o arquivo 'model.h' na pasta do Colab
with open('model.h', 'w') as f:
    f.write(header_content)

print("\n" + "="*50)
print("SUCESSO! Arquivo 'model.h' gerado.")
print("Verifique a aba de ARQUIVOS (ícone de pasta na esquerda) para baixar.")
print("="*50)

# ==========================================
# 6. IMPRIMIR CONSTANTES PARA O MAIN.CPP
# ==========================================
print("\n>>> COPIE E COLE ESTAS CONSTANTES NO SEU 'main.cpp' <<<")
print("// Copie abaixo dos includes")
print("// Médias e Desvios Padrão para Normalização (Z-Score)")
for i, col in enumerate(col_names):
    print(f"const int IDX_{col.upper()} = {i};")
    print(f"const float MEAN_{col.upper()} = {means[i]:.4f};")
    print(f"const float STD_{col.upper()} = {scales[i]:.4f};")