// **********************************
// * Settings                       *
// **********************************

// * Data pins
#define RFID_RST_PIN D3
#define RFID_SS_PIN  D4

#define OLED_RESET LED_BUILTIN  //4
#define BAUD_RATE 115200

#define MCP23017_I2C_ADDR 0x20

// * Hostname
#define HOSTNAME "alarmdisplay1.home"

// * The password used for uploading
#define OTA_PASSWORD "admin"

// * Wifi timeout in milliseconds
#define WIFI_TIMEOUT 30000

// * MQTT network settings
#define MQTT_MAX_PACKET_SIZE 512
#define MQTT_MAX_RECONNECT_TRIES 10

// * Watchdog timer
#define OSWATCH_RESET_TIME 300

// * Watchdog: Will be updated each loop
static unsigned long last_loop;

// * MQTT topic settings
char * MQTT_RFID_CHANNEL = (char*) "home/alarm/rfid";

// * General comms channel for all displays
char * MQTT_IN_WIDE      = (char*) "home/alarm/display";

// Solo topic will be home/alarm/display/<HOSTNAME>
char * MQTT_IN_SOLO      = (char*) "home/alarm/display/alarmdisplay1.home";

// * To be filled with EEPROM data
char MQTT_HOST[64]       = "";
char MQTT_PORT[6]        = "";
char MQTT_USER[32]       = "";
char MQTT_PASS[32]       = "";

// * MQTT Last reconnection counter
long LAST_RECONNECT_ATTEMPT     = 0;
const long MQTT_RECONNECT_DELAY = 5000;

// * Timer values for countdown on display
unsigned long DISPLAY_COUNTDOWN_TIMER   = 0;       // * Will store last time display counter was updated
const long DISPLAY_COUNTDOWN_INTERVAL   = 1000;    // * Interval at which to update the counter
int DISPLAY_COUNTDOWN_VALUE             = 0;       // * Will be set when countdown is triggered.

// * Available alarm states
char ALARM_PENDING_STATE[15]     = "pending";
char ALARM_TRIGGERED_STATE[15]   = "triggered";
char ALARM_ARMED_HOME_STATE[15]  = "armed_home";
char ALARM_ARMED_AWAY_STATE[15]  = "armed_away";
char ALARM_ARMED_NIGHT_STATE[15] = "armed_night";
char ALARM_DISARMED_STATE[15]    = "disarmed";

// * Will be updated with the current alarm state
char CURRENT_ALARM_STATE[15]     = "";

char* LAST_SCANNED_UID = (char*) "";             // * Will be set to the last uid scanned
const long LAST_SCANNED_DELAY    = 3000;         // * Anti bruteforce delay: ignore new cards for 10 seconds after scanning

// * Will be changed when the alarm state changes
bool DISPLAY_SHOULD_UPDATE          = true;         // * Will be set to true when display needs an update
unsigned long DISPLAY_LAST_UPDATE   = 0;            // * Will store the last time the display was updated
const long DISPLAY_UPDATE_FREQUENCY = 5000;         // * Update the display every 5 seconds

// * Buffer size for json input stream
const int BUFFER_SIZE = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 215;

// * Keypad Settings
const byte KEYPAD_ROWS = 4; // * Four rows
const byte KEYPAD_COLS = 3; // * Three columns

char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

// * Connect to the row pinouts of the keypad
byte KEYPAD_ROW_PINS[KEYPAD_ROWS] = {3, 4, 5, 6};

// * Connect to the column pinouts of the keypad
byte KEYPAD_COLUMN_PINS[KEYPAD_COLS] = {0, 1, 2};

// * Will be set to 1 when input completed
int PASSWORD_SET          = 0;
int RFID_UID_SET          = 0;
int ACTION_CHOSEN         = 0;

// *
#define MAX_PASSWORD_LENGTH 13

int ACTION                 = 0;

char STRING_TERMINATOR     = '\0';

const long RESET_WAIT_TIME = 10000;

char LAST_KEYPAD_PRESS = '0';

char PASSWORD_GUESS[MAX_PASSWORD_LENGTH];
int PASSWORD_POSITION = 0;
