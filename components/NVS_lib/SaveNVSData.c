#include "stdlib.h"
#include "../../main/PMUFinalmain.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "string.h"
#include "esp_mac.h"


void save_data_to_NVS(const char *namespace_name, const char *key, void *value, NVSDatatypes datatype) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Open the NVS namespace
    err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return;
    }

    // Write the value based on the data type
    switch (datatype) {
        case TYPE_BOOL_:
        case NVS_TYPE_U8:
            err = nvs_set_u8(nvs_handle, key, *(uint8_t *)value);
            break;
        case NVS_TYPE_U16:
            err = nvs_set_u16(nvs_handle, key, *(uint16_t *)value);
            break;
        case NVS_TYPE_STR:
            err = nvs_set_str(nvs_handle, key, (const char *)value);
            break;
        case NVS_TYPE_U64:
            err = nvs_set_u64(nvs_handle, key, *(uint64_t *)value);
            break;

        case NVS_TYPE_U32:
            err = nvs_set_u32(nvs_handle, key, *(uint32_t *)value);
            break;

        case NVS_TYPE_I32:
            err = nvs_set_i32(nvs_handle, key, *(int32_t *)value);
            break;
            
        default:
            ESP_LOGE(NVS_TAG, "Unsupported data type");
            nvs_close(nvs_handle);
            return;
    }

    // Check for errors
    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Error writing to NVS: %s", esp_err_to_name(err));
    } else {
        // Commit written value
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(NVS_TAG, "Error committing changes to NVS: %s", esp_err_to_name(err));
        }
    }

    // Close the NVS handle
    nvs_close(nvs_handle);
}