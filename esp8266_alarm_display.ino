#include <FS.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <Keypad_MC17.h>
#include <Keypad.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// * Include settings
#include "settings.h"

// * Include PubSubClient after setting the correct packet size
#include <PubSubClient.h>

// * Initiate led blinker library
Ticker ticker;

// * Initiate Watchdog
Ticker tickerOSWatch;

// * Initiate WIFI client
WiFiClient espClient;

// * Initiate MQTT client
PubSubClient mqtt_client(espClient);

// * Initiate display
Adafruit_SSD1306 display(OLED_RESET);

// * Initiate RFID reader
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);   // Create MFRC522 instance.

// * Initiate Keypad
Keypad_MC17 keypad(makeKeymap(KEYPAD_KEYS), KEYPAD_ROW_PINS, KEYPAD_COLUMN_PINS, KEYPAD_ROWS,KEYPAD_COLS, MCP23017_I2C_ADDR);

// * Set current time for last rfid scan and last keypad input
unsigned long LAST_KEYPAD_TIME  = millis();
unsigned long LAST_SCANNED_TIME = millis();

// * Used by wifi manager to determine if settings should be saved
bool shouldSaveConfig = false;

// **********************************
// * System Functions               *
// **********************************

// * Watchdog function
void ICACHE_RAM_ATTR osWatch(void)
{
    unsigned long t = millis();
    unsigned long last_run = abs(t - last_loop);
    if (last_run >= (OSWATCH_RESET_TIME * 1000)) {
        // save the hit here to eeprom or to rtc memory if needed
        ESP.restart();  // normal reboot
    }
}

// * Gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println(F("Entered config mode"));
    Serial.println(WiFi.softAPIP());

    // * If you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());

    // * Entered config mode, make led toggle faster
    ticker.attach(0.2, tick);
}

// * Callback notifying us of the need to save config
void save_wifi_config_callback ()
{

    shouldSaveConfig = true;
}

// * Blink on-board Led
void tick()
{
    // * Toggle state
    int state = digitalRead(LED_BUILTIN);    // * Get the current state of GPIO1 pin
    digitalWrite(LED_BUILTIN, !state);       // * Set pin to the opposite state
}

// **********************************
// * Display Functions              *
// **********************************

// * Update the display update timer
void set_display_updated()
{
    DISPLAY_LAST_UPDATE = millis();
    DISPLAY_SHOULD_UPDATE = false;
}

// * Force a display update
void set_display_should_update()
{

    DISPLAY_SHOULD_UPDATE = true;
}

// * Returns true if the display should update, otherwise false
bool display_should_update()
{

    return DISPLAY_SHOULD_UPDATE;
}

// * Setup the display: Print rounded square and go to position 0,0
void setup_display()
{
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.clearDisplay();
  display.drawRoundRect(3, 3, display.width() - 3 * 3, display.height() - 3 * 3, display.height() / 4, WHITE);

}

// * Print a message after authentication
void print_auth_message(const char* first_line, const char* second_line)
{
    setup_display();

    display.setTextSize(1);
    display.println("");
    display.println("");
    display.println("   " + String(first_line));
    display.println("");
    display.println("   " + String(second_line));
    display.println("");

    display.display();
    set_display_updated();
}

// * Print a 2 lines status message to the display
void print_boot_message(const char* first_line, const char* second_line)
{
    setup_display();
    display.setTextSize(2);
    display.println("");
    display.println("  " + String(first_line));
    display.println("  " + String(second_line));
    display.println("");

    display.display();
    set_display_updated();
}

// * Ask for user input on the display
void print_please_make_your_choice()
{
    setup_display();

    display.setTextSize(1);
    display.println("");
    display.println("   Select option");
    display.println("");
    display.println("    1: Disarm");
    display.println("    2: Home");
    display.println("    3: Away");
    display.println("");

    display.display();
    set_display_updated();
}

