#include <math.h>
#define sei()
#define cli()
#define WindSensorPin (2) // The pin location of the anemometer sensor

volatile unsigned long Rotations; // cup rotation counter used in interrupt routine
volatile unsigned long ContactBounceTime; // Timer to avoid contact bounce in interrupt routine

float WindSpeed; // speed miles per hour --> mph*1.6093=kph
int VaneValue;

void setup() {
Serial.begin(9600);

pinMode(WindSensorPin, A2);
attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING);

Serial.println("Davis Wind Speed Test");
Serial.println("Rotations\tMPH"); //change in kph
}

void loop() {
VaneValue = analogRead(A2);
Rotations = 0; // Set Rotations count to 0 ready for calculations

sei(); // Enables interrupts
delay (2000); // Wait 2 seconds to average
cli(); // Disable interrupts

// convert to mp/h using the formula V=P(2.25/T)
// V = P(2.25/3) = P * 0.75

WindSpeed = Rotations * 0.75;

Serial.print(Rotations); Serial.print("\t\t");
Serial.println(WindSpeed);

}

// This is the function that the interrupt calls to increment the rotation count
void isr_rotation () {

if ((millis() - ContactBounceTime) > 15 ) { // debounce the switch contact.
Rotations++;
ContactBounceTime = millis();
}

}
