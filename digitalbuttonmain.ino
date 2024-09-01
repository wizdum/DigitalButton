// This example if for processors with LittleFS capability (e.g. RP2040,
// ESP32, ESP8266). It renders a png file that is stored in LittleFS
// using the PNGdec library (available via library manager).

// The test image is in the sketch "data" folder (press Ctrl+K to see it).
// You must upload the image to LittleFS using the Arduino IDE Tools Data
// Upload menu option (you may need to install extra tools for that).

// Don't forget to use the Arduino IDE Tools menu to allocate a LittleFS
// memory partition before uploading the sketch and data!

/*
How to upload files to LittleFS with Arduino
Ctrl – shift – P, choose “upload littlefs” 
This will upload the files in the “Data” folder within the sketch folder, to the LittleFS partition on the ESP. 
Then compile and upload the sketch.


*/

/* 
################### To - Do ###############################
1. Download .txt file from github with list of .png urls
2. function that clears .png files from LittleFS storage
3. web server function that launches on WiFi failure and allows you to 
  list PNG URLs and set WiFi SSID and Password.
##################################################
*/


// Added to download file
#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>

const char* ssid = "IoTrash";
const char* password = "!Kingston1255....";
const char* fileURL = "https://raw.githubusercontent.com/wizdum/digitalbutton/main/wah.png";
const char* filePath = "/wah.png";
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

//====================================================================================
//                                    Setup
//====================================================================================
void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n Using the PNGdec library");

  // Initialise FS
  if (!FileSys.begin()) {
    Serial.println("LittleFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }

  // Initialise the TFT
  tft.begin();
  tft.fillScreen(TFT_BLACK);

  Serial.println("\r\nInitialisation done.");

  // Connect to WiFi

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to WiFi.");

  // Download and save the file
  downloadAndSaveFile(fileURL, filePath);
  // Check and print the size of the downloaded file

  printFileSize(filePath);
}


//====================================================================================
//                                    Loop
//====================================================================================
void loop()
{
  // Scan LittleFS and load any *.png files
  File root = LittleFS.open("/", "r");
  while (File file = root.openNextFile()) {
    String strname = file.name();
    strname = "/" + strname;
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

void downloadAndSaveFile(const char* fileURL, const char* filePath) {
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

void printFileSize(const char* filePath) {
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
