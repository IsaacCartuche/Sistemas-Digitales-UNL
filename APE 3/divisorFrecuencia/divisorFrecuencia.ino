const int pin555 = 2;
const int pinFF = 3; 

bool estadoAnterior1 = LOW;
unsigned long tiempoAnterior1 = 0;
unsigned long tiempoCambio1 = 0;
unsigned long tHigh1 = 0;
unsigned long tLow1 = 0;
float periodo1 = 0;
float frecuencia1 = 0;
float duty1 = 0;

bool estadoAnterior2 = LOW;
unsigned long tiempoAnterior2 = 0;
unsigned long tiempoCambio2 = 0;
unsigned long tHigh2 = 0;
unsigned long tLow2 = 0;
float periodo2 = 0;
float frecuencia2 = 0;
float duty2 = 0;

unsigned long lastPrintTime = 0;
const int printInterval = 100; 
const int plotInterval = 10;
unsigned long lastPlotTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(pin555, INPUT);
  pinMode(pinFF, INPUT);
}

void loop() {
  unsigned long tiempoActual = millis();

  procesarCanal(pin555, tiempoActual, estadoAnterior1, tiempoAnterior1, tiempoCambio1, tHigh1, tLow1, periodo1, frecuencia1, duty1);
  procesarCanal(pinFF, tiempoActual, estadoAnterior2, tiempoAnterior2, tiempoCambio2, tHigh2, tLow2, periodo2, frecuencia2, duty2);

  imprimirResultados(tiempoActual);
}

void procesarCanal(int pin, unsigned long tiempoActual, bool &estadoAnt, unsigned long &tiempoAnt, unsigned long &tiempoCamb, unsigned long &tH, unsigned long &tL, float &per, float &frec, float &dc) {
  
  bool estadoActual = digitalRead(pin);
  bool huboCambio = false;
  bool esAscendente = false;

  algoritmo1_detectarFlanco(estadoActual, estadoAnt, huboCambio, esAscendente);
  
  if (huboCambio) {
    algoritmo2_medirPeriodo(esAscendente, tiempoActual, tiempoAnt, per);
    algoritmo3_calcularFrecuencia(per, frec);
    algoritmo4_medirDuty(estadoActual, tiempoActual, tiempoCamb, tH, tL, dc);
  }
  
  estadoAnt = estadoActual;
}

void algoritmo1_detectarFlanco(bool estadoActual, bool estadoAnterior, bool &huboCambio, bool &esAscendente) {
  if (estadoActual != estadoAnterior) {
    huboCambio = true;
    if (estadoActual == HIGH && estadoAnterior == LOW) {
      esAscendente = true; 
    }
  }
}

void algoritmo2_medirPeriodo(bool esAscendente, unsigned long tiempoActual, unsigned long &tiempoAnterior, float &periodo) {
  if (esAscendente) { 
    periodo = tiempoActual - tiempoAnterior; 
    tiempoAnterior = tiempoActual; 
  }
}

void algoritmo3_calcularFrecuencia(float periodo, float &frecuencia) {
  if (periodo > 0) {
    frecuencia = 1000.0 / periodo;
  }
}

void algoritmo4_medirDuty(bool estadoActual, unsigned long tiempoActual, unsigned long &tiempoCambio, unsigned long &tHigh, unsigned long &tLow, float &duty) {
  if (estadoActual == HIGH) { 
    tLow = tiempoActual - tiempoCambio; 
  } else { 
    tHigh = tiempoActual - tiempoCambio;
  }
  
  tiempoCambio = tiempoActual; 

  if ((tHigh + tLow) > 0) {
    duty = (float)tHigh / (tHigh + tLow) * 100.0; 
  }
}

void imprimirResultados(unsigned long tiempoActual) {
  
  /*
  if (tiempoActual - lastPrintTime >= printInterval) {
    // Mostrar resultados en Serial Monitor / Plotter [cite: 80, 81]
    Serial.print("Freq_555:"); Serial.print(frecuencia1);
    Serial.print(", Duty_555:"); Serial.print(duty1);
    Serial.print(", Freq_FF:"); Serial.print(frecuencia2);
    Serial.print(", Duty_FF:"); Serial.println(duty2);
    lastPrintTime = tiempoActual;
  }
  */
  if (tiempoActual - lastPlotTime >= plotInterval) {
    int estado555 = digitalRead(pin555);
    int estadoFF = digitalRead(pinFF);
    Serial.print(estado555);
    Serial.print(","); 
    Serial.println(estadoFF + 2); 
    lastPlotTime = tiempoActual;
  }
}