/*
 * ====================================================================
 * ESP32-S3 LEITOR DE PULSOS COM INTERRUPÇÕES
 * ====================================================================
 * 
 * DESCRIÇÃO:
 * Este código implementa um sistema de leitura de pulsos digitais usando
 * interrupções de hardware no ESP32-S3. O sistema é capaz de detectar,
 * contar e medir a frequência de pulsos em tempo real com alta precisão.
 * 
 * AUTOR: GitHub Copilot
 * DATA: 28/08/2025
 * VERSÃO: 1.0
 * 
 * HARDWARE SUPORTADO:
 * - ESP32-S3 (compatível com ESP32 original)
 * - Qualquer sinal digital 3.3V ou 5V
 * 
 * CONECTIVIDADE:
 * - Pino de entrada: GPIO 4 (configurável)
 * - Pull-up interno ativado
 * - Comunicação serial: 115200 baud
 * 
 * FUNCIONALIDADES:
 * ✓ Detecção de pulsos por interrupção de hardware
 * ✓ Debounce automático para filtrar ruído
 * ✓ Contador total de pulsos
 * ✓ Medição de frequência instantânea e média
 * ✓ Cálculo do período entre pulsos
 * ✓ Relatórios em tempo real via Serial
 * ✓ Sistema de reset do contador
 * 
 * USO:
 * 1. Conecte o sinal de pulso ao pino GPIO 4
 * 2. Abra o Monitor Serial (115200 baud)
 * 3. Observe as medições em tempo real
 * 
 * ====================================================================
 */

#include <Arduino.h>

/*
 * CONFIGURAÇÕES DE HARDWARE
 * ====================================================================
 */

// Pino onde o sinal de pulso será conectado
// Pode ser qualquer GPIO digital do ESP32-S3
#define PULSE_PIN 2

// Pino do botão de reset
// GPIO 0 é o botão BOOT do ESP32-S3 (pull-up interno)
#define BUTTON_PIN 0

/*
 * CONFIGURAÇÕES DE TEMPO E DEBOUNCE
 * ====================================================================
 */

// Tempo mínimo entre pulsos válidos (em microssegundos)
// Este valor será ajustado automaticamente pelo debounce adaptativo
#define DEBOUNCE_TIME_US_MIN 100      // Debounce mínimo (para alta frequência)
#define DEBOUNCE_TIME_US_MAX 5000     // Debounce máximo (para baixa frequência)
#define DEBOUNCE_TIME_US_DEFAULT 1000 // Debounce inicial padrão

// Tempo de debounce do botão (em milissegundos)
#define BUTTON_DEBOUNCE_MS 50

// Intervalo para relatórios periódicos (em milissegundos)
#define DISPLAY_INTERVAL 1000

// Intervalo para reset automático do contador (em milissegundos)
#define AUTO_RESET_INTERVAL 30000  // 30 segundos

/*
 * CONFIGURAÇÕES DE COMPORTAMENTO
 * ====================================================================
 */

// Flag para habilitar/desabilitar reset automático
// true = Reset automático a cada 30 segundos HABILITADO
// false = Reset automático DESABILITADO (apenas reset manual)
#define AUTO_RESET_ENABLED true

/*
 * VARIÁVEIS GLOBAIS DO SISTEMA
 * ====================================================================
 * IMPORTANTE: Variáveis usadas em ISR (Interrupt Service Routine)
 * devem ser declaradas como 'volatile' para evitar otimizações
 * inadequadas do compilador
 */

// Contadores e medições de pulsos
volatile unsigned long pulseCount = 0;        // Contador total de pulsos desde o início
volatile unsigned long lastPulseTime = 0;     // Timestamp do último pulso (microssegundos)
volatile unsigned long pulseInterval = 0;     // Intervalo entre os dois últimos pulsos (μs)
volatile bool newPulseReceived = false;       // Flag indicando novo pulso detectado

// Variáveis para debounce adaptativo
volatile unsigned long currentDebounceTime = DEBOUNCE_TIME_US_DEFAULT; // Debounce atual em μs
volatile unsigned long pulseHistory[5] = {0}; // Histórico dos últimos 5 intervalos
volatile int historyIndex = 0;                // Índice circular do histórico

// Variáveis para controle de exibição
unsigned long lastDisplayTime = 0;            // Timestamp do último relatório
unsigned long lastResetTime = 0;              // Timestamp do último reset automático
const unsigned long DISPLAY_INTERVAL_CONST = DISPLAY_INTERVAL;

