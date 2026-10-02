#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_wifi.h>

// ============================================================
//                     ESP32-C3 CHEATBOX
// ============================================================

// ============================================================
// PIN CONFIGURATION
// ============================================================

#define BTN_UP      8
#define BTN_DOWN    9
#define BTN_ENTER   10

#define I2C_SDA     6
#define I2C_SCL     7

// ============================================================
// OLED CONFIGURATION
// ============================================================

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   32
#define OLED_ADDR       0x3C
#define OLED_RESET      -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ============================================================
// WIFI CONFIGURATION
// ============================================================

const char* AP_SSID = "ESP_Reader_C3";
const char* AP_PASS = "12345678";

IPAddress apIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// ============================================================
// APPLICATION STATES
// ============================================================

enum AppState {
  MAIN_MENU,
  FILE_BROWSER,
  FILE_VIEWER,
  UPLOAD_MODE
};

AppState currentState = MAIN_MENU;
AppState previousState = MAIN_MENU;

// ============================================================
// FILE SYSTEM
// ============================================================

#define MAX_FILES 20

String fileList[MAX_FILES];
int fileCount = 0;

// ============================================================
// FILE VIEWER
// ============================================================

String currentFileContent = "";

int totalViewerLines = 0;
int viewerScrollLine = 0;

// OLED 128x32, text size 1
// Approximately 21 characters per line
#define OLED_CHARS_PER_LINE 21
#define OLED_VISIBLE_LINES 4

// ============================================================
// MENU
// ============================================================

int menuIndex = 0;
int listScrollOffset = 0;

// ============================================================
// BUTTON HANDLING
// ============================================================

bool lastUpState = HIGH;
bool lastDownState = HIGH;
bool lastEnterState = HIGH;

unsigned long upPressStart = 0;

bool upHeld = false;
bool ignoreNextUpClick = false;

#define DEBOUNCE_DELAY       60
#define LONG_PRESS_TIME      1500

// ============================================================
// SCREEN
// ============================================================

bool screenOff = false;
bool screenDirty = true;

// ============================================================
// WIFI STATE
// ============================================================

bool wifiActive = false;

// ============================================================
// FILE UPLOAD
// ============================================================

File uploadFile;
String uploadFilename = "";

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================

void drawMainMenu();
void drawFileBrowser();
void drawFileViewer();
void drawUploadScreen();

void loadFiles();

int countWrappedLines(const String& text);
String getWrappedLine(const String& text, int requestedLine);

void handleButtons();
void handleApplication();

void startWiFiAP();
void stopWiFiAP();

void setupWebServer();

bool isSafeFilename(String filename);
String normalizeFilename(String filename);

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("       CHEATBOX ESP32-C3");
  Serial.println("================================");

  // ----------------------------------------------------------
  // BUTTONS
  // ----------------------------------------------------------

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);

  Serial.println("Buttons initialized");
  Serial.println("UP    : GPIO 8");
  Serial.println("DOWN  : GPIO 9");
  Serial.println("ENTER : GPIO 10");

  // ----------------------------------------------------------
  // I2C
  // ----------------------------------------------------------

  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("I2C initialized");
  Serial.println("SDA: GPIO 6");
  Serial.println("SCL: GPIO 7");

  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  Serial.println("Starting OLED...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {

    Serial.println("ERROR: OLED initialization failed!");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("OLED OK");

  display.setRotation(0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0, 0);

  display.println("CheatBox v3");
  display.println("ESP32-C3");
  display.println("Starting...");

  display.display();

  delay(1000);

  // ----------------------------------------------------------
  // LITTLEFS
  // ----------------------------------------------------------

  Serial.println("Mounting LittleFS...");

  if (!LittleFS.begin(false)) {

    Serial.println("LittleFS mount failed.");
    Serial.println("Formatting LittleFS...");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("LittleFS Error");
    display.println("Formatting...");
    display.display();

    delay(500);

    if (!LittleFS.begin(true)) {

      Serial.println("LittleFS formatting failed!");

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("LittleFS FAILED");
      display.display();

      while (true) {
        delay(1000);
      }
    }
  }

  Serial.println("LittleFS OK");

  // ----------------------------------------------------------
  // WEB SERVER
  // ----------------------------------------------------------

  setupWebServer();

  // ----------------------------------------------------------
  // INITIAL UI
  // ----------------------------------------------------------

  currentState = MAIN_MENU;
  previousState = MAIN_MENU;

  screenDirty = true;

  Serial.println();
  Serial.println("================================");
  Serial.println("READY");
  Serial.println("================================");
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // BUTTONS
  // ----------------------------------------------------------

  handleButtons();

  // ----------------------------------------------------------
  // APPLICATION STATE
  // ----------------------------------------------------------

  handleApplication();

  // ----------------------------------------------------------
  // WEB SERVER
  // ----------------------------------------------------------

  if (wifiActive) {
    server.handleClient();
  }

  // ----------------------------------------------------------
  // UI
  // ----------------------------------------------------------

  if (screenDirty) {

    if (!screenOff) {

      switch (currentState) {

        case MAIN_MENU:
          drawMainMenu();
          break;

        case FILE_BROWSER:
          drawFileBrowser();
          break;

        case FILE_VIEWER:
          drawFileViewer();
          break;

        case UPLOAD_MODE:
          drawUploadScreen();
          break;
      }
    }

    screenDirty = false;
  }

  // ----------------------------------------------------------
  // SMALL DELAY
  // ----------------------------------------------------------

  delay(10);
}

