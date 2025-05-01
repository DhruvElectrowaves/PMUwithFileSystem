#include "stdlib.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "string.h"
#include "../../main/PMUFinalmain.h"
#include <inttypes.h>
// void convert_seconds_to_time(uint32_t total_seconds, timeDuration_t *timeStatus);
// Function to fetch the flag value from NVS
/*uint8_t fetch_keys() {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    publishMsgFlag.config_data = 1; // Default value if key is not found (indicating no exchange yet)

    // Open the NVS namespace
    err = nvs_open(CHARGERCONFIGKEYS, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(CHARGERCONFIGKEYS, "Error (%s) opening NVS handle for fetching flag!", esp_err_to_name(err));
        return publishMsgFlag.config_data;
    }

    uint8_t temp_value = 0;
    // Retrieve the flag value from NVS
    err = nvs_get_u8(nvs_handle, SEND_CONFIG_FLAG_KEY, &temp_value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(NVS_TAG, "Flag not found in NVS, setting to default value: %u", temp_value);
    } else if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Error (%s) retrieving flag from NVS!", esp_err_to_name(err));
    } else {
        ESP_LOGI(NVS_TAG, "Flag fetched from NVS: %u", temp_value);
    }
    // Toggle the value of send_config_data
    if (temp_value == 1) {
        publishMsgFlag.config_data = 0;
    } else {
        publishMsgFlag.config_data = 1;
    }

    // Close the NVS handle
    nvs_close(nvs_handle);

    return publishMsgFlag.config_data;
}*/
void convert_seconds_to_time(uint32_t total_seconds, timeDuration_t *timeStatus) {

    timeStatus->days = total_seconds / (24 * 3600);
    total_seconds %= (24 * 3600);

    timeStatus->hours = total_seconds / 3600;
    total_seconds %= 3600;

    timeStatus->minutes = total_seconds / 60;
    timeStatus->seconds = total_seconds % 60;
}

