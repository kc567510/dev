
/**
 * @file oam_yang_def.h
 * @brief This file contains definition of various yang name and path
 * @version 1.0
 * @date 2020-10-23
 * @copyright Copyright (C) 2019 Synergy Design Technology Limited
 *
 * This file is proprietary and subject to the terms and conditions
 * defined in file 'SYN_LICENSE.txt', which is part of this source
 * code package. If 'SYN_LICENSE.txt' is not present, corrupted,
 * or there are duplicates, no rights are granted and it is forbidden
 * to use this file in any way.
 **/

#ifndef _OAM_YANG_DEF_H_
#define _OAM_YANG_DEF_H_

/***************************************************************
 * Include
 ***************************************************************/
/***************************************************************
 * Define
 ***************************************************************/
// module
#define MODULE_3GPP_NR_NRM          "_3gpp_nr_nrm"
#define MODULE_3GPP_COMMON_FM       "_3gpp_common_fm"
#define MODULE_3GPP_COMMON_PM       "_3gpp_common_measurements"
#define MODULE_ORAN_MODULE_CAP      "o-ran-module-cap"
#define MODULE_ORAN_UPLANE_CONF     "o-ran-uplane-conf"
#define MODULE_ORAN_SYNC            "o-ran-sync"
#define MODULE_ORAN_DELAY_MGMT      "o-ran-delay-management"
#define MODULE_ORAN_PROCESS_ELEM    "o-ran-processing-element"
#define MODULE_IETF_INTERFACES      "ietf-interfaces"
#define MODULE_ORAN_FM				"o-ran-fm"

// container
#define CTNR_ME                     "ME"

// xpath
#define XPATH_NRM_GNBDU_FUNCTION            "/_3gpp_nr_nrm:ME/GNBDUFunction"
#define XPATH_NRM_GNBDU_FUNCTION_ITER       XPATH_NRM_GNBDU_FUNCTION"/*//."

#define XPATH_ORAN_MODULE_CAP               "/o-ran-module-cap:module-capability"
#define XPATH_ORAN_UPLANE_CONF              "/o-ran-uplane-conf:user-plane-configuration"
#define XPATH_ORAN_UPLANE_CONF_ITER         XPATH_ORAN_UPLANE_CONF"/*//."
#define XPATH_ORAN_SYNC                     "/o-ran-sync:sync"
#define XPATH_ORAN_DELAY_MGMT               "/o-ran-delay-management:delay-management"
#define XPATH_ORAN_PROCESS_ELEM             "/o-ran-processing-element:processing-elements"
#define XPATH_IETF_INTERFACES               "/ietf-interfaces:interfaces"

#define XPATH_NRM_ALARM_LIST                "/_3gpp_nr_nrm:ME/AlarmList[id='0']"
#define XPATH_ORAN_STATIC_TX                XPATH_ORAN_UPLANE_CONF"/static-low-level-tx-endpoints"

// rpc
#define RPC_FM_GET_ALARM_COUNT_XPATH        "/_3gpp_common_fm:getAlarmCount"
#define RPC_FM_GET_ALARM_COUNT_OUTPUTNUM    6

// notification
#define NOTI_FM_NEW_ALARM                   "notifyNewAlarm"
#define NOTI_FM_CHANGED_ALARM               "notifyChangedAlarm"
#define NOTI_FM_CLEARED_ALARM               "notifyClearedAlarm"
#define NOTI_PM_FILE_READY                  "notifyFileReady"

#define NOTI_FM_NEW_ALARM_XPATH             "/_3gpp_common_fm:notifyNewAlarm"
#define NOTI_FM_CHANGED_ALARM_XPATH         "/_3gpp_common_fm:notifyChangedAlarm"
#define NOTI_FM_CLEARED_ALARM_XPATH         "/_3gpp_common_fm:notifyClearedAlarm"
#define NOTI_PM_FILE_READY_XPATH            "/_3gpp_common_measurements:notifyFileReady"
#define NOTI_SYNC_SYNC_STATE_CHANGE_XPATH   "/o-ran-sync:synchronization-state-change"
#define NOTI_SYNC_PTP_STATE_CHANGE_XPATH    "/o-ran-sync:ptp-state-change"
#define NOTI_SYNC_SYNCE_STATE_CHANGE_XPATH  "/o-ran-sync:synce-state-change"
#define NOTI_SYNC_GNSS_STATE_CHANGE_XPATH   "/o-ran-sync:gnss-state-change"

#define NOTI_PM_FILE_READY_objectClass       "NF"
#define NOTI_PM_FILE_READY_objectInstance    "0"

//SWM
#define SW_UPGRADE_UCI_FILE                  "/usr/local/fwdownload/job/fw_job"
#define SW_UPGRADE_NOTIFY_TRIGGER            "/usr/local/fwdownload/trigger"

//LOG
#define SYSLOG_PATH                          "/var/log/syslog"

//FM
#define FM_NOTIFY_TRIGGER                    "/usr/local/fm_report"

// length
#define XPATH_STR_MAX_LEN                   256
#define Mxpath                              128
// "/_3gpp_nr_nrm:ME/AlarmList[id='0']/AlarmRecordGrp[alarmId='CELL_ALARM_CELL_L1_ERROR_IND']/objectClass_objectInstance"

// STR
#define ORAN_STR_true                     "true"
#define ORAN_STR_false                    "false"
#define ORAN_STR_HOLDOVER                 "HOLDOVER"
#define ORAN_STR_FREERUN                  "FREERUN"
#define ORAN_STR_LOCKED                   "LOCKED"                // clock is synchronizing
#define ORAN_STR_UNLOCKED                 "UNLOCKED"              // clock is not synchronizing
#define ORAN_STR_SYNCHRONIZED             "SYNCHRONIZED"          // GNSS functionality is synchronized
#define ORAN_STR_ACQUIRING_SYNC           "ACQUIRING-SYNC"        // GNSS functionality is acquiring sync
#define ORAN_STR_ANTENNA_DISCONNECTED     "ANTENNA-DISCONNECTED"  // GNSS functionality has its antenna disconnected
#define ORAN_STR_BOOTING                  "BOOTING"               // GNSS functionality is booting
#define ORAN_STR_ANTENNA_SHORT_CIRCUIT    "ANTENNA-SHORT-CIRCUIT" // GNSS functionality has an antenna short circuit
#define ORAN_STR_NONE                     "NONE"
#define ORAN_STR_PRACH                    "PRACH"
#define ORAN_STR_SRS                      "SRS"
#define ORAN_STR_NON_MANAGED              "NON_MANAGED"
#define ORAN_STR_MANAGED                  "MANAGED"
#define ORAN_STR_BOTH                     "BOTH"

/***************************************************************
 * Struct
 ***************************************************************/
/***************************************************************
 * Function
 ***************************************************************/

#endif/*_OAM_YANG_DEF_H_*/

