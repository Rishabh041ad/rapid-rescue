#include <LiquidCrystal.h>
#include <TinyGPS.h>
LiquidCrystal lcd(4, 5, 6, 7, 8, 9);

const int relay_Pin = 2;
const int buzzer_Pin = 3;
const int ir_Sensor = 10;        // Seatbelt sensor
const int alcohol_Sensor = 11;
const int vibration_Sensor = 12;
TinyGPS gps;
long lat = 28563900, lon = 77159000;
bool ir_status = LOW;
bool alcohol_Status = LOW;
bool vibration_Status = LOW;

void setup() {
  pinMode(relay_Pin, OUTPUT);
  pinMode(buzzer_Pin, OUTPUT);
  pinMode(ir_Sensor, INPUT);
  pinMode(alcohol_Sensor, INPUT);
  pinMode(vibration_Sensor, INPUT);
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.print("73,78,95 ACCIDENT");
  lcd.setCursor(0, 1);
  lcd.print("DETECTION SYSTEM");
  delay(100); // Wait for 2 seconds to display the initial message
  
  checkInitialConditions(); // Check seatbelt and alcohol status at startup
}

void loop() {
  ir_status = digitalRead(ir_Sensor);
  alcohol_Status = digitalRead(alcohol_Sensor);
  vibration_Status = digitalRead(vibration_Sensor);

  if (ir_status == HIGH && alcohol_Status == LOW) {   // Both seatbelt detected and no alcohol
    digitalWrite(buzzer_Pin, LOW);   
//    lcd.clear();
//    lcd.print("System Ready");
//    delay(100);  // Slight delay for the "System Ready" message
    
//    lcd.clear();
//    lcd.print("Vehicle Starting...");
//    delay(100);  // Wait for the user to see the "Vehicle Starting" message
    
    digitalWrite(relay_Pin, HIGH);   // Start the vehicle (Relay ON)
//    lcd.clear();
//    lcd.print("Vehicle Started");
//    delay(500);  // Allow the user to see that the vehicle has started

    while (1) {
      // Continuously check for alcohol, seatbelt, and accidents during operation
      ir_status = digitalRead(ir_Sensor);
      alcohol_Status = digitalRead(alcohol_Sensor);
      vibration_Status = digitalRead(vibration_Sensor);

      if (vibration_Status == HIGH) {
        triggerAlert("Accident Detected");
        delay(100); // Allow time for the alert to display
        sendLocation(); // Send GPS location via SMS
        while (digitalRead(vibration_Sensor) == HIGH) {
          // Keep the vehicle stopped and the buzzer sounding until accident condition is cleared
        }
        stopAlert();  // Stop the buzzer when the accident condition is cleared
        lcd.clear();
        lcd.print("VehicleRestarted");
        delay(100);
        digitalWrite(relay_Pin, HIGH); // Restart the vehicle
      }

      // Check conditions without flickering the display
      if (alcohol_Status == HIGH) {
        triggerAlert("Alcohol Detected");
        // Keep the alert displayed until the condition changes
        while (digitalRead(alcohol_Sensor) == HIGH) {
          // Wait for alcohol to be cleared
        }
        stopAlert();  // Stop the buzzer when alcohol is no longer detected
        lcd.clear();
        lcd.print("VehicleRestarted");
        delay(100);
      }
      
      if (ir_status == LOW) {
        triggerAlert("Seatbelt Unbuckled");
        // Keep the alert displayed until the condition changes
        while (digitalRead(ir_Sensor) == LOW) {
          // Wait for the seatbelt to be buckled
        }
        stopAlert();  // Stop the buzzer when the seatbelt is buckled again
        lcd.clear();
        lcd.print("VehicleRestarted");
        delay(100);
      }
    }
  } else {
    // If seatbelt is unbuckled or alcohol is detected during initial check
    if (ir_status == LOW) {
      triggerAlert("Seatbelt Unbuckled");
    }
    
    if (alcohol_Status == HIGH) {
      triggerAlert("Alcohol Detected");
    }
  }
}