// ============================================================
// BUTTON HANDLING
// ============================================================

void handleButtons() {

  int up = digitalRead(BTN_UP);
  int down = digitalRead(BTN_DOWN);
  int enter = digitalRead(BTN_ENTER);

  // ==========================================================
  // SCREEN OFF MODE
  // ==========================================================

  if (screenOff) {

    if (down == LOW || enter == LOW) {

      screenOff = false;

      display.ssd1306_command(SSD1306_DISPLAYON);

      screenDirty = true;

      delay(200);
    }

    lastUpState = up;
    lastDownState = down;
    lastEnterState = enter;

    return;
  }

  // ==========================================================
  // UP BUTTON LONG PRESS
  // ==========================================================

  if (up == LOW && lastUpState == HIGH) {

    upPressStart = millis();

    upHeld = true;
  }

  if (up == LOW && upHeld) {

    if (millis() - upPressStart >= LONG_PRESS_TIME) {

      Serial.println("OLED OFF");

      screenOff = true;

      display.clearDisplay();
      display.display();

      display.ssd1306_command(SSD1306_DISPLAYOFF);

      upHeld = false;

      ignoreNextUpClick = true;
    }
  }

  if (up == HIGH && lastUpState == LOW) {

    upHeld = false;
  }

  // ==========================================================
  // UP SHORT PRESS
  // ==========================================================

  if (lastUpState == LOW &&
      up == HIGH &&
      !ignoreNextUpClick) {

    if (currentState == MAIN_MENU) {

      if (menuIndex > 0) {
        menuIndex--;
      }
    }

    else if (currentState == FILE_BROWSER) {

      if (menuIndex > 0) {
        menuIndex--;
      }
    }

    else if (currentState == FILE_VIEWER) {

      if (viewerScrollLine > 0) {
        viewerScrollLine--;
      }
    }

    screenDirty = true;
  }

  if (up == HIGH) {
    ignoreNextUpClick = false;
  }

  // ==========================================================
  // DOWN BUTTON
  // ==========================================================

  if (lastDownState == HIGH && down == LOW) {

    delay(DEBOUNCE_DELAY);

    if (digitalRead(BTN_DOWN) == LOW) {

      if (currentState == MAIN_MENU) {

        if (menuIndex < 1) {
          menuIndex++;
        }
      }

      else if (currentState == FILE_BROWSER) {

        if (menuIndex < fileCount - 1) {
          menuIndex++;
        }
      }

      else if (currentState == FILE_VIEWER) {

        if (viewerScrollLine < totalViewerLines - OLED_VISIBLE_LINES) {

          viewerScrollLine++;
        }
      }

      screenDirty = true;
    }
  }

  // ==========================================================
  // ENTER BUTTON
  // ==========================================================

  if (lastEnterState == HIGH && enter == LOW) {

    delay(DEBOUNCE_DELAY);

    if (digitalRead(BTN_ENTER) == LOW) {

      // ------------------------------------------------------
      // MAIN MENU
      // ------------------------------------------------------

      if (currentState == MAIN_MENU) {

        // READ FILES
        if (menuIndex == 0) {

          Serial.println("Opening file browser");

          loadFiles();

          menuIndex = 0;
          listScrollOffset = 0;

          currentState = FILE_BROWSER;

          screenDirty = true;
        }

        // WIFI UPLOAD
        else {

          Serial.println("Entering WiFi upload mode");

          currentState = UPLOAD_MODE;

          screenDirty = true;
        }
      }

      // ------------------------------------------------------
      // FILE BROWSER
      // ------------------------------------------------------

      else if (currentState == FILE_BROWSER) {

        // BACK
        if (menuIndex == 0) {

          currentState = MAIN_MENU;

          menuIndex = 0;
          listScrollOffset = 0;

          screenDirty = true;
        }

        // FILE
        else {

          String selectedFile = fileList[menuIndex];

          Serial.print("Opening: ");
          Serial.println(selectedFile);

          File f = LittleFS.open(selectedFile, "r");

          if (f) {

            currentFileContent = f.readString();

            f.close();

            totalViewerLines =
              countWrappedLines(currentFileContent);

            viewerScrollLine = 0;

            currentState = FILE_VIEWER;

            screenDirty = true;

            Serial.println("File opened");
          }

          else {

            Serial.println("Failed to open file");

            currentState = FILE_BROWSER;

            screenDirty = true;
          }
        }
      }

      // ------------------------------------------------------
      // FILE VIEWER
      // ------------------------------------------------------

      else if (currentState == FILE_VIEWER) {

        Serial.println("Closing file viewer");

        currentFileContent = "";
        totalViewerLines = 0;
        viewerScrollLine = 0;

        currentState = FILE_BROWSER;

        screenDirty = true;
      }

      // ------------------------------------------------------
      // UPLOAD MODE
      // ------------------------------------------------------

      else if (currentState == UPLOAD_MODE) {

        Serial.println("Leaving WiFi upload mode");

        currentState = MAIN_MENU;

        menuIndex = 0;

        screenDirty = true;
      }
    }
  }

  // ==========================================================
  // SAVE BUTTON STATES
  // ==========================================================

  lastUpState = up;
  lastDownState = down;
  lastEnterState = enter;
}

