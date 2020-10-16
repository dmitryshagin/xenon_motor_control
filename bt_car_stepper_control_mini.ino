//+PC0 - voltage in
//+PC1 - corrector in

//+PC2 - green LED
//+PC3 - blue LED
//+PC4 - RED LED

//+PD2 - DIR 
//+PD3 - STEP
//+PD4 - NSLEEP
//+PD5 - NRST
//+PD6 - MODE1
//+PD7 - MODE2

//+PB0 - MODE0
//+PB1 - NENBL
//+PB2 - NFAULT

const int dirPin = 2;
const int stepPin = 3;
const int sleepPin = 4;
const int resetPin = 5;
const int ms1Pin = 8;
const int ms2Pin = 7;
const int ms3Pin = 6;
const int enablePin = 9;
const int faultPin = 10;


const int voltageInPin = A0;
const int correctorInPin = A1;
const int greenLED = A2;
const int blueLED = A3;
const int redLED = A4;
const int step_delay = 1000;
const int steps_total = 360;

int currentPosition = steps_total; //let's assume we are all way up un the start

//for left headlight: GREEN - corrector input (pin 6), BLACK - +12 (pin2), RED - GND (pin 4)

// vin   = 13.4V
// corr0 = 11.1V . =>80% of vin => max
// corr1 = 9.7V
// corr2 = 7.7V
// corr3 = 5.4V <= 40% of vin => min.

void disable(){
  digitalWrite(sleepPin, LOW);
  digitalWrite(enablePin, HIGH);
  digitalWrite(blueLED, LOW);
}

void enable(){
  digitalWrite(blueLED, HIGH);
  digitalWrite(sleepPin, HIGH);
  digitalWrite(enablePin, LOW);
//  delayMicroseconds(2000);
  delay(2);//it requires at least 1.7ms to start accepting data.
}

void moveToPosition(int target) {
  enable(); //power up driver
  int steps_count = 0;
  if(target < currentPosition) {
    digitalWrite(dirPin, LOW); // retract
    steps_count = currentPosition - target;
  } else {
    digitalWrite(dirPin, HIGH); // extend
    steps_count = target - currentPosition;
  }
  for (int x = 0; x < steps_count; x++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(step_delay);
  }
  currentPosition = target;
  disable(); // Release motor to avoid overheating
}

void setup() {
  delayMicroseconds(10000);
  pinMode(redLED, OUTPUT);
  digitalWrite(redLED, LOW);
  pinMode(greenLED, OUTPUT);
  digitalWrite(greenLED, HIGH);
  pinMode(blueLED, OUTPUT);
  digitalWrite(blueLED, LOW);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  
  pinMode(ms1Pin, OUTPUT);
  pinMode(ms2Pin, OUTPUT);
  pinMode(ms3Pin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  pinMode(sleepPin, OUTPUT);

  pinMode(faultPin, INPUT);

  pinMode(correctorInPin, INPUT);
  pinMode(voltageInPin, INPUT);

  digitalWrite(ms1Pin, LOW);
  digitalWrite(ms2Pin, LOW);
  digitalWrite(ms3Pin, LOW); // Step to full step
  digitalWrite(resetPin, HIGH);
  disable();
  digitalWrite(blueLED, LOW);
  
  Serial.begin(9600);
  moveToPosition(0); //fully retract shaft on start (35.5mm min, 48mm max)*/
  delay(1000);
}

void loop() {
  //average 100 reads
  uint32_t voltageIn = 0;
  uint32_t correnctorIn = 0;
  for(int i=0; i<1000; i++){
    voltageIn += analogRead(voltageInPin);
    correnctorIn += analogRead(correctorInPin);
  }
  voltageIn = voltageIn/1000;
  correnctorIn = correnctorIn/1000;

//  Serial.print("VIN: ");Serial.println(voltageIn);
//  Serial.print("CIN: ");Serial.println(correnctorIn);
  
  int minVal = voltageIn * 0.4; //everything below 40% is 0
  int maxVal = voltageIn * 0.8; //everything below 80% is 100
  int val = constrain(correnctorIn, minVal, maxVal); // limit value
  val = map(val, minVal, maxVal, 0, steps_total); // scale value to produce desired step number
  
  digitalWrite(redLED, !digitalRead(faultPin));
  
  if(abs(val-currentPosition) > 2u){ //denoise
    moveToPosition(val);
    delay(500);
  }else{
    delay(500); // move once a half a second or so
  }
}
