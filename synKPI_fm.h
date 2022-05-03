/**
*@file synKPI_fm.h
*@brief This file contains functions to FM DB
*@version 1.0
*@date 2021-11-16
*@copyright Copyright (C) 2019 Synergy Design Technology Limited
*
*This file is proprietary and subject to the terms and conditions
*defined in file 'SYN_LICENSE.txt', which is part of this source
*code package. If 'SYN_LICENSE.txt' is not present, corrupted,
*or there are duplicates, no rights are granted and it is forbidden
* to use this file in any way.
**/

#ifndef HAVE_SYNKPI_FM_H
#define HAVE_SYNKPI_FM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>
#include <synKPI_def.h>
#include <oam_oran_fm.h>
#include <syslog.h>
/***************************************************************
* Define
***************************************************************/
#define SYNKPI_FM_SOURCE_NAME_MAX_LEN      (31)

typedef enum
{
  /* sync fault */
  FM_SYNC_FAULT_FIRST = 0,
  FM_SYNC_NO_EXT_SYNC_SRC = FM_SYNC_FAULT_FIRST,
  FM_SYNC_SYNC_ERROR,
  FM_SYNC_FAULT_MAX,
  /* ofh fault */
  FM_OFH_FAULT_FIRST = FM_SYNC_FAULT_MAX,
  FM_OFH_TXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY = FM_OFH_FAULT_FIRST,
  FM_OFH_TXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT,
  FM_OFH_RXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY,
  FM_OFH_RXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT,

  FM_OFH_TXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY,
  FM_OFH_TXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT,
  FM_OFH_RXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY,
  FM_OFH_RXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT,

  FM_OFH_TXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY,
  FM_OFH_TXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT,
  FM_OFH_RXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY,
  FM_OFH_RXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT,

  FM_OFH_TXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY,
  FM_OFH_TXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT,
  FM_OFH_RXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY,
  FM_OFH_RXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT,

  FM_OFH_DEVICE_REBOOT,
  FM_OFH_DEVICE_HIGH_TEMPERATURE,
  FM_OFH_DEVICE_HIGH_CPU_USAGE,
  FM_OFH_DEVICE_LOW_SPACE,
  FM_OFH_DEVICE_LOW_MEMORY,
  FM_OFH_DEVICE_NET_COMM_DOWN,
  
  

  FM_OFH_FAULT_MAX,
  /* other fault */

  /* MAX fault */
  FM_SOURCE_FAULT_MAX = FM_OFH_FAULT_MAX,
} E_SYNKPI_FM_FAULT_ID;

typedef enum
{
  FM_SOURCE_SYNC = 0,
  FM_SOURCE_OFH_TXLINK_1,
  FM_SOURCE_OFH_RXLINK_1,
  FM_SOURCE_OFH_TXLINK_2,
  FM_SOURCE_OFH_RXLINK_2,
  FM_SOURCE_OFH_TXLINK_3,
  FM_SOURCE_OFH_RXLINK_3,
  FM_SOURCE_OFH_TXLINK_4,
  FM_SOURCE_OFH_RXLINK_4,
  FM_SOURCE_OFH_OAM_AGENT,
  FM_SOURCE_MAX
} E_SYNKPI_FM_SOURCE_ID;

typedef struct
{
  bool is_raised;
  bool is_cleared;
  bool is_notified;
  uint64_t sn;
  uint16_t source_id;
  uint16_t fault_id;
  uint8_t n_affected_objects;
  E_SYNKPI_FM_SOURCE_ID affected_objects[FM_SOURCE_MAX];
  E_ORAN_FM_FAULT_SEVERITY fault_severity;
  uint8_t fault_text[ORAN_FM_FAULT_TEXT_MAX_LEN+1];
  uint8_t event_time[ORAN_FM_DATE_AND_TIME_MAX_LEN+1];
} S_SYNKPI_FM_ALARM;

typedef struct
{
  E_ORAN_FM_FAULT_SEVERITY fault_severity;
  uint8_t event_time[ORAN_FM_DATE_AND_TIME_MAX_LEN+1];
  uint8_t fault_text[ORAN_FM_FAULT_TEXT_MAX_LEN+1];
} S_SYNKPI_FM_ALARM_REQ;

typedef struct
{
  char name[SYNKPI_FM_SOURCE_NAME_MAX_LEN+1];
} S_SYNKPI_FM_SOURCE;

typedef struct
{
  uint64_t alarm_sn;
  S_SYNKPI_FM_SOURCE source[FM_SOURCE_MAX];
  S_SYNKPI_FM_ALARM alarm[FM_SOURCE_FAULT_MAX];
} S_SYNKPI_FM;

S_SYNKPI_FM* 	synKPI_fmPtrGet(void);
int8_t 				synKPI_fmReset(void);

/***************************************************************
* Function
***************************************************************/

/**
*@brief synKPI_fmTimeNowGet
*
*@details
*
*@param[in] buffer
*
*@return void
**/
void synKPI_fmTimeNowGet(char *buffer);

/**
*@brief synKPI_fmInit
*
*@details
*
*@param[in] pFm
*@param[in] bReinit
*
*@return [0] success [others] fail
**/
int8_t synKPI_fmInit(
    S_SYNKPI_FM  *pFm,
    int          bReinit);

/**
*@brief synKPI_fmSeverityNameGet
*
*@details
*
*@param[in] severityId
*
*@return name string
**/
char *synKPI_fmSeverityNameGet(
    E_ORAN_FM_FAULT_SEVERITY severityId);

/**
*@brief synKPI_fmSourceNameGet
*
*@details
*
*@param[in] sourceId
*
*@return name string
**/
char *synKPI_fmSourceNameGet(
    uint8_t sourceId);

/**
*@brief synKPI_fmAlarmLog
*
*@details
*
*@param[in] alarm_sn
*@param[in] pAlarm
*
*@return void
**/
void synKPI_fmAlarmLog(
    uint64_t alarm_sn,
    S_SYNKPI_FM_ALARM *pAlarm);

/**
*@brief synKPI_fmAlarmIsRaisedGet
*
*@details
*
*@param[in] alarmIdx
*
*@return true or false
**/
bool synKPI_fmAlarmIsRaisedGet(
    E_SYNKPI_FM_FAULT_ID  alarmIdx);

/**
*@brief synKPI_fmAlarmRaise
*
*@details
*
*@param[in] alarmIdx
*@param[in] pAlarm
*
*@return [0] success [others] fail
**/
int8_t synKPI_fmAlarmRaise(
    E_SYNKPI_FM_FAULT_ID  alarmIdx,
    S_SYNKPI_FM_ALARM_REQ *pAlarm);

/**
*@brief synKPI_fmAlarmClear
*
*@details
*
*@param[in] alarmIdx
*@param[in] pAlarm
*
*@return [0] success [others] fail
**/
int8_t synKPI_fmAlarmClear(
    E_SYNKPI_FM_FAULT_ID  alarmIdx,
    S_SYNKPI_FM_ALARM_REQ *pAlarm);

/**
*@brief synKPI_fmAlarmRemove
*
*@details
*
*@param[in] alarmIdx
*
*@return [0] success [others] fail
**/
int8_t synKPI_fmAlarmRemove(
    E_SYNKPI_FM_FAULT_ID  alarmIdx);

#ifdef __cplusplus
}
#endif

#endif //HAVE_SYNKPI_FM_H

