#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <LiquidCrystal.h> 

// ==========================================
// CONFIGURACIÓN DE HARDWARE Y CONSTANTES
// ==========================================
const int PIN_VENTILADORES = 9; 
const int PIN_LM35 = A0;      

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Constantes térmicas (Umbrales)
const float TEMP_MIN = 18.0;
const float TEMP_MAX = 20.0;

// Constantes para el cálculo de CPU
// (Aproximación de ciclos de Idle por segundo en un ATmega328P libre)
const unsigned long MAX_IDLE_TICKS_PER_SEC = 50000; 

// ==========================================
// RECURSOS DEL SISTEMA (FREERTOS)
// ==========================================
QueueHandle_t xQueueTemp;     
SemaphoreHandle_t xMutexCPU;  

// ==========================================
// VARIABLES GLOBALES (Protegidas o de solo lectura)
// ==========================================
volatile unsigned long ulIdleTicks = 0; // Se incrementa en el Idle Hook
float g_cpuUsage = 0.0;       
int g_pwmDutyPorcentaje = 0;  
float g_currentTemp = 0.0;    

// ==========================================
// PROTOTIPOS DE TAREAS
// ==========================================
void vTaskSensor(void *pvParameters);
void vTaskControl(void *pvParameters);
void vTaskTelemetry(void *pvParameters);
void vTaskCPU_Monitor(void *pvParameters);

// ==========================================
// FUNCIÓN SETUP (Inicialización)
// ==========================================
void setup() {
    // RNF-03: Inicialización segura para evitar picos de corriente
    pinMode(PIN_VENTILADORES, OUTPUT);
    digitalWrite(PIN_VENTILADORES, LOW); 

    Serial.begin(9600);
    while (!Serial) {;} 
    Serial.println(F("Iniciando SO de Centro de Datos Verde..."));

    // Inicializar la pantalla LCD (16 columnas, 2 filas)
    lcd.begin(16, 2);
    lcd.print("Iniciando SO...");

    // Inicialización de colas y mutex
    xQueueTemp = xQueueCreate(3, sizeof(float));
    xMutexCPU = xSemaphoreCreateMutex();

    if (xQueueTemp != NULL && xMutexCPU != NULL) {
        // Tarea del Sensor: Prioridad 2
        xTaskCreate(vTaskSensor, "Sensor", 90, NULL, 2, NULL);
        
        // Tarea de Control: Prioridad 3 (Máxima para reacción rápida RNF-01)
        xTaskCreate(vTaskControl, "Control", 90, NULL, 3, NULL);
        
        // Tarea de Telemetría: Prioridad 1 (Baja)
        xTaskCreate(vTaskTelemetry, "Telemetry", 110, NULL, 1, NULL);
        
        // Tarea Monitora de CPU: Prioridad 4 (Se ejecuta exactamente cada 1s)
        xTaskCreate(vTaskCPU_Monitor, "CPUMon", 85, NULL, 4, NULL);
        
        Serial.println(F("Planificador listo."));
        lcd.clear(); // Limpiar el LCD antes de empezar a mostrar telemetría
    } else {
        Serial.println(F("Error de memoria RAM."));
        lcd.clear();
        lcd.print("Error de RAM");
        while (1); 
    }
    // El planificador arranca automáticamente tras salir del setup
}

// ==========================================
// BUCLE PRINCIPAL
// ==========================================
void loop() {
    // Vacío. FreeRTOS controla la ejecución.
}

// ==========================================
// IMPLEMENTACIÓN DE TAREAS
// ==========================================