// ============================================================
// APPLICATION STATE MANAGEMENT
// ============================================================

void handleApplication() {

  // ----------------------------------------------------------
  // WIFI START
  // ----------------------------------------------------------

  if (currentState == UPLOAD_MODE && !wifiActive) {

    startWiFiAP();

    screenDirty = true;
  }

  // ----------------------------------------------------------
  // WIFI STOP
  // ----------------------------------------------------------

  if (currentState != UPLOAD_MODE && wifiActive) {

    stopWiFiAP();

    screenDirty = true;
  }

  // ----------------------------------------------------------
  // STATE CHANGE
  // ----------------------------------------------------------

  if (currentState != previousState) {

    previousState = currentState;

    screenDirty = true;
  }
}

// ============================================================
// START WIFI ACCESS POINT
// ============================================================

void startWiFiAP() {

  Serial.println();
  Serial.println("================================");
  Serial.println("STARTING WIFI AP");
  Serial.println("================================");

  // ----------------------------------------------------------
  // OLED STATUS
  // ----------------------------------------------------------

  display.clearDisplay();
  display.setCursor(0, 0);

  display.println("WiFi Starting...");
  display.println("Please wait...");

  display.display();

  // ----------------------------------------------------------
  // CLEAN WIFI STATE
  // ----------------------------------------------------------

  WiFi.disconnect(true);
  delay(100);

  WiFi.mode(WIFI_OFF);
  delay(200);

  // ----------------------------------------------------------
  // AP MODE
  // ----------------------------------------------------------

  WiFi.mode(WIFI_AP);

  delay(100);

  // ----------------------------------------------------------
  // LOW TX POWER
  // ----------------------------------------------------------

  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  delay(50);

  // ----------------------------------------------------------
  // STATIC IP
  // ----------------------------------------------------------

  if (!WiFi.softAPConfig(
        apIP,
        gateway,
        subnet
      )) {

    Serial.println("AP Config FAILED");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("AP CONFIG FAIL");
    display.display();

    delay(1500);

    currentState = MAIN_MENU;

    return;
  }

  // ----------------------------------------------------------
  // START AP
  // ----------------------------------------------------------

  bool result = WiFi.softAP(
    AP_SSID,
    AP_PASS
  );

  if (!result) {

    Serial.println("WiFi AP FAILED");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi AP FAILED");
    display.display();

    delay(1500);

    currentState = MAIN_MENU;

    return;
  }

  wifiActive = true;

  delay(300);

  server.begin();

  Serial.println("WiFi AP started");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);

  Serial.print("Password: ");
  Serial.println(AP_PASS);

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.println("Web server started");

  // ----------------------------------------------------------
  // SHOW RESULT
  // ----------------------------------------------------------

  display.clearDisplay();

  display.setCursor(0, 0);
  display.println("WIFI READY");

  display.setCursor(0, 10);
  display.println("ESP_Reader_C3");

  display.setCursor(0, 20);
  display.println("192.168.4.1");

  display.display();
}

