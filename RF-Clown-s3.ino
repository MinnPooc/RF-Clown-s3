#include "RF24.h"
#include <SPI.h>
#include <ezButton.h>
#include <Adafruit_NeoPixel.h>
#include "esp_bt.h"
#include "esp_wifi.h"

// -------------------- USER CONFIG --------------------
constexpr int BUTTON_PIN = 34;
constexpr int NEOPIXEL_PIN = 4;
constexpr int NUM_PIXELS = 1;
constexpr int SPI_SPEED = 16000000;

// FSPI bus (used as VSPI equivalent on ESP32-S3)
#define FSPI_MISO   13
#define FSPI_MOSI   11
#define FSPI_SCLK   12
#define FSPI_CS     10

// HSPI bus
#define HSPI_MISO   9
#define HSPI_MOSI   8
#define HSPI_SCLK   7
#define HSPI_CS     6
// -----------------------------------------------------

SPIClass *spiFSPI = nullptr;   // VSPI equivalent on S3
SPIClass *spiHSPI = nullptr;

RF24 radioFSPI(15, 5, SPI_SPEED);   // CE=15, CSN=5
RF24 radioHSPI(22, 21, SPI_SPEED);  // CE=22, CSN=21

Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

int bluetooth_channels[] = {32, 34, 46, 48, 50, 52, 0, 1, 2, 4, 6, 8, 22, 24, 26, 28, 30, 74, 76, 78, 80};
int ble_channels[] = {2, 26, 80};

int currentMode = 0;

ezButton modeButton(BUTTON_PIN);

// -------------------- FUNCTION DECLARATIONS --------------------
void configureRadio(RF24 &radio, int channel, SPIClass *spi);
void handleModeChange();
void executeMode();
void updateNeoPixel();
void jamBLE();
void jamBluetooth();
void jamAll();
// ----------------------------------------------------------------

void setup() {
    Serial.begin(115200);

    // disable WiFi + Bluetooth
    esp_bt_controller_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_wifi_disconnect();

    modeButton.setDebounceTime(100);
    pixels.begin();
    pixels.clear();
    pixels.show();

    // ---- FSPI (VSPI equivalent) ----
    spiFSPI = new SPIClass(FSPI);
    spiFSPI->begin(FSPI_SCLK, FSPI_MISO, FSPI_MOSI, FSPI_CS);
    configureRadio(radioFSPI, ble_channels[0], spiFSPI);

    // ---- HSPI ----
    spiHSPI = new SPIClass(HSPI);
    spiHSPI->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_CS);
    configureRadio(radioHSPI, bluetooth_channels[0], spiHSPI);
}

void configureRadio(RF24 &radio, int channel, SPIClass *spi) {
    if (radio.begin(spi)) {
        radio.setAutoAck(false);
        radio.stopListening();
        radio.setRetries(0, 0);
        radio.setPALevel(RF24_PA_MAX, true);
        radio.setDataRate(RF24_2MBPS);
        radio.setCRCLength(RF24_CRC_DISABLED);
        radio.startConstCarrier(RF24_PA_HIGH, channel);
    } else {
        Serial.println("RF24 init failed!");
    }
}

void loop() {
    modeButton.loop();
    if (modeButton.isPressed()) {
        handleModeChange();
    }
    executeMode();
}

void handleModeChange() {
    currentMode = (currentMode + 1) % 4;
    Serial.print("Mode changed to: ");
    Serial.println(currentMode);
    updateNeoPixel();
}

void updateNeoPixel() {
    switch (currentMode) {
        case 0:
            pixels.clear();
            pixels.show();
            break;
        case 1:
            pixels.setPixelColor(0, pixels.Color(0, 0, 25));
            pixels.show();
            break;
        case 2:
            pixels.setPixelColor(0, pixels.Color(0, 25, 0));
            pixels.show();
            break;
        case 3:
            pixels.setPixelColor(0, pixels.Color(25, 0, 0));
            pixels.show();
            break;
    }
}

void executeMode() {
    switch (currentMode) {
        case 0:
            delay(100);
            break;
        case 1:
            jamBLE();
            break;
        case 2:
            jamBluetooth();
            break;
        case 3:
            jamAll();
            break;
    }
}

void jamBLE() {
    int randomIndex = random(0, sizeof(ble_channels) / sizeof(ble_channels[0]));
    int channel = ble_channels[randomIndex];
    radioFSPI.setChannel(channel);
    radioHSPI.setChannel(channel);
}

void jamBluetooth() {
    int randomIndex = random(0, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0]));
    int channel = bluetooth_channels[randomIndex];
    radioFSPI.setChannel(channel);
    radioHSPI.setChannel(channel);
}

void jamAll() {
    if (random(0, 2)) {
        jamBluetooth();
    } else {
        jamBLE();
    }
}