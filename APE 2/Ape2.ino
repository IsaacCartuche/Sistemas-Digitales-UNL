
const int LED_PINS[] = {2, 3, 4, 5, 6, 7};
const int BOTON_PIN = 8;
int patronActual = 1;

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 6; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }
  
  pinMode(BOTON_PIN, INPUT);

}

void loop() {
  leerBoton();
  switch(patronActual) {
    case 0:
      patronSecuencia();
      break;
    case 1:
      patronPersecucion();
      break;
    case 2:
      patronParpadeo();
      break;
    case 3:
      patronAleatorio();
      break;
    case 4:
      patronOnda();
      break;
  }
}

void leerBoton(){
  Serial.print("pa: ");
  Serial.print(patronActual);
  static int ultimoEstadoBoton = 0; 
  int estadoBoton = digitalRead(BOTON_PIN);

  if (estadoBoton != ultimoEstadoBoton) {
    if (estadoBoton == LOW) { 
      Serial.print("r:");
      Serial.print(random(5));
      patronActual = random(5); 
    }
    delay(50);
  }
  ultimoEstadoBoton = estadoBoton;
}

void patronSecuencia() {
  int i = 0; 
  while (i < 6) {
    digitalWrite(LED_PINS[i], HIGH);
    delay(100);
    i++;
  }
  i = 0; 
  while (i < 6) {
    digitalWrite(LED_PINS[i], LOW);
    delay(100);
    i++;
  }
}

void patronPersecucion() {
  for (int i = 0; i < 6; i++) {
    apagarTodos();
    digitalWrite(LED_PINS[i], HIGH);
    delay(80);
  }
  for (int i = 6 - 2; i > 0; i--) {
    apagarTodos();
    digitalWrite(LED_PINS[i], HIGH);
    delay(80);
  }
}

void patronParpadeo() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PINS[i], HIGH);
  }
  delay(300);
  // Todos apagados
  apagarTodos();
  delay(300);
}

void patronAleatorio() {
  int ledAlAzar = random(6);
  digitalWrite(LED_PINS[ledAlAzar], HIGH);
  delay(100);
  digitalWrite(LED_PINS[ledAlAzar], LOW);
  delay(50);
}

void patronOnda() {
  for (int i = 0; i < 6 + 2; i++) {
    apagarTodos();
    if (i < 6) digitalWrite(LED_PINS[i], HIGH);
    if (i - 1 >= 0 && i - 1 < 6) digitalWrite(LED_PINS[i - 1], HIGH);
    if (i - 2 >= 0 && i - 2 < 6) digitalWrite(LED_PINS[i - 2], HIGH);
    delay(120);
  }
}

void apagarTodos() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}
