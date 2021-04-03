#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <DateTime.h>
#include <Ticker.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Labas.h>

#define FORMAT_SPIFFS_IF_FAILED true
#define TIMEZONE 2

#define TEMP_PIN 4

#define PREFS_KEY "katiline"

#define LOG_PATH "/katiline.log"
#define TEMP_LOG_PATH "/temperaturos_istorija.csv"

#define TEMP_TRIGGER_KEY "temp-trigger"
#define TEMP_RESET_KEY "temp-reset"
#define PHONES_KEY "phones"
#define STATE_KEY "state"

#define LIMIT_TODAY_KEY "today"
#define LIMIT_TODAY_TRIGGERED_KEY "triggers-tdy"

static String limit(String string, int n);
static String limitEnd(String string, int n);
static const float clamp(float value, float min, float max);
static void writeFile(String path, String contents);
static String readFile(String path);
static void log(String message);
static String formatTemp(float temp, const String &nullReturn = "--.-", const char &separator = '.');
static void logTemp();
static void changeState(States newState, String caller = String());
static bool incrementSMSCounter();
static void triggerState();
static void resetState();
static void checkTemp();
static void readTemp();
static String trimPhones(String string);
static String statusProcessor(const String &var);
static String templateProcessor(const String &var);
static void handlePost(AsyncWebServerRequest *request);
static void restart();

const static int logSize = 50;
const static int tempLogSize = 500;

const static int smsDayLimit = 10;

const static int tempPollingMS = 60000;  //  1 min
const static int tempLoggingMS = 600000; // 10 mins

const static int restartAfterMS = 432000000; // 5 days

const static float maxTemp = 150;
const static float minTemp = 0;

const static int labasAttempts = 5;

const char *wifiSSID = "WIFI_SSID";
const char *wifiPassword = "WIFI_PASSWORD";

const char *labasUsername = "LABAS_USERNAME";
const char *labasPassword = "LABAS_PASSWORD";

const char *editPassword = "EDIT_PASSWORD";

enum States
{
    Triggered = 0,
    Active = 1,
    Stopped = 2
};

States state;

Preferences prefs;
float temp = DEVICE_DISCONNECTED_C;
float tempTrigger;
float tempReset;
String phones;
String Log;

Labas labas;
AsyncWebServer server(80);
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

static String limit(String string, int n)
{
    unsigned int len = string.length();
    int j = len - 1;

    for (int i = 0; i <= n; i++)
    {
        j = string.lastIndexOf('\n', j) - 1;

        if (j < -1)
        {
            return string;
        }
    }

    return string.substring(j + 2, len);
}

static String limitEnd(String string, int n)
{
    unsigned int len = string.length();
    int j = 0;

    for (int i = 0; i < n; i++)
    {
        j = string.indexOf('\n', j) + 1;

        if (j < 0)
        {
            return string;
        }

        if (j >= len)
        {
            break;
        }
    }

    return string.substring(0, j);
}

static const float clamp(float value, float min, float max)
{
    return value < min ? min : value > max ? max
                                           : value;
}

static void writeFile(String path, String contents)
{
    File file = SPIFFS.open(path, FILE_WRITE);

    if (!file)
    {
        Serial.print(path);
        Serial.println(": failed to open");
        return;
    }

    if (!file.print(contents))
    {
        Serial.print(path);
        Serial.println(": failed to write");
    }
}

static String readFile(String path)
{
    File file = SPIFFS.open(path, FILE_READ);

    if (!file || file.isDirectory())
    {
        Serial.print(path);
        Serial.println(": failed to open");
        return "";
    }

    return file.readString();
}

static void log(String message)
{
    String str = DateTime.toString() + ' ' + message + '\n';
    Serial.print(str);

    Log = str + Log;
    Log = limitEnd(Log, logSize);

    writeFile(LOG_PATH, Log); // TODO: use appendFile...
}

