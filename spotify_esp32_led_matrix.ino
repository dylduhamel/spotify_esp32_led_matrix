/*******************************************************************
    Displays Album Art on 64x64 pixel LED matrix
  
    NOTE: You need to get a Refresh token to use this example
    Use the getRefreshToken example to get it.

    Thank you to https://github.com/witnessmenow for the incredible ESP32-Trinity development

 *******************************************************************/

// ----------------------------
// Standard Libraries
// ----------------------------
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <FS.h>
#include "SPIFFS.h"

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// This is the library for interfacing with the display

// Can be installed from the library manager (Search for "ESP32 MATRIX DMA")
// https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA

#include <JPEGDEC.h>
// Library for decoding Jpegs from the API responses

// Search for "JPEGDEC" in the Arduino Library manager
// https://github.com/bitbank2/JPEGDEC/

#include <SpotifyArduino.h>
// Library for interacting with Spotify API

// Download zip from Github and install to the Arduino IDE using that.
//https://github.com/witnessmenow/spotify-api-arduino

#include <ArduinoJson.h>
// JSON library

// Can be installed from the library manager (Search for "ArduinoJson")

// -------------------------------------
// -------   Matrix Config   ------
// -------------------------------------

const int panelResX = 64;   // Number of pixels wide of each INDIVIDUAL panel module.
const int panelResY = 64;   // Number of pixels tall of each INDIVIDUAL panel module.
const int panel_chain = 1;  // Total number of panels chained one to another

// -------------------------------------
//------- Other Config - Replace the following! ------
// -------------------------------------

char ssid[] = "network ID";           // your network SSID (name)
char password[] = "passwd";  // your network password

char clientId[] = "client";      // Your client ID of your spotify APP
char clientSecret[] = "client_secret";  // Your client Secret of your spotify APP (Do Not share this!)

// Country code, including this is advisable
#define SPOTIFY_MARKET "US"

#define SPOTIFY_REFRESH_TOKEN "AQDXPJFHs32W-bWKqa4PCPhFoalP_vTBfXvd-TZ_GM8hYgHcCoZYx6UxcqK7kBZVFjfjXfq9OTkA4GSnlyZcyzRxVrRzcWFlCvoIJIXvoWBEz8XXa8FNZPBWIQI4zjCLA9M"

#include <SpotifyArduinoCert.h>

// File name for where to save the image.
#define ALBUM_ART "/album.jpg"

// Ensures we only render on new songs
String lastAlbumArtUrl;

// Variable to hold image info
SpotifyImage smallestImage;

// Current song information
char *songName;
char *songArtist;

WiFiClientSecure client;
SpotifyArduino spotify(client, clientId, clientSecret, SPOTIFY_REFRESH_TOKEN);

unsigned long delayBetweenRequests = 1000;  // Time between requests (ms)
unsigned long requestDueTime;                // Time to send request (compared to millis())

// Matrix display
MatrixPanel_I2S_DMA *dma_display = nullptr;

// JPEG decoder
JPEGDEC jpeg;

// Function to decode JPEG file 
int displayOutput(JPEGDRAW *pDraw) {
  // Stop further decoding as image is running off bottom of screen
  if (pDraw->y >= dma_display->height()) return 0;

  dma_display->drawRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);

  // Return 1 to decode next block
  return 1;
}

void displaySetup() {
  HUB75_I2S_CFG mxconfig(
    panelResX,   // Martix width
    panelResY,   // Matrix height
    panel_chain  // Chain length
  );

  // If you are using a 64x64 matrix you need to pass a value for the E pin
  // The trinity connects GPIO 18 to E.
  // This can be commented out for any smaller displays (but should work fine with it)
  mxconfig.gpio.e = 18;

  // May or may not be needed depending on your matrix
  // Example of what needing it looks like:
  // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/134#issuecomment-866367216
  mxconfig.clkphase = false;

  // Some matrix panels use different ICs for driving them and some of them have strange quirks.
  // If the display is not working right, try this.
  //mxconfig.driver = HUB75_I2S_CFG::FM6126A;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
}

void setup() {
  Serial.begin(115200);

  displaySetup();

  // Clear screen
  dma_display->fillScreen(dma_display->color565(0, 0, 0));

  dma_display->setTextSize(1);      // size 1 == 8 pixels high
  dma_display->setTextWrap(false);  // Don't wrap at end of line - will do ourselves
  dma_display->setTextColor(dma_display->color565(255, 0, 255));

  dma_display->setCursor(1, 0);  // Start at top left, with 8 pixel of spacing

  dma_display->print("SPIFFS: ");
  dma_display->setCursor(48, 0);
  // Initialise SPIFFS, if this fails try .begin(true)
  if (!SPIFFS.begin() && !SPIFFS.begin(true)) {
    dma_display->print("NO!");
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield();  // Stay here twiddling thumbs waiting
  } else {
    dma_display->print("OK!");
  }

  dma_display->setCursor(1, 10);  // Start at top left, with 8 pixel of spacing
  dma_display->print("WiFi: ");
  dma_display->setCursor(48, 10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setCACert(spotify_server_cert);

  Serial.println("Refreshing Access Tokens");
  if (!spotify.refreshAccessToken()) {
    Serial.println("Failed to get access tokens");
  }

  dma_display->print("OK!");
}

fs::File myfile;

// Callbacks used by the jpeg library
void *myOpen(const char *filename, int32_t *size) {
  myfile = SPIFFS.open(filename);
  *size = myfile.size();
  return &myfile;
}
void myClose(void *handle) {
  if (myfile) myfile.close();
}
int32_t myRead(JPEGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!myfile) return 0;
  return myfile.read(buffer, length);
}
int32_t mySeek(JPEGFILE *handle, int32_t position) {
  if (!myfile) return 0;
  return myfile.seek(position);
}

