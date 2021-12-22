#include <string.h>
#include <esp_log.h>
#include <stdexcept>
#include "GenericGattsApplication.hpp"

#define LOG_TAG "GenericGattsApplication"

#define BLE_EXAMPLE_DEVICE_NAME "ESP_GATT"

#define CONFIGURATION_ADVERTISING_PENDING (1 << 0)
#define CONFIGURATION_SCAN_RESPONSE_PENDING (1 << 1)

namespace Esp32
{

static uint8_t rawAdvertisingData[] = {
        /* flags */
        0x02, 0x01, 0x06,
        /* tx power*/
        0x02, 0x0a, 0xeb,
        /* service uuid */
        0x03, 0x03, 0xFF, 0x00,
        /* device name */
        0x0f, 0x09, 'E', 'S', 'P', '_', 'G', 'A', 'T', 'T'
};

static uint8_t rawScanResponseData[] = {
        /* flags */
        0x02, 0x01, 0x06,
        /* tx power */
        0x02, 0x0a, 0xeb,
        /* service uuid */
        0x03, 0x03, 0xFF, 0x00
};

static esp_ble_adv_params_t advertisingParameters = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
   .peer_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

GenericGattsApplication::GenericGattsApplication(uint16_t applicationId):
    m_applicationId(applicationId),
    m_configurationDone(0),
    m_interface(ESP_GATT_IF_NONE)
{
}

GenericGattsApplication::~GenericGattsApplication()
{
}

uint16_t GenericGattsApplication::applicationId(void) const
{
    return m_applicationId;
}

esp_gatt_if_t GenericGattsApplication::interface(void) const
{
    return m_interface;
}

void GenericGattsApplication::gapEventCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param)
{
    switch (event)
    {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            handleGapEventRawAdvertisingDataSetComplete();
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            handleGapEventRawScanResponseDataSetComplete();
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            handleGapEventAdvertisementStartComplete(param);
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            handleGapEventUpdatedConnectionParameters(param);
            break;
        default:
            ESP_LOGI(LOG_TAG, "GenericGattsApplication::gapEventCallback(event=%d)", (int)event);
            throw std::runtime_error("not yet implemented");
    }
}

void GenericGattsApplication::gattsEventCallback(
    esp_gatts_cb_event_t event,
    esp_gatt_if_t gatts_if,
    esp_ble_gatts_cb_param_t* param)
{
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status != ESP_GATT_OK)
        {
            char buffer[64];
            snprintf(
                buffer,
                sizeof(buffer) - 1,
                "error registering the application %04x, status %d",
                param->reg.app_id,
                param->reg.status);
            throw std::runtime_error(buffer);
        }

        m_interface = gatts_if;
    }

    switch (event)
    {
        case ESP_GATTS_REG_EVT:
            handleGattsEventRegister();
            break;
        case ESP_GATTS_MTU_EVT:
            handleGattsEventMtu(gatts_if, param);
            break;
        case ESP_GATTS_CONNECT_EVT:
            handleGattsEventConnect(gatts_if, param);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            handleGattsEventDisconnect(gatts_if, param);
            break;
        default:
            ESP_LOGI(
                LOG_TAG,
                "GenericGattsApplication::gattsEventCallback(event=%d,gatts_if=%d)",
                (int)event,
                (int)gatts_if);
            throw std::runtime_error("not yet implemented");
    }
}

bool GenericGattsApplication::configurationDone(void) const
{
    return m_configurationDone == 0;
}

void GenericGattsApplication::handleGapEventRawAdvertisingDataSetComplete(void)
{
    setConfigurationAdvertisingDoneFlag();
    if (configurationDone())
    {
        esp_ble_gap_start_advertising(&advertisingParameters);
    }
}

void GenericGattsApplication::handleGapEventRawScanResponseDataSetComplete(void)
{
    setConfigurationScanResponseDoneFlag();
    if (configurationDone())
    {
        esp_ble_gap_start_advertising(&advertisingParameters);
    }
}

void GenericGattsApplication::handleGapEventAdvertisementStartComplete(esp_ble_gap_cb_param_t* param)
{
    if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS)
    {
        ESP_LOGI(LOG_TAG, "started advertising");
    }
    else
    {
        ESP_LOGE(LOG_TAG, "error starting advertising");
    }
}

