/*
 * Copyright (C) 2022, Linaro Ltd.
 * Author: Caleb Connolly <caleb.connolly@linaro.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * Provides friendly wrappers around common QMI sequences
 * and some complicated messages
 */
#include <errno.h>
#include <linux/qrtr.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "q_log.h"
#include "libqrtr.h"
#include "libqril_private.h"

#include "list.h"
#include "util.h"

#include "qmi_uim.h"
#include "qmi_dpm.h"
#include "qmi_dms.h"
#include "qmi_nas.h"
#include "qmi_wda.h"
#include "qmi_wds.h"

int libqril_dms_get_operating_mode(enum QmiDmsOperatingMode *mode)
{
	struct dms_get_operating_mode_req req = DMS_GET_OPERATING_MODE_REQ_INITIALIZER;
	struct dms_get_operating_mode_resp resp = DMS_GET_OPERATING_MODE_RESP_INITIALIZER;

	int rc = libqril_send_message_sync(&req.hdr, &resp.hdr);
	if (rc)
		return rc;

	*mode = resp.mode;

	return 0;
}

int libqril_dms_set_operating_mode(enum QmiDmsOperatingMode mode)
{
	struct dms_set_operating_mode_req req = DMS_SET_OPERATING_MODE_REQ_INITIALIZER;

	req.mode = mode;

	return libqril_send_message_sync(&req.hdr, NULL);
}

int libqril_get_modem_info(struct libqril_modem_info *info)
{
	struct dms_get_revision_resp rev_resp = DMS_GET_REVISION_RESP_INITIALIZER;
	struct dms_get_ids_resp ids_resp = DMS_GET_IDS_RESP_INITIALIZER;
	int rc;

	if (!info)
		return -EINVAL;

	rc = libqril_send_basic_request_sync(QMI_SERVICE_DMS, QMI_DMS_GET_REVISION, &rev_resp.hdr);
	if (rc)
		return rc;

	memcpy(info->revision, rev_resp.revision, rev_resp.revision_len);

	rc = libqril_send_basic_request_sync(QMI_SERVICE_DMS, QMI_DMS_GET_IDS, &ids_resp.hdr);
	if (rc)
		return rc;

	memcpy(info->esn, ids_resp.esn, ids_resp.esn_len);
	memcpy(info->imei, ids_resp.imei, ids_resp.imei_len);
	memcpy(info->imei_ver, ids_resp.imei_ver, ids_resp.imei_ver_len);
	memcpy(info->meid, ids_resp.meid, ids_resp.meid_len);

	return 0;
}

int libqril_modem_activate()
{
	enum QmiDmsOperatingMode mode;
	int rc;

	rc = libqril_dms_get_operating_mode(&mode);
	if (rc < 0)
		return rc;
	if (mode == QMI_DMS_OPERATING_MODE_ONLINE)
		return 0;

	return libqril_dms_set_operating_mode(QMI_DMS_OPERATING_MODE_ONLINE);
}

const struct enum_value qmi_error_names[];

const char *libqril_qmi_error_string(int err)
{
	const struct enum_value *v = &qmi_error_names[0];
	while (v->value_str) {
		if (v->value == err)
			return v->value_str;
		v++;
	}

	return "<UNKNOWN>";
}