static String formatTemp(float temp, const String &nullReturn = "--.-", const char &separator = '.')
{
    if (temp == DEVICE_DISCONNECTED_C)
        return nullReturn;

    String str = String(temp, 1);

    if (separator != '.')
        str.replace('.', separator);

    return str;
}

static void logTemp()
{
    if (temp == DEVICE_DISCONNECTED_C)
        return;

    String str = '\"' + DateTime.format("%F %H:%M") + "\"," + formatTemp(temp, "") + "\n";
    String temps = readFile(TEMP_LOG_PATH);
    temps = limit(temps + str, tempLogSize);

    writeFile(TEMP_LOG_PATH, temps);
}

static void changeState(States newState, String caller = String())
{
    if (state == newState)
    {
        return;
    }

    switch (newState)
    {
    case States::Triggered:
        log(caller + "pranešta");
        break;

    case States::Active:
        log(caller + "aktyvūs");
        break;

    case States::Stopped:
        log(caller + "sustabdyti");
        break;

    default:
        log(caller + "klaida");
        return;
    }

    state = newState;
}

static bool incrementSMSCounter()
{
    String todayPf = prefs.getString(LIMIT_TODAY_KEY);
    String today = DateTime.format(DateFormatter::DATE_ONLY);
    int tr = 0;

    if (todayPf == today)
        tr = prefs.getInt(LIMIT_TODAY_TRIGGERED_KEY);
    else
        prefs.putString(LIMIT_TODAY_KEY, today);

    prefs.putInt(LIMIT_TODAY_TRIGGERED_KEY, tr + 1);

    return tr < smsDayLimit;
}

static void triggerState()
{
    bool successful = false;
    for (int i = 0; !successful && i < labasAttempts; i++)
        successful = labas.login();

    if (!successful)
    {
        Serial.println("labas: failed to login");
        return;
    }

    changeState(States::Triggered, "temp " + formatTemp(temp) + "C: ");

    if (!incrementSMSCounter())
    {
        changeState(States::Stopped, "limitas: ");
    }

    String phone;
    String message = "Katilinės temperatūra nukrito iki " + formatTemp(temp, "--,-", ',') + "°C.";

    for (int i = 0; i >= 0; i = phones.indexOf('+', i))
    {
        phone = phones.substring(i, i + 12);
        i++;

        if (phone.length() == 12)
        {
            successful = false;
            for (int j = 0; !successful && j < labasAttempts; j++)
                successful = labas.sendSMS(phone, message);

            if (!successful)
                Serial.println("labas: failed to send sms");
        }
    }
}

static void resetState()
{
    changeState(States::Active, "temp " + formatTemp(temp) + "C: ");
}

static void checkTemp()
{
    if (temp == DEVICE_DISCONNECTED_C)
        return;

    if (state == States::Active && temp < tempTrigger)
    {
        triggerState();
    }
    else if (state == States::Triggered && temp > tempReset)
    {
        resetState();
    }
}

static void readTemp()
{
    sensors.requestTemperatures();
    float reading = sensors.getTempCByIndex(0);
    if (reading != DEVICE_DISCONNECTED_C)
    {
        temp = reading;
        checkTemp();
    }
}

static String trimPhones(String string)
{
    String str, phone;
    int j = 0;

    for (int i = 0; i < 10; i++)
    {
        j = string.indexOf('+', j) + 1;

        if (j <= 0)
            break;

        phone = string.substring(j - 1, j + 11);

        if (phone.length() != 12)
            continue;

        str += phone + "; ";
    }

    return str;
}

