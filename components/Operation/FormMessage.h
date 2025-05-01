#ifndef FORM_MESSAGE_H
#define FORM_MESSAGE_H

#include <stdint.h>
#include "../../main/PMUFinalmain.h"
#include "ValidateMessage.h"
#include "cJSON.h"

extern char* create_json_call_string(const char *action,  cJSON *payload);
extern char* create_json_call_result_string(const char *messageId,  cJSON *payload);
extern void send_json_call_error(const char *messageId, ErrorMessageCodeEnumType error_codes);
// extern char* create_pcmPeriodic_payload_ocpp_string(const char *action,  cJSON *payload);
// extern char* create_config_payload_ocpp_string(const char *action,  cJSON *payload);


#endif // FORM_MESSAGE_H