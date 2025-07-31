// Define pins and constants
const int SOS_BUTTON_PIN = 4; // Example pin for the SOS button
const int SLEEP_PIN = 5;      // Pin to control A9G sleep mode
String sos_number = "+9197xxxxxxxx"; // Pre-configured SOS contact number

String fromGSM;
String inputString;
bool stringComplete = false;
int sos_time = 0;
bool call_end = false;

void setup() {
  // Initialize Serial communication with PC
  Serial.begin(115200);
  
  // Initialize Serial1 communication with A9G module
  Serial1.begin(115200);
  
  pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SLEEP_PIN, OUTPUT);

  delay(1000);
  Serial.println("Initializing A9G Module...");
  digitalWrite(SLEEP_PIN, HIGH); // Wake up A9G
  delay(2000);
  
  Serial1.println("AT"); // Check communication
  delay(1000);
  Serial1.println("AT+GPS=1"); // Turn on GPS
  delay(1000);
  Serial1.println("AT+SLEEP=1"); // Enable light sleep mode
  Serial.println("System Initialized. Entering sleep mode.");
}

void loop() {
  // Check for incoming data from A9G
  if (Serial1.available()) {
    char inChar = Serial1.read();
    fromGSM += inChar;
    if (inChar == '\n') {
      handleGSMResponse(fromGSM);
      fromGSM = ""; // Clear the buffer
    }
  }

  // Check for incoming commands from Serial Monitor
  if (Serial.available()) {
    int inByte = Serial.read();
    Serial1.write(inByte);
  }

  // SOS Button Logic
  if (digitalRead(SOS_BUTTON_PIN) == LOW) {
    sos_time++;
    delay(1000);
    Serial.print("Calling In... ");
    Serial.println(5 - sos_time);
    
    if (sos_time >= 5) {
      triggerSOS();
      sos_time = 0; // Reset timer
    }
  } else {
    sos_time = 0; // Reset if button is released
  }
}

void handleGSMResponse(String response) {
  Serial.println(response); // Print GSM module response to Serial Monitor
  
  if (response.indexOf("RING") != -1) {
    digitalWrite(SLEEP_PIN, LOW); // Wake up module
    Serial.println("ITS RINGING");
    Serial1.println("ATA"); // Answer the call
  } else if (response.indexOf("NO CARRIER") != -1) {
    Serial.println("CALL ENDS");
    digitalWrite(SLEEP_PIN, HIGH); // Go back to sleep
  }
}

void triggerSOS() {
  digitalWrite(SLEEP_PIN, LOW); // Wake up module
  delay(1000);

  // Get GPS Location
  Serial.println("Getting location...");
  Serial1.println("AT+LOCATION=2");
  
  long startTime = millis();
  String locationData = "";
  
  while(millis() - startTime < 10000) { // Wait for 10 seconds for a response
    if (Serial1.available()) {
      locationData = Serial1.readStringUntil('\n');
      if (locationData.indexOf("+LOCATION:") != -1) {
        break;
      }
    }
  }

  if (locationData.indexOf("GPS NOT FIX") != -1 || locationData == "") {
    Serial.println("No Location data");
    sendSMS("Help! My location is not available.");
  } else {
    // Format the location into a Google Maps link
    locationData.remove(0, locationData.indexOf(":") + 1);
    String lat = locationData.substring(0, locationData.indexOf(','));
    String lon = locationData.substring(locationData.indexOf(',') + 1);
    String gmaps_link = "http://maps.google.com/maps?q=" + lat + "," + lon;
    
    Serial.print("Latitude: "); Serial.println(lat);
    Serial.print("Longitude: "); Serial.println(lon);
    
    String message = "I'm here: " + gmaps_link;
    sendSMS(message);
  }

  // Make the call
  makeCall();

  digitalWrite(SLEEP_PIN, HIGH); // Put module to sleep
}

void sendSMS(String message) {
  Serial.println("Sending SMS...");
  Serial1.println("AT+CMGF=1"); // Set SMS to text mode
  delay(1000);
  Serial1.println("AT+CMGS=\"" + sos_number + "\"");
  delay(1000);
  Serial1.print(message);
  delay(1000);
  Serial1.write(26); // Send CTRL+Z
  delay(1000);
  Serial.println("SMS Sent.");
}

void makeCall() {
  Serial.println("Calling Now...");
  Serial1.println("ATD" + sos_number + ";");
  // The call will be ended by the other party or by the "NO CARRIER" response
}