#define Broche_Echo 3  // Assigne le numéro de la broche Echo du HC-SR04 à la broche 2 (par exemple) //
#define Broche_Trigger 2  // Assigne le numéro de la broche Trigger du HC-SR04 à la broche 3 (par exemple) //

// Definition des variables
long Duree;
long Distance;

void setup() {
  pinMode(Broche_Trigger, OUTPUT); // Broche Trigger en sortie //
  pinMode(Broche_Echo, INPUT); // Broche Echo en entrée //
  Serial.begin(9600);
}

void loop() {
  // Debut de la mesure avec un signal de 10 μS appliqué sur TRIG //
  digitalWrite(Broche_Trigger, LOW); // On efface l'état logique de TRIG //
  delayMicroseconds(2);
  digitalWrite(Broche_Trigger, HIGH); // On met la broche TRIG à "1" pendant 10μS //
  delayMicroseconds(10);
  digitalWrite(Broche_Trigger, LOW); // On remet la broche TRIG à "0" //

  // On mesure combien de temps le niveau logique haut est actif sur ECHO //
  Duree = pulseIn(Broche_Echo, HIGH);

  // Calcul de la distance grâce au temps mesuré //
  Distance = Duree * 0.034 / 2; // *** voir explications après l'exemple de code *** //

  // Affichage dans le moniteur série de la distance mesurée //
  Serial.print("Temps mesure : ");
  Serial.print(Duree);
  Serial.print(" microseconds, Distance mesuree : ");
  Serial.print(Distance);
  Serial.println(" cm");

  delay(500); // On ajoute 1 seconde de délai entre chaque mesure // à modifier
}

