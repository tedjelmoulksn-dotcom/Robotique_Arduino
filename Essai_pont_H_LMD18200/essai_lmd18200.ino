// Définitions des broches
int motorDirInPin = 9;  // DIRIN sur le module (pour direction et activation)
int motorPWMPin = 10;   // PWM pour le contrôle de la vitesse

void setup() {
  // Définir les broches en sortie
  pinMode(motorDirInPin, OUTPUT);
  pinMode(motorPWMPin, OUTPUT);
}

void loop() {
  // Activer le moteur et tourner dans une direction (par exemple, sens horaire)
  digitalWrite(motorDirInPin, HIGH);  // Moteur activé, DIR en HIGH
  
  // Contrôler la vitesse du moteur (valeur PWM entre 0 et 255)
  analogWrite(motorPWMPin, 200); // Vitesse à 80%

  delay(5000); // Attendre 5 secondes

  // Changer de direction (sens antihoraire)
  digitalWrite(motorDirInPin, LOW);  // Moteur activé, DIR en LOW

  delay(5000); // Attendre 5 secondes

  // Arrêter le moteur en mettant DIRIN à LOW
  analogWrite(motorPWMPin, 0);    // Vitesse à 0 (moteur éteint)

  delay(2000); // Attendre 2 secondes

  // Réactiver le moteur après 2 secondes, dans l'autre direction
  digitalWrite(motorDirInPin, HIGH);  // Moteur activé, DIR en HIGH
}