const struct enum_value qmi_error_names[] = {
	{ QMI_ERR_NONE, "QMI_ERR_NONE", "" },
	{ QMI_ERR_MALFORMED_MESSAGE, "QMI_ERR_MALFORMED_MESSAGE", "" },
	{ QMI_ERR_NO_MEMORY, "QMI_ERR_NO_MEMORY", "" },
	{ QMI_ERR_INTERNAL, "QMI_ERR_INTERNAL", "" },
	{ QMI_ERR_ABORTED, "QMI_ERR_ABORTED", "" },
	{ QMI_ERR_CLIENT_IDS_EXHAUSTED, "QMI_ERR_CLIENT_IDS_EXHAUSTED", "" },
	{ QMI_ERR_UNABORTABLE_TRANSACTION, "QMI_ERR_UNABORTABLE_TRANSACTION", "" },
	{ QMI_ERR_INVALID_CLIENT_ID, "QMI_ERR_INVALID_CLIENT_ID", "" },
	{ QMI_ERR_NO_THRESHOLDS_PROVIDED, "QMI_ERR_NO_THRESHOLDS_PROVIDED", "" },
	{ QMI_ERR_INVALID_HANDLE, "QMI_ERR_INVALID_HANDLE", "" },
	{ QMI_ERR_INVALID_PROFILE, "QMI_ERR_INVALID_PROFILE", "" },
	{ QMI_ERR_INVALID_PIN_ID, "QMI_ERR_INVALID_PIN_ID", "" },
	{ QMI_ERR_INCORRECT_PIN, "QMI_ERR_INCORRECT_PIN", "" },
	{ QMI_ERR_NO_NETWORK_FOUND, "QMI_ERR_NO_NETWORK_FOUND", "" },
	{ QMI_ERR_CALL_FAILED, "QMI_ERR_CALL_FAILED", "" },
	{ QMI_ERR_OUT_OF_CALL, "QMI_ERR_OUT_OF_CALL", "" },
	{ QMI_ERR_NOT_PROVISIONED, "QMI_ERR_NOT_PROVISIONED", "" },
	{ QMI_ERR_MISSING_ARGUMENT, "QMI_ERR_MISSING_ARGUMENT", "" },
	{ QMI_ERR_ARGUMENT_TOO_LONG, "QMI_ERR_ARGUMENT_TOO_LONG", "" },
	{ QMI_ERR_INVALID_TRANSACTION_ID, "QMI_ERR_INVALID_TRANSACTION_ID", "" },
	{ QMI_ERR_DEVICE_IN_USE, "QMI_ERR_DEVICE_IN_USE", "" },
	{ QMI_ERR_NETWORK_UNSUPPORTED, "QMI_ERR_NETWORK_UNSUPPORTED", "" },
	{ QMI_ERR_DEVICE_UNSUPPORTED, "QMI_ERR_DEVICE_UNSUPPORTED", "" },
	{ QMI_ERR_NO_EFFECT, "QMI_ERR_NO_EFFECT", "" },
	{ QMI_ERR_NO_FREE_PROFILE, "QMI_ERR_NO_FREE_PROFILE", "" },
	{ QMI_ERR_INVALID_PDP_TYPE, "QMI_ERR_INVALID_PDP_TYPE", "" },
	{ QMI_ERR_INVALID_TECHNOLOGY_PREFERENCE, "QMI_ERR_INVALID_TECHNOLOGY_PREFERENCE", "" },
	{ QMI_ERR_INVALID_PROFILE_TYPE, "QMI_ERR_INVALID_PROFILE_TYPE", "" },
	{ QMI_ERR_INVALID_SERVICE_TYPE, "QMI_ERR_INVALID_SERVICE_TYPE", "" },
	{ QMI_ERR_INVALID_REGISTER_ACTION, "QMI_ERR_INVALID_REGISTER_ACTION", "" },
	{ QMI_ERR_INVALID_PS_ATTACH_ACTION, "QMI_ERR_INVALID_PS_ATTACH_ACTION", "" },
	{ QMI_ERR_AUTHENTICATION_FAILED, "QMI_ERR_AUTHENTICATION_FAILED", "" },
	{ QMI_ERR_PIN_BLOCKED, "QMI_ERR_PIN_BLOCKED", "" },
	{ QMI_ERR_PIN_ALWAYS_BLOCKED, "QMI_ERR_PIN_ALWAYS_BLOCKED", "" },
	{ QMI_ERR_UIM_UNINITIALIZED, "QMI_ERR_UIM_UNINITIALIZED", "" },
	{ QMI_ERR_MAXIMUM_QOS_REQUESTS_IN_USE, "QMI_ERR_MAXIMUM_QOS_REQUESTS_IN_USE", "" },
	{ QMI_ERR_INCORRECT_FLOW_FILTER, "QMI_ERR_INCORRECT_FLOW_FILTER", "" },
	{ QMI_ERR_NETWORK_QOS_UNAWARE, "QMI_ERR_NETWORK_QOS_UNAWARE", "" },
	{ QMI_ERR_INVALID_QOS_ID, "QMI_ERR_INVALID_QOS_ID", "" },
	{ QMI_ERR_REQUESTED_NUMBER_UNSUPPORTED, "QMI_ERR_REQUESTED_NUMBER_UNSUPPORTED", "" },
	{ QMI_ERR_INTERFACE_NOT_FOUND, "QMI_ERR_INTERFACE_NOT_FOUND", "" },
	{ QMI_ERR_FLOW_SUSPENDED, "QMI_ERR_FLOW_SUSPENDED", "" },
	{ QMI_ERR_INVALID_DATA_FORMAT, "QMI_ERR_INVALID_DATA_FORMAT", "" },
	{ QMI_ERR_GENERAL_ERROR, "QMI_ERR_GENERAL_ERROR", "" },
	{ QMI_ERR_UNKNOWN_ERROR, "QMI_ERR_UNKNOWN_ERROR", "" },
	{ QMI_ERR_INVALID_ARGUMENT, "QMI_ERR_INVALID_ARGUMENT", "" },
	{ QMI_ERR_INVALID_INDEX, "QMI_ERR_INVALID_INDEX", "" },
	{ QMI_ERR_NO_ENTRY, "QMI_ERR_NO_ENTRY", "" },
	{ QMI_ERR_DEVICE_STORAGE_FULL, "QMI_ERR_DEVICE_STORAGE_FULL", "" },
	{ QMI_ERR_DEVICE_NOT_READY, "QMI_ERR_DEVICE_NOT_READY", "" },
	{ QMI_ERR_NETWORK_NOT_READY, "QMI_ERR_NETWORK_NOT_READY", "" },
	{ QMI_ERR_WMS_CAUSE_CODE, "QMI_ERR_WMS_CAUSE_CODE", "" },
	{ QMI_ERR_WMS_MESSAGE_NOT_SENT, "QMI_ERR_WMS_MESSAGE_NOT_SENT", "" },
	{ QMI_ERR_WMS_MESSAGE_DELIVERY_FAILURE, "QMI_ERR_WMS_MESSAGE_DELIVERY_FAILURE", "" },
	{ QMI_ERR_WMS_INVALID_MESSAGE_ID, "QMI_ERR_WMS_INVALID_MESSAGE_ID", "" },
	{ QMI_ERR_WMS_ENCODING, "QMI_ERR_WMS_ENCODING", "" },
	{ QMI_ERR_AUTHENTICATION_LOCK, "QMI_ERR_AUTHENTICATION_LOCK", "" },
	{ QMI_ERR_INVALID_TRANSITION, "QMI_ERR_INVALID_TRANSITION", "" },
	{ QMI_ERR_NOT_MCAST_INTERFACE, "QMI_ERR_NOT_MCAST_INTERFACE", "" },
	{ QMI_ERR_MAXIMUM_MCAST_REQUESTS_IN_USE, "QMI_ERR_MAXIMUM_MCAST_REQUESTS_IN_USE", "" },
	{ QMI_ERR_INVALID_MCAST_HANDLE, "QMI_ERR_INVALID_MCAST_HANDLE", "" },
	{ QMI_ERR_INVALID_IP_FAMILY_PREFERENCE, "QMI_ERR_INVALID_IP_FAMILY_PREFERENCE", "" },
	{ QMI_ERR_SESSION_INACTIVE, "QMI_ERR_SESSION_INACTIVE", "" },
	{ QMI_ERR_SESSION_INVALID, "QMI_ERR_SESSION_INVALID", "" },
	{ QMI_ERR_SESSION_OWNERSHIP, "QMI_ERR_SESSION_OWNERSHIP", "" },
	{ QMI_ERR_INSUFFICIENT_RESOURCES, "QMI_ERR_INSUFFICIENT_RESOURCES", "" },
	{ QMI_ERR_DISABLED, "QMI_ERR_DISABLED", "" },
	{ QMI_ERR_INVALID_OPERATION, "QMI_ERR_INVALID_OPERATION", "" },
	{ QMI_ERR_INVALID_QMI_COMMAND, "QMI_ERR_INVALID_QMI_COMMAND", "" },
	{ QMI_ERR_WMS_T_PDU_TYPE, "QMI_ERR_WMS_T_PDU_TYPE", "" },
	{ QMI_ERR_WMS_SMSC_ADDRESS, "QMI_ERR_WMS_SMSC_ADDRESS", "" },
	{ QMI_ERR_INFORMATION_UNAVAILABLE, "QMI_ERR_INFORMATION_UNAVAILABLE", "" },
	{ QMI_ERR_SEGMENT_TOO_LONG, "QMI_ERR_SEGMENT_TOO_LONG", "" },
	{ QMI_ERR_SEGMENT_ORDER, "QMI_ERR_SEGMENT_ORDER", "" },
	{ QMI_ERR_BUNDLING_NOT_SUPPORTED, "QMI_ERR_BUNDLING_NOT_SUPPORTED", "" },
	{ QMI_ERR_OPERATION_PARTIAL_FAILURE, "QMI_ERR_OPERATION_PARTIAL_FAILURE", "" },
	{ QMI_ERR_POLICY_MISMATCH, "QMI_ERR_POLICY_MISMATCH", "" },
	{ QMI_ERR_SIM_FILE_NOT_FOUND, "QMI_ERR_SIM_FILE_NOT_FOUND", "" },
	{ QMI_ERR_EXTENDED_INTERNAL, "QMI_ERR_EXTENDED_INTERNAL", "" },
	{ QMI_ERR_ACCESS_DENIED, "QMI_ERR_ACCESS_DENIED", "" },
	{ QMI_ERR_HARDWARE_RESTRICTED, "QMI_ERR_HARDWARE_RESTRICTED", "" },
	{ QMI_ERR_ACK_NOT_SENT, "QMI_ERR_ACK_NOT_SENT", "" },
	{ QMI_ERR_INJECT_TIMEOUT, "QMI_ERR_INJECT_TIMEOUT", "" },
	{ QMI_ERR_INCOMPATIBLE_STATE, "QMI_ERR_INCOMPATIBLE_STATE", "" },
	{ QMI_ERR_FDN_RESTRICT, "QMI_ERR_FDN_RESTRICT", "" },
	{ QMI_ERR_SUPS_FAILURE_CASE, "QMI_ERR_SUPS_FAILURE_CASE", "" },
	{ QMI_ERR_NO_RADIO, "QMI_ERR_NO_RADIO", "" },
	{ QMI_ERR_NOT_SUPPORTED, "QMI_ERR_NOT_SUPPORTED", "" },
	{ QMI_ERR_NO_SUBSCRIPTION, "QMI_ERR_NO_SUBSCRIPTION", "" },
	{ QMI_ERR_CARD_CALL_CONTROL_FAILED, "QMI_ERR_CARD_CALL_CONTROL_FAILED", "" },
	{ QMI_ERR_NETWORK_ABORTED, "QMI_ERR_NETWORK_ABORTED", "" },
	{ QMI_ERR_MSG_BLOCKED, "QMI_ERR_MSG_BLOCKED", "" },
	{ QMI_ERR_INVALID_SESSION_TYPE, "QMI_ERR_INVALID_SESSION_TYPE", "" },
	{ QMI_ERR_INVALID_PB_TYPE, "QMI_ERR_INVALID_PB_TYPE", "" },
	{ QMI_ERR_NO_SIM, "QMI_ERR_NO_SIM", "" },
	{ QMI_ERR_PB_NOT_READY, "QMI_ERR_PB_NOT_READY", "" },
	{ QMI_ERR_PIN_RESTRICTION, "QMI_ERR_PIN_RESTRICTION", "" },
	{ QMI_ERR_PIN2_RESTRICTION, "QMI_ERR_PIN2_RESTRICTION", "" },
	{ QMI_ERR_PUK_RESTRICTION, "QMI_ERR_PUK_RESTRICTION", "" },
	{ QMI_ERR_PUK2_RESTRICTION, "QMI_ERR_PUK2_RESTRICTION", "" },
	{ QMI_ERR_PB_ACCESS_RESTRICTED, "QMI_ERR_PB_ACCESS_RESTRICTED", "" },
	{ QMI_ERR_PB_DELETE_IN_PROGRESS, "QMI_ERR_PB_DELETE_IN_PROGRESS", "" },
	{ QMI_ERR_PB_TEXT_TOO_LONG, "QMI_ERR_PB_TEXT_TOO_LONG", "" },
	{ QMI_ERR_PB_NUMBER_TOO_LONG, "QMI_ERR_PB_NUMBER_TOO_LONG", "" },
	{ QMI_ERR_PB_HIDDEN_KEY_RESTRICTION, "QMI_ERR_PB_HIDDEN_KEY_RESTRICTION", "" },
	{ QMI_ERR_PB_NOT_AVAILABLE, "QMI_ERR_PB_NOT_AVAILABLE", "" },
	{ QMI_ERR_DEVICE_MEMORY_ERROR, "QMI_ERR_DEVICE_MEMORY_ERROR", "" },
	{ QMI_ERR_NO_PERMISSION, "QMI_ERR_NO_PERMISSION", "" },
	{ QMI_ERR_TOO_SOON, "QMI_ERR_TOO_SOON", "" },
	{ QMI_ERR_TIME_NOT_ACQUIRED, "QMI_ERR_TIME_NOT_ACQUIRED", "" },
	{ QMI_ERR_OPERATION_IN_PROGRESS, "QMI_ERR_OPERATION_IN_PROGRESS", "" },
	{ QMI_ERR_FW_WRITE_FAILED, "QMI_ERR_FW_WRITE_FAILED", "" },
	{ QMI_ERR_FW_INFO_READ_FAILED, "QMI_ERR_FW_INFO_READ_FAILED", "" },
	{ QMI_ERR_FW_FILE_NOT_FOUND, "QMI_ERR_FW_FILE_NOT_FOUND", "" },
	{ QMI_ERR_FW_DIR_NOT_FOUND, "QMI_ERR_FW_DIR_NOT_FOUND", "" },
	{ QMI_ERR_FW_ALREADY_ACTIVATED, "QMI_ERR_FW_ALREADY_ACTIVATED", "" },
	{ QMI_ERR_FW_CANNOT_GENERIC_IMAGE, "QMI_ERR_FW_CANNOT_GENERIC_IMAGE", "" },
	{ QMI_ERR_FW_FILE_OPEN_FAILED, "QMI_ERR_FW_FILE_OPEN_FAILED", "" },
	{ QMI_ERR_FW_UPDATE_DISCONTINUOUS_FRAME, "QMI_ERR_FW_UPDATE_DISCONTINUOUS_FRAME", "" },
	{ QMI_ERR_FW_UPDATE_FAILED, "QMI_ERR_FW_UPDATE_FAILED", "" },
	{ QMI_ERR_CAT_EVENT_REGISTRATION_FAILED, "QMI_ERR_CAT_EVENT_REGISTRATION_FAILED", "" },
	{ QMI_ERR_CAT_INVALID_TERMINAL_RESPONSE, "QMI_ERR_CAT_INVALID_TERMINAL_RESPONSE", "" },
	{ QMI_ERR_CAT_INVALID_ENVELOPE_COMMAND, "QMI_ERR_CAT_INVALID_ENVELOPE_COMMAND", "" },
	{ QMI_ERR_CAT_ENVELOPE_COMMAND_BUSY, "QMI_ERR_CAT_ENVELOPE_COMMAND_BUSY", "" },
	{ QMI_ERR_CAT_ENVELOPE_COMMAND_FAILED, "QMI_ERR_CAT_ENVELOPE_COMMAND_FAILED", "" },
	{0, NULL, NULL },
};
