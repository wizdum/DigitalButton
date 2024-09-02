
/*
How to upload files to LittleFS with Arduino
Ctrl – shift – P, choose “upload littlefs” 
This will upload the files in the “Data” folder within the sketch folder, to the LittleFS partition on the ESP. 
Then compile and upload the sketch.


*/

/* 
################### To - Do ###############################
#1. Download .txt file from github with list of .png urls
#2. function that clears .png files from LittleFS storage
3. web server function that launches on WiFi failure and allows you to 
  list PNG URLs and set WiFi SSID and Password.
4. Touch screen swipe to change image
##################################################
*/


// Added to download file
#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>
#include <WebServer.h>
#include <stdlib.h> // For random number generation
#include <CST816S.h> // touch library

CST816S touch(6, 7, 13, 5);	// sda, scl, rst, irq

const char* ssid = "IoTrash";
const char* password = "!Kingston1255....";
const char* fileURL = "https://raw.githubusercontent.com/wizdum/digitalbutton/main/imageURLs.txt";
const char* filePath = "/imageURLs.txt";
// Root CA certificate for the server

const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n" \
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n" \
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n" \
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n" \
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n" \
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n" \
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n" \
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n" \
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n" \
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n" \
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n" \
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n" \
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n" \
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n" \
"MrY=\n" \
"-----END CERTIFICATE-----\n";
// ^ Added to download file

WebServer server(80); // HTTP server on port 80

unsigned long apStartTime = 0; // To store the time AP mode was started
const unsigned long apDuration = 600000; // 10 minutes in milliseconds (10 * 60 * 1000)
bool apModeActive = false; // To keep track of whether AP mode is currently active

#include <LittleFS.h>
#define FileSys LittleFS

// Include the PNG decoder library
#include <PNGdec.h>

PNG png;
#define MAX_IMAGE_WDITH 240 // Adjust for your images

int16_t xpos = 0;
int16_t ypos = 0;

// Include the TFT library https://github.com/Bodmer/TFT_eSPI
#include "SPI.h"
#include <TFT_eSPI.h>              // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library


struct TouchPoint {
  int x = 0;
  int y = 0;
  bool isTouched = false;
};