void fetch_keys(){
    nvs_handle_t nvs_handle;
    esp_err_t err; 

    // Open the NVS namespace
    err = nvs_open(CHARGERCONFIGKEYS, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(CHARGERCONFIGKEYS, "Error (%s) opening NVS handle for fetching flag!", esp_err_to_name(err));
        return;
    }

    uint8_t temp_value = 0;
    // Retrieve the flag value from NVS
    err = nvs_get_u8(nvs_handle, SEND_CONFIG_FLAG_KEY, &temp_value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(NVS_TAG, "Flag not found in NVS, using default value: %u", temp_value);
    } else if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Error (%s) retrieving flag from NVS!", esp_err_to_name(err));
    } else {
        ESP_LOGI(NVS_TAG, "Flag fetched from NVS: %u", temp_value);
    }
    // Toggle the value of send_config_data
    if(temp_value == 0)
    {
        //Data request Response havent been exchanged yet !!
        publishMsgFlag.config_data = 1;
        printf("config_data = %u", publishMsgFlag.config_data);
        publishMsgFlag.configDataExchanged = 0;
        memset(&flagConfig, 0xFF, sizeof(flagConfig)); // Set all bits to 1
       /* 
        ESP_LOGW("DEBUG-DHRUV", "numConnectorsFlag: %d", flagConfig.numConnectorsFlag);
        ESP_LOGW("DEBUG-DHRUV", "numPCMFlag: %d", flagConfig.numPCMFlag);
        ESP_LOGW("DEBUG-DHRUV", "powerInputTypeFlag: %d", flagConfig.powerInputTypeFlag);
        ESP_LOGW("DEBUG-DHRUV", "projectInfoFlag: %d", flagConfig.projectInfoFlag);

        // Parameter and Settings Flags
        ESP_LOGW("DEBUG-DHRUV", "parameterNameFlag: %d", flagConfig.parameterNameFlag);
        ESP_LOGW("DEBUG-DHRUV", "limitSettingParameterNameFlag: %d", flagConfig.limitSettingParameterNameFlag);
        ESP_LOGW("DEBUG-DHRUV", "configKeyValueFlag: %d", flagConfig.configKeyValueFlag);
        ESP_LOGW("DEBUG-DHRUV", "stringLengthFlag: %d", flagConfig.stringLengthFlag);

        // Billing and Brand Flags
        ESP_LOGW("DEBUG-DHRUV", "tarrifAmountFlag: %d", flagConfig.tarrifAmountFlag);
        ESP_LOGW("DEBUG-DHRUV", "BrandFlag: %d", flagConfig.BrandFlag);

        // Charging Control Flags
        ESP_LOGW("DEBUG-DHRUV", "AutoChargeFlag: %d", flagConfig.AutoChargeFlag);
        ESP_LOGW("DEBUG-DHRUV", "DynamicPowerSharingFlag: %d", flagConfig.DynamicPowerSharingFlag);
        ESP_LOGW("DEBUG-DHRUV", "EnergyMeterSettingFlag: %d", flagConfig.EnergyMeterSettingFlag);
        ESP_LOGW("DEBUG-DHRUV", "ZeroMeterStartEnableFlag: %d", flagConfig.ZeroMeterStartEnableFlag);

        // Serial Number Flags
        ESP_LOGW("DEBUG-DHRUV", "serialNumberFlag: %d", flagConfig.serialNumberFlag);
        ESP_LOGW("DEBUG-DHRUV", "serialNumber2Flag: %d", flagConfig.serialNumber2Flag);

        // Gun A Settings Flags
        ESP_LOGW("DEBUG-DHRUV", "gunAVoltageFlag: %d", flagConfig.gunAVoltageFlag);
        ESP_LOGW("DEBUG-DHRUV", "gunACurrentFlag: %d", flagConfig.gunACurrentFlag);
        ESP_LOGW("DEBUG-DHRUV", "gunAPowerFlag: %d", flagConfig.gunAPowerFlag);

        // Gun B Settings Flags
        ESP_LOGW("DEBUG-DHRUV", "gunBVoltageFlag: %d", flagConfig.gunBVoltageFlag);
        ESP_LOGW("DEBUG-DHRUV", "gunBCurrentFlag: %d", flagConfig.gunBCurrentFlag);
        ESP_LOGW("DEBUG-DHRUV", "gunBPowerFlag: %d", flagConfig.gunBPowerFlag);

        // Charger Power Flags
        ESP_LOGW("DEBUG-DHRUV", "chargerPowerFlag: %d", flagConfig.chargerPowerFlag);

        // Version Information Flags
        ESP_LOGW("DEBUG-DHRUV", "versionInfoFlag: %d", flagConfig.versionInfoFlag);
        ESP_LOGW("DEBUG-DHRUV", "commController1: %d", flagConfig.commController1);
        ESP_LOGW("DEBUG-DHRUV", "commController2: %d", flagConfig.commController2);
        ESP_LOGW("DEBUG-DHRUV", "hmiVersionFlag: %d", flagConfig.hmiVersionFlag);
        ESP_LOGW("DEBUG-DHRUV", "lcuVersionFlag: %d", flagConfig.lcuVersionFlag);
        ESP_LOGW("DEBUG-DHRUV", "mcuVersionFlag: %d", flagConfig.mcuVersionFlag);

        // Communication Settings Flags
        ESP_LOGW("DEBUG-DHRUV", "ocppURLFlag: %d", flagConfig.ocppURLFlag);
        ESP_LOGW("DEBUG-DHRUV", "wifi_ssidFlag: %d", flagConfig.wifi_ssidFlag);
        ESP_LOGW("DEBUG-DHRUV", "wifi_passwordFlag: %d", flagConfig.wifi_passwordFlag);
        ESP_LOGW("DEBUG-DHRUV", "mqtt_brokerFlag: %d", flagConfig.mqtt_brokerFlag);*/
    
    }
    else{
        publishMsgFlag.config_data = 0;
        publishMsgFlag.configDataExchanged = 1;
        configDataExchangedDetectedFromNVS = 1;
        memset(&flagConfig, 0, sizeof(flagConfig));
        ESP_LOGW("FLAG-Joshi", "Value of gunBCurrentFlag %d \n", flagConfig.gunBCurrentFlag);
        ESP_LOGW("FLAG-Joshi", "Value of AutoChargeFlag %d \n", flagConfig.AutoChargeFlag);
        ESP_LOGW("FLAG-Joshi", "Value of BrandFlag %d \n", flagConfig.BrandFlag);
        ESP_LOGW("FLAG-Joshi", "Value of chargerPowerFlag %d \n", flagConfig.chargerPowerFlag);
        ESP_LOGW("FLAG-Joshi", "Value of commController1 %d \n", flagConfig.commController1);
        ESP_LOGW("FLAG-Joshi", "Value of commController2 %d \n", flagConfig.commController2);
        ESP_LOGW("FLAG-Joshi", "Value of configKeyValueFlag %d \n", flagConfig.configKeyValueFlag);
        ESP_LOGW("FLAG-Joshi", "Value of ZeroMeterStartEnableFlag %d \n", flagConfig.ZeroMeterStartEnableFlag);
        ESP_LOGW("FLAG-Joshi", "Value of gunACurrentFlag %d \n", flagConfig.gunACurrentFlag);
    }
    // Close the NVS handle
    nvs_close(nvs_handle);
}

