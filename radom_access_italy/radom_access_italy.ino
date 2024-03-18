// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       radom_access_italy.ino
    Created:	23/2/2024 9:12:03 μμ
    Author:     ROUSISHOME\User
*/
#include <esp32fota.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPUI.h>
#include <EEPROM.h>

#define OPEN_HOT_SPOT 39
#define IN_BUTTON 36

#define EEP_WIFI_SSID 0
#define EEP_WIFI_PASS 32
#define EEP_USER_LOGIN 128
#define EEP_USER_PASS 160
#define EEP_SENSOR 196
#define EEP_DEFAULT_LOGIN 197
#define EEP_DEFAULT_WiFi 198
#define EEP_PERCENT_SELLECT 199
#define EEP_DURATION_SELLECT 200

const int buttonPin2 = 39;
const int ledPin =  13;
const int out1Pin =  15;
const int out2Pin =  2;
//==================================================================================
// GUI settings 
const String soft_version = "1.0.4";
const String soft_name = "Rousis Systems LTD - Random_Access Ver.: ";
//convert the soft name + soft_version to const char* soft_ID
//const char* soft_ID = (soft_name + soft_version + 0).c_str();
const char* soft_ID = "Rousis Systems LTD\nRandom_Access V.1.0.4";

//----------------------------------------------------------------------------------
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;

const char* ssid = "rousis";
const char* password = "rousis074520198";
const char* hostname = "espui";

uint8_t pass_count = 0;
uint8_t percent = 5;
uint8_t duration = 10;
uint8_t rundom_numpers[15] = { 0 };
long total_pass = 0;
uint16_t total_open = 0;
long led_timeout = 0;

//SSet the button to be active low
//#include <JC_Button.h>              // https://github.com/JChristensen/JC_Button
// pin assignments
   // define the buttons

//==================================================================================
// GUI settings
uint16_t percent_select, duration_select;

uint16_t password_text, user_text;
uint16_t wifi_ssid_text, wifi_pass_text;
uint8_t Default_login;

uint16_t status;
uint16_t WiFiStatus;

String stored_user, stored_password;
String stored_ssid, stored_pass;

void enterWifiDetailsCallback(Control* sender, int type) {
    if (type == B_UP) {
        if (sender->label == "WiFi")
        {
            Serial.println();
            Serial.print("Saving credentials to EPROM: ");
            Serial.print(ESPUI.getControl(wifi_ssid_text)->value);
            Serial.print(" - Size: ");
            Serial.println(sizeof(ESPUI.getControl(wifi_ssid_text)->value));
            /* Serial.println(ESPUI.getControl(wifi_pass_text)->value); */
            writeString(EEP_WIFI_SSID, ESPUI.getControl(wifi_ssid_text)->value, 32);
            writeString(EEP_WIFI_PASS, ESPUI.getControl(wifi_pass_text)->value, 64);
            //EEPROM.commit();

            ESPUI.getControl(status)->value = "Saved credentials to EEPROM<br>SSID: "
                + read_String(EEP_WIFI_SSID, 32)
                + "<br>Password: " + read_String(EEP_WIFI_PASS, 32);
            ESPUI.updateControl(status);

            Serial.println(read_String(EEP_WIFI_SSID, 32));
            Serial.println(read_String(EEP_WIFI_PASS, 32));
        }
        else if (sender->label == "User") {
            Serial.println();
            Serial.println("Saving User to EPROM: ");
            /*Serial.println(ESPUI.getControl(user_text)->value);
            Serial.println(ESPUI.getControl(password_text)->value);*/
            writeString(EEP_USER_LOGIN, ESPUI.getControl(user_text)->value, sizeof(ESPUI.getControl(user_text)->value));
            writeString(EEP_USER_PASS, ESPUI.getControl(password_text)->value, sizeof(ESPUI.getControl(password_text)->value));
            //EEPROM.commit();

            ESPUI.getControl(status)->value = "Saved login details to EEPROM<br>User name: "
                + read_String(EEP_USER_LOGIN, 32)
                + "<br>Password: " + read_String(EEP_USER_PASS, 32);
            ESPUI.updateControl(status);
            Serial.println(read_String(EEP_USER_LOGIN, 32));
            Serial.println(read_String(EEP_USER_PASS, 32));
        }


        /*Serial.println("Reset..");
        ESP.restart();*/
    }
}