//====================================================================================
//                                    Setup
//====================================================================================
void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n Using the PNGdec library");
  
  touch.begin();

  // Initialise FS
  if (!FileSys.begin()) {
    Serial.println("LittleFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }

  // Create a directory named "images"
  if (!LittleFS.mkdir("/images")) {
    Serial.println("Failed to create directory");
    return;
  } else {
    Serial.println("Directory created");
  }

  // Clear old PNG Files
  //deletePngFiles("/");
  //deletePngFiles("/images/");
  // Initialise the TFT
  tft.begin();
  tft.fillScreen(TFT_BLACK);

  Serial.println("\r\nInitialisation done.");

  // Connect to WiFi

  String ssid, password;
  if (readWifiConfig(ssid, password)) {
    connectToWifi(ssid, password);
  } else {
    Serial.println("Failed to read WiFi config");
    startAccessPoint();
    return;
  }

/*
WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi..");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    if (++attempts >= 10) { // After 10 attempts, start AP
      startAccessPoint();
      return;
    }
  }
  Serial.println("Connected to WiFi");
*/
  // Download and save the file
  downloadAndSaveFile(fileURL, filePath);
  // Check and print the size of the downloaded file
  //printFileSize(filePath);
  processURLList("/imageURLs.txt");
}


//====================================================================================
//                                    Loop
//====================================================================================
void loop()
{
    // If in AP mode, handle client requests
  if (WiFi.getMode() & WIFI_MODE_AP) {
    server.handleClient();
  }
  // Scan LittleFS and load any *.png files
  File root = LittleFS.open("/images/", "r");
  /*
  while (File file = root.openNextFile()) {
    String strname = file.name();
    strname = "/images/" + strname;
    Serial.println(file.name());
    // If it is not a directory and filename ends in .png then load it
    if (!file.isDirectory() && strname.endsWith(".png")) {
      // Pass support callback function names to library
      int16_t rc = png.open(strname.c_str(), pngOpen, pngClose, pngRead, pngSeek, pngDraw);
      if (rc == PNG_SUCCESS) {
        tft.startWrite();
        Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
        uint32_t dt = millis();
        if (png.getWidth() > MAX_IMAGE_WDITH) {
          Serial.println("Image too wide for allocated line buffer size!");
        }
        else {
          rc = png.decode(NULL, 0);
          png.close();
        }
        tft.endWrite();
        // How long did rendering take...
        Serial.print(millis()-dt); Serial.println("ms");
      }
    }
    delay(3000);
    //tft.fillScreen(random(0x10000));
  }
  */
/*
int16_t rc = png.open("/images/awoo.png", pngOpen, pngClose, pngRead, pngSeek, pngDraw);
if (rc == PNG_SUCCESS) {
  tft.startWrite();
  Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
  uint32_t dt = millis();
    if (png.getWidth() > MAX_IMAGE_WDITH) {
    Serial.println("Image too wide for allocated line buffer size!");
    }
    else {
      rc = png.decode(NULL, 0);
      png.close();
    }
  tft.endWrite();
  // How long did rendering take...
  Serial.print(millis()-dt); Serial.println("ms");
}
*/
  TouchPoint currentPoint = readTouchPoint();
  String gesture = detectSwipeGesture(currentPoint);
  if (gesture != "No Swipe") {
    Serial.println(gesture);
  }

if (apModeActive && millis() - apStartTime >= apDuration) {
    // 10 minutes have passed, turn off the AP
    WiFi.softAPdisconnect(true); // Disconnect and disable the AP
    apModeActive = false; // Set AP mode as inactive
    Serial.println("AP Mode turned off");
  }

}


//=========================================v==========================================
//                                      pngDraw
//====================================================================================
// This next function will be called during decoding of the png file to
// render each image line to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
// Callback function to draw pixels to the display
void pngDraw(PNGDRAW *pDraw) {
  uint16_t lineBuffer[MAX_IMAGE_WDITH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

void downloadAndSaveFile(String fileURL, String filePath) {
  HTTPClient http;
  http.begin(fileURL, rootCACertificate);
  http.setTimeout(5000);
  // Assuming http is an instance of HTTPClient
if (http.GET() == HTTP_CODE_OK) {
    WiFiClient *stream = http.getStreamPtr();
    File file = LittleFS.open(filePath, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    int totalBytes = http.getSize(); // Total expected size
    int readBytes = 0;

    // Read all data from server
    while (http.connected() && (totalBytes > 0 || totalBytes == -1)) {
        // Get available data size
        size_t size = stream->available();

        if (size) {
            // Allocate a buffer to store incoming data
            std::unique_ptr<char[]> buf(new char[size]);
            // Read the data
            int c = stream->readBytes(buf.get(), size);
            file.write((uint8_t*)buf.get(), c);
            readBytes += c;

            if (totalBytes > 0) {
                totalBytes -= c;
            }
        }
        yield(); // Allow background tasks to run
    }

    Serial.print("Read ");
    Serial.print(readBytes);
    Serial.println(" bytes");
    file.close();
} else {
    Serial.print("Error on HTTP request: ");
    Serial.println(http.errorToString(http.GET()).c_str());
}

  http.end();

}

void printFileSize(String filePath) {
  if(!LittleFS.exists(filePath)){
    Serial.println("File does not exist.");
    return;
  }
  
  File file = LittleFS.open(filePath, "r");
  if(!file){
    Serial.println("Failed to open file for reading.");
    return;
  }
  Serial.print("Size of ");
  Serial.print(filePath);
  Serial.print(" is ");
  Serial.print(file.size());
  Serial.println(" bytes.");
  file.close();

}

void processURLList(String filePath) {
  File urlList = LittleFS.open(filePath, "r");
  if (!urlList) {
    Serial.println("Failed to open URL list file for reading");
    return;
  }

  while (urlList.available()) {
    String url = urlList.readStringUntil('\n');
    url.trim(); // Remove any leading/trailing whitespace
    
    if (url.length() > 0) {
      // Assuming you have a predefined function to handle the file name extraction and download
      String fileName = extractFileName(url);
      downloadAndSaveFile(url, "/images/" + fileName);
    }
  }

  urlList.close();
}

String extractFileName(String url) {
  int lastSlashIndex = url.lastIndexOf('/');
  return url.substring(lastSlashIndex + 1);
}

void deletePngFiles(String path) {
  File root = LittleFS.open(path, "r");
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    file.close(); // Ensure the file is closed before attempting to delete
    if (fileName.endsWith(".png")) {
      String fullPath = path + fileName; // Ensure the path is correctly formatted
      // Attempt to delete the file
      if (LittleFS.remove(fullPath)) {
        Serial.println("Deleted: " + fullPath);
      } else {
        Serial.println("Failed to delete: " + fullPath);
      }
    }
    file = root.openNextFile();
  }
}

void startAccessPoint() {
  String baseSSID = "ESP32-Config"; // Base SSID
  String randomChars = "";
  // Initialize random seed
  randomSeed(analogRead(0));
  // Generate three random characters and append to the base SSID
  for (int i = 0; i < 3; i++) {
    randomChars += generateRandomChar();
  }
  String fullSSID = baseSSID + randomChars; // Concatenate base SSID with random characters

  WiFi.softAP(fullSSID.c_str(), ""); // Start the AP with the new SSID
  Serial.println("Started Access Point");
  Serial.print("Access Point Web Server URL: http://192.168.4.1");
  
  apStartTime = millis(); // Record the time AP mode started
  apModeActive = true; // Set AP mode as active
  
  tft.setTextSize(2); // Adjust the number to increase text size
  String text = "AP IP address: 192.168.4.1"; // Example text
  int textSize = 2; // Example text size
  int charWidth = 6 * textSize; // Approximate width of a character
  int charHeight = 8 * textSize; // Approximate height of a character

  int textWidth = text.length() * charWidth;
  int textHeight = charHeight; // Assuming single line of text

  int xPosition = (tft.width() - textWidth) / 2; // Center horizontally
  //int yPosition = (tft.height() - textHeight) / 2; // Center vertically
  tft.setCursor(xPosition, 55);
  //tft.setCursor(0, 0); // Set cursor at top left corner

  tft.println("      AP Mode");
  tft.println("  SSID: ");
  tft.println(fullSSID);
  tft.println("  URL: http://");
  tft.println("  192.168.4.1"); // Display the IP address as a string

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", "<form action=\"/save\" method=\"POST\"><input type=\"text\" name=\"ssid\" placeholder=\"SSID\"><br><input type=\"password\" name=\"password\" placeholder=\"Password\"><br><input type=\"submit\" value=\"Save\"></form>");
  });

  server.on("/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    saveCredentials(ssid, password);
    server.send(200, "text/html", "<h1>Credentials saved. Please reset the device.</h1>");
    // Optionally, you could attempt to connect to WiFi here using the new credentials
  });

  server.begin();
}

