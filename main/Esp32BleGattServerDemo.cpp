#include <stdio.h>
#include <esp_log.h>
#include <stdexcept>
#include "BleServer.hpp"
#include "GattsApplication.hpp"
#include "GattsService.hpp"
#include "NonVolatileStorage.hpp"
#include "UInt16GattCharacteristic.hpp"

#define LOG_TAG "Main"

#define GATTS_APPLICATION_ID (0x2104)
#define GATTS_DEVICE_APPEARANCE (0x0280)

static Esp32::GattsApplication gattsApplication(
    GATTS_APPLICATION_ID,
    "ESP32",
    "ESP32-GATT-Demo",
    GATTS_DEVICE_APPEARANCE);

static Esp32::GattsService gattsServiceA(0x4000);
static Esp32::GattsService gattsServiceB(0x4100);

static Esp32::UInt16GattCharacteristic characteristicA1(0x4010, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, "Foo", 0x4142);
static Esp32::UInt16GattCharacteristic characteristicA2(0x4020, ESP_GATT_PERM_READ, "Bar", 0x3132);
static Esp32::UInt16GattCharacteristic characteristicB(0x4110, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, "Blubb", 0x6162);

extern "C" {

void app_main(void)
{
    ESP_LOGI(LOG_TAG, "ESP32/C++ - BLE test");
    ESP_LOGI(LOG_TAG, "====================");

    try
    {
        Esp32::NonVolatileStorage::instance()->probe();
        ESP_LOGI(LOG_TAG, "NonVolatileStorage: probing done");
        Esp32::BleServer::instance()->probe();
        ESP_LOGI(LOG_TAG, "BleServer: probing done");

        gattsServiceA.addCharacteristic(&characteristicA1);
        gattsServiceA.addCharacteristic(&characteristicA2);
        gattsApplication.addService(&gattsServiceA);

        gattsServiceB.addCharacteristic(&characteristicB);
        gattsApplication.addService(&gattsServiceB);

        Esp32::BleServer::instance()->setGattsApplication(&gattsApplication);
        ESP_LOGI(LOG_TAG, "BleServer: GATTS application successfully set");
    }
    catch(const std::exception& e)
    {
        ESP_LOGE(LOG_TAG, "Caught exception: %s\n", e.what());
    }
}

}