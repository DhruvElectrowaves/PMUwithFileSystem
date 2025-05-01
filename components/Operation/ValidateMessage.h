#ifndef VALIDATE_MESSAGE_H
#define VALIDATE_MESSAGE_H

#include <stdint.h>
#include <stddef.h>



/* MessageType ID */
#define CALL 2
#define CALL_RESULT 3
#define CALL_ERROR 4

// Define ErrorMessageCodeEnumType 
typedef enum {
    VALID_MSG = 0,
    JSON_ARRAY_NOT_FOUND,
    INVALID_JSON_FORMAT,
    JSON_ARRAY_ELEMENTS_OUT_OF_RANGE,
    INVALID_MESSAGE_TYPE_ID,
    INVALID_MESSAGE_ID,
    INVALID_ACTION,
    INVALID_PAYLOAD,
    COMPONENT_STATUS_PAYLOAD_ERROR,
    PERIODIC_DATA_PAYLOAD_ERROR,
    CONFIGURATION_DATA_PAYLOAD_ERROR,
    CHARGING_SESSION_PAYLOAD_ERROR,
    FAULTLOG_PAYLOAD_ERROR,
    REQ_RES_PAIR_NOT_FOUND,
    NOT_SUPPORTED,
    COMPONENT_NOT_SUPPORTED,
    ID_NOT_SUPPORTED,
    INVALID_MSG = 255,
    
} ErrorMessageCodeEnumType;

extern ErrorMessageCodeEnumType error;

// Define ConnectorStatusEnumType
typedef enum {
    CALL_ =4,
    CALL_RESULT_= 3, 
    CALL_ERROR_ = 5,
} OCPPMSGEnumType;

extern ErrorMessageCodeEnumType validate_json_string(char *msg, char **error_uuid);
// extern ErrorMessageCodeEnumType error;
#endif // VALIDATE_MESSAGE_H