void saveCredentials(String ssid, String password) {
  File file = LittleFS.open("/config.txt", "w");
  if (!file) {
    Serial.println("Failed to open config file for writing");
    return;
  }
  file.println(ssid);
  file.println(password);
  file.close();
  Serial.println("Credentials saved");
}

char generateRandomChar() {
  int randomValue = random(48, 123); // Generate a random number between 48 and 122
  // Ensure the value is within the alphanumeric range
  if ((randomValue > 57 && randomValue < 65) || (randomValue > 90 && randomValue < 97)) {
    return generateRandomChar(); // Recursively generate a different character if it's not alphanumeric
  }
  return (char)randomValue; // Cast the random value to a char and return it
}

bool readWifiConfig(String &ssid, String &password) {
  File configFile = LittleFS.open("/config.txt", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  ssid = configFile.readStringUntil('\n');
  password = configFile.readStringUntil('\n');

  // Remove any trailing newline or carriage return characters
  ssid.trim();
  password.trim();

  configFile.close();
  return true;
}

void connectToWifi(const String &ssid, const String &password) {
  WiFi.begin(ssid.c_str(), password.c_str());
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

TouchPoint readTouchPoint() {
  TouchPoint point;
  // Assuming your library has a method to check if the screen is being touched
  // and methods to get the current touch coordinates
  if (touch.available()) {
    point.x = touch.data.x;
    point.y = touch.data.y;
    point.isTouched = true;
  }
  return point;
}

// Global variables to hold the start touch point and a flag to indicate a gesture is in progress
TouchPoint startTouchPoint;
bool gestureInProgress = false;

// Function to detect swipe gesture
String detectSwipeGesture(TouchPoint currentPoint) {
  if (currentPoint.isTouched && !gestureInProgress) {
    // Touch start
    if (!startTouchPoint.isTouched) {
      startTouchPoint = currentPoint;
      gestureInProgress = true;
    }
  } else if (gestureInProgress && !currentPoint.isTouched) {
    // Touch end, check for swipe
    int deltaX = currentPoint.x - startTouchPoint.x;
    int deltaY = currentPoint.y - startTouchPoint.y;
    
    // Reset for next gesture
    startTouchPoint.isTouched = false;
    gestureInProgress = false;
    
    if (abs(deltaX) > 50 || abs(deltaY) > 50) { // Threshold for swipe detection
      if (abs(deltaX) > abs(deltaY)) {
        // Horizontal swipe
        return deltaX > 0 ? "Swipe Right" : "Swipe Left";
      } else {
        // Vertical swipe
        return deltaY > 0 ? "Swipe Down" : "Swipe Up";
      }
    }
  }
  return "No Swipe";
}
