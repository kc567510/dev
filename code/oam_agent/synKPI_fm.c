/**
*@file synKPI_fm.c
*@brief This file contains functions to fm db
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

/***************************************************************
* Include
***************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <synKPI_def.h>
#include <synKPI_msgLog.h>
#include <synKPI_if.h>
#include <synKPI_fm.h>
#include <synKPI_synPlat.h>

/***************************************************************
* Definition
***************************************************************/
#define PRINTF printf
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

/***************************************************************
* Local
***************************************************************/
static S_SYNKPI_FM  *g_pFm = NULL;

/***************************************************************
* Gloable
***************************************************************/
/**
*@brief synKPI_fmTimeNowGet
*
*@details
*
*@param[in] time
*
*@return void
**/
void synKPI_fmTimeNowGet(char *timestr)
{
  time_t rawtime;
  struct tm* timeinfo = NULL;
  char buffer[36]={0};
  char tz_str1[4]={0};
  char tz_str2[4]={0};
  rawtime = time(NULL);
  timeinfo = localtime(&rawtime);
  if (timeinfo && timestr)
  {
    //strftime format = yyyy-mm-ddThh:mm:ss+xxxx
    strftime(buffer, 50, "%FT%H:%M:%S%z", timeinfo);

    //need format     = yyyy-mm-ddThh:mm:ss+xx:xx
    memcpy(tz_str1, &buffer[20], 2);
    memcpy(tz_str2, &buffer[22], 2);
    buffer[20] = '\0';
    sprintf(timestr, "%s%s:%s", buffer, tz_str1, tz_str2);
  }
}

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
    int           bReinit)
{
  int i;
  syslog(LOG_INFO, "<%s,%d> synKPI_fmInit, pFm= %x, bReinit= %d\n",__FUNCTION__,__LINE__,pFm, bReinit);
  if (pFm == NULL)
  {
    return -1;
  }

  g_pFm = pFm;

  if (bReinit == 0)
    return 0;

  #if 1 // enable for test
  synKPI_msgLogLevelSet(SYNKPI_MSGLOGL_M5);
  synKPI_msgLogStatusSet(KPI_MSG_LOG_WRITEFILE);
  #endif

  pFm->alarm_sn = 0;
  /*
   * Init source name
   */
  strcpy(pFm->source[FM_SOURCE_SYNC].name,         "sync");
  strcpy(pFm->source[FM_SOURCE_OFH_TXLINK_1].name, "txlink1");
  strcpy(pFm->source[FM_SOURCE_OFH_RXLINK_1].name, "rxlink1");
  strcpy(pFm->source[FM_SOURCE_OFH_TXLINK_2].name, "txlink2");
  strcpy(pFm->source[FM_SOURCE_OFH_RXLINK_2].name, "rxlink2");
  strcpy(pFm->source[FM_SOURCE_OFH_TXLINK_3].name, "txlink3");
  strcpy(pFm->source[FM_SOURCE_OFH_RXLINK_3].name, "rxlink3");
  strcpy(pFm->source[FM_SOURCE_OFH_TXLINK_4].name, "txlink4");
  strcpy(pFm->source[FM_SOURCE_OFH_RXLINK_4].name, "rxlink4");
  strcpy(pFm->source[FM_SOURCE_OFH_OAM_AGENT].name, "oamAgent");

  /*
   * Init alarm list info
   */
  { // SYNC
    /* FM_SYNC_NO_EXT_SYNC_SRC */
    pFm->alarm[FM_SYNC_NO_EXT_SYNC_SRC].source_id = FM_SOURCE_SYNC;
    pFm->alarm[FM_SYNC_NO_EXT_SYNC_SRC].fault_id = ORAN_FM_NO_EXTERNAL_SYNC_SOURCE;
    pFm->alarm[FM_SYNC_NO_EXT_SYNC_SRC].n_affected_objects = FM_SOURCE_MAX; // affect all of source
    for (i=0;i<FM_SOURCE_MAX;i++)
    {
      pFm->alarm[FM_SYNC_NO_EXT_SYNC_SRC].affected_objects[i] = i;
    }

    /* FM_SYNC_SYNC_ERROR */
    pFm->alarm[FM_SYNC_SYNC_ERROR].source_id = FM_SOURCE_SYNC;
    pFm->alarm[FM_SYNC_SYNC_ERROR].fault_id = ORAN_FM_SYNCHRONIZATION_ERROR;
    pFm->alarm[FM_SYNC_SYNC_ERROR].n_affected_objects = FM_SOURCE_MAX; // affect all of source
    for (i=0;i<FM_SOURCE_MAX;i++)
    {
      pFm->alarm[FM_SYNC_SYNC_ERROR].affected_objects[i] = i;
    }
  }

  { // TRX_1
    /* FM_OFH_TXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY */
    pFm->alarm[FM_OFH_TXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY].source_id = FM_SOURCE_OFH_TXLINK_1;
    pFm->alarm[FM_OFH_TXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY].fault_id = ORAN_FM_CU_PLANE_LOGICAL_CONNECTION_FAULTY;
    pFm->alarm[FM_OFH_TXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_TXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY].affected_objects[0] = FM_SOURCE_OFH_TXLINK_1;

    /* FM_OFH_TXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT */
    pFm->alarm[FM_OFH_TXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].source_id = FM_SOURCE_OFH_TXLINK_1;
    pFm->alarm[FM_OFH_TXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].fault_id = ORAN_FM_UNEXPECTED_CU_PLANE_MESSSAGE_CONTENT_FAULT;
    pFm->alarm[FM_OFH_TXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_TXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].affected_objects[0] = FM_SOURCE_OFH_TXLINK_1;

    /* FM_OFH_RXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY */
    pFm->alarm[FM_OFH_RXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY].source_id = FM_SOURCE_OFH_RXLINK_1;
    pFm->alarm[FM_OFH_RXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY].fault_id = ORAN_FM_CU_PLANE_LOGICAL_CONNECTION_FAULTY;
    pFm->alarm[FM_OFH_RXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_RXLINK_1_CUPLANE_LOGICAL_CONN_FAULTY].affected_objects[0] = FM_SOURCE_OFH_RXLINK_1;

    /* FM_OFH_RXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT */
    pFm->alarm[FM_OFH_RXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].source_id = FM_SOURCE_OFH_RXLINK_1;
    pFm->alarm[FM_OFH_RXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].fault_id = ORAN_FM_UNEXPECTED_CU_PLANE_MESSSAGE_CONTENT_FAULT;
    pFm->alarm[FM_OFH_RXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_RXLINK_1_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].affected_objects[0] = FM_SOURCE_OFH_RXLINK_1;
  }

  { // TRX_2
    /* FM_OFH_TXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY */
    pFm->alarm[FM_OFH_TXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY].source_id = FM_SOURCE_OFH_TXLINK_2;
    pFm->alarm[FM_OFH_TXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY].fault_id = ORAN_FM_CU_PLANE_LOGICAL_CONNECTION_FAULTY;
    pFm->alarm[FM_OFH_TXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_TXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY].affected_objects[0] = FM_SOURCE_OFH_TXLINK_2;

    /* FM_OFH_TXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT */
    pFm->alarm[FM_OFH_TXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].source_id = FM_SOURCE_OFH_TXLINK_2;
    pFm->alarm[FM_OFH_TXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].fault_id = ORAN_FM_UNEXPECTED_CU_PLANE_MESSSAGE_CONTENT_FAULT;
    pFm->alarm[FM_OFH_TXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_TXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].affected_objects[0] = FM_SOURCE_OFH_TXLINK_2;

    /* FM_OFH_RXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY */
    pFm->alarm[FM_OFH_RXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY].source_id = FM_SOURCE_OFH_RXLINK_2;
    pFm->alarm[FM_OFH_RXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY].fault_id = ORAN_FM_CU_PLANE_LOGICAL_CONNECTION_FAULTY;
    pFm->alarm[FM_OFH_RXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_RXLINK_2_CUPLANE_LOGICAL_CONN_FAULTY].affected_objects[0] = FM_SOURCE_OFH_RXLINK_2;

    /* FM_OFH_RXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT */
    pFm->alarm[FM_OFH_RXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].source_id = FM_SOURCE_OFH_RXLINK_2;
    pFm->alarm[FM_OFH_RXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].fault_id = ORAN_FM_UNEXPECTED_CU_PLANE_MESSSAGE_CONTENT_FAULT;
    pFm->alarm[FM_OFH_RXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_RXLINK_2_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].affected_objects[0] = FM_SOURCE_OFH_RXLINK_2;
  }

  { // TRX_3
    /* FM_OFH_TXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY */
    pFm->alarm[FM_OFH_TXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY].source_id = FM_SOURCE_OFH_TXLINK_3;
    pFm->alarm[FM_OFH_TXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY].fault_id = ORAN_FM_CU_PLANE_LOGICAL_CONNECTION_FAULTY;
    pFm->alarm[FM_OFH_TXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_TXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY].affected_objects[0] = FM_SOURCE_OFH_TXLINK_3;

    /* FM_OFH_TXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT */
    pFm->alarm[FM_OFH_TXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].source_id = FM_SOURCE_OFH_TXLINK_3;
    pFm->alarm[FM_OFH_TXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].fault_id = ORAN_FM_UNEXPECTED_CU_PLANE_MESSSAGE_CONTENT_FAULT;
    pFm->alarm[FM_OFH_TXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_TXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].affected_objects[0] = FM_SOURCE_OFH_TXLINK_3;

    /* FM_OFH_RXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY */
    pFm->alarm[FM_OFH_RXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY].source_id = FM_SOURCE_OFH_RXLINK_3;
    pFm->alarm[FM_OFH_RXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY].fault_id = ORAN_FM_CU_PLANE_LOGICAL_CONNECTION_FAULTY;
    pFm->alarm[FM_OFH_RXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_RXLINK_3_CUPLANE_LOGICAL_CONN_FAULTY].affected_objects[0] = FM_SOURCE_OFH_RXLINK_3;

    /* FM_OFH_RXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT */
    pFm->alarm[FM_OFH_RXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].source_id = FM_SOURCE_OFH_RXLINK_3;
    pFm->alarm[FM_OFH_RXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].fault_id = ORAN_FM_UNEXPECTED_CU_PLANE_MESSSAGE_CONTENT_FAULT;
    pFm->alarm[FM_OFH_RXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_RXLINK_3_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].affected_objects[0] = FM_SOURCE_OFH_RXLINK_3;
  }

  { // TRX_4
    /* FM_OFH_TXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY */
    pFm->alarm[FM_OFH_TXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY].source_id = FM_SOURCE_OFH_TXLINK_4;
    pFm->alarm[FM_OFH_TXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY].fault_id = ORAN_FM_CU_PLANE_LOGICAL_CONNECTION_FAULTY;
    pFm->alarm[FM_OFH_TXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_TXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY].affected_objects[0] = FM_SOURCE_OFH_TXLINK_4;

    /* FM_OFH_TXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT */
    pFm->alarm[FM_OFH_TXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].source_id = FM_SOURCE_OFH_TXLINK_4;
    pFm->alarm[FM_OFH_TXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].fault_id = ORAN_FM_UNEXPECTED_CU_PLANE_MESSSAGE_CONTENT_FAULT;
    pFm->alarm[FM_OFH_TXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_TXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].affected_objects[0] = FM_SOURCE_OFH_TXLINK_4;

    /* FM_OFH_RXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY */
    pFm->alarm[FM_OFH_RXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY].source_id = FM_SOURCE_OFH_RXLINK_4;
    pFm->alarm[FM_OFH_RXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY].fault_id = ORAN_FM_CU_PLANE_LOGICAL_CONNECTION_FAULTY;
    pFm->alarm[FM_OFH_RXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_RXLINK_4_CUPLANE_LOGICAL_CONN_FAULTY].affected_objects[0] = FM_SOURCE_OFH_RXLINK_4;

    /* FM_OFH_RXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT */
    pFm->alarm[FM_OFH_RXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].source_id = FM_SOURCE_OFH_RXLINK_4;
    pFm->alarm[FM_OFH_RXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].fault_id = ORAN_FM_UNEXPECTED_CU_PLANE_MESSSAGE_CONTENT_FAULT;
    pFm->alarm[FM_OFH_RXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].n_affected_objects = 1; // affect itself
    pFm->alarm[FM_OFH_RXLINK_4_UNEXPECTED_CUPLANE_MSG_CONTENT_FAULT].affected_objects[0] = FM_SOURCE_OFH_RXLINK_4;
  }

  //FM_OFH_DEVICE_REBOOT
  pFm->alarm[FM_OFH_DEVICE_REBOOT].source_id = FM_SOURCE_OFH_OAM_AGENT;
  pFm->alarm[FM_OFH_DEVICE_REBOOT].fault_id = ORAN_FM_RESET_REQUESTED;
  pFm->alarm[FM_OFH_DEVICE_REBOOT].n_affected_objects = 1; // affect itself
  pFm->alarm[FM_OFH_DEVICE_REBOOT].affected_objects[0] = FM_SOURCE_OFH_OAM_AGENT;
  
  //FM_OFH_DEVICE_HIGH_TEMPERATURE
  pFm->alarm[FM_OFH_DEVICE_HIGH_TEMPERATURE].source_id = FM_SOURCE_OFH_OAM_AGENT;
  pFm->alarm[FM_OFH_DEVICE_HIGH_TEMPERATURE].fault_id = ORAN_FM_RESET_REQUESTED;
  pFm->alarm[FM_OFH_DEVICE_HIGH_TEMPERATURE].n_affected_objects = 1; // affect itself
  pFm->alarm[FM_OFH_DEVICE_HIGH_TEMPERATURE].affected_objects[0] = FM_SOURCE_OFH_OAM_AGENT;
  
  //FM_OFH_DEVICE_HIGH_CPU_USAGE
  pFm->alarm[FM_OFH_DEVICE_HIGH_CPU_USAGE].source_id = FM_SOURCE_OFH_OAM_AGENT;
  pFm->alarm[FM_OFH_DEVICE_HIGH_CPU_USAGE].fault_id = ORAN_FM_RESET_REQUESTED;
  pFm->alarm[FM_OFH_DEVICE_HIGH_CPU_USAGE].n_affected_objects = 1; // affect itself
  pFm->alarm[FM_OFH_DEVICE_HIGH_CPU_USAGE].affected_objects[0] = FM_SOURCE_OFH_OAM_AGENT;
  
  //FM_OFH_DEVICE_LOW_SPACE
  pFm->alarm[FM_OFH_DEVICE_LOW_SPACE].source_id = FM_SOURCE_OFH_OAM_AGENT;
  pFm->alarm[FM_OFH_DEVICE_LOW_SPACE].fault_id = ORAN_FM_RESET_REQUESTED;
  pFm->alarm[FM_OFH_DEVICE_LOW_SPACE].n_affected_objects = 1; // affect itself
  pFm->alarm[FM_OFH_DEVICE_LOW_SPACE].affected_objects[0] = FM_SOURCE_OFH_OAM_AGENT;
  
  //FM_OFH_DEVICE_LOW_MEMORY
  pFm->alarm[FM_OFH_DEVICE_LOW_MEMORY].source_id = FM_SOURCE_OFH_OAM_AGENT;
  pFm->alarm[FM_OFH_DEVICE_LOW_MEMORY].fault_id = ORAN_FM_RESET_REQUESTED;
  pFm->alarm[FM_OFH_DEVICE_LOW_MEMORY].n_affected_objects = 1; // affect itself
  pFm->alarm[FM_OFH_DEVICE_LOW_MEMORY].affected_objects[0] = FM_SOURCE_OFH_OAM_AGENT;
  
  //FM_OFH_DEVICE_NET_COMM_DOWN
  pFm->alarm[FM_OFH_DEVICE_NET_COMM_DOWN].source_id = FM_SOURCE_OFH_OAM_AGENT;
  pFm->alarm[FM_OFH_DEVICE_NET_COMM_DOWN].fault_id = ORAN_FM_RESET_REQUESTED;
  pFm->alarm[FM_OFH_DEVICE_NET_COMM_DOWN].n_affected_objects = 1; // affect itself
  pFm->alarm[FM_OFH_DEVICE_NET_COMM_DOWN].affected_objects[0] = FM_SOURCE_OFH_OAM_AGENT;


  for (i=0; i<FM_SOURCE_FAULT_MAX;i++)
  {
    pFm->alarm[i].sn = 0;
    pFm->alarm[i].is_raised = false;
    pFm->alarm[i].is_cleared = false;
    pFm->alarm[i].is_notified = false;
    memset(pFm->alarm[i].fault_text, 0, sizeof(pFm->alarm[i].fault_text));
    memset(pFm->alarm[i].event_time, 0, sizeof(pFm->alarm[i].event_time));
  }

  return 0;
}

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
    E_ORAN_FM_FAULT_SEVERITY severityId)
{
  static char *severity_str[] = { "NONE", "CRITICAL", "MAJOR", "MINOR", "WARNING" };
  return severity_str[severityId];
}

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
    uint8_t sourceId)
{
  char *name;
  if (g_pFm == NULL)
    return NULL;

  if (sourceId < FM_SOURCE_MAX)
  {
    name = g_pFm->source[sourceId].name;
  }
  else
  {
    name = "UNKNOWN";
  }
  return name;
}

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
    S_SYNKPI_FM_ALARM *pAlarm)
{
  char tmpBuf[512]={0};
  char alarmTag[2] = {0};

  if (pAlarm->is_cleared == false)
  {
    strcpy(alarmTag,"R");
  }
  else
  {
    strcpy(alarmTag,"C");
  }
  // event_time alarm_sn [Raise/Clear][SourceName][FaultId][SN][Severity] fault_text
  snprintf(tmpBuf, sizeof(tmpBuf), "%s %4lu [%s][%s][%d][%lu][%s] %s\n",
    pAlarm->event_time,
    alarm_sn,
    alarmTag,
    synKPI_fmSourceNameGet(pAlarm->source_id),
    pAlarm->fault_id,
    pAlarm->sn,
    synKPI_fmSeverityNameGet(pAlarm->fault_severity),
    pAlarm->fault_text);

  E_SYNKPI_MSGLOGL logl;
  if (pAlarm->fault_severity == ORAN_FM_FAULT_SEVERITY_CRITICAL)
  {
    logl = SYNKPI_MSGLOGL_CRI;
  }
  else if (pAlarm->fault_severity == ORAN_FM_FAULT_SEVERITY_MAJOR)
  {
    logl = SYNKPI_MSGLOGL_MAJ;
  }
  else if (pAlarm->fault_severity == ORAN_FM_FAULT_SEVERITY_MINOR)
  {
    logl = SYNKPI_MSGLOGL_MIN;
  }
  else
  {
    logl = SYNKPI_MSGLOGL_WRN;
  }
  synKPI_msgLog(SYNKPI_MSGLOGM_ALARM, logl, "%s", tmpBuf);
}

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
    E_SYNKPI_FM_FAULT_ID  alarmIdx)
{
  return g_pFm->alarm[alarmIdx].is_raised;
}

