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
#define READS_TO_AVERAGE 50

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
const int step_delay = 600; //1000 is minimum for a full step
const int steps_total = 360; //its 360 full steps

int currentPosition = steps_total; //let's assume we are all way up un the start
uint32_t motor_last_moved_at = 0;

int previous_direction = 0;
uint8_t driver_enabled = 0;

uint32_t voltageIn = 0;
uint32_t correctorIn = 0;

uint32_t newVoltageIn = 0;
uint32_t newCorrectorIn = 0;
uint16_t reads_made = 0;
uint16_t reads_reference_made = 0;

int minVal = 0;
int maxVal = 1023;
int target = 0;

//for left headlight: GREEN - corrector input (pin 6), BLACK - +12 (pin2), RED - GND (pin 4)

// vin   = 13.4V
// corr0 = 11.1V . =>80% of vin => max
// corr1 = 9.7V
// corr2 = 7.7V
// corr3 = 5.4V <= 40% of vin => min.

void disable(){
//  digitalWrite(sleepPin, LOW);
  digitalWrite(enablePin, HIGH);
  digitalWrite(blueLED, LOW);
  driver_enabled = 0;
}

void enable(){
  if(driver_enabled){
    return;
  }
  digitalWrite(blueLED, HIGH);
  digitalWrite(enablePin, LOW);
//  delayMicroseconds(2000);
//  delay(3);//it requires at least 1.7ms to start accepting data.
  driver_enabled = 1;
}

void moveToPosition(int target) {
  enable(); //power up driver
  int steps_count = 0;
  if(target < currentPosition) {
    digitalWrite(dirPin, LOW); // retract
    steps_count = currentPosition - target;
    previous_direction = 1;
  } else {
    digitalWrite(dirPin, HIGH); // extend
    steps_count = target - currentPosition;
    previous_direction = 0;
  }
  for (int x = 0; x < steps_count * 2; x++) { //x2 due to microstepping
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(step_delay);
  }
  currentPosition = target;
//  disable(); // Release motor to avoid overheating
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

  digitalWrite(ms1Pin, HIGH);
  digitalWrite(ms2Pin, LOW);
  digitalWrite(ms3Pin, LOW); // Step to 1/2 step
  digitalWrite(resetPin, HIGH);
  digitalWrite(sleepPin, HIGH);
  disable();
  digitalWrite(blueLED, LOW);

  analogReference(EXTERNAL);
  
  Serial.begin(19200);
  moveToPosition(0); //fully retract shaft on start (35.5mm min, 48mm max)*/
  delay(10);
  disable();
  delay(1000);

  voltageIn = 0;
  correctorIn = 0;
  for(int i = 0; i < READS_TO_AVERAGE; i++){
    voltageIn += analogRead(voltageInPin);
    correctorIn += analogRead(correctorInPin);
  }
  voltageIn = voltageIn / READS_TO_AVERAGE;
  correctorIn = correctorIn / READS_TO_AVERAGE;
  delay(500);
}

void loop() {
  int time_since_last_moved = (millis() - motor_last_moved_at);

  if(!driver_enabled && (time_since_last_moved > target)){ //dont measure while motor is moving and some time after (proportional to position, to avoid oscillations
    if(reads_made < READS_TO_AVERAGE){
      newCorrectorIn += analogRead(correctorInPin);
      reads_made++;
    }else{
      correctorIn = newCorrectorIn / READS_TO_AVERAGE;
      newCorrectorIn = 0;
      minVal = voltageIn * 0.4; //everything below 40% is 0
      maxVal = voltageIn * 0.8; //everything below 80% is 100
      target = constrain(correctorIn, minVal, maxVal); // limit value
      target = map(target, minVal, maxVal, 100, steps_total); // scale value to produce desired step number. Will omit useless lowest positions
      reads_made = 0;
    }
  }
  
  digitalWrite(redLED, !digitalRead(faultPin));

  int error = abs(target-currentPosition);

  if( ( error > 0 && //no denoise if no direction change
        (
          (target < currentPosition && previous_direction == 1) ||
          (target > currentPosition && previous_direction == 0)
        )
      ) ||
      ((error > 3u) && ( time_since_last_moved > 1500 )) || //denoise for small steps
      ((error > 25u) && (time_since_last_moved > 200)) //to avoid rapid direction changes
     ){
    
    digitalWrite(stepPin, LOW);//keep it low just in case
    /*enable(); //enable driver
    if(target < currentPosition) {
      digitalWrite(dirPin, LOW); // retract
      previous_direction = 1;
      currentPosition--;
    } else {
      digitalWrite(dirPin, HIGH); // extend
      previous_direction = 0;
      currentPosition++;
    }
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(step_delay);

    digitalWrite(stepPin, HIGH); //2 times due to microstepping.
    delayMicroseconds(step_delay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(step_delay);*/
    delay(currentPosition); //if motors are at different positions - delays will be different and it will reduce oscillations
    moveToPosition(target);
    delay(10);
    disable(); // Release motor to avoid overheating
    motor_last_moved_at = millis();
    delay(target);
  }else{
//    if(driver_enabled && (time_since_last_moved > 100)){ //disable driver, but don't do it too often. Driver affects readings heavily! And they interfeir between two lamps
//      //Serial.println(currentPosition);
//      disable();
//      delay(50);
//    }
//    delay(1);
  }

  /*if(!driver_enabled){// don't adjust reference - it will not adrift father than regulation inputs, but causes plenty of issues
    if(reads_reference_made < READS_REFERENCE_AVERAGE){
      newVoltageIn += analogRead(voltageInPin);
      reads_reference_made++;
    }else{
      voltageIn = newVoltageIn / READS_REFERENCE_AVERAGE;
      newVoltageIn = 0;
      reads_reference_made = 0;
    }
  }*/

}