// ============================================================
// STOP WIFI ACCESS POINT
// ============================================================

void stopWiFiAP() {

  Serial.println("Stopping WiFi...");

  server.stop();

  WiFi.softAPdisconnect(true);

  delay(100);

  WiFi.mode(WIFI_OFF);

  delay(100);

  wifiActive = false;

  Serial.println("WiFi OFF");
}

// ============================================================
// WEB SERVER SETUP
// ============================================================

void setupWebServer() {

  // ==========================================================
  // HOME PAGE
  // ==========================================================

  server.on("/", HTTP_GET, []() {

    String html;

    html.reserve(6000);

    html += "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";

    html += "<meta name='viewport' ";
    html += "content='width=device-width,initial-scale=1'>";

    html += "<title>ESP32-C3 File Manager</title>";

    html += "<style>";

    html += "body{";
    html += "font-family:Arial,sans-serif;";
    html += "background:#121212;";
    html += "color:#eee;";
    html += "padding:20px;";
    html += "margin:0;";
    html += "}";

    html += ".container{";
    html += "max-width:500px;";
    html += "margin:auto;";
    html += "}";

    html += ".card{";
    html += "background:#1e1e1e;";
    html += "padding:20px;";
    html += "border-radius:12px;";
    html += "margin-bottom:20px;";
    html += "box-shadow:0 4px 15px rgba(0,0,0,.4);";
    html += "}";

    html += "h1,h2{";
    html += "text-align:center;";
    html += "}";

    html += "input[type=file]{";
    html += "width:100%;";
    html += "margin-bottom:15px;";
    html += "}";

    html += "input[type=submit]{";
    html += "width:100%;";
    html += "padding:12px;";
    html += "border:0;";
    html += "border-radius:7px;";
    html += "background:#03dac6;";
    html += "color:#000;";
    html += "font-weight:bold;";
    html += "}";

    html += "ul{";
    html += "padding:0;";
    html += "margin:0;";
    html += "list-style:none;";
    html += "}";

    html += "li{";
    html += "background:#2b2b2b;";
    html += "padding:12px;";
    html += "margin-bottom:6px;";
    html += "border-radius:7px;";
    html += "display:flex;";
    html += "justify-content:space-between;";
    html += "align-items:center;";
    html += "}";

    html += ".fname{";
    html += "overflow:hidden;";
    html += "text-overflow:ellipsis;";
    html += "white-space:nowrap;";
    html += "max-width:70%;";
    html += "}";

    html += ".delete{";
    html += "background:#cf6679;";
    html += "color:white;";
    html += "padding:6px 10px;";
    html += "border-radius:5px;";
    html += "text-decoration:none;";
    html += "font-size:12px;";
    html += "}";

    html += ".info{";
    html += "text-align:center;";
    html += "color:#aaa;";
    html += "font-size:14px;";
    html += "}";

    html += "</style>";

    html += "</head>";

    html += "<body>";

    html += "<div class='container'>";

    html += "<h1>ESP32-C3</h1>";

    html += "<div class='card'>";

    html += "<h2>File Upload</h2>";

    html += "<form method='POST' ";
    html += "action='/upload' ";
    html += "enctype='multipart/form-data'>";

    html += "<input type='file' name='f' required>";

    html += "<input type='submit' value='UPLOAD FILE'>";

    html += "</form>";

    html += "</div>";

    html += "<div class='card'>";

    html += "<h2>Files</h2>";

    html += "<ul>";

    File root = LittleFS.open("/");

    if (root) {

      File file = root.openNextFile();

      bool found = false;

      while (file) {

        found = true;

        String name = file.name();

        if (!name.startsWith("/")) {
          name = "/" + name;
        }

        html += "<li>";

        html += "<span class='fname'>";
        html += name;
        html += "</span>";

        html += "<a class='delete' ";
        html += "href='/delete?name=";
        html += name;
        html += "' ";
        html += "onclick=\"return confirm('Delete this file?');\">";
        html += "DELETE";
        html += "</a>";

        html += "</li>";

        file = root.openNextFile();
      }

      root.close();

      if (!found) {

        html += "<li>";
        html += "No files found";
        html += "</li>";
      }
    }

    html += "</ul>";

    html += "</div>";

    html += "<div class='info'>";

    html += "ESP32-C3 File Manager<br>";
    html += "IP: 192.168.4.1";

    html += "</div>";

    html += "</div>";

    html += "</body>";

    html += "</html>";

    server.send(
      200,
      "text/html",
      html
    );
  });

  // ==========================================================
  // DELETE
  // ==========================================================

  server.on("/delete", HTTP_GET, []() {

    if (!server.hasArg("name")) {

      server.send(
        400,
        "text/plain",
        "Missing filename"
      );

      return;
    }

    String filename = server.arg("name");

    if (!isSafeFilename(filename)) {

      server.send(
        400,
        "text/plain",
        "Invalid filename"
      );

      return;
    }

    filename = normalizeFilename(filename);

    Serial.print("Delete request: ");
    Serial.println(filename);

    if (LittleFS.exists(filename)) {

      if (LittleFS.remove(filename)) {

        Serial.println("File deleted");
      }
      else {

        Serial.println("Delete failed");
      }
    }

    server.sendHeader(
      "Location",
      "/"
    );

    server.send(
      303
    );
  });

  // ==========================================================
  // UPLOAD COMPLETE
  // ==========================================================

  server.on(
    "/upload",
    HTTP_POST,
    []() {

      String html;

      html += "<!DOCTYPE html>";
      html += "<html>";
      html += "<head>";
      html += "<meta name='viewport' ";
      html += "content='width=device-width,initial-scale=1'>";
      html += "<style>";
      html += "body{";
      html += "background:#121212;";
      html += "color:white;";
      html += "font-family:Arial;";
      html += "text-align:center;";
      html += "padding:50px;";
      html += "}";
      html += "a{";
      html += "color:#03dac6;";
      html += "font-size:20px;";
      html += "}";
      html += "</style>";
      html += "</head>";
      html += "<body>";

      html += "<h2>File Uploaded!</h2>";
      html += "<p>";
      html += uploadFilename;
      html += "</p>";

      html += "<br>";

      html += "<a href='/'>Back to File List</a>";

      html += "</body>";
      html += "</html>";

      server.send(
        200,
        "text/html",
        html
      );
    },

    []() {

      HTTPUpload& upload = server.upload();

      // ------------------------------------------------------
      // START
      // ------------------------------------------------------

      if (upload.status == UPLOAD_FILE_START) {

        uploadFilename = upload.filename;

        uploadFilename =
          normalizeFilename(uploadFilename);

        Serial.println();
        Serial.print("Upload start: ");
        Serial.println(uploadFilename);

        if (!isSafeFilename(uploadFilename)) {

          Serial.println("Invalid filename");

          return;
        }

        uploadFile =
          LittleFS.open(
            uploadFilename,
            "w"
          );

        if (!uploadFile) {

          Serial.println(
            "ERROR: Cannot create upload file"
          );
        }
      }

      // ------------------------------------------------------
      // WRITE
      // ------------------------------------------------------

      else if (
        upload.status == UPLOAD_FILE_WRITE
      ) {

        if (uploadFile) {

          size_t written =
            uploadFile.write(
              upload.buf,
              upload.currentSize
            );

          if (written != upload.currentSize) {

            Serial.println(
              "WARNING: Partial write"
            );
          }
        }
      }

      // ------------------------------------------------------
      // END
      // ------------------------------------------------------

      else if (
        upload.status == UPLOAD_FILE_END
      ) {

        if (uploadFile) {

          uploadFile.close();
        }

        Serial.print("Upload complete: ");
        Serial.print(uploadFilename);
        Serial.print("  Size: ");
        Serial.println(upload.totalSize);

        uploadFilename = "";
      }

      // ------------------------------------------------------
      // ABORT
      // ------------------------------------------------------

      else if (
        upload.status == UPLOAD_FILE_ABORTED
      ) {

        Serial.println("Upload aborted");

        if (uploadFile) {
          uploadFile.close();
        }

        uploadFilename = "";
      }
    }
  );

  Serial.println("Web server handlers ready");
}

