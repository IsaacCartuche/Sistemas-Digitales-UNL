#include <LiquidCrystal.h>

// ==========================================
// CONFIGURACIÓN DE PINES Y LCD
// ==========================================
const int PIN_VENTILADORES = 9; 
const int PIN_LM35 = A0;        

// Inicialización de la pantalla LCD sin módulo I2C (RS, EN, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Umbrales térmicos
const float TEMP_MIN = 27.0;
const float TEMP_MAX = 40.0;

// Base para calcular el 100% de CPU inactiva (Requiere calibración en Tinkercad)
const unsigned long MAX_IDLE_LOOPS_PER_SEC = 120000; 

// ==========================================
// VARIABLES DE SIMULACIÓN RTOS
// ==========================================
volatile unsigned long ulIdleLoops = 0; 
float g_cpuUsage = 0.0;        
int g_pwmDutyPorcentaje = 0;  
float g_currentTemp = 0.0;    

// Simulación de la Cola de Mensajes
float g_tempQueueValue = 0.0;
bool g_tempDataAvailable = false;

// Temporizadores (Tick Rate)
unsigned long lastSensorTime = 0;
unsigned long lastTelemetryTime = 0;
unsigned long lastCPUTime = 0;

// Variables para eliminar el delay del Splash Screen
unsigned long splashStartTime = 0;
bool splashActive = true;

void setup() {
    // Inicialización segura
    pinMode(PIN_VENTILADORES, OUTPUT);
    digitalWrite(PIN_VENTILADORES, LOW); 

    Serial.begin(9600);
    
    // Inicialización de Interfaz LCD
    lcd.begin(16, 2);
    lcd.print("Centro Datos");
    lcd.setCursor(0, 1);
    lcd.print("Verde Iniciando");
    
    // Registramos el tiempo en lugar de usar delay(1500)
    splashStartTime = millis(); 
}

void loop() {
    unsigned long currentMillis = millis();
    bool taskExecuted = false; // Bandera para simular el Idle Hook

    // ==========================================================
    // TAREA 0: SPLASH SCREEN (Se ejecuta una sola vez a los 1500 ms)
    // Reemplazo no bloqueante del delay()
    // ==========================================================
    if (splashActive && (currentMillis - splashStartTime >= 1500)) {
        lcd.clear();
        Serial.println("Planificador Cooperativo listo.");
        splashActive = false;
        taskExecuted = true;
    }

    // ==========================================================
    // TAREA 4: MONITOR DE CPU (Prioridad máxima de cálculo)
    // Se ejecuta cada 1000 ms
    // ==========================================================
    if (currentMillis - lastCPUTime >= 1000) {
        lastCPUTime = currentMillis;
        taskExecuted = true;
        
        unsigned long loops = ulIdleLoops;
        ulIdleLoops = 0; 

        if (loops >= MAX_IDLE_LOOPS_PER_SEC) {
            g_cpuUsage = 0.0;
        } else {
            g_cpuUsage = 100.0 - ((float)loops / MAX_IDLE_LOOPS_PER_SEC * 100.0);
        }
    }

    // ==========================================================
    // TAREA 3: CONTROL PWM (Activada por Evento de la "Cola")
    // ==========================================================
    if (g_tempDataAvailable) {
        taskExecuted = true;
        g_tempDataAvailable = false; // Consumir el evento de la cola
        
        int pwmValue = 0;
        if (g_tempQueueValue < TEMP_MIN) {
            pwmValue = 0;
        } else if (g_tempQueueValue > TEMP_MAX) {
            pwmValue = 255;
        } else {
            float rangoTemp = TEMP_MAX - TEMP_MIN;
            pwmValue = (int)(((g_tempQueueValue - TEMP_MIN) / rangoTemp) * 255.0);
        }

        analogWrite(PIN_VENTILADORES, pwmValue);
        g_pwmDutyPorcentaje = map(pwmValue, 0, 255, 0, 100);
    }

    // ==========================================================
    // TAREA 2: SENSOR (Cada 500 ms)
    // ==========================================================
    if (currentMillis - lastSensorTime >= 500) {
        lastSensorTime = currentMillis;
        taskExecuted = true;

        int rawADC = analogRead(PIN_LM35);
        float voltaje = (rawADC * 5.0) / 1024.0;
        
        // NOTA TINKERCAD: El componente es un TMP36, requiere restar 0.5V.
        // Si usas el LM35 físico, elimina el "- 0.5".
        float tempActual = (voltaje) * 100.0; 
        
        // Filtro de promedio móvil
        static float samples[5] = {0};
        static int sampleIdx = 0;
        static bool bufferLleno = false;

        samples[sampleIdx] = tempActual;
        sampleIdx++;
        if (sampleIdx >= 5) {
            sampleIdx = 0;
            bufferLleno = true;
        }

        float sum = 0;
        int count = bufferLleno ? 5 : sampleIdx;
        for (int i = 0; i < count; i++) {
            sum += samples[i];
        }
        g_currentTemp = sum / count;

        // Enviar dato a la "Cola de Mensajes"
        g_tempQueueValue = g_currentTemp;
        g_tempDataAvailable = true;
    }

    // ==========================================================
    // TAREA 1: TELEMETRÍA (Cada 2000 ms)
    // ==========================================================
    if (currentMillis - lastTelemetryTime >= 2000) {
        lastTelemetryTime = currentMillis;
        taskExecuted = true;

        // Imprimir en Monitor Serie
        Serial.print("Temp: ");
        Serial.print(g_currentTemp, 1);
        Serial.print(" C | PWM: ");
        Serial.print(g_pwmDutyPorcentaje);
        Serial.print(" % | CPU: ");
        Serial.print(g_cpuUsage, 1);
        Serial.println(" %");

        // Imprimir en Pantalla LCD
        lcd.setCursor(0, 0);
        lcd.print("T:"); lcd.print(g_currentTemp, 1); lcd.print("C ");
        lcd.print("P:"); lcd.print(g_pwmDutyPorcentaje); lcd.print("%  ");
        
        lcd.setCursor(0, 1);
        lcd.print("CPU: "); lcd.print(g_cpuUsage, 1); lcd.print("%    ");
    }

    // ==========================================================
    // IDLE HOOK SIMULADO
    // ==========================================================
    // Si ninguna tarea se ejecutó en esta vuelta del loop, el procesador está libre.
    if (!taskExecuted) {
        ulIdleLoops++;
    }
}