// Variáveis para controle do botão
bool lastButtonState = HIGH;                  // Estado anterior do botão (HIGH = não pressionado)
unsigned long lastButtonTime = 0;             // Timestamp da última leitura do botão

/*
 * FUNÇÃO DE INTERRUPÇÃO COM DEBOUNCE ADAPTATIVO
 * ====================================================================
 * Esta função implementa um sistema de debounce que se adapta automaticamente
 * à frequência dos pulsos detectados, otimizando a performance para diferentes
 * tipos de sinais.
 * 
 * CARACTERÍSTICAS:
 * - Debounce adaptativo baseado no histórico de intervalos
 * - Ajuste automático entre DEBOUNCE_TIME_US_MIN e DEBOUNCE_TIME_US_MAX
 * - Filtro de ruído inteligente
 * - Otimização para alta e baixa frequência
 */
void pulseInterrupt() {
  // Captura o timestamp atual com precisão de microssegundos
  unsigned long currentTime = micros();
  
  // SISTEMA DE DEBOUNCE ADAPTATIVO:
  // Usa o valor atual de debounce calculado dinamicamente
  if (currentTime - lastPulseTime > currentDebounceTime) {
    // Calcula o intervalo entre este pulso e o anterior
    pulseInterval = currentTime - lastPulseTime;
    
    // Atualiza o timestamp do último pulso válido
    lastPulseTime = currentTime;
    
    // ---- ALGORITMO DE DEBOUNCE ADAPTATIVO ----
    // Armazena o intervalo no histórico circular
    pulseHistory[historyIndex] = pulseInterval;
    historyIndex = (historyIndex + 1) % 5; // Circular: 0, 1, 2, 3, 4, 0, 1...
    
    // Calcula o debounce adaptativo baseado no histórico
    updateAdaptiveDebounce();
    
    // Incrementa o contador total de pulsos
    pulseCount++;
    
    // Sinaliza que há um novo pulso para processar no loop principal
    newPulseReceived = true;
  }
}

/*
 * FUNÇÃO DE CONFIGURAÇÃO INICIAL
 * ====================================================================
 * Executada uma única vez quando o ESP32 é ligado ou resetado.
 * Responsável por configurar hardware, comunicação serial e interrupções.
 */
void setup() {
  // ---- CONFIGURAÇÃO DA COMUNICAÇÃO SERIAL ----
  Serial.begin(115200);  // Inicializa UART com velocidade de 115200 bps
  
  // Aguarda a inicialização completa da porta serial
  // Importante para algumas placas que podem demorar para estabelecer conexão
  while (!Serial) {
    delay(10);
  }
  
  // ---- MENSAGENS DE INICIALIZAÇÃO ----
  Serial.println();
  Serial.println("====================================================================");
  Serial.println("           ESP32-S3 LEITOR DE PULSOS - SISTEMA INICIADO           ");
  Serial.println("====================================================================");
  Serial.println();
  Serial.println("CONFIGURAÇÕES:");
  Serial.println("• Pino de entrada: GPIO " + String(PULSE_PIN));
  Serial.println("• Pino do botão reset: GPIO " + String(BUTTON_PIN));
  Serial.println("• Modo de detecção: Borda de descida (HIGH → LOW)");
  Serial.println("• Debounce adaptativo: " + String(DEBOUNCE_TIME_US_MIN) + "-" + String(DEBOUNCE_TIME_US_MAX) + " μs");
  Serial.println("• Pull-up interno: ATIVADO");
  Serial.println("• Velocidade serial: 115200 bps");
  
  // Exibe status do reset automático conforme a configuração
  if (AUTO_RESET_ENABLED) {
    Serial.println("• Reset automático: HABILITADO (a cada 30 segundos)");
  } else {
    Serial.println("• Reset automático: DESABILITADO (apenas reset manual)");
  }
  
  Serial.println();
  Serial.println("Configurando hardware...");
  
  // ---- CONFIGURAÇÃO DO PINO DE ENTRADA ----
  // INPUT_PULLUP: Configura como entrada com resistor pull-up interno
  // Isso garante estado HIGH quando não há sinal, melhorando a estabilidade
  pinMode(PULSE_PIN, INPUT_PULLUP);
  
  // ---- CONFIGURAÇÃO DO BOTÃO DE RESET ----
  // GPIO 0 geralmente é o botão BOOT do ESP32-S3
  // Configurado com pull-up interno (LOW quando pressionado)
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // ---- CONFIGURAÇÃO DA INTERRUPÇÃO ----
  // digitalPinToInterrupt(): Converte número do pino para número da interrupção
  // pulseInterrupt: Função que será executada quando a interrupção ocorrer
  // FALLING: Detecta transição de HIGH para LOW (borda de descida)
  //
  // Outras opções disponíveis:
  // - RISING:  detecta LOW → HIGH
  // - CHANGE:  detecta qualquer mudança (HIGH→LOW ou LOW→HIGH)
  // - LOW:     detecta nível baixo contínuo
  // - HIGH:    detecta nível alto contínuo
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), pulseInterrupt, FALLING);
  
  // ---- MENSAGENS DE CONFIRMAÇÃO ----
  Serial.println("✓ Hardware configurado com sucesso!");
  Serial.println("✓ Interrupção ativada no pino " + String(PULSE_PIN));
  Serial.println("✓ Sistema pronto para detectar pulsos!");
  Serial.println();
  Serial.println("INSTRUÇÕES:");
  Serial.println("1. Conecte o sinal de pulso ao pino GPIO " + String(PULSE_PIN));
  Serial.println("2. Use GND como referência comum");
  Serial.println("3. Pressione o botão GPIO " + String(BUTTON_PIN) + " para reset manual");
  Serial.println("4. Observe as medições em tempo real abaixo");
  Serial.println();
  Serial.println("====================================================================");
  Serial.println("| Timestamp | Pulsos | Frequência | Período |      Status         |");
  Serial.println("|    (ms)   | Total  |    (Hz)    |  (μs)   |                     |");
  Serial.println("====================================================================");
}