// * Print a password prompt
void print_password_prompt()
{

    String stars = "  ";
    for (int i=0; i < MAX_PASSWORD_LENGTH; i++) {
        if (i < PASSWORD_POSITION) {
            stars += "*";
        }
        else {
            stars += ".";
        }
    }

    setup_display();
    display.setTextSize(1);
    display.println("");
    display.println("");
    display.println("    Insert pin");
    display.println("    Followed by #");
    display.println("");
    display.println("  " + stars);
    display.display();
    set_display_updated();
}

// **********************************
// * Alarm display                  *
// **********************************

// * Update the alarm state with the new received state
void set_alarm_state(char* alarm_state)
{
    if (strcmp(alarm_state, CURRENT_ALARM_STATE) != 0) {
        // * Set the current alarm state to the given new state
        memcpy(CURRENT_ALARM_STATE, alarm_state, sizeof(CURRENT_ALARM_STATE));

        // * Set display to be updated
        set_display_should_update();
    }
}

// * Process the incoming json message by dispatching each element to it's own library function
bool process_json_input(char* payload)
{
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(payload);

    if (root.success()) {
        if (root.containsKey("uid") && root["uid"] == String(LAST_SCANNED_UID)) {
            if ((root.containsKey("name")) && (root.containsKey("access"))) {
                if (root["access"] ==  "GRANTED") {
                    char * message = new char[3 + strlen(root["name"]) + 1];
                    strcpy(message, "Hi ");
                    strcat(message, root["name"]);
                    print_auth_message("Access Granted", message);
                }
                else if (root["access"] == "DENIED") {
                    char * message = new char[6 + strlen(root["name"]) + 1];
                    strcpy(message, "Sorry ");
                    strcat(message, root["name"]);
                    print_auth_message("Access Denied", message);
                }
            }
            else {
                print_auth_message("Access Denied", "Please fuck off");
            }
        }
        else {
            print_auth_message("Auth Error", "Please rescan token.");
        }
        return true;
    }
    else {
        return false;
    }
}

// **********************************
// * MQTT                           *
// **********************************

// * Send a message to a broker topic
void send_mqtt_message(const char *topic, char *payload)
{
    Serial.printf("MQTT Outgoing on %s: ", topic);
    Serial.println(payload);

    bool result = mqtt_client.publish(topic, payload, false);

    if (!result) {
        Serial.printf("MQTT publish to topic %s failed\n", topic);
    }
}

// * Callback for incoming MQTT messages
void mqtt_callback(char* topic, byte* payload_in, unsigned int length)
{
    char* payload = (char *) malloc(length + 1);
    memcpy(payload, payload_in, length);
    payload[length] = 0;

    Serial.printf("MQTT Incoming on %s: ", topic);
    Serial.println(payload);

    if (strcmp(topic, MQTT_IN_WIDE) == 0) {
        Serial.println(F("State change requested"));
        set_alarm_state(payload);
    }
    else if (strcmp(topic, MQTT_IN_SOLO) == 0) {
        bool result = process_json_input(payload);
        if (result) {
            Serial.print(F("Incoming json message processed."));
        } else {
            Serial.println(F("Input error: parseObject() failed."));
        }
    }
    free(payload);
}

