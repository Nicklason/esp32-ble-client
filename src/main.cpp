#include "Arduino.h"
#include "BLEDevice.h"

static BLEUUID serviceUUID("033214d2-0ff0-4cba-814e-c5074c1ad00c");
static BLEUUID charUUID("ac6744a7-77f3-43e9-b3c8-9955ac6bb0d4");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLERemoteCharacteristic *remoteCharacteristic;
static BLEAdvertisedDevice *server;

static void notifyCallback(BLERemoteCharacteristic *remoteCharacteristic, uint8_t *data, size_t length, bool isNotify) {
    Serial.println("onNotify");

    Serial.println((char *)data);
}

class ClientCallbacks : public BLEClientCallbacks {
    void onConnect(BLEClient *client) {
        Serial.println("Connected");
    }

    void onDisconnect(BLEClient *client) {
        connected = false;
        Serial.println("Disconnected");
        doScan = true;
    }
};

void connectToServer() {
    Serial.print("Connecting to ");
    Serial.print(server->getAddress().toString().c_str());
    Serial.println("...");

    BLEClient *client = BLEDevice::createClient();
    client->setClientCallbacks(new ClientCallbacks());

    client->connect(server);

    BLERemoteService *remoteService = client->getService(serviceUUID);
    if (remoteService == nullptr) {
        Serial.println("Failed to connect to the service");
        client->disconnect();
        doScan = true;
        return;
    }

    remoteCharacteristic = remoteService->getCharacteristic(charUUID);
    if (remoteCharacteristic == nullptr) {
        Serial.println("Failed to find characteristic");
        client->disconnect();
        return;
    }

    if (remoteCharacteristic->canRead()) {
        std::string value = remoteCharacteristic->readValue();
        Serial.print("The characteristic value was: ");
        Serial.println(value.c_str());
    }

    if (remoteCharacteristic->canNotify()) {
        remoteCharacteristic->registerForNotify(notifyCallback);
    }

    connected = true;
}

class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("Advertised device: ");
        Serial.println(advertisedDevice.toString().c_str());

        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            Serial.println("Found service");

            BLEDevice::getScan()->stop();
            server = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = false;
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE client...");
    BLEDevice::init("");

    BLEScan *scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    scan->start(5);
}

void loop() {
    if (doConnect == true) {
        connectToServer();
        doConnect = false;
    }

    if (connected) {
        String newValue = "Epoch: " + String(millis() / 1000);
        Serial.println("Setting new characteristic value to \"" + newValue + "\"");

        remoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    } else if (doScan) {
        BLEDevice::getScan()->start(5);
    }

    delay(1000);
}
