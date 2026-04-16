byte varByte = 0b11001100;     
int varInt = 255;              
long varLong = 100000;         
float varFloat = 3.14;         
bool varBool = true;           

byte estadoLed = 0;
int contador = 0;
const int LED_PIN = 13;
void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  
  Serial.println("=== DEMOSTRACION DE TIPOS DE DATOS Y OPERACIONES ===");
  Serial.println("\n1. Tipo BYTE (Bitwise):");
  Serial.print("   varByte original: "); Serial.println(varByte, BIN);
  Serial.print("   varByte AND 0b10101010: "); Serial.println(varByte & 0b10101010, BIN);
  Serial.print("   NOT varByte (~): "); Serial.println((byte)~varByte, BIN);
  Serial.println("\n2. Tipo INT (Bitwise):");
  Serial.print("   varInt original: "); Serial.println(varInt);
  Serial.print("   varInt SHIFT LEFT (<< 2): "); Serial.println(varInt << 2);
  Serial.print("   varInt XOR 15: "); Serial.println(varInt ^ 15);
  Serial.println("\n3. Tipo LONG (Bitwise):");
  Serial.print("   varLong original: "); Serial.println(varLong);
  Serial.print("   varLong OR 500: "); Serial.println(varLong | 500);
  Serial.println("\n4. Tipo FLOAT (Aritmetica):");
  Serial.print("   varFloat original: "); Serial.println(varFloat);
  Serial.print("   varFloat * 2.5: "); Serial.println(varFloat * 2.5);
  Serial.println("\n5. Tipo BOOL (Logica):");
  Serial.print("   varBool original (1=true): "); Serial.println(varBool);
  Serial.print("   varBool AND false (&&): "); Serial.println(varBool && false);
  Serial.println("\n====================================================");
  Serial.println("Iniciando bucle principal del LED (Bitwise en accion)...");
  Serial.println("====================================================");
  estadoLed = estadoLed | (1 << 0);
}
void loop() {
  estadoLed = estadoLed ^ 0b00000001;
  
  if ((estadoLed & 1) == 1) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  
  delay(500);
  
  contador = (contador + 1) % 8;
  
  Serial.print("Desplazamiento LED (1 << ");
  Serial.print(contador);
  Serial.print(") en binario: ");
  Serial.println(1 << contador, BIN);
  
  delay(500);
}