/**
*@brief synKPI_fmAlarmRaise
*
*@details
*
*@param[in] alarmIdx
*@param[in] pAlarm
*
*@return [0] success [others] fail
*@return [-1] null point
*@return [1] parameter error
*@return [2] alarm notification program is not ready
*@return [3] alarm already raised
**/
int8_t synKPI_fmAlarmRaise(
    E_SYNKPI_FM_FAULT_ID  alarmIdx,
    S_SYNKPI_FM_ALARM_REQ *pAlarm)
{
  //E_ORAN_FM_FAULT_SEVERITY local_severity;
  syslog(LOG_INFO, "<%s,%d> alarmIdx = %d\n",__FUNCTION__,__LINE__, alarmIdx);
  if (g_pFm == NULL)
  {
    printf("synKPI null point!\n");
    return -1;
  }

  if (alarmIdx > FM_SOURCE_FAULT_MAX)
  {
    printf("Alarm index unknown!\n");
    return 1;
  }

  if ((pAlarm->fault_severity < ORAN_FM_FAULT_SEVERITY_CRITICAL) ||
      (pAlarm->fault_severity > ORAN_FM_FAULT_SEVERITY_WARNING))
  {
    printf("Alarm severity unknown!\n");
    return 1;
  }

  if (synKPI_synplatStatusGet() == 0)
  {
    printf("Alarm notification program is not ready! Can't process alarm.\n");
    return 2;
  }
  /*
  local_severity = g_pFm->alarm[alarmIdx].fault_severity;
  if (g_pFm->alarm[alarmIdx].is_raised == true)
  {
    printf("Alarm already raised with severity = %s. Raise alarm failed.\n", synKPI_fmSeverityNameGet(local_severity));
    return 3;
  }
  */

  /* 1: update alarm data (time & text & severity) */
  g_pFm->alarm[alarmIdx].sn = g_pFm->alarm_sn;
  memcpy(g_pFm->alarm[alarmIdx].event_time, pAlarm->event_time ,sizeof(g_pFm->alarm[alarmIdx].event_time));
  memcpy(g_pFm->alarm[alarmIdx].fault_text, pAlarm->fault_text ,sizeof(g_pFm->alarm[alarmIdx].fault_text));
  g_pFm->alarm[alarmIdx].fault_severity = pAlarm->fault_severity;

  /* 2: update alarm flag */
  g_pFm->alarm[alarmIdx].is_raised = true;
  g_pFm->alarm[alarmIdx].is_notified = false;

  /* 3: log this alarm */
  synKPI_fmAlarmLog(g_pFm->alarm_sn, &g_pFm->alarm[alarmIdx]);

  /* 4: update sn (it will trigger notify check) */
  g_pFm->alarm_sn++;

  return 0;
}