void checkInitialConditions() {
  ir_status = digitalRead(ir_Sensor);
  alcohol_Status = digitalRead(alcohol_Sensor);

  // Check if seatbelt is unbuckled or alcohol is detected
  if (ir_status == LOW || alcohol_Status == HIGH) {
    // Trigger alert if either condition fails
    if (ir_status == LOW) {
      triggerAlert("Seatbelt Unbuckled");
       while (ir_status == LOW) {
      ir_status = digitalRead(ir_Sensor);
      alcohol_Status = digitalRead(alcohol_Sensor);
      delay(100); // Small delay to debounce sensor readings
    }
    }
    
    if (alcohol_Status == HIGH) {
      triggerAlert("Alcohol Detected");
      while (alcohol_Status == HIGH) {
      ir_status = digitalRead(ir_Sensor);
      alcohol_Status = digitalRead(alcohol_Sensor);
      delay(100); // Small delay to debounce sensor readings
    }
    }
    
    // Keep the vehicle stopped and wait for both conditions to be resolved
//    while (ir_status == LOW || alcohol_Status == HIGH) {
//      ir_status = digitalRead(ir_Sensor);
//      alcohol_Status = digitalRead(alcohol_Sensor);
//      delay(100); // Small delay to debounce sensor readings
//    }

    // Stop the alert once both conditions are resolved
    stopAlert();
    lcd.clear();
    lcd.print("Vehicle Starting");
    delay(100); // Display "Vehicle Starting" before turning on the relay
    digitalWrite(relay_Pin, HIGH);  // Start the vehicle after both conditions are met
    lcd.clear();
    lcd.print("Vehicle Started");
    delay(100);  // Allow the user to see the "Vehicle Started" message
  } else {
    // If no issues, the vehicle is ready to start
    lcd.clear();
    lcd.print("Seatbelt and");
    lcd.setCursor(0, 1);
    lcd.print("Alcohol OK");
    delay(100);
    lcd.clear();
    lcd.print("Vehicle Starting");
    delay(100);  // Display "Vehicle Starting" before turning on the relay
    digitalWrite(relay_Pin, HIGH);  // Allow the vehicle to start
    lcd.clear();
    lcd.print("Vehicle Started");
    delay(100);  // Allow the user to see the "Vehicle Started" message
  }
}

void triggerAlert(String reason) {
  lcd.clear();
  lcd.print(reason);  // Display the reason for stopping (Alcohol or Seatbelt)
  digitalWrite(relay_Pin, LOW);  // Stop the motor (Relay OFF)
  digitalWrite(buzzer_Pin, HIGH);  // Keep the buzzer on
}

void stopAlert() {
  digitalWrite(buzzer_Pin, LOW);  // Stop the buzzer
  digitalWrite(relay_Pin, HIGH);  // Allow the vehicle to start again
}

void sendLocation() {
  // Read GPS data and send it via SMS
  gps_read();
  Serial.println("AT+CMGF=1"); // Set GSM Module to Text Mode
  delay(100);
  Serial.print("AT+CMGS=\"+918072022561\"\r"); // Replace with your phone number
  delay(100);
  Serial.println("Accident Detected!\n"); // The SMS text you want to send
  Serial.println("Location:");
  Serial.print("Latitude: ");
  Serial.println((lat * 0.000001), 8);
  Serial.print(", Longitude: ");
  Serial.println((lon * 0.000001), 8);
  delay(100);
  Serial.println("To check the nearest hospital click:");
  Serial.print("https://rahuljiv2004.github.io/accident-alert/?lat=");
  Serial.print((lat * 0.000001), 8);  // Insert actual latitude value
  Serial.print("&lon=");
  Serial.print((lon * 0.000001), 8);
 
  Serial.write(26); // Send Ctrl+Z to indicate the end of the message
}

void gps_read() { 
  byte a;
  
  if (Serial.available()) {  // Check if data is available from GPS
    a = Serial.read();
    while (gps.encode(a)) {  // Encode GPS data 
      gps.get_position(&lat, &lon); // Get latitude and longitude
    }
  }
}