void drawImageFile(char *albumArtUrl) {
  unsigned long lTime = millis();
  lTime = millis();
  jpeg.open(albumArtUrl, myOpen, myClose, myRead, mySeek, displayOutput);
  jpeg.decode(0, 0, 0);
  jpeg.close();
  Serial.print("Time taken to decode and display Image (ms): ");
  Serial.println(millis() - lTime);
}

int displayImage(char *albumArtUrl) {
  // If album art is no different, do not change the rendered image
  if (SPIFFS.exists(ALBUM_ART) == true) {
    Serial.println("Removing existing image");
    SPIFFS.remove(ALBUM_ART);
  }

  fs::File f = SPIFFS.open(ALBUM_ART, "w+");
  if (!f) {
    Serial.println("file open failed");
    return -1;
  }

  // Spotify uses a different cert for the Image server, so we need to swap to that for the call
  client.setCACert(spotify_image_server_cert);
  bool gotImage = spotify.getImage(albumArtUrl, &f);

  // Swapping back to the main spotify cert
  client.setCACert(spotify_server_cert);

  // Make sure to close the file!
  f.close();

  if (gotImage) {
    drawImageFile((char *)ALBUM_ART);
    return 0;
  } else {
    return -2;
  }
}

void printCurrentlyPlayingToSerial(CurrentlyPlaying currentlyPlaying) {
  // Clear the Text every time a new song is created
  Serial.println("--------- Currently Playing ---------");

  Serial.print("Is Playing: ");
  if (currentlyPlaying.isPlaying) {
    Serial.println("Yes");
  } else {
    Serial.println("No");
  }

  Serial.print("Track: ");
  Serial.println(currentlyPlaying.trackName);
  // Save the song name to a variable
  songName = const_cast<char *>(currentlyPlaying.trackName);
  drawMessage(0, 120, songName);
  Serial.print("Track URI: ");
  Serial.println(currentlyPlaying.trackUri);
  Serial.println();

  Serial.println("Artists: ");
  for (int i = 0; i < currentlyPlaying.numArtists; i++) {
    Serial.print("Name: ");
    // Save the song artist name to a variable
    Serial.println(currentlyPlaying.artists[i].artistName);
    songArtist = const_cast<char *>(currentlyPlaying.artists[0].artistName);
    drawMessage(0, 130, songArtist);
    Serial.print("Artist URI: ");
    Serial.println(currentlyPlaying.artists[i].artistUri);
    Serial.println();
  }

  Serial.print("Album: ");
  Serial.println(currentlyPlaying.albumName);
  Serial.print("Album URI: ");
  Serial.println(currentlyPlaying.albumUri);
  Serial.println();

  long progress = currentlyPlaying.progressMs;  // duration passed in the song
  long duration = currentlyPlaying.durationMs;  // Length of Song
  Serial.print("Elapsed time of song (ms): ");
  Serial.print(progress);
  Serial.print(" of ");
  Serial.println(duration);
  Serial.println();

  float percentage = ((float)progress / (float)duration) * 100;
  int clampedPercentage = (int)percentage;
  Serial.print("<");
  for (int j = 0; j < 50; j++) {
    if (clampedPercentage >= (j * 2)) {
      Serial.print("=");
    } else {
      Serial.print("-");
    }
  }
  Serial.println(">");
  Serial.println();

  for (int i = 0; i < currentlyPlaying.numImages; i++) {
    // Using the 64x64 pixel image (since we are rendering on a 64x64 matrix)
    smallestImage = currentlyPlaying.albumImages[2];
    Serial.println("------------------------");
    Serial.print("Album Image: ");
    Serial.println(currentlyPlaying.albumImages[i].url);
    Serial.print("Dimensions: ");
    Serial.print(currentlyPlaying.albumImages[i].width);
    Serial.print(" x ");
    Serial.print(currentlyPlaying.albumImages[i].height);
    Serial.println();
  }
  Serial.println("------------------------");
}

void loop() {
  if (millis() > requestDueTime) {
    Serial.print("\n\n\nFree Heap: ");
    Serial.println(ESP.getFreeHeap());

    Serial.println("getting currently playing song:");
    // Check if music is playing currently on the account.
    int status = spotify.getCurrentlyPlaying(printCurrentlyPlayingToSerial, SPOTIFY_MARKET);
    Serial.print("STATUS CODE: ");
    Serial.println(status);

    if (status == 200) {
      Serial.println("Successfully got currently playing");
      String newAlbum = String(smallestImage.url);
      if (newAlbum != lastAlbumArtUrl) {
        Serial.println("Updating Art");
        char *my_url = const_cast<char *>(smallestImage.url);
        Serial.print("URL: ");
        Serial.println(my_url);
        int displayImageResult = displayImage(my_url);

        if (displayImageResult == 0) {
          lastAlbumArtUrl = newAlbum;
        } else {
          Serial.print("failed to display image: ");
          Serial.println(displayImageResult);
        }
      }
    } else if (status == 204) {
      Serial.println("Doesn't seem to be anything playing");
      dma_display->fillScreen(dma_display->color565(0, 0, 0));

    } else {
      Serial.print("Error: ");
      Serial.println(status);
    }

    requestDueTime = millis() + delayBetweenRequests;
  }
}

// Method to draw messages at a certain point on a TFT Display.
void drawMessage(int x, int y, char *message) {
  Serial.println(message);
}