void writeString(char add, String data, uint8_t length)
{
    int _size = length; // data.length();
    int i;
    for (i = 0; i < _size; i++)
    {
        if (data[i] == 0) { break; }
        EEPROM.write(add + i, data[i]);
    }
    EEPROM.write(add + i, '\0');   //Add termination null character for String Data
    EEPROM.commit();
}

String read_String(char add, uint8_t length)
{
    int i;
    char data[100]; //Max 100 Bytes
    int len = 0;
    unsigned char k;
    k = EEPROM.read(add);
    while (k != '\0' && len < length)   //Read until null character
    {
        k = EEPROM.read(add + len);
        data[len] = k;
        len++;
    }
    data[len] = '\0';
    return String(data);
}

void textCallback(Control* sender, int type) {
    //This callback is needed to handle the changed values, even though it doesn't do anything itself.
}

void ResetCallback(Control* sender, int type) {
    if (type == B_UP) {
        Serial.println("Reset..");
        ESP.restart();
    }
}

void generalCallback(Control* sender, int type) {
    Serial.print("CB: id(");
    Serial.print(sender->id);
    Serial.print(") Type(");
    Serial.print(type);
    Serial.print(") '");
    Serial.print(sender->label);
    Serial.print("' = ");
    Serial.println(sender->value);
    
    if (sender->label == "Percent %")
    {
        percent = sender->value.toInt();
		EEPROM.write(EEP_PERCENT_SELLECT, percent);
		EEPROM.commit();
	}
	else if (sender->label == "Duration Out")
	{
		duration = sender->value.toInt();
		EEPROM.write(EEP_DURATION_SELLECT, duration);
		EEPROM.commit();
    }
    
}

void ReadWiFiCrententials(void) {
    //uint8_t DefaultWiFi = EEPROM.read(EEP_DEFAULT_WiFi);
    if (Default_login || digitalRead(OPEN_HOT_SPOT) == 0) { ///testing to default
        Serial.println("Load default WiFi Crententials..");
        unsigned int i;
        stored_ssid = "Zyxel_816E51"; // "rousis";
        stored_pass = "8GJ4B3GP"; // "rousis074520198";
        writeString(EEP_WIFI_SSID, stored_ssid, sizeof(stored_ssid));
        writeString(EEP_WIFI_PASS, stored_pass, sizeof(stored_pass));
        stored_user = "espboard";
        stored_password = "12345678";
        writeString(EEP_USER_LOGIN, stored_user, sizeof(stored_user));
        writeString(EEP_USER_PASS, stored_password, sizeof(stored_password));

        EEPROM.write(EEP_DEFAULT_WiFi, 0); //Erase the EEP_DEFAULT_WiFi
        EEPROM.write(EEP_DEFAULT_LOGIN, 0);
        EEPROM.commit();
    }
    else {
        stored_ssid = read_String(EEP_WIFI_SSID, 32);
        stored_pass = read_String(EEP_WIFI_PASS, 32);
    }
}

void Readuserdetails(void) {
    stored_user = read_String(EEP_USER_LOGIN, 32);
    stored_password = read_String(EEP_USER_PASS, 32);

    Serial.println();
    Serial.print("stored_user: ");
    Serial.println(stored_user);
    Serial.print("stored_password: ");
    Serial.println(stored_password);
    /*readStringFromEEPROM(stored_user, 128, 32);
    readStringFromEEPROM(stored_password, 160, 32);*/
}