void GenericGattsApplication::handleGapEventUpdatedConnectionParameters(esp_ble_gap_cb_param_t* param)
{
    ESP_LOGI(
        LOG_TAG,
        "UPDATED CONN PARAMS, status=%d, min_int=%d, max_int=%d, conn_int=%d, latency=%d, timeout=%d",
        param->update_conn_params.status,
        (param->update_conn_params.min_int * 125) / 100,
        (param->update_conn_params.max_int * 125) / 100,
        param->update_conn_params.conn_int,
        param->update_conn_params.latency,
        param->update_conn_params.timeout * 10);
}

void GenericGattsApplication::handleGattsEventConnect(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param)
{
    ESP_LOGI(
        LOG_TAG,
        "CONNECT, conn_id=%d, address=%02x:%02x:%02x:%02x:%02x:%02x",
        param->connect.conn_id,
        param->connect.remote_bda[0],
        param->connect.remote_bda[1],
        param->connect.remote_bda[2],
        param->connect.remote_bda[3],
        param->connect.remote_bda[4],
        param->connect.remote_bda[5]);

    esp_ble_conn_update_params_t connectionParameters;
    bzero(&connectionParameters, sizeof(connectionParameters));
    memcpy(connectionParameters.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
    /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
    connectionParameters.latency = 0;
    connectionParameters.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
    connectionParameters.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
    connectionParameters.timeout = 400;    // timeout = 400*10ms = 4000ms

    if (esp_ble_gap_update_conn_params(&connectionParameters) != ESP_OK)
    {
        ESP_LOGW(LOG_TAG, "Could not update connection parameters! Skipping.");
    }
}

void GenericGattsApplication::handleGattsEventDisconnect(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param)
{
    ESP_LOGI(
        LOG_TAG,
        "DISCONNECT, conn_id=%d, address=%02x:%02x:%02x:%02x:%02x:%02x, reason=0x%04x",
        param->disconnect.conn_id,
        param->disconnect.remote_bda[0],
        param->disconnect.remote_bda[1],
        param->disconnect.remote_bda[2],
        param->disconnect.remote_bda[3],
        param->disconnect.remote_bda[4],
        param->disconnect.remote_bda[5],
        param->disconnect.reason);

    if (configurationDone())
    {
        esp_ble_gap_start_advertising(&advertisingParameters);
    }
    else
    {
        ESP_LOGW(LOG_TAG, "not starting advertising again. Configuration not yet completed");
    }
}

void GenericGattsApplication::handleGattsEventMtu(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param)
{
    ESP_LOGI(LOG_TAG, "MTU, conn_id=%d, mtu=%d", param->mtu.conn_id, param->mtu.mtu);
}

void GenericGattsApplication::handleGattsEventRegister(void)
{
    if (esp_ble_gap_set_device_name(BLE_EXAMPLE_DEVICE_NAME) != ESP_OK)
    {
        throw std::runtime_error("error setting the device name");
    }

    if (esp_ble_gap_config_adv_data_raw(rawAdvertisingData, sizeof(rawAdvertisingData)) != ESP_OK)
    {
        throw std::runtime_error("error setting raw advertising data");
    }
    setConfigurationAdvertisingPendingFlag();

    if (esp_ble_gap_config_scan_rsp_data_raw(rawScanResponseData, sizeof(rawScanResponseData)) != ESP_OK)
    {
        throw std::runtime_error("error setting raw scan response data");
    }
    setConfigurationScanResponsePendingFlag();

    // TODO register table
}

void GenericGattsApplication::setConfigurationAdvertisingPendingFlag(void)
{
    m_configurationDone |= CONFIGURATION_ADVERTISING_PENDING;
}

void GenericGattsApplication::setConfigurationAdvertisingDoneFlag(void)
{
    m_configurationDone &= (~CONFIGURATION_ADVERTISING_PENDING);
}

void GenericGattsApplication::setConfigurationScanResponsePendingFlag(void)
{
    m_configurationDone |= CONFIGURATION_SCAN_RESPONSE_PENDING;
}

void GenericGattsApplication::setConfigurationScanResponseDoneFlag(void)
{
    m_configurationDone &= (~CONFIGURATION_SCAN_RESPONSE_PENDING);
}

} /* namespace Esp32 */