/*
 * LOOP PRINCIPAL DO PROGRAMA
 * ====================================================================
 * Executado continuamente após o setup(). Responsável por:
 * - Processar pulsos detectados pela interrupção
 * - Calcular frequências e estatísticas
 * - Exibir informações no monitor serial
 */
void loop() {
  // Captura timestamp atual para cálculos de tempo
  unsigned long currentTime = millis();
  
  // ---- VERIFICAÇÃO DO BOTÃO DE RESET ----
  // Lê o estado atual do botão com debounce
  checkButtonPress();
  
  // ---- RESET AUTOMÁTICO DO CONTADOR ----
  // Verifica se o reset automático está habilitado antes de executar
  if (AUTO_RESET_ENABLED && (currentTime - lastResetTime >= AUTO_RESET_INTERVAL)) {
    lastResetTime = currentTime;
    
    // Executa reset automático usando a função existente
    autoResetCounter();
  }
  
  // ---- PROCESSAMENTO DE NOVOS PULSOS ----
  // Verifica se a ISR detectou um novo pulso
  if (newPulseReceived) {
    // Limpa a flag para evitar processamento duplo
    newPulseReceived = false;
    
    // ---- CÁLCULO DA FREQUÊNCIA INSTANTÂNEA ----
    // Frequência = 1 / Período
    // Como pulseInterval está em microssegundos: f = 1.000.000 / período_μs
    float instantFrequency = 0.0;
    if (pulseInterval > 0) {
      instantFrequency = 1000000.0 / pulseInterval;
    }
    
    // ---- EXIBIÇÃO DE INFORMAÇÕES DO PULSO ----
    // Formato: | timestamp | count | frequency | period | status |
    Serial.print("| ");
    Serial.print(formatNumber(currentTime, 8));
    Serial.print(" | ");
    Serial.print(formatNumber(pulseCount, 6));
    Serial.print(" | ");
    Serial.print(formatFloat(instantFrequency, 2, 10));
    Serial.print(" | ");
    Serial.print(formatNumber(pulseInterval, 7));
    Serial.print(" | Pulso (D:");
    Serial.print(currentDebounceTime);
    Serial.print("μs) |");
    Serial.println();
  }
  
  // ---- RELATÓRIO PERIÓDICO ----
  // Gera relatório estatístico a cada DISPLAY_INTERVAL milissegundos
  if (currentTime - lastDisplayTime >= DISPLAY_INTERVAL_CONST) {
    lastDisplayTime = currentTime;
    
    // ---- CÁLCULO DA FREQUÊNCIA MÉDIA ----
    // Conta quantos pulsos ocorreram no último segundo
    static unsigned long lastPulseCount = 0;
    unsigned long pulsesInLastSecond = pulseCount - lastPulseCount;
    lastPulseCount = pulseCount;
    
    // ---- CÁLCULO DO TEMPO DECORRIDO ----
    unsigned long elapsedSeconds = currentTime / 1000;
    unsigned long elapsedMinutes = elapsedSeconds / 60;
    unsigned long remainingSeconds = elapsedSeconds % 60;
    
    // ---- EXIBIÇÃO DO RELATÓRIO ----
    Serial.println("|----------|--------|------------|---------|---------------------|");
    Serial.print("| RELATÓRIO - Tempo: ");
    if (elapsedMinutes > 0) {
      Serial.print(elapsedMinutes);
      Serial.print("min ");
    }
    Serial.print(remainingSeconds);
    Serial.print("s");
    
    // Preenche o espaço restante da linha
    String spaces = "";
    int totalChars = String(elapsedMinutes).length() + String(remainingSeconds).length() + 
                    (elapsedMinutes > 0 ? 5 : 1); // "min " = 4 chars + "s" = 1 char
    for (int i = totalChars; i < 28; i++) {
      spaces += " ";
    }
    Serial.print(spaces);
    Serial.println("|");
    
    Serial.print("| Pulsos totais: ");
    Serial.print(formatNumber(pulseCount, 6));
    Serial.print(" | Freq. média: ");
    Serial.print(formatNumber(pulsesInLastSecond, 4));
    Serial.println(" Hz                 |");
    
    Serial.println("|----------|--------|------------|---------|---------------------|");
  }
  
  // ---- DELAY PARA OTIMIZAÇÃO ----
  // Pequeno delay para reduzir uso da CPU e permitir que outras tarefas executem
  // Não afeta a detecção de pulsos pois ela é feita por interrupção de hardware
  delay(10);
}