static String statusProcessor(const String &var)
{
    if (var == "TEMP")
        return formatTemp(temp, "null");

    if (var == "TEMP_TRIGGER")
        return formatTemp(tempTrigger, "null");

    if (var == "TEMP_RESET")
        return formatTemp(tempReset, "null");

    if (var == "STATE")
    {
        if (state == States::Triggered)
            return "\"triggered\"";

        if (state == States::Active)
            return "\"active\"";

        if (state == States::Stopped)
            return "\"stopped\"";

        return "null";
    }

    if (var == "DATETIME")
        return '"' + DateTime.toString() + '"';

    if (var == "HEAP_FREE")
        return String(ESP.getFreeHeap());

    if (var == "HEAP_SIZE")
        return String(ESP.getHeapSize());

    if (var == "HEAP_PERCENTAGE")
        return String((float)ESP.getFreeHeap() / (float)ESP.getHeapSize() * (float)100, 1);

    if (var == "SPIFFS_USED")
        return String(SPIFFS.usedBytes());

    if (var == "SPIFFS_TOTAL")
        return String(SPIFFS.totalBytes());

    if (var == "SPIFFS_PERCENTAGE")
        return String((float)SPIFFS.usedBytes() / (float)SPIFFS.totalBytes() * (float)100, 1);

    return "null";
}

static String templateProcessor(const String &var)
{
    if (var == "TEMP")
        return formatTemp(temp, "--,-", ',');

    if (var == "STATE_TRIGGERED")
        return state == States::Triggered ? "selected" : "";

    if (var == "STATE_ACTIVE")
        return state == States::Active ? "selected" : "";

    if (var == "STATE_STOPPED")
        return state == States::Stopped ? "selected" : "";

    if (var == "TEMP_TRIGGER")
        return formatTemp(tempTrigger, "0.0");

    if (var == "TEMP_RESET")
        return formatTemp(tempReset, "0.0");

    if (var == "PHONES")
        return phones;

    if (var == "ALERT")
        return "hidden";

    if (var == "LOG")
        return Log.substring(0, Log.length() - 1);

    if (var == "TIME")
        return DateTime.format(DateFormatter::TIME_ONLY);

    return "";
}

static void handlePost(AsyncWebServerRequest *request)
{
    String alert = "alert-danger";
    String alertMsg = "Klaidingi duomenys.";

    if (request->hasParam("password", true))
    {
        AsyncWebParameter *pass = request->getParam("password", true);

        if (pass->value() == editPassword)
        {
            if (request->hasParam(TEMP_TRIGGER_KEY, true))
            {
                AsyncWebParameter *p = request->getParam(TEMP_TRIGGER_KEY, true);
                tempTrigger = clamp(p->value().toFloat(), minTemp, maxTemp);
                prefs.putFloat(TEMP_TRIGGER_KEY, tempTrigger);
            }

            if (request->hasParam(TEMP_RESET_KEY, true))
            {
                AsyncWebParameter *p = request->getParam(TEMP_RESET_KEY, true);
                tempReset = clamp(p->value().toFloat(), tempTrigger, maxTemp);
                prefs.putFloat(TEMP_RESET_KEY, tempReset);
            }

            if (request->hasParam(PHONES_KEY, true))
            {
                AsyncWebParameter *p = request->getParam(PHONES_KEY, true);
                phones = trimPhones(p->value());
                prefs.putString(PHONES_KEY, phones);
            }

            if (request->hasParam(STATE_KEY, true))
            {
                AsyncWebParameter *p = request->getParam(STATE_KEY, true);
                States newState = (States)p->value().toInt();
                changeState(newState, "nustatymai: ");
            }

            alert = "alert-success";
            alertMsg = "Pakeitimai išsaugoti.";
        }
        else
        {
            alertMsg = "Klaidingas slaptažodis.";
        }
    }

    request->send(SPIFFS, "/nustatymai.min.html", "text/html", false, [alert, alertMsg, request](const String &var) {
        if (var == "ALERT")
            return String();

        if (var == "ALERT_TYPE")
            return alert;

        if (var == "ALERT_MSG")
            return alertMsg;

        if (alert == "alert-danger")
        {
            if (var == "TEMP_TRIGGER" && request->hasParam(TEMP_TRIGGER_KEY, true))
                return request->getParam(TEMP_TRIGGER_KEY, true)->value();

            if (var == "TEMP_RESET" && request->hasParam(TEMP_RESET_KEY, true))
                return request->getParam(TEMP_RESET_KEY, true)->value();

            if (var == "PHONES" && request->hasParam(PHONES_KEY, true))
                return request->getParam(PHONES_KEY, true)->value();

            if (var == "STATE_TRIGGERED" && request->getParam(STATE_KEY, true))
                return String((States)request->getParam(STATE_KEY, true)->value().toInt() == States::Triggered ? "selected" : "");

            if (var == "STATE_ACTIVE" && request->getParam(STATE_KEY, true))
                return String((States)request->getParam(STATE_KEY, true)->value().toInt() == States::Active ? "selected" : "");

            if (var == "STATE_STOPPED" && request->getParam(STATE_KEY, true))
                return String((States)request->getParam(STATE_KEY, true)->value().toInt() == States::Stopped ? "selected" : "");
        }

        return templateProcessor(var);
    });
}