void open_ccg_namespace(){
    nvs_handle_t nvs_handle;
    esp_err_t err;   
    // Opening Configurations handle
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI(NVS_TAG, "Error opening NVS_NAMESPACE handle!\n");
        return;
    }
    err = nvs_set_u8(nvs_handle, NVS_NAMESPACE, 1);
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

void fetch_uptime_keys() {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS namespace: %s\n", esp_err_to_name(err));
        return;
    }

    // Read stored values with error handling
    uint32_t value;

    // // Read stored values (default to 0 if not found)
    // nvs_get_u32(my_handle, NVS_INTERNET_TOTAL_UP_TIME,  &LCUinternetStatus.totalUpTime);
    // nvs_get_u32(my_handle, NVS_INTERNET_TOTAL_DOWN_TIME, &LCUinternetStatus.totalDownTime);
    // nvs_get_u32(my_handle, NVS_WEBSOCKET_TOTAL_UP_TIME, &websocketStatus.totalUpTime);
    // nvs_get_u32(my_handle, NVS_WEBSOCKET_TOTAL_DOWN_TIME, &websocketStatus.totalDownTime);

    err = nvs_get_u32(my_handle, NVS_INTERNET_TOTAL_UP_TIME, &value);
    if (err == ESP_OK) {
        LCUinternetStatus.totalUpTime = value;
        convert_seconds_to_time(LCUinternetStatus.totalUpTime, &LCUinternetStatus.upTime);
        // printf("Days: %" PRIu32 "Hours: %" PRIu32 "Minutes: %" PRIu32 "Seconds: %" PRIu32 "\n",
        //     LCUinternetStatus.upTime.days, 
        //     LCUinternetStatus.upTime.hours,
        //     LCUinternetStatus.upTime.minutes, 
        //     LCUinternetStatus.upTime.seconds);
    } else {
        //printf("Failed to read NVS_INTERNET_TOTAL_UP_TIME: %s\n", esp_err_to_name(err));
    }

    err = nvs_get_u32(my_handle, NVS_INTERNET_TOTAL_DOWN_TIME, &value);
    if (err == ESP_OK) {
        LCUinternetStatus.totalDownTime = value;
        convert_seconds_to_time(LCUinternetStatus.totalDownTime, &LCUinternetStatus.downTime);
    } else {
        //printf("Failed to read NVS_INTERNET_TOTAL_DOWN_TIME: %s\n", esp_err_to_name(err));
    }

    err = nvs_get_u32(my_handle, NVS_WEBSOCKET_TOTAL_UP_TIME, &value);
    if (err == ESP_OK) {
        websocketStatus.totalUpTime = value;
        convert_seconds_to_time(websocketStatus.totalUpTime, &websocketStatus.upTime);
    } else {
        //printf("Failed to read NVS_WEBSOCKET_TOTAL_UP_TIME: %s\n", esp_err_to_name(err));
    }

    err = nvs_get_u32(my_handle, NVS_WEBSOCKET_TOTAL_DOWN_TIME, &value);
    if (err == ESP_OK) {
        websocketStatus.totalDownTime = value;
        convert_seconds_to_time(websocketStatus.totalDownTime, &websocketStatus.downTime);
    } else {
        //printf("Failed to read NVS_WEBSOCKET_TOTAL_DOWN_TIME: %s\n", esp_err_to_name(err));
    }

    // Close NVS
    nvs_close(my_handle);

    // Debug print values
    //printf("Fetched Values:\n");
    // printf("Internet Up Time: %lu seconds\n", LCUinternetStatus.totalUpTime);
    // printf("Internet Down Time: %lu seconds\n",LCUinternetStatus.totalDownTime);
    // printf("WebSocket Up Time: %lu seconds\n", websocketStatus.totalUpTime);
    // printf("WebSocket Down Time: %lu seconds\n", websocketStatus.totalDownTime);
}