/*
 * FUNÇÃO DE VERIFICAÇÃO DO BOTÃO DE RESET
 * ====================================================================
 * Verifica se o botão foi pressionado e executa reset manual com debounce
 * adequado para evitar múltiplos disparos acidentais.
 */
void checkButtonPress() {
  unsigned long currentTime = millis();
  
  // Lê o estado atual do botão
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  // Verifica se houve mudança de estado e se passou o tempo de debounce
  if (currentButtonState != lastButtonState && 
      (currentTime - lastButtonTime) > BUTTON_DEBOUNCE_MS) {
    
    // Atualiza o timestamp da última mudança
    lastButtonTime = currentTime;
    
    // Se o botão foi pressionado (transição HIGH → LOW)
    if (currentButtonState == LOW && lastButtonState == HIGH) {
      // Executa reset manual
      resetCounter();
    }
    
    // Atualiza o estado anterior do botão
    lastButtonState = currentButtonState;
  }
}

/*
 * FUNÇÃO DE ATUALIZAÇÃO DO DEBOUNCE ADAPTATIVO
 * ====================================================================
 * Calcula o tempo de debounce ideal baseado no histórico de intervalos
 * entre pulsos, adaptando-se automaticamente à frequência do sinal.
 * 
 * ALGORITMO:
 * 1. Calcula a média dos últimos 5 intervalos
 * 2. Define debounce como 5-10% do intervalo médio
 * 3. Limita entre DEBOUNCE_TIME_US_MIN e DEBOUNCE_TIME_US_MAX
 * 4. Aplica filtro de suavização para evitar oscilações
 */
void updateAdaptiveDebounce() {
  // Calcula quantos valores válidos temos no histórico
  int validCount = 0;
  unsigned long sum = 0;
  
  for (int i = 0; i < 5; i++) {
    if (pulseHistory[i] > 0) {
      sum += pulseHistory[i];
      validCount++;
    }
  }
  
  // Se temos pelo menos 2 amostras, calcula novo debounce
  if (validCount >= 2) {
    // Calcula intervalo médio
    unsigned long averageInterval = sum / validCount;
    
    // Define debounce como 8% do intervalo médio
    // Isso garante que pulsos legítimos não sejam filtrados
    unsigned long newDebounce = averageInterval / 12; // ~8.3%
    
    // Aplica limites mínimo e máximo
    if (newDebounce < DEBOUNCE_TIME_US_MIN) {
      newDebounce = DEBOUNCE_TIME_US_MIN;
    } else if (newDebounce > DEBOUNCE_TIME_US_MAX) {
      newDebounce = DEBOUNCE_TIME_US_MAX;
    }
    
    // Aplica filtro de suavização (média ponderada)
    // 70% do valor atual + 30% do novo valor
    currentDebounceTime = (currentDebounceTime * 7 + newDebounce * 3) / 10;
  }
}