// * Reconnect to MQTT server and subscribe to in and out topics
bool mqtt_reconnect()
{
    // * Loop until we're reconnected
    int MQTT_RECONNECT_RETRIES = 0;

    print_boot_message("System", "Connect");

    while (!mqtt_client.connected() && MQTT_RECONNECT_RETRIES < MQTT_MAX_RECONNECT_TRIES) {
        MQTT_RECONNECT_RETRIES++;

        Serial.printf("MQTT connection attempt %d / %d ...\n", MQTT_RECONNECT_RETRIES, MQTT_MAX_RECONNECT_TRIES);

        // * Attempt to connect
        if (mqtt_client.connect(HOSTNAME, MQTT_USER, MQTT_PASS)) {
            Serial.println(F("MQTT connected!"));

            // * Once connected, publish an announcement...
            char * message = new char[23 + strlen(HOSTNAME) + 1];
            strcpy(message, "Alarm display alive: ");
            strcat(message, HOSTNAME);
            mqtt_client.publish("hass/status", message);

            Serial.printf("MQTT wide topic in: %s\n", MQTT_IN_WIDE);
            Serial.printf("MQTT solo topic in: %s\n", MQTT_IN_SOLO);
            Serial.printf("MQTT RFID topic out: %s\n", MQTT_RFID_CHANNEL);

            // * Resubscribe to the state and verify mqtt topic
            mqtt_client.subscribe(MQTT_IN_WIDE);
            mqtt_client.subscribe(MQTT_IN_SOLO);

            // * Set display to "System Online"
            print_boot_message("System", "Online");
        }
        else {
            // * Set display to "System Offline"
            print_boot_message("System", "Offline");

            Serial.print(F("MQTT Connection failed: rc="));
            Serial.println(mqtt_client.state());
            Serial.println(F(" Retrying in 5 seconds"));
            Serial.println("");

            // * Wait 5 seconds before retrying
            delay(5000);
        }
    }

    if (MQTT_RECONNECT_RETRIES >= MQTT_MAX_RECONNECT_TRIES) {
        Serial.printf("*** MQTT connection failed, giving up after %d tries ...\n", MQTT_RECONNECT_RETRIES);
        return false;
    }

    return true;
}

// **********************************
// * KEYPAD                         *
// **********************************

// * Process which action to take after input
bool process_action_choice(char input)
{
    switch (input) {
        case '1':
            ACTION = 1;
            ACTION_CHOSEN = 1;
            break;
        case '2':
            ACTION = 2;
            ACTION_CHOSEN = 1;
            break;
        case '3':
            ACTION = 3;
            ACTION_CHOSEN = 1;
            break;
    }
}

// * Manage keypad input for keys 0-9
bool process_keypad_input(char character)
{
    Serial.print("Keypad input: ");
    Serial.println(character);

    LAST_KEYPAD_TIME = millis();
    set_display_should_update();

    if ((PASSWORD_SET == 1) && (ACTION_CHOSEN == 0)) {
        // * If password is already set: Process input choice
        process_action_choice(character);
        return true;
    }
    else {
        // * Check if max password input is reached
        if ((PASSWORD_POSITION + 1 == MAX_PASSWORD_LENGTH)) {
            return false;
        }
        else {
            // * Process the password input
            PASSWORD_GUESS[PASSWORD_POSITION++] = character;
            PASSWORD_GUESS[PASSWORD_POSITION] = STRING_TERMINATOR;
            return true;
        }
    }
}

// * Reset the input state for password, choice and rfid
void reset_input()
{
    Serial.println("Resetting Input");

    PASSWORD_SET = 0;
    PASSWORD_POSITION = 0;
    PASSWORD_GUESS[PASSWORD_POSITION] = STRING_TERMINATOR;

    RFID_UID_SET = 0;

    ACTION = 0;
    ACTION_CHOSEN = 0;

    set_display_should_update();
}

// * Set the password state to being set (when # is pressed)
void end_password_input()
{
    Serial.println("Mark password as completed");
    PASSWORD_SET = 1;

    set_display_should_update();

}

// * Will be executed when the keypad receives input
void keypad_event_callback(KeypadEvent key_input)
{
    switch (keypad.getState()) {
        case PRESSED:
        switch (key_input) {
            case '#':
                end_password_input();
                break;
            case '*':
                reset_input();
                break;
            default:
                process_keypad_input(key_input);
        }
    }
}

// **********************************
// * EEPROM & OTA helpers           *
// **********************************

// * Read mqtt credentials from eeprom
String read_eeprom(int offset, int len)
{
    String res = "";

    for (int i = 0; i < len; ++i) {
        res += char(EEPROM.read(i + offset));
    }
    Serial.print(F("read_eeprom(): "));
    Serial.println(res.c_str());

    return res;
}