/**
*@brief synKPI_fmAlarmClear
*
*@details
*
*@param[in] alarmIdx
*@param[in] pAlarm
*
*@return [0] success [others] fail
*@return [-1] null point
*@return [1] parameter error
*@return [2] alarm notification program is not ready
*@return [3] alarm already cleared
**/
int8_t synKPI_fmAlarmClear(
    E_SYNKPI_FM_FAULT_ID  alarmIdx,
    S_SYNKPI_FM_ALARM_REQ *pAlarm)
{
  if (g_pFm == NULL)
  {
    printf("synKPI null point!\n");
    return -1;
  }

  if (alarmIdx > FM_SOURCE_FAULT_MAX)
  {
    printf("Unknown alarm index!\n");
    return 1;
  }

  if (synKPI_synplatStatusGet() == 0)
  {
    printf("Alarm notification program is not ready! Can't process alarm.\n");
    return 2;
  }

  if ((g_pFm->alarm[alarmIdx].is_cleared == true) ||
      (g_pFm->alarm[alarmIdx].is_raised == false))
  {
    printf("Alarm already cleared. Clear alarm failed.\n");
    return 3;
  }

  /* 1: update alarm data (time) */
  memcpy(g_pFm->alarm[alarmIdx].event_time, pAlarm->event_time ,sizeof(g_pFm->alarm[alarmIdx].event_time));

  /* 2: update alarm flag */
  g_pFm->alarm[alarmIdx].is_cleared = true;
  g_pFm->alarm[alarmIdx].is_notified = false;

  /* 3: log this alarm */
  synKPI_fmAlarmLog(g_pFm->alarm_sn, &g_pFm->alarm[alarmIdx]);

  /* 4: update sn (it will trigger notify check) */
  g_pFm->alarm_sn++;

  return 0;
}