esp32FOTA esp32FOTA("esp32-fota-http", soft_version, false);
const char* manifest_url = "http://smart-q.eu/fota/radom_access_italy.json";

void setup()
{
    EEPROM.begin(255);
    Serial.begin(115200);
    pinMode(OPEN_HOT_SPOT, INPUT_PULLUP);
    pinMode(IN_BUTTON, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);
    pinMode(out1Pin, OUTPUT);
	pinMode(out2Pin, OUTPUT);

    delay(100);
    Serial.println("Start..");

    esp32FOTA.setManifestURL(manifest_url);
    esp32FOTA.printConfig();

    //-----------------------------------------------------------------------
  // GUI interface setup
    WiFi.setHostname(hostname);
    ReadWiFiCrententials();

    Serial.println("Stored SSID:");
    Serial.println(stored_ssid);
    Serial.println("Stored password:");
    Serial.println(stored_pass);
    Serial.println(".....................");
    /*Serial.println("Stored User:");
    Serial.println(stored_user);
    Serial.println("Stored User pass:");
    Serial.println(stored_password);*/
    // try to connect to existing network
    WiFi.mode(WIFI_AP_STA);

    WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
    //WiFi.begin(ssid, password);
    Serial.print("\n\nTry to connect to existing network");

    uint8_t timeout = 10;

    // Wait for connection, 5s timeout
    do {
        delay(500);
        Serial.print(".");
        timeout--;
    } while (timeout && WiFi.status() != WL_CONNECTED);

    // not connected -> create hotspot
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("\n\nCreating hotspot");

        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP(ssid);
        timeout = 5;

        do {
            delay(500);
            Serial.print(".");
            timeout--;
        } while (timeout);
    }

    //------------------------------------------------------------------------

    dnsServer.start(DNS_PORT, "*", apIP);

    Serial.println("\n\nWiFi parameters:");
    Serial.print("Mode: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? "Station" : "Client");
    Serial.print("IP address: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());

    Serial.println("WiFi mode:");
    Serial.println(WiFi.getMode());
    //---------------------------------------------------------------------------------------------
    //We need this CSS style rule, which will remove the label's background and ensure that it takes up the entire width of the panel
    String clearLabelStyle = "background-color: unset; width: 100%;";

    uint16_t tab1 = ESPUI.addControl(ControlType::Tab, "Main Display", "Main Display");
    uint16_t tab2 = ESPUI.addControl(ControlType::Tab, "User Settings", "User");
    uint16_t tab3 = ESPUI.addControl(ControlType::Tab, "WiFi Settings", "WiFi");

    percent = EEPROM.read(EEP_PERCENT_SELLECT);
    duration = EEPROM.read(EEP_DURATION_SELLECT);

    if (percent > 15)
        percent = 15;
    if (percent < 2)
        percent = 2;
    percent_select = ESPUI.addControl(Number, "Percent %", String(percent), Peterriver, tab1, generalCallback);
    ESPUI.addControl(Min, "", "2", None, percent_select);
    ESPUI.addControl(Max, "", "15", None, percent_select);

    if (duration > 50)
		duration = 50;
    if (duration < 5)
        duration = 5;
    duration_select = ESPUI.addControl(Number, "Duration Out", String(duration), Peterriver, tab1, generalCallback);
    
    ESPUI.addControl(Min, "", "5", None, duration_select);
    ESPUI.addControl(Max, "", "50", None, duration_select);

    Readuserdetails();
    user_text = ESPUI.addControl(Text, "User name", stored_user, Alizarin, tab2, textCallback); // stored_user
    ESPUI.addControl(Max, "", "32", None, user_text);
    password_text = ESPUI.addControl(Text, "Password", stored_password, Alizarin, tab2, textCallback); //stored_password
    ESPUI.addControl(Max, "", "32", None, password_text);
    ESPUI.setInputType(password_text, "password");
    ESPUI.addControl(Button, "User", "Save", Peterriver, tab2, enterWifiDetailsCallback);

    wifi_ssid_text = ESPUI.addControl(Text, "SSID", stored_ssid, Alizarin, tab3, textCallback); // stored_ssid
    ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
    wifi_pass_text = ESPUI.addControl(Text, "Password", stored_pass, Alizarin, tab3, textCallback); //stored_pass
    ESPUI.addControl(Max, "", "64", None, wifi_pass_text);
    ESPUI.addControl(Button, "WiFi", "Save", Peterriver, tab3, enterWifiDetailsCallback);

    ESPUI.addControl(ControlType::Button, "Reset device", "Reset", ControlColor::Emerald, tab3, &ResetCallback);

    //WiFiStatus = ESPUI.addControl(Label, "WiFi Status", "Wifi Status ok", ControlColor::None, tab3);

    status = ESPUI.addControl(Label, "Status", "System status: OK", Wetasphalt, Control::noParent);
    ESPUI.getControl(status)->value = "Status: = ";
    ESPUI.addControl(Separator, soft_ID, "", Peterriver, tab1);

    ESPUI.begin("- Accces Control", stored_user.c_str(), stored_password.c_str()); //"espboard", 

    //Status update
    ESPUI.getControl(status)->value = "Total open: " + String(total_open) + "\nTotal pass: " + String(total_pass);
    ESPUI.updateControl(status);


    digitalWrite(out1Pin, LOW);
    digitalWrite(out2Pin, LOW);
    random_access();
}

// Add the main program code into the continuous loop() function
void loop()
{
    dnsServer.processNextRequest();
    esp32FOTA.handle();
    
    //check if IN_BUTTON pin gets LOW
    if (digitalRead(IN_BUTTON) == LOW) {
        //delay 100ms to debounce
        delay(80); if (digitalRead(IN_BUTTON) == LOW) {
            //increment the pass_count
            pass_count++; total_pass++;
            if (pass_count > 100)
            {
                pass_count = 1;
                random_access();
            }
            //Check if the pass_count is in the rundom_numpers array
            // if it is, open the door
            // if not, do nothing
            for (int i = 0; i < percent; i++) {
                if (pass_count == rundom_numpers[i]) {
                    total_open++;
                    //open the door
                    digitalWrite(out1Pin, HIGH);
                    digitalWrite(out2Pin, HIGH);
                    delay(duration * 1000);
                    digitalWrite(out1Pin, LOW);
                    digitalWrite(out2Pin, LOW);
                    break;
                }
            }
            //Status update
            ESPUI.getControl(status)->value = "Total open: " + String(total_open) + "\nTotal pass: " + String(total_pass);
            ESPUI.updateControl(status);
            //loop while button is pressed
            while (digitalRead(IN_BUTTON) == LOW) {
                delay(50);
            }
            delay(10);
        }
    }

    //flash the LED ledPin every 1000ms
    if (millis() - led_timeout > 500) {
		led_timeout = millis();
		//toggle the LED
        digitalWrite(ledPin, !digitalRead(ledPin));
	}
}

// Make a funcion to choose random numbers while the pass_count it counts until 100 depending on the percent_select
void random_access(void) {
	// clear the rundom_numpers array
    for (int i = 0; i < percent; i++) {
		rundom_numpers[i] = 0;
	}    
    // find rundom unique numbers as many as the percent_select and store them in the rundom_numpers array
    for (int i = 0; i < percent; i++) {
		int temp = random(1, 100);
		for (int j = 0; j < i; j++) {
			if (temp == rundom_numpers[j]) {
				temp = random(1, 100);
				j = -1;
			}
		}
		rundom_numpers[i] = temp;
	}

    // print the rundom_numpers array
    Serial.println();
    Serial.println("The new rundom_numpers");
    for (int i = 0; i < percent; i++) {
        Serial.print(rundom_numpers[i]);
        Serial.print(" ");        
    }
    
    Serial.println();
    Serial.println("-----------------------------");
}