// ============================================================
// DRAW MAIN MENU
// ============================================================

void drawMainMenu() {

  display.clearDisplay();

  display.setCursor(0, 0);

  display.print(
    menuIndex == 0 ? "> Read Files" : "  Read Files"
  );

  display.setCursor(0, 12);

  display.print(
    menuIndex == 1 ? "> WiFi Upload" : "  WiFi Upload"
  );

  display.display();
}

// ============================================================
// DRAW FILE BROWSER
// ============================================================

void drawFileBrowser() {

  display.clearDisplay();

  if (fileCount <= 0) {

    display.setCursor(0, 0);
    display.println("No files");

    display.setCursor(0, 12);
    display.println("ENTER = Back");

    display.display();

    return;
  }

  const int visibleLines = 3;

  // ----------------------------------------------------------
  // SCROLL
  // ----------------------------------------------------------

  if (menuIndex < listScrollOffset) {

    listScrollOffset = menuIndex;
  }

  if (
    menuIndex >=
    listScrollOffset + visibleLines
  ) {

    listScrollOffset =
      menuIndex - visibleLines + 1;
  }

  // ----------------------------------------------------------
  // DRAW
  // ----------------------------------------------------------

  for (
    int i = 0;
    i < visibleLines;
    i++
  ) {

    int index =
      listScrollOffset + i;

    if (index >= fileCount) {
      break;
    }

    display.setCursor(
      0,
      i * 10
    );

    if (index == menuIndex) {
      display.print(">");
    }
    else {
      display.print(" ");
    }

    String name = fileList[index];

    // Keep filename within OLED
    if (name.length() > 18) {
      name = name.substring(0, 18);
    }

    display.setCursor(
      10,
      i * 10
    );

    display.print(name);
  }

  display.display();
}