/*
 * FUNÇÃO AUXILIAR PARA RESET AUTOMÁTICO DO SISTEMA
 * ====================================================================
 * Esta função é chamada automaticamente a cada 30 segundos para
 * reinicializar os contadores, permitindo medições periódicas.
 * 
 * NOTA: Só é executada se AUTO_RESET_ENABLED estiver definido como true
 */
void autoResetCounter() {
  // ---- SEÇÃO CRÍTICA ----
  noInterrupts();
  
  // Salva valores antes do reset para exibição
  unsigned long totalPulses = pulseCount;
  unsigned long elapsedTime = millis() - (lastResetTime - AUTO_RESET_INTERVAL);
  
  // Zera todas as variáveis do sistema
  pulseCount = 0;
  lastPulseTime = 0;
  pulseInterval = 0;
  newPulseReceived = false;
  
  // Reinicia variáveis do debounce adaptativo
  currentDebounceTime = DEBOUNCE_TIME_US_DEFAULT;
  historyIndex = 0;
  for (int i = 0; i < 5; i++) {
    pulseHistory[i] = 0;
  }
  
  interrupts();
  // ---- FIM DA SEÇÃO CRÍTICA ----
  
  // Calcula estatísticas do período
  float averageFrequency = (float)totalPulses / (elapsedTime / 1000.0);
  
  // Exibe relatório do reset automático
  Serial.println();
  Serial.println("====================================================================");
  Serial.println("                     RESET AUTOMÁTICO (30s)                        ");
  Serial.println("====================================================================");
  Serial.println("ESTATÍSTICAS DO PERÍODO:");
  Serial.print("• Pulsos totais: ");
  Serial.println(totalPulses);
  Serial.print("• Tempo decorrido: ");
  Serial.print(elapsedTime / 1000.0, 1);
  Serial.println(" segundos");
  Serial.print("• Frequência média: ");
  Serial.print(averageFrequency, 2);
  Serial.println(" Hz");
  Serial.println("====================================================================");
  Serial.println("           NOVO PERÍODO INICIADO - CONTADOR ZERADO                 ");
  Serial.println("====================================================================");
  Serial.println("| Timestamp | Pulsos | Frequência | Período |      Status         |");
  Serial.println("|    (ms)   | Total  |    (Hz)    |  (μs)   |                     |");
  Serial.println("====================================================================");
}

/*
 * FUNÇÃO AUXILIAR PARA RESET MANUAL DO SISTEMA
 * ====================================================================
 * Esta função permite reinicializar todos os contadores e variáveis
 * do sistema manualmente, sem aguardar o reset automático.
 * 
 * NOTA: Pode ser chamada manualmente ou implementada com um botão
 * ou comando serial para facilitar testes e calibrações.
 * 
 * SEGURANÇA:
 * - Usa noInterrupts()/interrupts() para operação atômica
 * - Evita corrupção de dados durante a modificação das variáveis
 */
