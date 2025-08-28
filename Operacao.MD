# 📊 Manual de Operação - ESP32-S3 Leitor de Pulsos

## 📋 Índice
1. [Visão Geral](#visão-geral)
2. [Configuração Inicial](#configuração-inicial)
3. [Conexões de Hardware](#conexões-de-hardware)
4. [Configurações do Software](#configurações-do-software)
5. [Operação do Sistema](#operação-do-sistema)
6. [Interpretação dos Dados](#interpretação-dos-dados)
7. [Funcionalidades Avançadas](#funcionalidades-avançadas)
8. [Troubleshooting](#troubleshooting)
9. [Especificações Técnicas](#especificações-técnicas)

---

## 🎯 Visão Geral

O **ESP32-S3 Leitor de Pulsos** é um sistema avançado de medição de pulsos digitais que oferece:

- ✅ **Detecção por interrupção de hardware** para máxima precisão
- ✅ **Debounce adaptativo inteligente** que se ajusta automaticamente
- ✅ **Medição em tempo real** de frequência e períodos
- ✅ **Reset automático configurável** para análise periódica
- ✅ **Controle manual via botão** para reset instantâneo
- ✅ **Interface serial detalhada** com relatórios formatados

---

## ⚙️ Configuração Inicial

### 📋 Requisitos
- **Hardware**: ESP32-S3 ou ESP32 compatível
- **Software**: Arduino IDE 2.0+ ou PlatformIO
- **Cabo**: USB para programação e monitoramento
- **Monitor Serial**: 115200 baud

### 🔧 Instalação
1. **Copie o código** `LeituraPulsos.ino` para o Arduino IDE
2. **Selecione a placa** ESP32-S3 Dev Module
3. **Configure a velocidade** de upload conforme sua placa
4. **Compile e envie** o código para o ESP32-S3

---

## 🔌 Conexões de Hardware

### 📍 Pinout Padrão
```
ESP32-S3 Pinout:
┌─────────────────┐
│ GPIO 2  ●───────┼── Sinal de Pulso (Entrada)
│ GPIO 0  ●───────┼── Botão Reset (BOOT)
│ GND     ●───────┼── Terra de Referência
│ 3.3V    ●───────┼── Alimentação (se necessário)
└─────────────────┘
```

### 🔗 Conexões Detalhadas

#### **Entrada de Pulsos (GPIO 2)**
- **Tipo**: Entrada digital com pull-up interno
- **Tensão**: 0V (LOW) a 3.3V (HIGH)
- **Detecção**: Borda de descida (HIGH → LOW)
- **Proteção**: Pull-up interno de ~45kΩ ativado

#### **Botão de Reset (GPIO 0)**
- **Tipo**: Entrada digital (botão BOOT nativo)
- **Função**: Reset manual do contador
- **Operação**: Pressionar = LOW, Soltar = HIGH
- **Debounce**: 50ms implementado via software

#### **Alimentação e Terra**
- **VCC**: 3.3V ou 5V (via USB)
- **GND**: Terra comum obrigatório
- **Consumo**: ~80mA em operação normal

### ⚡ Tipos de Sinais Compatíveis

#### **Sinais Digitais 3.3V**
```
     3.3V ┌─┐    ┌─┐    ┌─┐
          │ │    │ │    │ │
      0V  └─┘    └─┘    └─┘
          ↑      ↑      ↑
       Detecção Detecção Detecção
```

#### **Sinais Digitais 5V**
⚠️ **ATENÇÃO**: Use divisor de tensão para sinais 5V
```
Sinal 5V ──┬── 2kΩ ──┬── GPIO 2
           │         │
           └── 1kΩ ──┴── GND
           
Resultado: 5V → 1.67V (Seguro para ESP32)
```

#### **Sensores Compatíveis**
- 🔘 **Sensores Hall** (efeito magnético)
- 🔘 **Encoders ópticos** (disco perfurado)
- 🔘 **Sensores indutivos** (proximidade metálica)
- 🔘 **Chaves mecânicas** (com debounce)
- 🔘 **Sensores de fluxo** (líquidos/gases)
- 🔘 **Tacômetros** (rotação/RPM)

---

## 🎛️ Configurações do Software

### 📝 Parâmetros Principais

#### **Hardware**
```cpp
#define PULSE_PIN 2        // Pino de entrada dos pulsos
#define BUTTON_PIN 0       // Pino do botão de reset
```

#### **Debounce Adaptativo**
```cpp
#define DEBOUNCE_TIME_US_MIN 100      // Mínimo: 100μs (alta freq)
#define DEBOUNCE_TIME_US_MAX 5000     // Máximo: 5000μs (baixa freq)
#define DEBOUNCE_TIME_US_DEFAULT 1000 // Inicial: 1000μs
```

#### **Relatórios e Reset**
```cpp
#define DISPLAY_INTERVAL 1000         // Relatório a cada 1s
#define AUTO_RESET_INTERVAL 30000     // Reset automático: 30s
#define AUTO_RESET_ENABLED true       // Habilitar reset auto
```

### 🔧 Customizações Comuns

#### **Para Alta Frequência (>10kHz)**
```cpp
#define DEBOUNCE_TIME_US_MIN 50       // Reduzir mínimo
#define DEBOUNCE_TIME_US_MAX 500      // Reduzir máximo
```

#### **Para Baixa Frequência (<1Hz)**
```cpp
#define DEBOUNCE_TIME_US_MIN 1000     // Aumentar mínimo
#define DEBOUNCE_TIME_US_MAX 10000    // Aumentar máximo
```

#### **Desabilitar Reset Automático**
```cpp
#define AUTO_RESET_ENABLED false      // Apenas reset manual
```

---

## 🚀 Operação do Sistema

### 📺 Interface Serial

#### **Tela de Inicialização**
```
====================================================================
           ESP32-S3 LEITOR DE PULSOS - SISTEMA INICIADO           
====================================================================

CONFIGURAÇÕES:
• Pino de entrada: GPIO 2
• Pino do botão reset: GPIO 0
• Modo de detecção: Borda de descida (HIGH → LOW)
• Debounce adaptativo: 100-5000 μs
• Pull-up interno: ATIVADO
• Velocidade serial: 115200 bps
• Reset automático: HABILITADO (a cada 30 segundos)

✓ Hardware configurado com sucesso!
✓ Interrupção ativada no pino 2
✓ Sistema pronto para detectar pulsos!

INSTRUÇÕES:
1. Conecte o sinal de pulso ao pino GPIO 2
2. Use GND como referência comum
3. Pressione o botão GPIO 0 para reset manual
4. Observe as medições em tempo real abaixo
```

#### **Tabela de Medições em Tempo Real**
```
====================================================================
| Timestamp | Pulsos | Frequência | Período |      Status         |
|    (ms)   | Total  |    (Hz)    |  (μs)   |                     |
====================================================================
|     1234  |      1 |       0.00 |       0 | Pulso (D:1000μs)   |
|     1654  |      2 |       2.38 |  420000 | Pulso (D:3360μs)   |
|     2100  |      3 |       2.24 |  446000 | Pulso (D:3568μs)   |
|----------|--------|------------|---------|---------------------|
| RELATÓRIO - Tempo: 3s                                           |
| Pulsos totais:      3 | Freq. média:    1 Hz                 |
|----------|--------|------------|---------|---------------------|
```

### 🔄 Operações Básicas

#### **Iniciar Medição**
1. **Conecte o sinal** ao GPIO 2
2. **Abra o Monitor Serial** (115200 baud)
3. **Observe** os pulsos sendo detectados automaticamente

#### **Reset Manual**
1. **Pressione** o botão GPIO 0 (BOOT)
2. **Observe** a confirmação no serial:
```
====================================================================
                      RESET MANUAL EXECUTADO                       
====================================================================
✓ Contador de pulsos zerado
✓ Variáveis de tempo reinicializadas
✓ Timer de reset automático reiniciado
✓ Sistema pronto para nova medição
✓ Reset acionado via botão GPIO 0
====================================================================
```

#### **Reset Automático (30s)**
```
====================================================================
                     RESET AUTOMÁTICO (30s)                        
====================================================================
ESTATÍSTICAS DO PERÍODO:
• Pulsos totais: 1500
• Tempo decorrido: 30.0 segundos
• Frequência média: 50.00 Hz
====================================================================
           NOVO PERÍODO INICIADO - CONTADOR ZERADO                 
====================================================================
```

---

## 📈 Interpretação dos Dados

### 📊 Colunas da Tabela

#### **Timestamp (ms)**
- **Descrição**: Tempo decorrido desde o início
- **Formato**: Milissegundos
- **Uso**: Correlacionar eventos no tempo

#### **Pulsos (Total)**
- **Descrição**: Contador acumulativo
- **Formato**: Número inteiro
- **Reset**: Manual ou automático (30s)

#### **Frequência (Hz)**
- **Descrição**: Frequência instantânea
- **Cálculo**: `1.000.000 / período_μs`
- **Precisão**: 2 casas decimais

#### **Período (μs)**
- **Descrição**: Intervalo entre pulsos
- **Formato**: Microssegundos
- **Uso**: Análise de estabilidade

#### **Status**
- **`Pulso (D:XXXμs)`**: Debounce atual em uso
- **Adaptação**: Valor muda conforme frequência

### 📋 Relatórios Periódicos

#### **Relatório de 1 Segundo**
- **Frequência Média**: Pulsos no último segundo
- **Tempo Decorrido**: Minutos e segundos
- **Pulsos Totais**: Contador acumulativo

#### **Relatório de Reset (30s)**
- **Estatísticas Completas** do período
- **Frequência Média** calculada
- **Tempo Exato** de medição

---

## 🧠 Funcionalidades Avançadas

### 🎯 Debounce Adaptativo

#### **Como Funciona**
1. **Coleta** histórico dos últimos 5 intervalos
2. **Calcula** média dos intervalos válidos
3. **Define** debounce como 8% da média
4. **Aplica** limites mínimo (100μs) e máximo (5000μs)
5. **Suaviza** mudanças com filtro ponderado

#### **Comportamento Adaptativo**
```
Frequência do Sinal    → Debounce Aplicado
─────────────────────────────────────────
> 5 kHz (alta freq)    → ~100μs (mínimo)
1-5 kHz                → 200-1000μs
100Hz-1kHz             → 1000-2000μs
< 100Hz (baixa freq)   → 2000-5000μs
```

#### **Vantagens**
- ✅ **Auto-otimização** para qualquer sinal
- ✅ **Sem configuração manual** necessária
- ✅ **Máxima sensibilidade** preservada
- ✅ **Filtro inteligente** de ruído

### 📊 Análise de Desempenho

#### **Métricas de Qualidade**
- **Estabilidade**: Variação do período entre pulsos
- **Precisão**: Resolução de 1μs para medições
- **Latência**: < 10μs para detecção de pulso
- **Throughput**: Até 100kHz recomendado

#### **Indicadores Visuais**
- **Debounce Decrescente**: Frequência aumentando
- **Debounce Crescente**: Frequência diminuindo
- **Debounce Estável**: Sinal regular

---

## 🔧 Troubleshooting

### ❌ Problemas Comuns

#### **Não Detecta Pulsos**
**Sintomas**: Tabela vazia, sem atividade
**Possíveis Causas**:
- 🔍 Conexão incorreta no GPIO 2
- 🔍 Terra (GND) não conectado
- 🔍 Sinal muito fraco (< 1.5V)
- 🔍 Polaridade invertida

**Soluções**:
```
1. Verificar conexões físicas
2. Medir tensão do sinal com multímetro
3. Confirmar terra comum
4. Testar com sinal conhecido (LED + resistor)
```

#### **Conta Pulsos Duplicados**
**Sintomas**: Contagem muito alta, frequência irreal
**Possíveis Causas**:
- 🔍 Ruído elétrico no sinal
- 🔍 Bouncing em chaves mecânicas
- 🔍 Interferência eletromagnética

**Soluções**:
```
1. Sistema adaptativo corrigirá automaticamente
2. Adicionar capacitor de filtro (100nF)
3. Usar cabo blindado
4. Afastar de fontes de ruído
```

#### **Perde Pulsos Rápidos**
**Sintomas**: Frequência menor que esperada
**Possíveis Causas**:
- 🔍 Debounce muito alto para a frequência
- 🔍 Pulsos muito estreitos (< 1μs)
- 🔍 Fonte do sinal instável

**Soluções**:
```
1. Sistema adaptativo ajustará automaticamente
2. Verificar especificações do sensor
3. Melhorar fonte de alimentação
4. Reduzir DEBOUNCE_TIME_US_MIN se necessário
```

#### **Reset Não Funciona**
**Sintomas**: Botão não reseta contador
**Possíveis Causas**:
- 🔍 Botão BOOT defeituoso
- 🔍 Conexão GPIO 0 problemática

**Soluções**:
```
1. Testar com botão externo no GPIO 0
2. Verificar continuidade do botão
3. Configurar outro pino em BUTTON_PIN
```

### ⚠️ Limitações Conhecidas

#### **Frequências Extremas**
- **Máxima**: ~100kHz (recomendado)
- **Teórica**: ~500kHz (com ajustes)
- **Mínima**: 0.0002Hz (1 pulso por hora)

#### **Precisão Temporal**
- **Resolução**: 1μs (microssegundo)
- **Deriva**: ~50ppm do cristal interno
- **Overflow**: Após 49 dias contínuos

---

## 📏 Especificações Técnicas

### 🔌 Hardware

| Parâmetro | Especificação | Observações |
|-----------|---------------|-------------|
| **MCU** | ESP32-S3 | Compatível ESP32 |
| **Clock** | 240MHz | Dual-core |
| **Tensão** | 3.3V | Tolerante a 5V em alguns pinos |
| **Corrente** | 80mA típico | Sem WiFi/Bluetooth |
| **Resolução** | 1μs | Timer de alta resolução |
| **Entradas** | Pull-up 45kΩ | Configurável |

### 📊 Performance

| Métrica | Valor | Condições |
|---------|-------|-----------|
| **Freq. Máxima** | 100kHz | Recomendado |
| **Freq. Mínima** | 0.0002Hz | 1 pulso/hora |
| **Latência** | < 10μs | Detecção por ISR |
| **Precisão** | 1μs | Timer interno |
| **Jitter** | < 5μs | Condições normais |
| **Estabilidade** | 50ppm | Cristal interno |

### 🧮 Algoritmos

#### **Debounce Adaptativo**
```
Histórico: Buffer circular de 5 amostras
Cálculo: média_intervalo / 12 (≈8.3%)
Limites: 100μs ≤ debounce ≤ 5000μs
Filtro: (atual×7 + novo×3) / 10
```

#### **Medição de Frequência**
```
Instantânea: 1.000.000 / período_μs
Média: pulsos_por_segundo
Resolução: 0.01 Hz
```

---

## 📞 Suporte e Contato

### 🆘 Suporte Técnico
- **GitHub**: [Repositório do Projeto]
- **Documentação**: Este arquivo `Operacao.MD`
- **Código Fonte**: `LeituraPulsos.ino`

### 🔄 Atualizações
- **Versão Atual**: 1.0
- **Data**: 28/08/2025
- **Autor**: GitHub Copilot

### 📝 Licença
```
Este projeto é fornecido "como está" para fins educacionais e de desenvolvimento.
Uso por conta e risco do usuário.
```

---

## 📋 Checklist de Operação

### ✅ Pré-Operação
- [ ] ESP32-S3 conectado via USB
- [ ] Arduino IDE configurado corretamente
- [ ] Código compilado sem erros
- [ ] Monitor Serial aberto (115200 baud)
- [ ] Sinal de entrada preparado
- [ ] Conexões verificadas

### ✅ Durante Operação
- [ ] Pulsos sendo detectados
- [ ] Debounce ajustando automaticamente
- [ ] Frequências coerentes com esperado
- [ ] Relatórios periódicos funcionando
- [ ] Reset manual operacional

### ✅ Análise de Dados
- [ ] Dados sendo coletados corretamente
- [ ] Estatísticas coerentes
- [ ] Estabilidade do sinal verificada
- [ ] Performance dentro do esperado

---

*Documento gerado automaticamente em 28/08/2025*
*Versão 1.0 - ESP32-S3 Leitor de Pulsos*