// * Write the mqtt credentials to eeprom
void write_eeprom(int offset, int len, String value)
{
    Serial.print(F("write_eeprom(): "));
    Serial.println(value.c_str());

    for (int i = 0; i < len; ++i) {
        if ((unsigned)i < value.length()) {
            EEPROM.write(i + offset, value[i]);
        } else {
            EEPROM.write(i + offset, 0);
        }
    }
}

// * Setup update over the air
void setup_ota()
{
    Serial.println(F("Arduino OTA activated."));

    // * Port defaults to 8266
    ArduinoOTA.setPort(8266);

    // * Set hostname for OTA
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        Serial.println(F("Arduino OTA: Start"));
    });

    ArduinoOTA.onEnd([]() {
        Serial.println(F("Arduino OTA: End (Running reboot)"));
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Arduino OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println(F("Arduino OTA: Auth Failed"));
        else if (error == OTA_BEGIN_ERROR)
            Serial.println(F("Arduino OTA: Begin Failed"));
        else if (error == OTA_CONNECT_ERROR)
            Serial.println(F("Arduino OTA: Connect Failed"));
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println(F("Arduino OTA: Receive Failed"));
        else if (error == OTA_END_ERROR)
            Serial.println(F("Arduino OTA: End Failed"));
    });
    ArduinoOTA.begin();
    Serial.println(F("Arduino OTA finished"));
}

// **********************************
// * Setup                          *
// **********************************

void setup()
{
    // Configure Watchdog
    last_loop = millis();
    tickerOSWatch.attach_ms(((OSWATCH_RESET_TIME / 3) * 1000), osWatch);

    // * Configure Serial and EEPROM
    Serial.begin(BAUD_RATE);

    // * Initiate EEPROM
    EEPROM.begin(512);

    // * Initiate SPI bus
    SPI.begin();

    // * Initiate MFRC522
    mfrc522.PCD_Init();

    // * Initiate Keypad through MCP23017 IO Expander
    keypad.begin();

    // * Set led pin as output
    pinMode(LED_BUILTIN, OUTPUT);

    // * Start ticker with 0.5 because we start in AP mode and try to connect
    ticker.attach(0.6, tick);

    // * Setup display
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();

    // * Set display to "System Booting"
    print_boot_message("System", "Booting");

    // * Get MQTT Server settings
    String settings_available = read_eeprom(134, 1);

    if (settings_available == "1") {
        read_eeprom(0, 64).toCharArray(MQTT_HOST, 64);   // * 0-63
        read_eeprom(64, 6).toCharArray(MQTT_PORT, 6);    // * 64-69
        read_eeprom(70, 32).toCharArray(MQTT_USER, 32);  // * 70-101
        read_eeprom(102, 32).toCharArray(MQTT_PASS, 32); // * 102-133
    }

    WiFiManagerParameter CUSTOM_MQTT_HOST("host", "MQTT hostname", MQTT_HOST, 64);
    WiFiManagerParameter CUSTOM_MQTT_PORT("port", "MQTT port",     MQTT_PORT, 6);
    WiFiManagerParameter CUSTOM_MQTT_USER("user", "MQTT user",     MQTT_USER, 32);
    WiFiManagerParameter CUSTOM_MQTT_PASS("pass", "MQTT pass",     MQTT_PASS, 32);

    // * WiFiManager local initialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // * Reset settings - uncomment for testing
    //   wifiManager.resetSettings();

    // * Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);

    // * Set timeout
    wifiManager.setConfigPortalTimeout(WIFI_TIMEOUT);

    // * Set save config callback
    wifiManager.setSaveConfigCallback(save_wifi_config_callback);

    // * Add all your parameters here
    wifiManager.addParameter(&CUSTOM_MQTT_HOST);
    wifiManager.addParameter(&CUSTOM_MQTT_PORT);
    wifiManager.addParameter(&CUSTOM_MQTT_USER);
    wifiManager.addParameter(&CUSTOM_MQTT_PASS);

    // * Fetches SSID and pass and tries to connect
    // * Reset when no connection after 10 seconds
    if (!wifiManager.autoConnect()) {
        Serial.println(F("Failed to connect to WIFI and hit timeout"));

        // * Reset and try again
        ESP.reset();
        delay(WIFI_TIMEOUT);
    }

    // * Read updated parameters
    strcpy(MQTT_HOST, CUSTOM_MQTT_HOST.getValue());
    strcpy(MQTT_PORT, CUSTOM_MQTT_PORT.getValue());
    strcpy(MQTT_USER, CUSTOM_MQTT_USER.getValue());
    strcpy(MQTT_PASS, CUSTOM_MQTT_PASS.getValue());

    // * Save the custom parameters to FS
    if (shouldSaveConfig) {
        Serial.println(F("Saving WiFiManager config"));

        write_eeprom(0, 64, MQTT_HOST);   // * 0-63
        write_eeprom(64, 6, MQTT_PORT);   // * 64-69
        write_eeprom(70, 32, MQTT_USER);  // * 70-101
        write_eeprom(102, 32, MQTT_PASS); // * 102-133
        write_eeprom(134, 1, "1");        // * 134 --> always "1"
        EEPROM.commit();
    }

    // * If you get here you have connected to the WiFi
    Serial.println(F("Connected to WIFI..."));

    // * Keep LED on
    ticker.detach();
    digitalWrite(LED_BUILTIN, LOW);

    // * Configure OTA
    setup_ota();

    // * Startup MDNS Service
    Serial.println(F("Starting MDNS responder service"));
    MDNS.begin(HOSTNAME);

    // * Setup Keypad
    keypad.addEventListener(keypad_event_callback);

    // * Setup MQTT
    mqtt_client.setServer(MQTT_HOST, atoi(MQTT_PORT));
    mqtt_client.setCallback(mqtt_callback);
    Serial.printf("MQTT connecting to: %s:%s\n", MQTT_HOST, MQTT_PORT);

    // * Set the display to "System Connect"
    print_boot_message("System", "Connect");
}