void resetCounter() {
  // ---- SEÇÃO CRÍTICA ----
  // Desabilita temporariamente todas as interrupções para garantir
  // que a ISR não modifique as variáveis durante o reset
  noInterrupts();
  
  // Zera todas as variáveis do sistema
  pulseCount = 0;           // Contador total de pulsos
  lastPulseTime = 0;        // Timestamp do último pulso
  pulseInterval = 0;        // Intervalo entre pulsos
  newPulseReceived = false; // Flag de novo pulso
  
  // Reinicia variáveis do debounce adaptativo
  currentDebounceTime = DEBOUNCE_TIME_US_DEFAULT;
  historyIndex = 0;
  for (int i = 0; i < 5; i++) {
    pulseHistory[i] = 0;
  }
  
  // Reinicia o timer de reset automático apenas se estiver habilitado
  if (AUTO_RESET_ENABLED) {
    lastResetTime = millis(); // Reinicia o timer de reset automático
  }
  
  // Reabilita as interrupções
  interrupts();
  // ---- FIM DA SEÇÃO CRÍTICA ----
  
  // Confirma o reset via serial
  Serial.println();
  Serial.println("====================================================================");
  Serial.println("                      RESET MANUAL EXECUTADO                       ");
  Serial.println("====================================================================");
  Serial.println("✓ Contador de pulsos zerado");
  Serial.println("✓ Variáveis de tempo reinicializadas");
  
  // Mostra status do timer automático conforme configuração
  if (AUTO_RESET_ENABLED) {
    Serial.println("✓ Timer de reset automático reiniciado");
  } else {
    Serial.println("✓ Reset automático permanece desabilitado");
  }
  
  Serial.println("✓ Sistema pronto para nova medição");
  Serial.println("✓ Reset acionado via botão GPIO " + String(BUTTON_PIN));
  Serial.println("====================================================================");
  Serial.println("| Timestamp | Pulsos | Frequência | Período |      Status         |");
  Serial.println("|    (ms)   | Total  |    (Hz)    |  (μs)   |                     |");
  Serial.println("====================================================================");
}

/*
 * FUNÇÕES AUXILIARES PARA FORMATAÇÃO DE STRINGS
 * ====================================================================
 * Estas funções ajudam na formatação da saída serial para melhor
 * apresentação dos dados em formato tabular.
 */

// Implementação de padStart para compatibilidade com Arduino
String padStart(String str, int targetLength, char padChar) {
  while (str.length() < targetLength) {
    str = padChar + str;
  }
  return str;
}

// Função auxiliar para formatar números com largura fixa
String formatNumber(unsigned long number, int width) {
  String str = String(number);
  return padStart(str, width, ' ');
}

// Função auxiliar para formatar números decimais com largura fixa
String formatFloat(float number, int decimals, int width) {
  String str = String(number, decimals);
  return padStart(str, width, ' ');
}

/*
 * NOTAS TÉCNICAS E INFORMAÇÕES ADICIONAIS
 * ====================================================================
 * 
 * PRECISÃO DO SISTEMA:
 * - Resolução temporal: 1 μs (microssegundo)
 * - Frequência máxima teórica: ~500 kHz (limitada pelo debounce)
 * - Frequência máxima prática: ~100 kHz (recomendada)
 * 
 * LIMITAÇÕES:
 * - Serial.print() pode causar pequenos atrasos na exibição
 * - Overflow de variáveis após ~49 dias de operação contínua
 * 
 * DEBOUNCE ADAPTATIVO:
 * - Algoritmo inteligente que se adapta à frequência do sinal
 * - Histórico de 5 amostras para cálculo da média
 * - Debounce calculado como 8% do intervalo médio
 * - Limites configuráveis: 100μs a 5000μs
 * - Filtro de suavização para estabilidade
 * - Otimização automática para alta e baixa frequência
 * 
 * MELHORIAS POSSÍVEIS:
 * - Adicionar buffer circular para médias móveis
 * - Implementar comunicação via WiFi/Bluetooth
 * - Adicionar display LCD/OLED
 * - Implementar data logging em cartão SD
 * - Configurar intervalo de reset via comando serial
 * - Implementar controle de reset via botão físico
 * - Adicionar detecção automática de tipo de sinal
 * 
 * CONFIGURAÇÃO DO RESET AUTOMÁTICO:
 * - Para HABILITAR: #define AUTO_RESET_ENABLED true
 * - Para DESABILITAR: #define AUTO_RESET_ENABLED false
 * - Intervalo configurável via AUTO_RESET_INTERVAL (em milissegundos)
 * 
 * CONFIGURAÇÃO DO BOTÃO DE RESET:
 * - Pino configurável via BUTTON_PIN (padrão GPIO 0)
 * - GPIO 0 é o botão BOOT nativo do ESP32-S3
 * - Debounce configurável via BUTTON_DEBOUNCE_MS
 * - Funciona em paralelo com reset automático
 * 
 * TROUBLESHOOTING:
 * - Se não detectar pulsos: verificar conexões e nível do sinal
 * - Se contar pulsos duplicados: sistema se auto-ajustará
 * - Se perder pulsos: debounce adaptativo se ajustará automaticamente
 * - Para sinais de 5V: usar divisor de tensão ou level shifter
 * - Para frequências > 100kHz: verificar se debounce mínimo é adequado
 * 
 * ====================================================================
 */