static void restart()
{
    ESP.restart();
}

Ticker tempTicker(readTemp, tempPollingMS, 0, MILLIS);
Ticker tempLogTicker(logTemp, tempLoggingMS, 0, MILLIS);
Ticker restartTicker(restart, restartAfterMS, 1, MILLIS);

void setup()
{
    Serial.begin(115200);

    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println("Failed to mount SPIFFS.");
        return;
    }

    if (!SPIFFS.exists(TEMP_LOG_PATH))
        SPIFFS.open(TEMP_LOG_PATH, FILE_WRITE);

    if (!SPIFFS.exists(LOG_PATH))
        SPIFFS.open(LOG_PATH, FILE_WRITE);

    if (!prefs.begin(PREFS_KEY))
    {
        Serial.println("Failed to load preferences.");
        return;
    }
    tempTrigger = prefs.getFloat(TEMP_TRIGGER_KEY, 60);
    tempReset = prefs.getFloat(TEMP_RESET_KEY, 66);
    phones = prefs.getString(PHONES_KEY);
    Log = readFile(LOG_PATH);
    state = States::Triggered;

    // Network
    IPAddress local_ip(192, 168, 10, 54);
    IPAddress gateway(192, 168, 10, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns1(1, 1, 1, 2);
    IPAddress dns2(1, 0, 0, 2);

    if (!WiFi.config(local_ip, gateway, subnet, dns1, dns2))
        Serial.println("Failed to configure Static IP.");

    Serial.print("Connecting to ");
    Serial.print(wifiSSID);

    WiFi.begin(wifiSSID, wifiPassword);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(1000);
    }

    Serial.print("\nConnected: ");
    Serial.println(WiFi.localIP());

    // Time
    Serial.print("Getting time");
    DateTime.setTimeZone(TIMEZONE);
    while (!DateTime.begin())
    {
        Serial.print('.');
        delay(1000);
    }
    Serial.println();

    // Labas
    labas = Labas(labasUsername, labasPassword);

    // Routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.min.html", "text/html", false, templateProcessor);
    });

    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.min.html", "text/html", false, templateProcessor);
    });

    server.on("/nustatymai.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/nustatymai.min.html", "text/html", false, templateProcessor);
    });

    server.on("/nustatymai.html", HTTP_POST, handlePost);

    server.on(TEMP_LOG_PATH, HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, TEMP_LOG_PATH, "text/csv", true);
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/status.min.json", "application/json", false, statusProcessor);
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/favicon.ico", "image/x-icon");
    });

    server.on("/site.webmanifest", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/site.webmanifest", "application/manifest+json");
    });

    for (const char *png : {
             "/android-chrome-192x192.png",
             "/android-chrome-512x512.png",
             "/apple-touch-icon.png",
             "/favicon-16x16.png",
             "/favicon-32x32.png"})
    {
        server.on(png, HTTP_GET, [png](AsyncWebServerRequest *request) {
            request->send(SPIFFS, png, "image/png");
        });
    }

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/404.min.html", "text/html");
    });

    log("esp32: įsijungė");
    sensors.begin();
    server.begin();

    tempTicker.start();
    tempLogTicker.start();
    restartTicker.start();
}

void loop()
{
    tempTicker.update();
    tempLogTicker.update();
    restartTicker.update();
}