// ============================================================
// DRAW FILE VIEWER
// ============================================================

void drawFileViewer() {

  display.clearDisplay();

  if (currentFileContent.length() == 0) {

    display.setCursor(0, 0);
    display.println("Empty file");

    display.setCursor(0, 16);
    display.println("ENTER = Back");

    display.display();

    return;
  }

  for (
    int i = 0;
    i < OLED_VISIBLE_LINES;
    i++
  ) {

    int lineIndex =
      viewerScrollLine + i;

    if (
      lineIndex >=
      totalViewerLines
    ) {
      break;
    }

    String line =
      getWrappedLine(
        currentFileContent,
        lineIndex
      );

    // Limit to OLED width
    if (line.length() > OLED_CHARS_PER_LINE) {

      line =
        line.substring(
          0,
          OLED_CHARS_PER_LINE
        );
    }

    display.setCursor(
      0,
      i * 8
    );

    display.print(line);
  }

  display.display();
}

// ============================================================
// DRAW UPLOAD SCREEN
// ============================================================

void drawUploadScreen() {

  display.clearDisplay();

  display.setCursor(0, 0);

  if (wifiActive) {

    display.println("WIFI READY");

    display.setCursor(0, 10);
    display.println("SSID: ESP_Reader_C3");

    display.setCursor(0, 20);
    display.println("192.168.4.1");
  }

  else {

    display.println("Starting WiFi...");
  }

  display.display();
}