// RF-01: Tarea del Sensor (Lectura y Filtro cada 500ms)
void vTaskSensor(void *pvParameters) {
    (void) pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrecuencia = 500 / portTICK_PERIOD_MS; // 500 ms

    // Variables para el filtro de promedio móvil
    const int NUM_SAMPLES = 5;
    float samples[NUM_SAMPLES] = {0};
    int sampleIdx = 0;
    bool bufferLleno = false;

    for (;;) {
        // Leer ADC y convertir a voltaje (5V referenciales / 1024 pasos)
        int rawADC = analogRead(PIN_LM35);
        float voltaje = (rawADC * 5.0) / 1024.0;
        
        // Convertir a grados centígrados (LM35 = 10mV/°C)
        float tempActual = voltaje * 100.0;

        // Filtro de Promedio Móvil Simple
        samples[sampleIdx] = tempActual;
        sampleIdx++;
        if (sampleIdx >= NUM_SAMPLES) {
            sampleIdx = 0;
            bufferLleno = true;
        }

        float sum = 0;
        int count = bufferLleno ? NUM_SAMPLES : sampleIdx;
        for (int i = 0; i < count; i++) {
            sum += samples[i];
        }
        float tempFiltrada = sum / count;

        // Enviar a la cola de FreeRTOS
        xQueueSend(xQueueTemp, &tempFiltrada, 0);

        // Actualizar variable global para telemetría
        g_currentTemp = tempFiltrada;

        // Bloquear tarea exactamente 500ms
        vTaskDelayUntil(&xLastWakeTime, xFrecuencia);
    }
}

// RF-02 / RNF-01: Tarea de Control PWM (Activada por Evento)
void vTaskControl(void *pvParameters) {
    (void) pvParameters;
    float tempRecibida;

    for (;;) {
        // Bloquear indefinidamente hasta que llegue un dato a la cola (reacción inmediata)
        if (xQueueReceive(xQueueTemp, &tempRecibida, portMAX_DELAY) == pdPASS) {
            
            int pwmValue = 0;

            // Evaluación de Umbrales
            if (tempRecibida < TEMP_MIN) {
                pwmValue = 0; // 0%
            } else if (tempRecibida > TEMP_MAX) {
                pwmValue = 255; // 100%
            } else {
                // Escalado lineal manual (Regla de 3 para floats)
                float rangoTemp = TEMP_MAX - TEMP_MIN;
                float rangoPWM = 255.0;
                pwmValue = (int)(((tempRecibida - TEMP_MIN) / rangoTemp) * rangoPWM);
            }

            // Aplicar ciclo de trabajo físico (< 100ms gracias a la prioridad alta)
            analogWrite(PIN_VENTILADORES, pwmValue);

            // Calcular y guardar el porcentaje (0-100%) para telemetría
            g_pwmDutyPorcentaje = map(pwmValue, 0, 255, 0, 100);
        }
    }
}

// RF-03 / RNF-02: Monitor de CPU (Ventana de 1 segundo)
void vTaskCPU_Monitor(void *pvParameters) {
    (void) pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrecuencia = 1000 / portTICK_PERIOD_MS; // 1 segundo exacto

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrecuencia);

        // Proteger lectura y reinicio con Mutex
        if (xSemaphoreTake(xMutexCPU, portMAX_DELAY) == pdTRUE) {
            unsigned long ticksInactivos = ulIdleTicks;
            ulIdleTicks = 0; // Reiniciar contador para el próximo segundo
            xSemaphoreGive(xMutexCPU);

            // Calcular porcentaje (protección contra división por cero o desbordamiento)
            if (ticksInactivos >= MAX_IDLE_TICKS_PER_SEC) {
                g_cpuUsage = 0.0;
            } else {
                g_cpuUsage = 100.0 - ((float)ticksInactivos / MAX_IDLE_TICKS_PER_SEC * 100.0);
            }
        }
    }
}

// RF-04: Tarea de Telemetría (Cada 2 segundos)
void vTaskTelemetry(void *pvParameters) {
    (void) pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrecuencia = 2000 / portTICK_PERIOD_MS; // 2000 ms

    for (;;) {
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

        // Ceder el control y esperar hasta el siguiente ciclo (sin usar delay)
        vTaskDelayUntil(&xLastWakeTime, xFrecuencia);
    }
}

// ==========================================
// FUNCIÓN HOOK (Gancho del sistema inactivo)
// ==========================================
// Esta función nativa se ejecuta CADA VEZ que el microcontrolador
// no tiene tareas activas que procesar.
extern "C" void vApplicationIdleHook(void) {
    // Modificar variable compartida bajo protección del Mutex (o sin bloquear si es lectura rápida)
    // Para no ralentizar el Hook, solo incrementamos. El Mutex se usa al leer/reiniciar.
    ulIdleTicks++;
}
