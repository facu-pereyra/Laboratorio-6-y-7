#define MIC A0
int sig = 0;
void setup() {
 pinMode(2, OUTPUT);
}
void led() {
 sig = analogRead(MIC)*50;
 if (sig>100)  {digitalWrite(2, HIGH);} else {digitalWrite(2, LOW)
}
void loop() {
 led();
}