/**
*@brief synKPI_fmAlarmRemove
*
*@details
*
*@param[in] alarmIdx
*
*@return [0] success [others] fail
*@return [-1] null point
*@return [1] unknown alarm index
**/
int8_t synKPI_fmAlarmRemove(
    E_SYNKPI_FM_FAULT_ID  alarmIdx)
{
  if (g_pFm == NULL)
  {
    printf("synKPI null point!\n");
    return -1;
  }

  if (alarmIdx > FM_SOURCE_FAULT_MAX)
  {
    printf("Unknown alarm index!\n");
    return 1;
  }

  if (synKPI_synplatStatusGet() == 0)
  {
    printf("Alarm notification program is not ready! Can't process alarm.\n");
    return 2;
  }

  /* 1: reset alarm data */
  g_pFm->alarm[alarmIdx].sn = 0;
  memset(g_pFm->alarm[alarmIdx].fault_text, 0, sizeof(g_pFm->alarm[alarmIdx].fault_text));
  memset(g_pFm->alarm[alarmIdx].event_time, 0, sizeof(g_pFm->alarm[alarmIdx].event_time));
  g_pFm->alarm[alarmIdx].fault_severity = ORAN_FM_FAULT_SEVERITY_NONE;

  /* 2: reset alarm flag */
  g_pFm->alarm[alarmIdx].is_raised = false;
  g_pFm->alarm[alarmIdx].is_cleared = false;
  g_pFm->alarm[alarmIdx].is_notified = false;

  return 0;
}