// Broche du capteur infrarouge
const int irSensorPin = 2; // Remplacez par la broche que vous avez utilisée

void setup() {
  Serial.begin(9600);
  pinMode(irSensorPin, INPUT);
}

void loop() {
  // Lit la valeur du capteur infrarouge
  int sensorValue = digitalRead(irSensorPin);

  if (sensorValue == HIGH) {
    Serial.println("Obstacle détecté !");
  } else {
    Serial.println("Aucun obstacle.");
  }

  delay(500);
}