// ============================================================
// LOAD FILES FROM LITTLEFS
// ============================================================

void loadFiles() {

  fileCount = 0;

  // ----------------------------------------------------------
  // BACK OPTION
  // ----------------------------------------------------------

  fileList[fileCount++] = "[..] BACK";

  // ----------------------------------------------------------
  // OPEN ROOT
  // ----------------------------------------------------------

  File root = LittleFS.open("/");

  if (!root) {

    Serial.println("Cannot open LittleFS root");

    return;
  }

  File file =
    root.openNextFile();

  // ----------------------------------------------------------
  // READ FILES
  // ----------------------------------------------------------

  while (
    file &&
    fileCount < MAX_FILES
  ) {

    if (!file.isDirectory()) {

      String name =
        file.name();

      if (!name.startsWith("/")) {

        name =
          "/" + name;
      }

      fileList[fileCount++] =
        name;
    }

    file =
      root.openNextFile();
  }

  root.close();

  Serial.print("Files found: ");
  Serial.println(fileCount - 1);
}

// ============================================================
// COUNT WRAPPED LINES
// ============================================================

int countWrappedLines(
  const String& text
) {

  if (text.length() == 0) {
    return 1;
  }

  int lines = 0;
  int chars = 0;

  for (
    unsigned int i = 0;
    i < text.length();
    i++
  ) {

    char c =
      text[i];

    // New line
    if (c == '\n') {

      lines++;
      chars = 0;

      continue;
    }

    // Ignore CR
    if (c == '\r') {
      continue;
    }

    chars++;

    if (
      chars >=
      OLED_CHARS_PER_LINE
    ) {

      lines++;
      chars = 0;
    }
  }

  // Remaining characters
  if (chars > 0) {
    lines++;
  }

  if (lines == 0) {
    lines = 1;
  }

  return lines;
}

// ============================================================
// GET WRAPPED LINE
// ============================================================

String getWrappedLine(
  const String& text,
  int requestedLine
) {

  int currentLine = 0;

  String line = "";

  for (
    unsigned int i = 0;
    i < text.length();
    i++
  ) {

    char c =
      text[i];

    // --------------------------------------------------------
    // NEWLINE
    // --------------------------------------------------------

    if (c == '\n') {

      if (
        currentLine ==
        requestedLine
      ) {

        return line;
      }

      currentLine++;

      line = "";

      continue;
    }

    // --------------------------------------------------------
    // CARRIAGE RETURN
    // --------------------------------------------------------

    if (c == '\r') {
      continue;
    }

    // --------------------------------------------------------
    // CHARACTER
    // --------------------------------------------------------

    line += c;

    // --------------------------------------------------------
    // OLED LINE WRAP
    // --------------------------------------------------------

    if (
      line.length() >=
      OLED_CHARS_PER_LINE
    ) {

      if (
        currentLine ==
        requestedLine
      ) {

        return line;
      }

      currentLine++;

      line = "";
    }
  }

  // ----------------------------------------------------------
  // LAST LINE
  // ----------------------------------------------------------

  if (
    currentLine ==
    requestedLine
  ) {

    return line;
  }

  return "";
}

// ============================================================
// FILENAME VALIDATION
// ============================================================

bool isSafeFilename(
  String filename
) {

  if (filename.length() == 0) {
    return false;
  }

  // Must start with /
  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }

  // Prevent directory traversal
  if (filename.indexOf("..") >= 0) {
    return false;
  }

  // Prevent Windows path
  if (filename.indexOf("\\") >= 0) {
    return false;
  }

  // Only root-level files
  int slashPosition =
    filename.indexOf(
      '/',
      1
    );

  if (slashPosition >= 0) {
    return false;
  }

  return true;
}

// ============================================================
// NORMALIZE FILENAME
// ============================================================

String normalizeFilename(
  String filename
) {

  filename.trim();

  // Convert backslash
  filename.replace("\\", "/");

  // Remove leading spaces/slashes
  while (
    filename.startsWith("/")
  ) {

    filename =
      filename.substring(1);
  }

  // Remove query/path fragments
  int question =
    filename.indexOf("?");

  if (question >= 0) {

    filename =
      filename.substring(
        0,
        question
      );
  }

  return "/" + filename;
}