// **********************************
// * Loop                           *
// **********************************

void loop()
{
    // * Update last loop watchdog value
    last_loop = millis();

    // * Accept ota requests if offered
    ArduinoOTA.handle();

    // **********************************
    // Maintain MQTT Connection
    // **********************************

    if (!mqtt_client.connected()) {
        unsigned long now = millis();

        if (now - LAST_RECONNECT_ATTEMPT >= MQTT_RECONNECT_DELAY) {
            LAST_RECONNECT_ATTEMPT = now;

            if (mqtt_reconnect()) {
                LAST_RECONNECT_ATTEMPT = 0;
            }
        }
    }
    else {
        mqtt_client.loop();
    }

    // **********************************
    // * Scan for RFID Tokens
    // **********************************

    if (RFID_UID_SET == 0) {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            // * If a card was read, get the uid and send it to the MQTT broker
            String reading = "";
            for (byte i = 0; i < mfrc522.uid.size; i++) {
                reading.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
                reading.concat(String(mfrc522.uid.uidByte[i], HEX));
            }
            reading.toUpperCase();

            char * UID = new char[reading.length() + 1];
            reading.toCharArray(UID, reading.length() + 1);

            unsigned long now = millis();
            if ((strcmp(LAST_SCANNED_UID, UID) != 0) || (now - LAST_SCANNED_TIME >= LAST_SCANNED_DELAY)) {
                Serial.print("New UID tag: ");
                Serial.println(UID);

                LAST_SCANNED_UID = UID;
                LAST_SCANNED_TIME = now;
                RFID_UID_SET = 1;
                print_boot_message("Got", "Token");
            }
        }
    }

    // **********************************
    // * Scan for new keypad input
    // **********************************

    // * Get input from keypad when no action chosen or password not set
    if (ACTION_CHOSEN == 0 || PASSWORD_SET == 0) {
        keypad.getKey();
    }

    // **********************************
    // * Update the display
    // **********************************

    unsigned long NOW = millis();

    // * If password not set, token not scanned, postition at zero and no choice made: Update display to current state
    if ((PASSWORD_SET == 0) && (PASSWORD_POSITION == 0) && (RFID_UID_SET == 0) && (ACTION_CHOSEN == 0)) {
        if ((display_should_update()) || (NOW - DISPLAY_LAST_UPDATE >= DISPLAY_UPDATE_FREQUENCY))  {
            if (strcmp(CURRENT_ALARM_STATE, ALARM_PENDING_STATE) == 0) {
                print_boot_message("Alarm", "Pending");
            }
            else if (strcmp(CURRENT_ALARM_STATE, ALARM_TRIGGERED_STATE) == 0) {
                print_boot_message("Alarm", "Active");
            }
            else if (strcmp(CURRENT_ALARM_STATE, ALARM_DISARMED_STATE) == 0) {
                print_boot_message("Alarm", "Disarm");
            }
            else if (strcmp(CURRENT_ALARM_STATE, ALARM_ARMED_HOME_STATE) == 0) {
                print_boot_message("Armed", "Home");
            }
            else if (strcmp(CURRENT_ALARM_STATE, ALARM_ARMED_AWAY_STATE) == 0) {
                print_boot_message("Armed", "Away");
            }
            else if (strcmp(CURRENT_ALARM_STATE, ALARM_ARMED_NIGHT_STATE) == 0) {
                print_boot_message("Armed", "Night");
            }
        }
    }

    else if ((PASSWORD_SET == 0) && ((PASSWORD_POSITION > 0) || ((RFID_UID_SET == 1) && (ACTION_CHOSEN == 0)))) {

        if ((RFID_UID_SET == 1) && (NOW - LAST_SCANNED_TIME >= RESET_WAIT_TIME)) {
            // * Reset if no input for 10 seconds
            reset_input();
        }
        else if ((PASSWORD_SET == 0) && (PASSWORD_POSITION > 0) && (NOW - LAST_KEYPAD_TIME >= RESET_WAIT_TIME))  {
            // * Reset if no input for 10 seconds
            reset_input();
        }
        print_password_prompt();
    }

    else if ((PASSWORD_SET == 1) && ((RFID_UID_SET == 0) && (ACTION_CHOSEN == 0))) {
        if (NOW - LAST_KEYPAD_TIME >= RESET_WAIT_TIME)  {
            // * Reset if no input for 10 seconds
            reset_input();
        }
        print_boot_message("Present", "Token");
    }

    else if ((PASSWORD_SET == 1) && (RFID_UID_SET == 1) && (ACTION_CHOSEN == 0)) {
        if ((NOW - LAST_SCANNED_TIME >= RESET_WAIT_TIME) || (NOW - LAST_KEYPAD_TIME >= RESET_WAIT_TIME)) {
            // * Reset if no input for 10 seconds
            reset_input();
        }
        print_please_make_your_choice();
    }

    if ((PASSWORD_SET == 1) && (RFID_UID_SET == 1) && (ACTION_CHOSEN == 1)) {
        Serial.println(F("Sending data to broker"));
        print_boot_message("Please", "Wait");

        char MQTT_OUT_MESSAGE[120];
        snprintf(MQTT_OUT_MESSAGE, sizeof(MQTT_OUT_MESSAGE), "{\"hostname\":\"%s\",\"uid\":\"%s\",\"code\":\"%s\",\"action\":%d}", HOSTNAME, LAST_SCANNED_UID, PASSWORD_GUESS, ACTION);
        send_mqtt_message(MQTT_RFID_CHANNEL, MQTT_OUT_MESSAGE);
        reset_input();
    }
}
