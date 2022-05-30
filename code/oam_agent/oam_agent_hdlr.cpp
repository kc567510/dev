/*!*****************************************************************************************************************************
 * \file         oam_agent_hdlr.cpp
 *
 * \brief        This source file contains functions for oam  ConfD logic
 ******************************************************************************************************************************/
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>

#include "oam_agent_hdlr.h"
#include "oam_confd_types.h"
#include "tinyxml2.h"
#include "swm_inventory.h"
#include "swm_download.h"
#include "swm_install.h"
#include "swm_activate.h"
#include <utime.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include "oam_yang_def.h"
#include "oam_o-ran-fm.h"
#define RESET_SWMGR_SH				"/usr/local/fwdownload/swmgr_reset.sh"
#define SWMGR_LOACTION				"/usr/sbin/swmgr"

static uint64_t local_alarm_sn = 0;
/*!
 * \namespace   oam
 * \brief       This namespace is used for LIB OAM AGENT ConfD implementation
 */
namespace oam_agent
{
    oam_agent_hdlr* oam_agent_hdlr::instance = nullptr;
    /*!
     * \fn       deallocated_and_reset_config_request
     * \brief    This function is used to free and reset the config request local variable in the class
     * \return   void
     */
    void oam_agent_hdlr::deallocated_and_reset_config_request()
    {
        if (nullptr != local_config_request)
        {
            //delete local_config_request;
            local_config_request = nullptr;
        }
    }

    /*!
     * \fn   allocated_and_return_config_request
     * \brief    This function is used to allocate and store the config request local variable in the class
     * \return   void
     */
    oam::config_request* oam_agent_hdlr::allocated_and_return_config_request()
    {
        if (nullptr != local_config_request)
        {
            deallocated_and_reset_config_request();
        }

        local_config_request = new oam::config_request();
        return local_config_request;
    }

    /*!
     * \fn   close_confd_subscription_connection
     * \brief    This function is used to close the ConfD connection
     * \return   void
     */
    void oam_agent_hdlr::close_confd_subscription_connection()
    {
        std::cout << "close_confd_subscription_connection" << std::endl;
        //cdb_close(confd_subs_sock);
        sr_disconnect(gSrCdb.subs_conn);
    }

    /*!
     * \fn   close_confd_read_connection
     * \brief    This function is used to close the ConfD connection
     * \return   void
     */
    void oam_agent_hdlr::close_confd_read_connection()
    {
        //cdb_close(confd_read_sock);
        sr_disconnect(gSrCdb.connection);
    }

    /*!
     * \fn   change_ietf_intf_cb
     * \brief    This function is the callback function for dynamic config (interfaces)
     * \return   sr_error_t
     */
    static int
    change_ietf_intf_cb(sr_session_ctx_t *session, const char *module_name, const char *xpath,
            sr_event_t event, uint32_t request_id, void *private_data)
    {
      //(void)session;
      (void)private_data;
      //(void)event;
      //(void)request_id;

      //printf("\n\n ========== CHANGE \"%s\" RECEIVED =======================\n\n", xpath);
      //printf("module: %s, request_id = %d\n", module_name, request_id);

      if (event == SR_EV_DONE)
      {
        oam_agent::oam_agent_hdlr::get_instance()->static_cfg_read();
      }
      return SR_ERR_OK;
    }

    /*!
     * \fn   change_oran_process_elem_cb
     * \brief    This function is the callback function for dynamic config (processing-elements)
     * \return   sr_error_t
     */
    static int
    change_oran_process_elem_cb(sr_session_ctx_t *session, const char *module_name, const char *xpath,
            sr_event_t event, uint32_t request_id, void *private_data)
    {
      //(void)session;
      (void)private_data;
      //(void)event;
      //(void)request_id;

      //printf("\n\n ========== CHANGE \"%s\" RECEIVED =======================\n\n", xpath);
      //printf("module: %s, request_id = %d\n", module_name, request_id);

      if (event == SR_EV_DONE)
      {
        oam_agent::oam_agent_hdlr::get_instance()->static_cfg_read();
      }
      return SR_ERR_OK;
    }

    /*!
     * \fn   change_oran_delay_mgmt_cb
     * \brief    This function is the callback function for dynamic config (delay-management)
     * \return   sr_error_t
     */
    static int
    change_oran_delay_mgmt_cb(sr_session_ctx_t *session, const char *module_name, const char *xpath,
            sr_event_t event, uint32_t request_id, void *private_data)
    {
      //(void)session;
      (void)private_data;
      //(void)event;
      //(void)request_id;

      //printf("\n\n ========== CHANGE \"%s\" RECEIVED =======================\n\n", xpath);
      //printf("module: %s, request_id = %d\n", module_name, request_id);

      if (event == SR_EV_DONE)
      {
        oam_agent::oam_agent_hdlr::get_instance()->static_cfg_read();
      }
      return SR_ERR_OK;
    }

    /*!
     * \fn   change_oran_uplane_conf_cb
     * \brief    This function is the callback function for dynamic config (user-plane-configuration)
     * \return   sr_error_t
     */
    static int
    change_oran_uplane_conf_cb(sr_session_ctx_t *session, const char *module_name, const char *xpath,
            sr_event_t event, uint32_t request_id, void *private_data)
    {
      //(void)session;
      (void)private_data;
      //(void)event;
      //(void)request_id;

      //printf("\n\n ========== CHANGE \"%s\" RECEIVED =======================\n\n", xpath);
      //printf("module: %s, request_id = %d\n", module_name, request_id);
      #if 0
      if (event == SR_EV_DONE)
      {
        int ret;
        sr_change_iter_t *iter;
        sr_change_oper_t op;
        sr_val_t *old_val, *new_val;

        /* get changes iter */
        ret = sr_get_changes_iter(session, XPATH_ORAN_UPLANE_CONF_ITER, &iter);
        if (ret != SR_ERR_OK)
        {
            printf("[sr_get_changes_iter] error\n");
            return -1;
        }

        ret = sr_get_change_next(session, iter, &op, &old_val, &new_val);
        t_cfg_action action = T_CFG_CREATE;
        while (ret == SR_ERR_OK)
        {
            switch (op)
            {
                case SR_OP_CREATED:
                    printf("[ SR_OP_CREATED ]\n");
                    printf("%s =\n", new_val->xpath);
                    cdb_print_val(new_val);
                    oam_agent::oam_agent_hdlr::get_instance()->dynamic_cfg_read(XPATH_ORAN_UPLANE_CONF, action, op, new_val);
                    sr_free_val(new_val);
                    break;
                case SR_OP_MODIFIED:
                    printf("[ SR_OP_MODIFIED ]\n");
                    printf("%s =\n", new_val->xpath);
                    printf("old = ");
                    cdb_print_val(old_val);
                    printf("\nnew = ");
                    cdb_print_val(new_val);
                    printf("\n");
                    oam_agent::oam_agent_hdlr::get_instance()->dynamic_cfg_read(XPATH_ORAN_UPLANE_CONF, action, op, new_val);
                    sr_free_val(old_val);
                    sr_free_val(new_val);
                    break;
                case SR_OP_DELETED:
                    printf("[ SR_OP_DELETED ]\n");
                    break;
                case SR_OP_MOVED:
                    printf("[ SR_OP_MOVED ]\n");
                    break;
                default:
                    break;
            }

            ret = sr_get_change_next(session, iter, &op, &old_val, &new_val);
            action = T_CFG_APPEND;
        }
        sr_free_change_iter(iter);
        oam_agent::oam_agent_hdlr::get_instance()->dynamic_cfg_read(XPATH_ORAN_UPLANE_CONF, T_CFG_SEND, op, nullptr);
      }
      #else

      if (event == SR_EV_DONE)
      {
        oam_agent::oam_agent_hdlr::get_instance()->static_cfg_read();
      }
      #endif
      return SR_ERR_OK;
    }

    /*!
     * \fn   ORANModuleCapGetCb
     * \brief    This function is the callback function for get operational data of module-capability
     * \return   sr_error_t
     */
    static int
    ORANModuleCapGetCb(sr_session_ctx_t *session, const char *module_name, const char *xpath, const char *request_xpath,
                     uint32_t request_id, struct lyd_node **parent, void *private_data)
    {
      (void)session;
      (void)request_xpath;
      (void)request_id;
      (void)private_data;

      //printf("\n\n ========== DATA FOR \"%s\" \"%s\" REQUESTED =======================\n\n", module_name, xpath);
      //printf("Request: xpath = %s, id = %d\n", request_xpath, request_id);
      #if 1
      const struct ly_ctx *ctx = sr_get_context(sr_session_get_connection(session));

      char m_xPath[XPATH_STR_MAX_LEN] = {0};
      char m_leaf_xPath[XPATH_STR_MAX_LEN] = {0};
      *parent = lyd_new_path(nullptr, ctx, XPATH_ORAN_MODULE_CAP "/ru-capabilities",
                              nullptr, LYD_ANYDATA_CONSTSTRING, 0);
      // ru-capabilities
      {
        // number-of-spatial-streams
        {
          uint8_t m_ant_num = 1;
          char num_value[32] = {0};
          sprintf(num_value, "%d", m_ant_num);
          memset(m_xPath, 0 ,sizeof(m_xPath));
          sprintf(m_xPath, XPATH_ORAN_MODULE_CAP "/ru-capabilities/number-of-spatial-streams");
          lyd_new_path(*parent, ctx, m_xPath,
                      (void *)num_value, LYD_ANYDATA_CONSTSTRING, 0);
        }
      }
      // band-capabilities
      {
        uint16_t m_band_number = 1;
        memset(m_xPath, 0 ,sizeof(m_xPath));
        sprintf(m_xPath, XPATH_ORAN_MODULE_CAP "/band-capabilities[band-number=\'%d\']", m_band_number);
        lyd_new_path(*parent, ctx, m_xPath, nullptr, LYD_ANYDATA_CONSTSTRING, 0);

        // max-num-component-carriers
        uint8_t m_cc_num = 0;
        {
          char num_value[32] = {0};
          sprintf(num_value, "%d", m_cc_num);
          sprintf(m_leaf_xPath, "%s/max-num-component-carriers", m_xPath);
          lyd_new_path(*parent, ctx, m_leaf_xPath,
                      (void *)num_value, LYD_ANYDATA_CONSTSTRING, 0);
        }
      }
      #endif

      return SR_ERR_OK;
    }

   /*!
   * \fn   FMActiveAlarmListCb
   * \brief    This function is the callback function for get operational data of active-alarm-list
   * \return   sr_error_t
   */
#ifdef SYSREPO_V2
  static int
  FMActiveAlarmListCb(sr_session_ctx_t *session, uint32_t sub_id, const char *module_name, const char *xpath, const char *request_xpath,
                    uint32_t request_id, struct lyd_node **parent, void *private_data)
  {
    (void)sub_id;
#else
  static int
  FMActiveAlarmListCb(sr_session_ctx_t *session, const char *module_name, const char *xpath, const char *request_xpath,
                    uint32_t request_id, struct lyd_node **parent, void *private_data)
  {
#endif
    (void)session;
    (void)request_xpath;
    (void)request_id;
    (void)private_data;
    oam::ret_t ret;
	
    //LOG_INFO_VSTR(oam::module_ids::OAM_FM, "DATA RECEIVED : %s", xpath);
    const struct ly_ctx *ctx = sr_get_context(sr_session_get_connection(session));
    oam::oran_fm fm;
    ret = fm.FM_ActiveAlarmListXmlGet(ctx, parent);
	
    if (ret != oam::ret_t::SUCCESS)
    {
      return SR_ERR_LY;
    }
    return SR_ERR_OK;
  }

    /*!
     * \fn   ORANUplaneConfGetCb
     * \brief    This function is the callback function for get operational data of uplane-conf
     * \return   sr_error_t
     */
    static int
    ORANUplaneConfGetCb(sr_session_ctx_t *session, const char *module_name, const char *xpath, const char *request_xpath,
                     uint32_t request_id, struct lyd_node **parent, void *private_data)
    {
      (void)session;
      (void)request_xpath;
      (void)request_id;
      (void)private_data;

      //printf("\n\n ========== DATA FOR \"%s\" \"%s\" REQUESTED =======================\n\n", module_name, xpath);
      //printf("Request: xpath = %s, id = %d\n", request_xpath, request_id);
      const struct ly_ctx *ctx = sr_get_context(sr_session_get_connection(session));
      oam::uplane_conf_cap::get_instance()->UplaneConfCapToXml(ctx, parent);

      return SR_ERR_OK;
    }

    /*!
     * \fn   ORANDelayManageGetCb
     * \brief    This function is the callback function for get operational data of delay management
     * \return   sr_error_t
     */
    static int
    ORANDelayManageGetCb(sr_session_ctx_t *session, const char *module_name, const char *xpath, const char *request_xpath,
                     uint32_t request_id, struct lyd_node **parent, void *private_data)
    {
      (void)session;
      (void)request_xpath;
      (void)request_id;
      (void)private_data;

      //printf("\n\n ========== DATA FOR \"%s\" \"%s\" REQUESTED =======================\n\n", module_name, xpath);

      if (!strcmp(module_name, MODULE_ORAN_DELAY_MGMT) &&
          !strcmp(xpath, XPATH_ORAN_DELAY_MGMT))
      {
        const struct ly_ctx *ctx = sr_get_context(sr_session_get_connection(session));
        oam::delay_management_group_status::get_instance()->DMStatusToXml(ctx, parent);
      }

      return SR_ERR_OK;
    }
    
    /* execute command by forking a child process and don't wait for the child process */
static int safe_vasprintf(char **strp, const char *fmt, va_list ap)
{
    int retval;

    retval = vasprintf(strp, fmt, ap);

    if (retval == -1)
    {
        printf("Failed to vasprintf: %s.  Bailing out", strerror(errno));
        exit (1);
    }
    return (retval);
}
int safe_asprintf(char **strp, const char *fmt, ...)
{
    va_list ap;
    int retval;

    va_start(ap, fmt);
    retval = safe_vasprintf(strp, fmt, ap);
    va_end(ap);

    return (retval);
}


int execute_no_wait(char *cmd_line)
{
    int pid;
    //status,c;

    const char *new_argv[4];
    new_argv[0] = "/bin/sh";
    new_argv[1] = "-c";
    new_argv[2] = cmd_line;
    new_argv[3] = NULL;

    pid = fork();
    if (pid == 0)
    {
        if (execvp("/bin/sh", (char *const *)new_argv) == -1)      /* execute the command  */
        {
            printf("execvp(): %s", strerror(errno));
        }
        else
        {
            printf("execvp() failed");
        }
        exit(1);
    }
    return 1;
}


int execute_command_no_wait(const char *format, ...)
{
    va_list vlist;
    char *fmt_cmd;
    char *cmd;
    int rc;

    va_start(vlist, format);
    safe_vasprintf(&fmt_cmd, format, vlist);
    va_end(vlist);

    safe_asprintf(&cmd, "%s", fmt_cmd);
    free(fmt_cmd);


    printf("Executing command: %s", cmd);

    rc = execute_no_wait(cmd);
#if 1
    if (rc !=0 )
        printf("executed command failed(%d): %s", rc, cmd);
#endif
    free(cmd);

    return 1; //rc;
}

void notifyFmAlarm(S_SYNKPI_FM *pFm)
{
    if (pFm != NULL)
    {
      if (pFm->alarm_sn != local_alarm_sn)
      {
        local_alarm_sn = pFm->alarm_sn;
        syslog(LOG_INFO, "<%s,%d> alarm_sn update = %lu\n",__FUNCTION__,__LINE__,local_alarm_sn);

        for (int i =0; i < FM_SOURCE_FAULT_MAX; i++)
        {
          S_SYNKPI_FM_ALARM alarm;
          memset(&alarm, 0, sizeof(S_SYNKPI_FM_ALARM));
          memcpy(&alarm, &pFm->alarm[i], sizeof(S_SYNKPI_FM_ALARM));

          if ((alarm.is_notified == false) &&
              (alarm.is_raised == true))
          {
            if ((alarm.fault_severity > ORAN_FM_FAULT_SEVERITY_NONE) &&
                (alarm.fault_severity < ORAN_FM_FAULT_SEVERITY_WARNING))
            {
              oam::oran_fm fm;
              fm.FM_AlarmNotifSend(&alarm);
              pFm->alarm[i].is_notified = true;
            }
            if (alarm.is_cleared == true)
            {
              synKPI_fmAlarmRemove((E_SYNKPI_FM_FAULT_ID)i);
            }
          }
        }
      }
    }
}
    oam::ret_t oam_agent_hdlr::SWM_moniter_notify_polling(void)
    {   
        struct timeval tout;
        int eventIndex;
        int active_fds;
        int recvlen;
        struct inotify_event *pIEvent;
        char recvBuf[64];
        tout.tv_sec = 30;
        tout.tv_usec = 0;
        std::string err_sts = "";
        std::string err_msg = "";
        int g_Terminate = 0;
        int max_readfd = 0;
        fd_set master_readfds;
        fd_set readfds;
        time_t time_diff_sec; 
        time_t timeout_action_sec = 600;
        //int time_debug = 0;

        syslog(LOG_INFO, "<%s,%d> start",__FUNCTION__,__LINE__);

        //clear the master and temp sets
        FD_ZERO(&readfds);
        FD_ZERO(&master_readfds);
        max_readfd = 0;

        // set inotifyFD to the set
        FD_SET(inotifyFD,&master_readfds);
        if (inotifyFD > max_readfd)
        {   //keep track of the max 
            max_readfd = inotifyFD;
        }

		
        while (!g_Terminate)
        {
            //tout setting will clear after select()
            tout.tv_sec = 30; //default=30
            tout.tv_usec = 0;
			
			memcpy(&readfds,&master_readfds,sizeof(readfds));
            active_fds = select(max_readfd + 1,&readfds,NULL,NULL,&tout);
				
			//syslog(LOG_INFO, "0. <%s,%d> check time out 30sec\n",__FUNCTION__,__LINE__);
			
		    //1.check high temperature 
		    { 
			  static char temp_raise=0;
			  double read_temperature=0;   
			  
			  size_t result;  
              FILE *fptr;
			  fptr = fopen(INFO_DEICE_TEMPERATURE, "r");
		     
              if(fptr == NULL)
              {
                syslog(LOG_INFO, "<%s,%d> FM_log - open 'temperature information file' fail \n",__FUNCTION__,__LINE__);
              }
              else
			  {
				
                std::string temp_read( 32, '\0' );
		     
                fseek(fptr , 0, SEEK_SET);
                result = fread(&temp_read[0], sizeof(char), (size_t)32, fptr);
                read_temperature = std::atof(temp_read.c_str());
		      
                fclose(fptr);
                
                syslog(LOG_INFO, "<%s,%d> FM_log - read_temperature=%.1f (%.0f), result=%d\n",__FUNCTION__,__LINE__, 
                    (double)(read_temperature/1000), read_temperature, (int)result);
				
                read_temperature /= 1000;
				
                if( read_temperature >= THRESHOLD_DEV_TEMPERATURE )
                {   //raise
                    //gnb::fm_alarm_intf::get_instance()->raise_alarm_alarm_temperature_high();

					S_SYNKPI_FM_ALARM_REQ alarm;
                    std::string str_val = oam::OAM_TimeNowGet();
                    memset(alarm.event_time, 0, sizeof(alarm.event_time));
                    memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                    snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                    snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device high temperature ");
				    
                    alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
                    synKPI_fmAlarmRaise(FM_OFH_DEVICE_HIGH_TEMPERATURE, &alarm);
				    
                    S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                    notifyFmAlarm(pSynKPI_fm);
						
                    temp_raise = 1;
                    syslog(LOG_INFO, "<%s,%d> FM_log - raise 'temperature high' (%.1f > %d) \n",__FUNCTION__,__LINE__, read_temperature, THRESHOLD_DEV_TEMPERATURE);
					
                }
                else
                {
                    if( temp_raise && (read_temperature < THRESHOLD_DEV_TEMPERATURE) )
                    {   //clear
                        //gnb::fm_alarm_intf::get_instance()->clear_alarm_alarm_temperature_high(); 
						S_SYNKPI_FM_ALARM_REQ alarm;
                        std::string str_val = oam::OAM_TimeNowGet();
                        memset(alarm.event_time, 0, sizeof(alarm.event_time));
                        memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                        snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                        snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device high temperature ");
				        
                        alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
                        synKPI_fmAlarmClear(FM_OFH_DEVICE_HIGH_TEMPERATURE, &alarm);
				        
                        S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                        notifyFmAlarm(pSynKPI_fm);
                        temp_raise = 0;
                        syslog(LOG_INFO, "<%s,%d> FM_log - clear 'temperature high' (%.1f)\n",__FUNCTION__,__LINE__, read_temperature);
                    }
                }
              }
			}//end check high temperature
			
			//2.check high CPU usage 
            {
				std::ifstream filestat(INFO_DEICE_CPU_USAGE);
                std::string line;
                std::string new_line;
                std::getline(filestat, line);
                int count;
                unsigned time;
                double proc_time = 0, total_time = 0, cpu_usage_per = 0; 
                static double last_proc_time = 0, last_total_time = 0;
				static char cpu_raise=0;
				
                    
                new_line = line.substr(4,line.length());        //ignore "cpu" string
                std::istringstream cpu_time(new_line);
		    
                //"/proc/stat" => time of [user, nice, system, idle, iowait, irq, softirq, steal, guest, and guest_nice]
                //syslog(LOG_INFO, "<%s,%d> FM_log - cpu=%s\n",__FUNCTION__,__LINE__,new_line.c_str());
                
                for(count = 0 ; cpu_time >> time ; count ++)
                {  
                    if(count < 3)
                    {   // user + nice + nice
                        proc_time += time;
                    }
		    
                    total_time += time;
                }  
		    
                if( last_total_time )
                {   //calculate cpu usage for timer duration   
		    
                    cpu_usage_per = (proc_time - last_proc_time)/(total_time - last_total_time) * 100;
		    
                    syslog(LOG_INFO, "<%s,%d> FM_log - cpu usage[ %.0f / %.0f ] = %.1f %% \n",
                        __FUNCTION__,__LINE__, proc_time - last_proc_time, total_time - last_total_time, cpu_usage_per);  
		    
                    if( cpu_usage_per >= THRESHOLD_PER_CPU_USAGE )
                    {
                        //raise
                        //gnb::fm_alarm_intf::get_instance()->raise_alarm_alarm_cpu_usage_high(); 
						S_SYNKPI_FM_ALARM_REQ alarm;
                        std::string str_val = oam::OAM_TimeNowGet();
                        memset(alarm.event_time, 0, sizeof(alarm.event_time));
                        memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                        snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                        snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device high CPU usage ");
				        
                        alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
                        synKPI_fmAlarmRaise(FM_OFH_DEVICE_HIGH_CPU_USAGE, &alarm);
				        
                        S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                        notifyFmAlarm(pSynKPI_fm);
						
                        cpu_raise = 1;
                        syslog(LOG_INFO, "<%s,%d> FM_log - raise 'CPU usage high'  (%.1f > %d) \n",__FUNCTION__,__LINE__, cpu_usage_per, THRESHOLD_PER_CPU_USAGE);                   
                    }
                    else
                    {
                        if( cpu_raise && (cpu_usage_per < THRESHOLD_PER_CPU_USAGE) )
                        {   //clear
                            //gnb::fm_alarm_intf::get_instance()->clear_alarm_alarm_cpu_usage_high();
							S_SYNKPI_FM_ALARM_REQ alarm;
                            std::string str_val = oam::OAM_TimeNowGet();
                            memset(alarm.event_time, 0, sizeof(alarm.event_time));
                            memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                            snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                            snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device high CPU usage ");
				            
                            alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
                            synKPI_fmAlarmClear(FM_OFH_DEVICE_HIGH_CPU_USAGE, &alarm);
				            
                            S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                            notifyFmAlarm(pSynKPI_fm);
                            cpu_raise = 0;
                            syslog(LOG_INFO, "<%s,%d> FM_log - clear 'CPU usage high'  (%.1f)\n",__FUNCTION__,__LINE__, cpu_usage_per);
                        }
                    }
                }
		    
                //syslog(LOG_INFO, "<%s,%d> FM_log - now (proc/total)=( %.0f / %.0f )\n", __FUNCTION__,__LINE__, proc_time, total_time);   
		    
                //record time
                last_proc_time = proc_time;
                last_total_time = total_time;
            }//end check CPU usage
			
			//3.check low disk free space
            {
                double free_size_GB, total_size_GB, per_free_size;
				static char fs_raise=0;
				struct statvfs buf;
				
                if (statvfs(".", &buf) == -1)
                {
                    syslog(LOG_INFO, "<%s,%d> FM_log - statvfs() error\n",__FUNCTION__,__LINE__);
                }
                else 
                {
                    free_size_GB = ((double)((buf.f_bavail * buf.f_bsize) >> 20)) / 1024; 
                    total_size_GB = ((double)((buf.f_blocks * buf.f_bsize) >> 20)) / 1024;
                    per_free_size = (free_size_GB / total_size_GB)  * 100;
		    
                    syslog(LOG_INFO, "<%s,%d> FM_log - system left %.1f %% space.( %.1f GB / %.1f GB )",
                        __FUNCTION__,__LINE__, per_free_size, free_size_GB, total_size_GB);
		    
                    
                    if( per_free_size <= THRESHOLD_PER_FS_FREE_SPACE )
                    {
                        //raise
                        //gnb::fm_alarm_intf::get_instance()->raise_alarm_alarm_file_system_free_space_low();
						S_SYNKPI_FM_ALARM_REQ alarm;
                        std::string str_val = oam::OAM_TimeNowGet();
                        memset(alarm.event_time, 0, sizeof(alarm.event_time));
                        memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                        snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                        snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device disk low free space");
				        
                        alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
                        synKPI_fmAlarmRaise(FM_OFH_DEVICE_LOW_SPACE, &alarm);
				        
                        S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                        notifyFmAlarm(pSynKPI_fm);
						
                        fs_raise = 1;
                        syslog(LOG_INFO, "<%s,%d> FM_log - raise 'file system free space low'  (%.1f < %d) \n",__FUNCTION__,__LINE__, per_free_size, THRESHOLD_PER_FS_FREE_SPACE);                   
                    }
                    else
                    {
                        if( fs_raise && per_free_size > THRESHOLD_PER_FS_FREE_SPACE)
                        {   //clear
                            //gnb::fm_alarm_intf::get_instance()->clear_alarm_alarm_file_system_free_space_low();
						    S_SYNKPI_FM_ALARM_REQ alarm;
                            std::string str_val = oam::OAM_TimeNowGet();
                            memset(alarm.event_time, 0, sizeof(alarm.event_time));
                            memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                            snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                            snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device disk low free space");
				            
                            alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
                            synKPI_fmAlarmClear(FM_OFH_DEVICE_LOW_SPACE, &alarm);
				            
                            S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                            notifyFmAlarm(pSynKPI_fm);							
                            fs_raise = 0;
                            syslog(LOG_INFO, "<%s,%d> FM_log - clear 'file system free space low'  (%.1f)\n",__FUNCTION__,__LINE__, per_free_size);
                        }
                    }
                }
            }//end check low disk space
			
			//4.check low device memory
            {
			  FILE *fptr;
			  
			  static char mem_raise = 0;
              double free_mem_percent=0, avail_ram_MB=0;
			  double total_ram_MB=0, free_ram_MB=0;
			  
			  char cmd[128];
	          char recvbuf[8];
			  
			  struct sysinfo s_info;

              sysinfo(&s_info);
                          
              free_ram_MB = s_info.freeram >> 20;     //bytes to MB
              total_ram_MB = s_info.totalram >> 20;
              
              syslog(LOG_INFO, "<%s,%d> FM_log - fm init, total_ram_MB=%.1f, free_ram_MB=%.1f\n",__FUNCTION__,__LINE__, total_ram_MB, free_ram_MB);
			  
			  
              memset(cmd,0x0,sizeof(cmd));
              sprintf(cmd,"%s",CMD_GET_DEICE_MEMORY);
			  
              fptr = popen(cmd, "r");
              if( fgets(recvbuf,8,fptr) != NULL )
              {
                  avail_ram_MB = atoi(recvbuf);
                  pclose(fptr);
              }
              else
              {
                  syslog(LOG_INFO, "<%s,%d> process cmd fail. (%s)\n",__FUNCTION__,__LINE__, CMD_GET_DEICE_MEMORY);       
              }
              
			  
              free_mem_percent = (avail_ram_MB / total_ram_MB) * 100;
			  
              syslog(LOG_INFO, "<%s,%d> FM_log - system left %.1f %% memory.( %.1f MB / %.1f MB )",
			             __FUNCTION__,__LINE__, free_mem_percent, avail_ram_MB, total_ram_MB);
              
              if( free_mem_percent <= THRESHOLD_PER_DEV_MEMORY )
              {   //raise
                  //gnb::fm_alarm_intf::get_instance()->raise_alarm_alarm_device_memory_low();

				  // raise alarm
				  
                  S_SYNKPI_FM_ALARM_REQ alarm;
                  std::string str_val = oam::OAM_TimeNowGet();
                  memset(alarm.event_time, 0, sizeof(alarm.event_time));
                  memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                  snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                  snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device low memory");

                  alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
                  synKPI_fmAlarmRaise(FM_OFH_DEVICE_LOW_MEMORY, &alarm);

                  S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                  notifyFmAlarm(pSynKPI_fm);
				  
                  mem_raise = 1;
                  syslog(LOG_INFO, "<%s,%d> FM_log - raise alarm 'device low memory'  (%.1f < %d) \n",__FUNCTION__,__LINE__, free_mem_percent, THRESHOLD_PER_DEV_MEMORY);
				  
              }
              else
              {
                  if( mem_raise && free_mem_percent > THRESHOLD_PER_DEV_MEMORY)
                  {   //clear
                      //gnb::fm_alarm_intf::get_instance()->clear_alarm_alarm_device_memory_low();
					  
					  //clear alarm				
					  S_SYNKPI_FM_ALARM_REQ alarm;
                      std::string str_val = oam::OAM_TimeNowGet();
                      memset(alarm.event_time, 0, sizeof(alarm.event_time));
                      memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                      snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                      snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device low memory");

                      alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
					  synKPI_fmAlarmClear(FM_OFH_DEVICE_LOW_MEMORY, &alarm);
					  
					  S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                      notifyFmAlarm(pSynKPI_fm);
                      mem_raise = 0;
                      syslog(LOG_INFO, "<%s,%d> FM_log - clear alarm 'device low memory'  (%.1f)\n",__FUNCTION__,__LINE__, free_mem_percent);
                  }
              }
            }//end check low memory 
			
			
			//5.check network link status
            {
                int skfd = 0;
                struct ifreq ifr;
				static char cn_comm_raise = 0;
				
                memset(&ifr, 0, sizeof(struct ifreq));
		    
                skfd = socket(AF_INET, SOCK_DGRAM, 0);
                if(skfd < 0) 
                {
                    syslog(LOG_INFO, "<%s,%d> FM_log - Open socket error!\n",__FUNCTION__,__LINE__);
                }
                else
                {
                    strncpy(ifr.ifr_name, CORE_NETWORK_INTERFACE, IFNAMSIZ);
                    
                    if( ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
                    {
                        close(skfd);
                        syslog(LOG_INFO, "<%s,%d> FM_log - ioctl error!\n",__FUNCTION__,__LINE__);
                    }
                    else
                    {
                        syslog(LOG_INFO, "<%s,%d> FM_log - CN interface(%s), state=( %s, %s ) \n",__FUNCTION__,__LINE__,
                            ifr.ifr_name, 
                            (ifr.ifr_flags & IFF_UP) ? "UP":"Down",
                            (ifr.ifr_flags & IFF_RUNNING) ? "Running":"No Running");
		    
                        if( (ifr.ifr_flags & IFF_UP) == 0)//if 0=down, else 1=up
                        {   //raise
                            //gnb::fm_alarm_intf::get_instance()->raise_alarm_alarm_device_cn_comm_down();
							S_SYNKPI_FM_ALARM_REQ alarm;
                            std::string str_val = oam::OAM_TimeNowGet();
                            memset(alarm.event_time, 0, sizeof(alarm.event_time));
                            memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                            snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                            snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device core-network communication down");
				            
                            alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
                            synKPI_fmAlarmRaise(FM_OFH_DEVICE_NET_COMM_DOWN, &alarm);
				            
                            S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                            notifyFmAlarm(pSynKPI_fm);
							
                            cn_comm_raise = 1;
                            syslog(LOG_INFO, "<%s,%d> FM_log - raise 'device core-network communication down' \n",__FUNCTION__,__LINE__);   
                        } 
                        else 
                        {   
                            if(cn_comm_raise)
                            {   //clear
                                //gnb::fm_alarm_intf::get_instance()->clear_alarm_alarm_device_cn_comm_down();
							    S_SYNKPI_FM_ALARM_REQ alarm;
                                std::string str_val = oam::OAM_TimeNowGet();
                                memset(alarm.event_time, 0, sizeof(alarm.event_time));
                                memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                                snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                                snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device core-network communication down");
				                
                                alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
                                synKPI_fmAlarmClear(FM_OFH_DEVICE_NET_COMM_DOWN, &alarm);
				                
                                S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                                notifyFmAlarm(pSynKPI_fm);								
                                cn_comm_raise = 0;
                                syslog(LOG_INFO, "<%s,%d> FM_log - clear 'device core-network communication down' \n",__FUNCTION__,__LINE__);
                            }
                        }
                    }
                } 
            }//end check network link status
			
			
            if (active_fds < 0)
            {
                syslog(LOG_INFO, "<%s,%d> select fail (%s)\n",__FUNCTION__,__LINE__,strerror(errno));

                if (errno == EINTR || errno == EAGAIN)
                {
                    continue; // normal fail, try again 
                }
                else
                {
                    g_Terminate = 1; //fatal fail, terminate
                }
            }
            else if (active_fds == 0)
            {
                //syslog(LOG_INFO, "<%s,%d> timeout!!\n",__FUNCTION__,__LINE__);
                int status;

                //timeout handling 
                waitpid(-1, &status, WNOHANG);
                if(timer_in_progress)
                {
                    clock_gettime(CLOCK_MONOTONIC, &time_end);
                    time_diff_sec = time_end.tv_sec - time_start.tv_sec;                
#if 0               
                    //for debug           
                    if(time_debug != time_diff_sec)
                    {
                        syslog(LOG_INFO, "<%s,%d> time_diff=%ld (%ld - %ld)\n",__FUNCTION__,__LINE__,time_diff_sec, time_start.tv_sec, time_end.tv_sec);
                        time_debug = time_diff_sec;
                    }
#endif
                    if( time_diff_sec > timeout_action_sec )
                    {   
                        int ret;
                        char cmd[64];
                        
                        //1.kill swmgr & remove image 
                        memset(cmd, 0, sizeof(cmd));
                        sprintf(cmd,"%s", RESET_SWMGR_SH);
                        ret = system(cmd);   

                        syslog(LOG_INFO, "<%s,%d> timeout(%ld), cmd=%s\n",__FUNCTION__,__LINE__,timeout_action_sec, cmd);

                        if (ret != 0)
                        {
                            syslog(LOG_INFO, "<%s,%d> kill swmgr failed(%s)\n",__FUNCTION__,__LINE__,RESET_SWMGR_SH); 
                        }
                        else
                        {
                            syslog(LOG_INFO, "<%s,%d> launch %s\n",__FUNCTION__,__LINE__,SWMGR_LOACTION); 
                            //2.launch swmgr
                            memset(cmd, 0, sizeof(cmd));
                            sprintf(cmd,"%s", SWMGR_LOACTION);
                            execute_no_wait(cmd);

                            //3.send notify event and reset oam_agent flag
                            if(get_download_flag())
                            {   //download timeout
                                syslog(LOG_INFO, "<%s,%d> Download timeout\n",__FUNCTION__,__LINE__);                                
                                err_msg.assign("download timeout, reset swmgr module");  
                                swm::download_input::get_instance()->SWM_DL_DownloadEventSend(swm::E_DL_STATUS::TIMEOUT, err_msg);   
                                set_download_flag(ACTION_COMPLETE);            
                            }
                            else if(get_install_flag())
                            {   //install timeout
                                syslog(LOG_INFO, "<%s,%d> Install timeout\n",__FUNCTION__,__LINE__);                                
                                err_msg.assign("Install timeout, reset swmgr module");  
                                swm::install_input::get_instance()->SWM_INST_InstallEventSend(swm::E_INST_STATUS::TIMEOUT, err_msg);    
                                set_install_flag(ACTION_COMPLETE);
                            }
                            else if(get_activate_flag())
                            {   //activate timeout
                                syslog(LOG_INFO, "<%s,%d> Activate timeout\n",__FUNCTION__,__LINE__);                                
                                err_msg.assign("Activate timeout, reset swmgr module");  
                                swm::activate_input::get_instance()->SWM_ACTV_ActivateEventSend(swm::E_ACTV_STATUS::APPLICATION_ERROR, err_msg);    
                                set_activate_flag(ACTION_COMPLETE);
                            }   
                        }
                    }
                } //timer_in_progress
                continue;
            }  
            //syslog(LOG_INFO, "<%s,%d> active_fds=%d\n",__FUNCTION__,__LINE__,active_fds);

            memset(recvBuf,0x0,64);
            if (FD_ISSET(inotifyFD,&readfds))
            {
                eventIndex = 0;
                pIEvent = NULL;
                recvlen = read(inotifyFD,recvBuf,64);

                // run through the recv buffer
                while (eventIndex < recvlen)
                {
                    pIEvent = (struct inotify_event *)&recvBuf[eventIndex];

                    if (pIEvent->wd == triggerWatchFD)
                    {
                        clock_gettime(CLOCK_MONOTONIC, &time_end);
                        time_diff_sec = time_end.tv_sec - time_start.tv_sec;
                        syslog(LOG_INFO, "<%s,%d> time_diff=%ld sec\n",__FUNCTION__,__LINE__,time_diff_sec);

                        if(strstr(pIEvent->name,"fw_download"))
                        {   //download
                            if (!strcmp(pIEvent->name,"fw_download_done"))
                            {
                                syslog(LOG_INFO, "<%s,%d> fw_download_done\n",__FUNCTION__,__LINE__);
                                err_msg.assign("N/A");
                                swm::download_input::get_instance()->SWM_DL_DownloadEventSend(swm::E_DL_STATUS::COMPLETED, err_msg);
                            }
                            else
                            {
                                FILE *fptr;
                                size_t result;
                                fptr = fopen(INFORM_FIRMWARE_JOB_DL_ERROR_MESG, "r+");

                                if(fptr != NULL)
                                {
                                    int len = 256;
                                    std::string temp_read( len, '\0' );          
                                    fseek(fptr , 0, SEEK_SET);
                                    result = fread(&temp_read[0], sizeof(char), (size_t)len, fptr);
                                    err_sts.assign(temp_read.c_str(),1);   
                                    err_msg.assign(temp_read.c_str());
                                    fclose(fptr);
                                    syslog(LOG_INFO, "<%s,%d> fw_download_fail, result=%d, error=%s\n",__FUNCTION__,__LINE__,(int)result,temp_read.c_str());
                                }
                                else
                                {
                                    syslog(LOG_INFO, "<%s,%d> open 'fw_download_fail' fail \n",__FUNCTION__,__LINE__);
                                    err_sts.assign("4");    //APPLICATION_ERROR      
                                    err_msg.assign("open 'fw_download_fail' fail!! ");
                                }
                                syslog(LOG_INFO, "<%s,%d> status=%s, err_msg=%s\n",__FUNCTION__,__LINE__,err_sts.c_str(),err_msg.c_str());
                                swm::download_input::get_instance()->SWM_DL_DownloadEventSend((swm::E_DL_STATUS)atoi(err_sts.c_str()), err_msg);                             
                            }
                            set_download_flag(ACTION_COMPLETE);

                        }
                        else if(strstr(pIEvent->name,"fw_install"))
                        {   //install
                            if (!strcmp(pIEvent->name,"fw_install_done"))
                            {
                                syslog(LOG_INFO, "<%s,%d> fw_install_done\n",__FUNCTION__,__LINE__);
                                err_msg.assign("N/A");
                                swm::install_input::get_instance()->SWM_INST_InstallEventSend(swm::E_INST_STATUS::COMPLETED, err_msg);
                            }
                            else
                            {
                                FILE *fptr;
                                size_t result;
                                fptr = fopen(INFORM_FIRMWARE_JOB_IST_ERROR_MESG, "r+");

                                if(fptr != NULL)
                                {
                                    int len = 256;
                                    std::string temp_read( len, '\0' );          
                                    fseek(fptr , 0, SEEK_SET);
                                    result = fread(&temp_read[0], sizeof(char), (size_t)len, fptr);
                                    err_sts.assign(temp_read.c_str(),1);   
                                    err_msg.assign(temp_read.c_str());
                                    fclose(fptr);
                                    syslog(LOG_INFO, "<%s,%d> fw_instal_fail, result=%d, error=%s\n",__FUNCTION__,__LINE__,(int)result,temp_read.c_str());
                                }
                                else
                                {
                                    syslog(LOG_INFO, "<%s,%d> open 'fw_instal_fail' fail \n",__FUNCTION__,__LINE__);
                                    err_sts.assign("3");    //APPLICATION_ERROR      
                                    err_msg.assign("open 'fw_instal_fail' fail!! ");
                                }
                                syslog(LOG_INFO, "<%s,%d> status=%s, err_msg=%s\n",__FUNCTION__,__LINE__,err_sts.c_str(),err_msg.c_str());
                                swm::install_input::get_instance()->SWM_INST_InstallEventSend((swm::E_INST_STATUS)atoi(err_sts.c_str()), err_msg);
                            }
                            set_install_flag(ACTION_COMPLETE);
                        }
                        else if(strstr(pIEvent->name,"fw_activate"))
                        {   //activate
                            if (!strcmp(pIEvent->name,"fw_activate_done"))
                            {
                                syslog(LOG_INFO, "<%s,%d> fw_activate_done\n",__FUNCTION__,__LINE__);
                                err_msg.assign("N/A");
                                swm::activate_input::get_instance()->SWM_ACTV_ActivateEventSend(swm::E_ACTV_STATUS::COMPLETED, err_msg);
                            }
                            else
                            {
                                FILE *fptr;
                                size_t result;
                                fptr = fopen(INFORM_FIRMWARE_JOB_ACT_ERROR_MESG, "r+");

                                if(fptr != NULL)
                                {
                                    int len = 256;
                                    std::string temp_read( len, '\0' );          
                                    fseek(fptr , 0, SEEK_SET);
                                    result = fread(&temp_read[0], sizeof(char), (size_t)len, fptr);
                                    err_sts.assign(temp_read.c_str(),1);   
                                    err_msg.assign(temp_read.c_str());
                                    fclose(fptr);
                                    syslog(LOG_INFO, "<%s,%d> fw_activate_fail, result=%d, error=%s\n",__FUNCTION__,__LINE__,(int)result,temp_read.c_str());
                                }
                                else
                                {
                                    syslog(LOG_INFO, "<%s,%d> open 'fw_activate_fail' fail \n",__FUNCTION__,__LINE__);
                                    err_sts.assign("1");    //APPLICATION_ERROR       
                                    err_msg.assign("open 'fw_activate_fail' fail!! ");
                                }
                                syslog(LOG_INFO, "<%s,%d> status=%s, err_msg=%s\n",__FUNCTION__,__LINE__,err_sts.c_str(),err_msg.c_str());
                                swm::activate_input::get_instance()->SWM_ACTV_ActivateEventSend((swm::E_ACTV_STATUS)atoi(err_sts.c_str()), err_msg);
                            }
                            set_activate_flag(ACTION_COMPLETE);

                        }
                        else
                        {
                            FILE *fptr;
                            size_t result;
                            fptr = fopen(INFORM_FIRMWARE_JOB_SW_ERROR_MESG, "r+");
                            int send_status_directly = 0;

                            if(fptr == NULL)
                            {
                                syslog(LOG_INFO, "<%s,%d> open 'sw_fail' fail \n",__FUNCTION__,__LINE__);    
                                err_msg.assign("open 'sw_fail' fail!! ");
                                send_status_directly = 1;
                            }

                            if(get_download_flag())
                            {
                                if(send_status_directly == 0)
                                {
                                    int len =256;
                                    std::string temp_read( len, '\0' );          
                                    fseek(fptr , 0, SEEK_SET);
                                    result = fread(&temp_read[0], sizeof(char), (size_t)len, fptr);
                                    err_msg.assign(temp_read.c_str());
                                    fclose(fptr);
                                    syslog(LOG_INFO, "<%s,%d> fw_sw_fail, result=%d, error=%s\n",__FUNCTION__,__LINE__,(int)result,temp_read.c_str());

                                }                      
                                syslog(LOG_INFO, "<%s,%d> send_sts_directly=%d, err_msg=%s\n",__FUNCTION__,__LINE__,send_status_directly,err_msg.c_str());
                                swm::download_input::get_instance()->SWM_DL_DownloadEventSend(swm::E_DL_STATUS::APPLICATION_ERROR, err_msg);
                                set_download_flag(ACTION_COMPLETE);
                            }
                            else if(get_install_flag())
                            {
                                if(send_status_directly == 0)
                                {
                                    int len = 256;
                                    std::string temp_read( len, '\0' );          
                                    fseek(fptr , 0, SEEK_SET);
                                    result = fread(&temp_read[0], sizeof(char), (size_t)len, fptr);
                                    err_msg.assign(temp_read.c_str());
                                    fclose(fptr);
                                    syslog(LOG_INFO, "<%s,%d> fw_sw_fail, result=%d, error=%s\n",__FUNCTION__,__LINE__,(int)result,temp_read.c_str());
                                }
                                syslog(LOG_INFO, "<%s,%d> err_msg=%s\n",__FUNCTION__,__LINE__,err_msg.c_str());
                                swm::install_input::get_instance()->SWM_INST_InstallEventSend(swm::E_INST_STATUS::APPLICATION_ERROR, err_msg);
                                set_install_flag(ACTION_COMPLETE);
                            }
                            else if(get_activate_flag())
                            {
                                if(send_status_directly == 0)
                                {
                                    int len = 256;
                                    std::string temp_read( len, '\0' );          
                                    fseek(fptr , 0, SEEK_SET);
                                    result = fread(&temp_read[0], sizeof(char), (size_t)len, fptr);
                                    err_msg.assign(temp_read.c_str());
                                    fclose(fptr);
                                    syslog(LOG_INFO, "<%s,%d> fw_activate_fail, result=%d, error=%s\n",__FUNCTION__,__LINE__,(int)result,temp_read.c_str());
                                }
                                
                                syslog(LOG_INFO, "<%s,%d> err_msg=%s\n",__FUNCTION__,__LINE__,err_msg.c_str());
                                swm::activate_input::get_instance()->SWM_ACTV_ActivateEventSend(swm::E_ACTV_STATUS::APPLICATION_ERROR, err_msg);
                                set_activate_flag(ACTION_COMPLETE);
                            }
                        }

                    }   //if (pIEvent->wd == triggerWatchFD)
                    else if(pIEvent->wd == triggerWatchFD_FM)
                    {
                        if(strstr(pIEvent->name,"reboot"))
                        {   //notify EMS reboot by FM report
                            syslog(LOG_INFO, "<%s,%d> FM_log - raise 'device reboot'\n",__FUNCTION__,__LINE__);

                            //raise alarm
                            S_SYNKPI_FM_ALARM_REQ alarm;
                            std::string str_val = oam::OAM_TimeNowGet();
                            memset(alarm.event_time, 0, sizeof(alarm.event_time));
                            memset(alarm.fault_text, 0, sizeof(alarm.fault_text));
                            snprintf((char*)alarm.event_time, sizeof(alarm.event_time), "%s", str_val.c_str());
                            snprintf((char*)alarm.fault_text, sizeof(alarm.fault_text), "%s", "device sw reboot");

                            alarm.fault_severity = ORAN_FM_FAULT_SEVERITY_CRITICAL;
                            synKPI_fmAlarmRaise(FM_OFH_DEVICE_REBOOT, &alarm);

                            S_SYNKPI_FM* pSynKPI_fm = synKPI_fmPtrGet();
                            notifyFmAlarm(pSynKPI_fm);
                        }
                     }
                    if (pIEvent)
                    {
                        syslog(LOG_INFO,"<%s,%d>, wd=%d mask=%d cookie=%d len=%d dir=%s name=%s\n",
                            __FUNCTION__,__LINE__, pIEvent->wd, pIEvent->mask,pIEvent->cookie,pIEvent->len,
                            (pIEvent->mask & IN_ISDIR)?"yes":"no",pIEvent->len>0?pIEvent->name:"[EMPTY]");

                        syslog(LOG_INFO, "<%s,%d> len=>%d,%zu,%d\n",__FUNCTION__,__LINE__,eventIndex,sizeof(struct inotify_event), pIEvent->len);
                        eventIndex += sizeof(struct inotify_event) + pIEvent->len;
                    }
            
                }
            }
        }

        return oam::ret_t::SUCCESS;
    }

    oam::ret_t oam_agent_hdlr::SendTimerNotify()
    {
        sr_conn_ctx_t *connection = nullptr;
        sr_session_ctx_t *session = nullptr;
        const struct ly_ctx *ctx = nullptr;
        struct lyd_node *notif = nullptr;
        int rc = SR_ERR_OK;

        //connect to sysrepo
        rc = sr_connect(0, &connection);
        if (rc != SR_ERR_OK) 
        {
            syslog(LOG_INFO, "<%s,%d> sr_connect fail",__FUNCTION__,__LINE__);
            goto cleanupC;
        }

        //start session
        rc = sr_session_start(connection, SR_DS_RUNNING, &session);
        if (rc != SR_ERR_OK) 
        {
            syslog(LOG_INFO, "<%s,%d> start session fail",__FUNCTION__,__LINE__);
            goto cleanupC;
        }

        ctx = sr_get_context(connection);
        if (!ctx)
        {
            syslog(LOG_INFO, "<%s,%d> sr_get_context FAILED",__FUNCTION__,__LINE__);
            goto cleanupD;
        }

        //create the notification 
        notif = lyd_new_path(nullptr, ctx, NOTI_SUPERVISION_XPATH, nullptr, LYD_ANYDATA_CONSTSTRING, 0);
        if (!notif)
        {
            syslog(LOG_INFO, "<%s,%d> Creating notification failed, path=%s",__FUNCTION__,__LINE__, NOTI_SUPERVISION_XPATH);
            goto cleanup;
        }
                        
        /*send notification*/
        rc = sr_event_notif_send_tree(session, notif);
        if (rc != SR_ERR_OK) {
            goto cleanup;
        } 
        
        cleanup:
            lyd_free_withsiblings(notif);

        cleanupD:
            sr_session_stop(session);

        cleanupC:
            sr_disconnect(connection);

        return oam::ret_t::SUCCESS;
    }
    static void* SWM_moniter_notify_thread(void *data)
    {
        syslog(LOG_INFO, "<%s,%d> start thread",__FUNCTION__,__LINE__);
        oam_agent_hdlr::get_instance()->SWM_moniter_notify_polling();

        return NULL;
    }

    static void* timer_thread(void *data)
    {
        syslog(LOG_INFO, "<%s,%d> start timer thread...",__FUNCTION__,__LINE__);
        int rst_flag, timer;
        oam_agent::oam_agent_hdlr::get_instance()->set_resetTimer_flag(Enable);
        oam_agent::oam_agent_hdlr::get_instance()->set_resetTimer_value(10); //default 70s
                       
        while(1)
        {
            usleep(500*1000);
            rst_flag = oam_agent::oam_agent_hdlr::get_instance()->get_resetTimer_flag();
            if (rst_flag)
            {
                timer = oam_agent::oam_agent_hdlr::get_instance()->get_resetTimer_value(); //get result from rpc info
                while(timer)
                {
                    sleep(1);
                    oam_agent::oam_agent_hdlr::get_instance()->set_resetTimer_flag(Disable);
                    timer --;
                    if (timer == 0)
                    {
                        /*send notification*/
                        oam_agent_hdlr::get_instance()->SendTimerNotify();
                    }
                }
            }
        }

        return NULL;
    }


    oam::ret_t oam_agent_hdlr::SWM_inotify_initial()
    {   //initialize inotify

        pthread_t thread_id;

        syslog(LOG_INFO, "<%s,%d>initialize inotify",__FUNCTION__,__LINE__);


        inotifyFD = inotify_init();
        if (inotifyFD == -1)
        {
            syslog(LOG_INFO, "<%s,%d>initialize inotify failed(%s)",__FUNCTION__,__LINE__,strerror(errno));
            return oam::ret_t::FAILURE;
        }

        triggerWatchFD = inotify_add_watch(inotifyFD, SW_UPGRADE_NOTIFY_TRIGGER,IN_ATTRIB);
        if (triggerWatchFD == -1)
        {
            syslog(LOG_INFO, "<%s,%d>add %s watch failed(%s)",__FUNCTION__,__LINE__,SW_UPGRADE_NOTIFY_TRIGGER,strerror(errno));
            close(inotifyFD);
            return oam::ret_t::FAILURE;
        }
        triggerWatchFD_FM = inotify_add_watch(inotifyFD, FM_NOTIFY_TRIGGER,IN_ATTRIB);
        if (triggerWatchFD_FM == -1)
        {
           syslog(LOG_INFO, "<%s,%d>add %s watch failed(%s)",__FUNCTION__,__LINE__,FM_NOTIFY_TRIGGER,strerror(errno));
           close(inotifyFD);
           return oam::ret_t::FAILURE;
        }
        //initial SWM, why not take effect in Constructor function??
        swm::inventory::get_instance()->SWM_swm_initial();
        swm::download_input::get_instance()->SWM_swm_initial();
        swm::install_input::get_instance()->SWM_swm_initial();
        swm::activate_input::get_instance()->SWM_swm_initial();

        //create thread to write uci and wait notify
        syslog(LOG_INFO, "<%s,%d> Starting monitor system events thread",__FUNCTION__,__LINE__);
        pthread_create(&thread_id,NULL,SWM_moniter_notify_thread,NULL);

        return oam::ret_t::SUCCESS;
    }
    
    oam::ret_t oam_agent_hdlr::timer_init()
    {   //initialize inotify

        pthread_t thread_id;

        syslog(LOG_INFO, "<%s,%d>initialize timer",__FUNCTION__,__LINE__);


        //create thread to write uci and wait notify
        syslog(LOG_INFO, "<%s,%d> Starting timer thread",__FUNCTION__,__LINE__);
        pthread_create(&thread_id,NULL,timer_thread,NULL);

        return oam::ret_t::SUCCESS;
    }

    int
    SWMSoftareInventoryCallback(sr_session_ctx_t *session, const char *module_name, const char *xpath, const char *request_xpath,
    uint32_t request_id, struct lyd_node **parent, void *private_data)
    {
        (void)session;
        (void)request_xpath;
        (void)request_id;
        (void)private_data;

        swm::ret ret;

        syslog(LOG_INFO, "<%s,%d>\n\n ========== SWM DATA \"%s\" RECEIVED =======================\n\n",__FUNCTION__,__LINE__, xpath);

        if (!strcmp(module_name, MODULE_ORAN_SWM) &&
        !strcmp(xpath, GET_SWM_SW_INVENTORY_XPATH))
        {
            const struct ly_ctx *ctx = sr_get_context(sr_session_get_connection(session));
            ret = swm::inventory::get_instance()->INV_SlotGrpXmlGet(ctx, parent);
            if (ret != swm::ret::SUCCESS)
            {
                return SR_ERR_LY;
            }
        }
        return SR_ERR_OK;
    }
    static int
    SWMSoftwareDownloadCallback(sr_session_ctx_t *session, const char *path, const sr_val_t *input, const size_t input_cnt,
    sr_event_t event, uint32_t request_id, sr_val_t **output, size_t *output_cnt, void *private_data)
    {
        (void)session;
        (void)input_cnt;
        (void)event;
        (void)request_id;
        (void)private_data;

        syslog(LOG_INFO, "<%s,%d>\n\n ========== SWM Download \"%s\" RECEIVED =======================\n\n",__FUNCTION__,__LINE__, path);

        //pthread_t thread_id;
        swm::download_input *dl_input = swm::download_input::get_instance()->get_input_data();
        swm::ret ret = swm::ret::SUCCESS;
        std::size_t found_s,found_e;

        if(oam_agent::oam_agent_hdlr::get_instance()->get_download_flag() ||
            oam_agent::oam_agent_hdlr::get_instance()->get_install_flag() ||
            oam_agent::oam_agent_hdlr::get_instance()->get_activate_flag())
        {
            char err_mesg[64] = {0};
            int download, install, avtivate;
            download = oam_agent::oam_agent_hdlr::get_instance()->get_download_flag();
            install = oam_agent::oam_agent_hdlr::get_instance()->get_install_flag();
            avtivate = oam_agent::oam_agent_hdlr::get_instance()->get_activate_flag();
            syslog(LOG_INFO, "<%s,%d> download=%d, install=%d, active=%d, ",__FUNCTION__,__LINE__,download,install,avtivate);
            sprintf(err_mesg, "%s%s%s in progress", download ? "Download":"", install ? " Install":"", avtivate ? " Activate":"" );
            dl_input->err_msg.assign(err_mesg);
            ret = swm::ret::FAILURE;
            goto output;
        }

        dl_input->remote_file_path.assign(input[0].data.string_val);
        dl_input->password.assign(input[2].data.string_val);
        
        //dl_input->algorithm.assign(input[5].data.string_val);
        //dl_input->public_key.assign(input[6].data.string_val);

        syslog(LOG_INFO, "<%s,%d> input, path=%s,password=%s",__FUNCTION__,__LINE__,dl_input->remote_file_path.c_str(), dl_input->password.c_str());

        found_s = dl_input->remote_file_path.find_last_of("-");     //get fw version
        found_e = dl_input->remote_file_path.find_last_of("/");     //get url
        if ((found_s != 0) && (found_e != 0))
        {
            dl_input->file_name.assign(dl_input->remote_file_path.substr(found_s+1));
            dl_input->host_ip.assign(dl_input->remote_file_path.substr(0,found_e));
            syslog(LOG_INFO, "<%s,%d> host-url=%s, fw-version=%s",__FUNCTION__,__LINE__, dl_input->host_ip.c_str(),dl_input->file_name.c_str());
        }
        else
        {
            ret = swm::ret::FAILURE;
            dl_input->err_msg.assign("Input data \"/remote_file_path\" not find version name.");
            syslog(LOG_INFO, "<%s,%d> Not find version name",__FUNCTION__,__LINE__);
            goto output;
        }

        //write to uci file
        ret = swm::download_input::get_instance()->SWM_dl_inform_app_events(dl_input);

output:
        //generate output
        if (ret != swm::ret::SUCCESS)
        {
            *output = (sr_val_t *)malloc((sizeof **output) * 2);
            *output_cnt = 2;

            (*output)[0].xpath = strdup(RPC_SWM_SW_DOWNLOAD_XPATH "/status");
            (*output)[0].type = SR_ENUM_T;
            (*output)[0].dflt = 0;
            (*output)[0].data.enum_val = strdup(SWM_STR_FAILED);
            (*output)[1].xpath = strdup(RPC_SWM_SW_DOWNLOAD_XPATH "/error-message");
            (*output)[1].type = SR_STRING_T;
            (*output)[1].dflt = 0;
            (*output)[1].data.string_val = strdup(dl_input->err_msg.c_str());
            oam_agent::oam_agent_hdlr::get_instance()->set_download_flag(ACTION_COMPLETE);
            syslog(LOG_INFO, "<%s,%d> output-fail (%s)",__FUNCTION__,__LINE__,dl_input->err_msg.c_str());
        }
        else
        {
            *output = (sr_val_t *)malloc(sizeof **output);
            *output_cnt = 1;

            (*output)[0].xpath = strdup(RPC_SWM_SW_DOWNLOAD_XPATH "/status");
            (*output)[0].type = SR_ENUM_T;
            (*output)[0].dflt = 0;
            (*output)[0].data.enum_val = strdup(SWM_STR_STARTED);

            oam_agent::oam_agent_hdlr::get_instance()->set_download_flag(ACTION_IN_PROGRESS);
            syslog(LOG_INFO, "<%s,%d> output-Started",__FUNCTION__,__LINE__);
        }
        
        
        return SR_ERR_OK;
    }
static int
    SWMSoftwareInstallCallback(sr_session_ctx_t *session, const char *path, const sr_val_t *input, const size_t input_cnt,
    sr_event_t event, uint32_t request_id, sr_val_t **output, size_t *output_cnt, void *private_data)
    {
        (void)session;
        (void)input_cnt;
        (void)event;
        (void)request_id;
        (void)private_data;

        syslog(LOG_INFO, "<%s,%d>\n\n ========== SWM Install \"%s\" RECEIVED =======================\n\n",__FUNCTION__,__LINE__, path);

        swm::ret ret = swm::ret::SUCCESS;
        std::size_t found_s;
        swm::install_input *inst_input = swm::install_input::get_instance()->get_input_data();

        if(oam_agent::oam_agent_hdlr::get_instance()->get_download_flag() ||
            oam_agent::oam_agent_hdlr::get_instance()->get_install_flag() ||
            oam_agent::oam_agent_hdlr::get_instance()->get_activate_flag())
        {
            char err_mesg[64] = {0};
            int download, install, avtivate;
            download = oam_agent::oam_agent_hdlr::get_instance()->get_download_flag();
            install = oam_agent::oam_agent_hdlr::get_instance()->get_install_flag();
            avtivate = oam_agent::oam_agent_hdlr::get_instance()->get_activate_flag();
            syslog(LOG_INFO, "<%s,%d> download=%d, install=%d, active=%d, ",__FUNCTION__,__LINE__,download,install,avtivate);
            sprintf(err_mesg, "%s%s%s in progress", download ? "":"Download", install ? "":" Install", avtivate ? "":" Activate" );
            inst_input->err_msg.assign(err_mesg);
            ret = swm::ret::FAILURE;
            goto output;
        }

        //inst_input->slot_name.assign(input[0].data.string_val);
        inst_input->slot_name.assign(input[0].data.string_val);
        inst_input->file_name.assign(input[1].data.string_val);
        syslog(LOG_INFO, "<%s,%d> slot_name=%s, file_name=%s",__FUNCTION__,__LINE__,inst_input->slot_name.c_str(),inst_input->file_name.c_str());

        found_s = inst_input->file_name.find_last_of("-");     //get fw version
        if (found_s != 0)
        {
            inst_input->version.assign(inst_input->file_name.substr(found_s+1));
            syslog(LOG_INFO, "<%s,%d> version=%s",__FUNCTION__,__LINE__, inst_input->version.c_str());
        }
        else
        {
            ret = swm::ret::FAILURE;
            inst_input->err_msg.assign("Input data \"/file_name\" not find version .");
            syslog(LOG_INFO, "<%s,%d> Not find version name",__FUNCTION__,__LINE__);
            goto output;
        }

        //write to uci file
        ret = swm::install_input::get_instance()->SWM_inst_inform_app_events(inst_input);

output:
        //generate output 
        if (ret != swm::ret::SUCCESS)
        {
            *output = (sr_val_t *)malloc((sizeof **output) * 2);
            *output_cnt = 2;

            (*output)[0].xpath = strdup(RPC_SWM_SW_INSTALL_XPATH "/status");
            (*output)[0].type = SR_ENUM_T;
            (*output)[0].dflt = 0;
            (*output)[0].data.enum_val = strdup(SWM_STR_FAILED);
            (*output)[1].xpath = strdup(RPC_SWM_SW_INSTALL_XPATH "/error-message");
            (*output)[1].type = SR_STRING_T;
            (*output)[1].dflt = 0;
            (*output)[1].data.string_val = strdup(inst_input->err_msg.c_str());
            oam_agent::oam_agent_hdlr::get_instance()->set_install_flag(ACTION_COMPLETE);
            syslog(LOG_INFO, "<%s,%d> output-fail (%s)",__FUNCTION__,__LINE__,inst_input->err_msg.c_str());
        }
        else
        {
            *output = (sr_val_t *)malloc(sizeof **output);
            *output_cnt = 1;

            (*output)[0].xpath = strdup(RPC_SWM_SW_INSTALL_XPATH "/status");
            (*output)[0].type = SR_ENUM_T;
            (*output)[0].dflt = 0;
            (*output)[0].data.enum_val = strdup(SWM_STR_STARTED);
            oam_agent::oam_agent_hdlr::get_instance()->set_install_flag(ACTION_IN_PROGRESS);
            syslog(LOG_INFO, "<%s,%d> output-Started",__FUNCTION__,__LINE__);
        }

        return SR_ERR_OK;
    }

    static int
    SWMSoftwareActivateCallback(sr_session_ctx_t *session, const char *path, const sr_val_t *input, const size_t input_cnt,
    sr_event_t event, uint32_t request_id, sr_val_t **output, size_t *output_cnt, void *private_data)
    {
        (void)session;
        (void)input_cnt;
        (void)event;
        (void)request_id;
        (void)private_data;

        syslog(LOG_INFO, "<%s,%d>\n\n ========== SWM Activate \"%s\" RECEIVED =======================\n\n",__FUNCTION__,__LINE__, path);

        swm::ret ret = swm::ret::SUCCESS;
        swm::activate_input *actv_input = swm::activate_input::get_instance()->get_input_data();

        if(oam_agent::oam_agent_hdlr::get_instance()->get_download_flag() ||
            oam_agent::oam_agent_hdlr::get_instance()->get_install_flag() ||
            oam_agent::oam_agent_hdlr::get_instance()->get_activate_flag())
        {
            char err_mesg[64] = {0};
            int download, install, avtivate;
            download = oam_agent::oam_agent_hdlr::get_instance()->get_download_flag();
            install = oam_agent::oam_agent_hdlr::get_instance()->get_install_flag();
            avtivate = oam_agent::oam_agent_hdlr::get_instance()->get_activate_flag();
            syslog(LOG_INFO, "<%s,%d> download=%d, install=%d, active=%d, ",__FUNCTION__,__LINE__,download,install,avtivate);
            sprintf(err_mesg, "%s%s%s in progress", download ? "":"Download", install ? "":" Install", avtivate ? "":" Activate" );
            actv_input->err_msg.assign(err_mesg);
            ret = swm::ret::FAILURE;
            goto output;
        }

        ///parser input
        actv_input->slot_name.assign(input[0].data.string_val);
        syslog(LOG_INFO, "<%s,%d> slot_name=%s",__FUNCTION__,__LINE__,actv_input->slot_name.c_str());
        
        //write to uci file
        ret = swm::activate_input::get_instance()->SWM_actv_inform_app_events(actv_input);

output:
        if (ret != swm::ret::SUCCESS)
        {
            *output = (sr_val_t *)malloc((sizeof **output) * 2);
            *output_cnt = 2;

            (*output)[0].xpath = strdup(RPC_SWM_SW_ACTIVATE_XPATH "/status");
            (*output)[0].type = SR_ENUM_T;
            (*output)[0].dflt = 0;
            (*output)[0].data.enum_val = strdup(SWM_STR_FAILED);
            (*output)[1].xpath = strdup(RPC_SWM_SW_ACTIVATE_XPATH "/error-message");
            (*output)[1].type = SR_STRING_T;
            (*output)[1].dflt = 0;
            (*output)[1].data.string_val = strdup(actv_input->err_msg.c_str());
            oam_agent::oam_agent_hdlr::get_instance()->set_activate_flag(ACTION_COMPLETE);
            syslog(LOG_INFO, "<%s,%d> output-fail (%s)",__FUNCTION__,__LINE__,actv_input->err_msg.c_str());
        }
        else
        {
            *output = (sr_val_t *)malloc(sizeof **output);
            *output_cnt = 1;

            (*output)[0].xpath = strdup(RPC_SWM_SW_ACTIVATE_XPATH "/status");
            (*output)[0].type = SR_ENUM_T;
            (*output)[0].dflt = 0;
            (*output)[0].data.enum_val = strdup(SWM_STR_STARTED);
            oam_agent::oam_agent_hdlr::get_instance()->set_activate_flag(ACTION_IN_PROGRESS);
            syslog(LOG_INFO, "<%s,%d> output-Started",__FUNCTION__,__LINE__);

        }
        return SR_ERR_OK;
    }
    
    static int
    LogGenCallback(sr_session_ctx_t *session, const char *path, const sr_val_t *input, const size_t input_cnt,
    sr_event_t event, uint32_t request_id, sr_val_t **output, size_t *output_cnt, void *private_data)
    {
        (void)session;
        (void)input_cnt;
        (void)event;
        (void)request_id;
        (void)private_data;

        sr_conn_ctx_t *connection = nullptr;
        // sr_session_ctx_t *session = nullptr;
        const struct ly_ctx *ctx = nullptr;
        struct lyd_node *notif = nullptr;
        int rc = SR_ERR_OK;
        char val[32] = {0};

        syslog(LOG_INFO, "<%s,%d>\n\n ========== Log Generate \"%s\" RECEIVED =======================\n\n",__FUNCTION__,__LINE__, path);
        /* connect to sysrepo */
        rc = sr_connect(0, &connection);
        if (rc != SR_ERR_OK) {
            goto cleanupC;
        }

        /* start session */
        rc = sr_session_start(connection, SR_DS_RUNNING, &session);
        if (rc != SR_ERR_OK) {
            goto cleanupC;
        }

        ctx = sr_get_context(connection);
        if (!ctx)
        {
            syslog(LOG_INFO, "<%s,%d> sr_get_context FAILED \n",__FUNCTION__,__LINE__);
            goto cleanupD;
        }
        /* create the notification */
        notif = lyd_new_path(nullptr, ctx, NOTI_LOG_MANAGEMENT_XPATH, nullptr, LYD_ANYDATA_CONSTSTRING, 0);
        if (!notif)
        {
          syslog(LOG_INFO, "<%s,%d> Creating notification failed, path=%s \n",__FUNCTION__,__LINE__,NOTI_SWM_ACTIVATE_EVNET_XPATH);
          goto cleanup;
        }
        
        /*Check syslog whether exist will response and send the notification*/       
        FILE *log;
        if ((log = fopen(SYSLOG_PATH, "r"))) 
        {
            fclose(log);
            std::cout << "syslog exist" << std::endl;
            sprintf(val, "%s", "syslog");
            /* output */ 
            *output = (sr_val_t *)malloc(sizeof **output);
            *output_cnt = 1;

            (*output)[0].xpath = strdup(RPC_LOG_MANAGEMENT_XPATH "/status");
            (*output)[0].type = SR_ENUM_T;
            (*output)[0].dflt = 0;
            (*output)[0].data.enum_val = strdup(LOG_STR_SUCCESS);

            /* fill log-file-name */     
            if (!lyd_new_path(notif, NULL, "log-file-name", (void *)val, LYD_ANYDATA_CONSTSTRING, 0))
            {
              syslog(LOG_INFO, "<%s,%d> Creating value failed - log-file-name\n",__FUNCTION__,__LINE__);
              goto cleanup;
            }
            
            /* send the notification */
            rc = sr_event_notif_send_tree(session, notif);
            if (rc != SR_ERR_OK) {
                goto cleanup;
            }         
        }
        else
        {
            std::cout << "syslog doesn't exist" << std::endl;
            /* output */
            *output = (sr_val_t *)malloc((sizeof **output) * 1);
            *output_cnt = 1;

            (*output)[0].xpath = strdup(RPC_LOG_MANAGEMENT_XPATH "/status");
            (*output)[0].type = SR_ENUM_T;
            (*output)[0].dflt = 0;
            (*output)[0].data.enum_val = strdup(LOG_STR_FAILURE);
            // (*output)[1].xpath = strdup(RPC_LOG_MANAGEMENT_XPATH "/error-message");
            // (*output)[1].type = SR_STRING_T;
            // (*output)[1].dflt = 0;
            // (*output)[1].data.string_val = strdup(actv_input->err_msg.c_str());
        }
        cleanup:
            lyd_free_withsiblings(notif);

        cleanupD:
            sr_session_stop(session);

        cleanupC:
            sr_disconnect(connection);

        return SR_ERR_OK;
    }


    static int
    LogUploadCallback(sr_session_ctx_t *session, const char *path, const sr_val_t *input, const size_t input_cnt,
    sr_event_t event, uint32_t request_id, sr_val_t **output, size_t *output_cnt, void *private_data)
    {
        (void)session;
        (void)input_cnt;
        (void)event;
        (void)request_id;
        (void)private_data;

        sr_conn_ctx_t *connection = nullptr;
        const struct ly_ctx *ctx = nullptr;
        struct lyd_node *notif = nullptr;
        int rc = SR_ERR_OK;
        char val[32] = {0};
        char *curlcmd;
        int ret;

        syslog(LOG_INFO, "<%s,%d>\n\n ========== Log Upload \"%s\" RECEIVED =======================\n\n",__FUNCTION__,__LINE__, path);
        /* connect to sysrepo */
        rc = sr_connect(0, &connection);
        if (rc != SR_ERR_OK) {
            goto cleanupC;
        }

        /* start session */
        rc = sr_session_start(connection, SR_DS_RUNNING, &session);
        if (rc != SR_ERR_OK) {
            goto cleanupC;
        }

        ctx = sr_get_context(connection);
        if (!ctx)
        {
            syslog(LOG_INFO, "<%s,%d> sr_get_context FAILED \n",__FUNCTION__,__LINE__);
            goto cleanupD;
        } 

        /* create the notification */
        notif = lyd_new_path(nullptr, ctx, NOTI_LOG_UPLOAD_XPATH, nullptr, LYD_ANYDATA_CONSTSTRING, 0);
        if (!notif)
        {
          syslog(LOG_INFO, "<%s,%d> Creating notification failed, path=%s \n",__FUNCTION__,__LINE__,NOTI_SWM_ACTIVATE_EVNET_XPATH);
          goto cleanup;
        }

        curlcmd = input[1].data.string_val;
        ret = system(curlcmd);
         
        if (ret != 0)
        {
          std::cout << "Log execute fail" << std::endl;
          /* output */
          *output = (sr_val_t *)malloc((sizeof **output) * 1);
          *output_cnt = 1;
          (*output)[0].xpath = strdup(RPC_LOG_UPLOAD_XPATH "/status");
          (*output)[0].type = SR_ENUM_T;
          (*output)[0].dflt = 0;
          (*output)[0].data.enum_val = strdup(LOG_STR_FAILURE);
        }
        else
        {
          std::cout << "Log execute success" << std::endl;
          /* output */ 
          *output = (sr_val_t *)malloc(sizeof **output);
          *output_cnt = 1;
          (*output)[0].xpath = strdup(RPC_LOG_UPLOAD_XPATH "/status");
          (*output)[0].type = SR_ENUM_T;
          (*output)[0].dflt = 0;
          (*output)[0].data.enum_val = strdup(LOG_STR_SUCCESS);
          /* fill local-logical-file-path */
          sprintf(val, "%s", "/var/log/");     
          if (!lyd_new_path(notif, NULL, "local-logical-file-path", (void *)val, LYD_ANYDATA_CONSTSTRING, 0))
          {
            syslog(LOG_INFO, "<%s,%d> Creating value failed - local-logical-file-path\n",__FUNCTION__,__LINE__);
            goto cleanup;
          }
          /* fill status */
          sprintf(val, "%s", "SUCCESS");     
          if (!lyd_new_path(notif, NULL, "status", (void *)val, LYD_ANYDATA_CONSTSTRING, 0))
          {
            syslog(LOG_INFO, "<%s,%d> Creating value failed - staus\n",__FUNCTION__,__LINE__);
            goto cleanup;
          }
          /* fill remote-file-path */
          if (!lyd_new_path(notif, NULL, "remote-file-path", (void *)curlcmd, LYD_ANYDATA_CONSTSTRING, 0))
          {
            syslog(LOG_INFO, "<%s,%d> Creating value failed - remote-file-path\n",__FUNCTION__,__LINE__);
            goto cleanup;
          }
          /* send the notification */
          rc = sr_event_notif_send_tree(session, notif);
          if (rc != SR_ERR_OK) {
              goto cleanup;
          }    
        }
        cleanup:
            lyd_free_withsiblings(notif);

        cleanupD:
            sr_session_stop(session);

        cleanupC:
            sr_disconnect(connection);
        
        return SR_ERR_OK;
    }
    static int
    SupervisonCallback(sr_session_ctx_t *session, const char *path, const sr_val_t *input, const size_t input_cnt,
    sr_event_t event, uint32_t request_id, sr_val_t **output, size_t *output_cnt, void *private_data)
    {
        (void)session;
        (void)input_cnt;
        (void)event;
        (void)request_id;
        (void)private_data;
        syslog(LOG_INFO, "<%s,%d>\n\n ========== Log Supervision \"%s\" RECEIVED =======================\n\n",__FUNCTION__,__LINE__, path);
        int noti_interval;
        int guard_timer_overhead;
        int supervision_timer;

        noti_interval = input[0].data.uint16_val;
        guard_timer_overhead = input[1].data.uint16_val;
        supervision_timer = noti_interval + guard_timer_overhead;
        oam_agent::oam_agent_hdlr::get_instance()->set_resetTimer_value(supervision_timer);
        oam_agent::oam_agent_hdlr::get_instance()->set_resetTimer_flag(Enable);
        printf("supervision_timer = %d\n", supervision_timer);

        time_t rawtime;
        struct tm *info;
        char datetime[64];
        time( &rawtime );

        info = localtime( &rawtime );
        printf("current date and time<A1>G%s", asctime(info));
        // sprintf(datetime,"%s", "2021-12-16T12:05:33Z"); /*output format*/
        info->tm_year = info->tm_year + 1900; /* The number of years since 1900 */
        info->tm_mon = info->tm_mon + 1; /* month, range 0 to 11  */
        sprintf(datetime, "%d-%02d-%02dT%02d:%02d:%02dZ", info->tm_year, info->tm_mon, info->tm_mday, info->tm_hour, info->tm_min, info->tm_sec);
        /* output */ 
        *output = (sr_val_t *)malloc(sizeof **output);
        *output_cnt = 1;
        (*output)[0].xpath = strdup(RPC_SUPERVISION_XPATH "/next-update-at");
        (*output)[0].type = SR_ENUM_T;
        (*output)[0].dflt = 0;
        (*output)[0].data.enum_val = strdup(datetime); //input format need to check

        return SR_ERR_OK;
    }
    /*!
     * \fn   establish_confd_subscription_connection
     * \brief    This function is used to establish the ConfD connection
     * \return   void
     */
    oam::ret_t oam_agent_hdlr::establish_confd_subscription_connection()
    {   	
        std::cout << "[liboam] establish_confd_subscription_connection" << std::endl;
        int rc = SR_ERR_OK;    
        
        /* connect to sysrepo */
        rc = sr_connect(0, &gSrCdb.subs_conn);        
        if (rc != SR_ERR_OK) {      
            std::cout << "[liboam] establish_confd_subscription_connection" << std::endl;      
            goto cleanupB;
        }
    
        /* start session */
        rc = sr_session_start(gSrCdb.subs_conn, SR_DS_RUNNING, &gSrCdb.subs_sess);        
        if (rc != SR_ERR_OK) {            
            std::cout << "[liboam] establish_confd_subscription_connection" << std::endl;
            goto cleanupB;
        }

#if 1
        /* subscribe for change of uplane conf (for dynazzmic config) */
        rc = sr_module_change_subscribe(gSrCdb.subs_sess, MODULE_ORAN_UPLANE_CONF,
          XPATH_ORAN_UPLANE_CONF, change_oran_uplane_conf_cb, NULL, 0,
          SR_SUBSCR_PASSIVE|SR_SUBSCR_DONE_ONLY, &gSrCdb.subscription);
        if (rc != SR_ERR_OK) {
            std::cout << "subscribe for change of uplane conf (for dynazzmic config) fail" << std::endl;
            goto cleanupB;
        }
        /* subscribe for change of delay mgmt (for dynamic config) */
        rc = sr_module_change_subscribe(gSrCdb.subs_sess, MODULE_ORAN_DELAY_MGMT,
          XPATH_ORAN_DELAY_MGMT, change_oran_delay_mgmt_cb, NULL, 0,
          SR_SUBSCR_PASSIVE|SR_SUBSCR_DONE_ONLY, &gSrCdb.subscription);
        if (rc != SR_ERR_OK) {
            std::cout << "subscribe for change of delay mgmt (for dynamic config) fail" << std::endl;
            goto cleanupB;
        }
        /* subscribe for change of processing-elements (for dynamic config) */
        rc = sr_module_change_subscribe(gSrCdb.subs_sess, MODULE_ORAN_PROCESS_ELEM,
          XPATH_ORAN_PROCESS_ELEM, change_oran_process_elem_cb, NULL, 0,
          SR_SUBSCR_PASSIVE|SR_SUBSCR_DONE_ONLY, &gSrCdb.subscription);
        if (rc != SR_ERR_OK) {
            std::cout << "subscribe for change of processing-elements (for dynamic config) fail" << std::endl;
            goto cleanupB;
        }
        /* subscribe for change of interfaces (for dynamic config) */
        rc = sr_module_change_subscribe(gSrCdb.subs_sess, MODULE_IETF_INTERFACES,
          XPATH_IETF_INTERFACES, change_ietf_intf_cb, NULL, 0,
          SR_SUBSCR_PASSIVE|SR_SUBSCR_DONE_ONLY, &gSrCdb.subscription);
        if (rc != SR_ERR_OK) {
            std::cout << "subscribe for change of interfaces (for dynamic config) fail" << std::endl;
            goto cleanupB;
        }

        /* subscribe for get operational data of module-capability */
        rc = sr_oper_get_items_subscribe(gSrCdb.subs_sess, MODULE_ORAN_MODULE_CAP,
                                         XPATH_ORAN_MODULE_CAP, ORANModuleCapGetCb, NULL, SR_SUBSCR_CTX_REUSE, &gSrCdb.subscription);
        if (rc != SR_ERR_OK)
        {
            std::cout << "subscribe for get operational data of module-capability fail" << std::endl;
            goto cleanupB;
        }

        /* subscribe for get operational data of o-ran uplane conf */
        rc = sr_oper_get_items_subscribe(gSrCdb.subs_sess, MODULE_ORAN_UPLANE_CONF,
                                         XPATH_ORAN_UPLANE_CONF, ORANUplaneConfGetCb, NULL, SR_SUBSCR_CTX_REUSE, &gSrCdb.subscription);
        if (rc != SR_ERR_OK)
        {
            std::cout << "subscribe for get operational data of o-ran uplane conf fail" << std::endl;
            goto cleanupB;
        }

        // subscribe for get operational data of delay management
        rc = sr_oper_get_items_subscribe(gSrCdb.subs_sess, MODULE_ORAN_DELAY_MGMT,
                                         XPATH_ORAN_DELAY_MGMT, ORANDelayManageGetCb, NULL, SR_SUBSCR_CTX_REUSE, &gSrCdb.subscription);
        if (rc != SR_ERR_OK)
        {
            std::cout << "subscribe for get operational data of delay management fail" << std::endl;
            goto cleanupB;
        }
		
		// subscribe for get operational data of active-alarm-list  20220429 by kevin.chen
		rc = sr_oper_get_items_subscribe(gSrCdb.subs_sess, MODULE_ORAN_FM,
						                 XPATH_ORAN_FM_ALARM_LIST, FMActiveAlarmListCb, NULL, 0, &gSrCdb.subscription);
	    if (rc != SR_ERR_OK)
        {
            std::cout << "subscribe for get operational data of active-alarm-list fail" << std::endl;
            goto cleanupB;
        }
		
#endif

        //RPC subscribe for supervision
        syslog(LOG_INFO, "<%s,%d>Log: sr_rpc_subscribe for o-ran yang model: /o-ran-supervision:supervision-watchdog-reset",__FUNCTION__,__LINE__);
        rc = sr_rpc_subscribe(gSrCdb.subs_sess, RPC_SUPERVISION_XPATH, SupervisonCallback,
                          NULL, 0, SR_SUBSCR_CTX_REUSE, &gSrCdb.subscription);
        if (rc != SR_ERR_OK)
        {
            goto cleanupB;
        }

        //RPC subscribe for log generate
        syslog(LOG_INFO, "<%s,%d>Log: sr_rpc_subscribe for o-ran yang model: /o-ran-troubleshooting:start-troubleshooting-logs",__FUNCTION__,__LINE__);
        rc = sr_rpc_subscribe(gSrCdb.subs_sess, RPC_LOG_MANAGEMENT_XPATH, LogGenCallback,
                          NULL, 0, SR_SUBSCR_CTX_REUSE, &gSrCdb.subscription);
        if (rc != SR_ERR_OK)
        {
            goto cleanupB;
        }

        //RPC subscribe for log upload
        syslog(LOG_INFO, "<%s,%d>Log: sr_rpc_subscribe for o-ran yang model: /o-ran-file-management:file-upload",__FUNCTION__,__LINE__);
        rc = sr_rpc_subscribe(gSrCdb.subs_sess, RPC_LOG_UPLOAD_XPATH, LogUploadCallback,
                          NULL, 0, SR_SUBSCR_CTX_REUSE, &gSrCdb.subscription);
        if (rc != SR_ERR_OK)
        {
            goto cleanupB;
        }

        //subscribe for get operational data of software-slot
        syslog(LOG_INFO, "<%s,%d>SWM: sr_oper_get_items_subscribe for o-ran yang model: /o-ran-software-management:software-slot",__FUNCTION__,__LINE__);
        rc = sr_oper_get_items_subscribe(gSrCdb.subs_sess, MODULE_ORAN_SWM,
              GET_SWM_SW_INVENTORY_XPATH, SWMSoftareInventoryCallback, NULL, 0, &gSrCdb.subscription);
        if (rc != SR_ERR_OK)
        {
            syslog(LOG_INFO, "<%s,%d>SWM: subscribe oper-slot name fail, rc=%d",__FUNCTION__,__LINE__, rc);
            goto cleanupB;
        }
        //RPC subscribe for software upgrade
        syslog(LOG_INFO, "<%s,%d>SWM: sr_rpc_subscribe for o-ran yang model: /o-ran-software-management:software-download",__FUNCTION__,__LINE__);
        rc = sr_rpc_subscribe(gSrCdb.subs_sess, RPC_SWM_SW_DOWNLOAD_XPATH, SWMSoftwareDownloadCallback, 
                          NULL, 0, SR_SUBSCR_CTX_REUSE, &gSrCdb.subscription);
        if (rc != SR_ERR_OK)
        {
            syslog(LOG_INFO, "<%s,%d>SWM: subscribe download fail, rc=%d",__FUNCTION__,__LINE__, rc);
            goto cleanupB;
        }
        syslog(LOG_INFO, "<%s,%d>SWM: sr_rpc_subscribe for o-ran yang model: /o-ran-software-management:software-install",__FUNCTION__,__LINE__);
        rc = sr_rpc_subscribe(gSrCdb.subs_sess, RPC_SWM_SW_INSTALL_XPATH, SWMSoftwareInstallCallback,
                          NULL, 0, SR_SUBSCR_CTX_REUSE, &gSrCdb.subscription);
        if (rc != SR_ERR_OK)
        {
            goto cleanupB;
        }
        syslog(LOG_INFO, "<%s,%d>SWM: sr_rpc_subscribe for o-ran yang model: /o-ran-software-management:software-activate",__FUNCTION__,__LINE__);
        rc = sr_rpc_subscribe(gSrCdb.subs_sess, RPC_SWM_SW_ACTIVATE_XPATH, SWMSoftwareActivateCallback,
                            NULL, 0, SR_SUBSCR_CTX_REUSE, &gSrCdb.subscription);
        if (rc != SR_ERR_OK)
        {
            goto cleanupB;
        }
        if(oam_agent_hdlr::get_instance()->SWM_inotify_initial() == oam::ret_t::FAILURE)
        {
            goto cleanupB;
        }
        if(oam_agent_hdlr::get_instance()->timer_init() == oam::ret_t::FAILURE)
        {
            goto cleanupB;
        }
        std::cout << "[liboam] establish_confd_subscription_connection down" << std::endl;
        return oam::ret_t::SUCCESS;
        
                
    cleanupB:
        //sr_disconnect(gSrCdb.subs_conn);
        sysrepo_broker_close();
        printf("[liboam] establish_confd_subscription_connection fail\n");
        return oam::ret_t::FAILURE;
    }

    /*!
     * \fn   establish_confd_read_connection
     * \brief    This function is used to establish the ConfD connection
     * \return   void
     */
    oam::ret_t oam_agent_hdlr::establish_confd_read_connection()
    {
        //confd_init("DU", stderr, debug_level);


        int rc = SR_ERR_OK;
        
        if (oam_confd_count)
        {
            while (oam_confd_count--)
            {
                /* connect to sysrepo */
                rc = sr_connect(0, &gSrCdb.connection);
                if (rc != SR_ERR_OK) 
                {
                    std::cout << "SYSREPO_CONNECTION_FAILED" << "\t Attempt -> " << oam_confd_count << std::endl;
                    std::this_thread::sleep_for (std::chrono::seconds(oam_confd_timer));
                    continue;
                }

                break;
            }

            if ((0 == oam_confd_count) &&
                    ((sr_connect(0, &gSrCdb.connection)) != SR_ERR_OK))
            {
                sr_disconnect(gSrCdb.connection);
                return oam::ret_t::FAILURE;
            }
        }
        else
        {
            oam_confd_count = 1;

            while (oam_confd_count++)
            {
                /* connect to sysrepo */
                rc = sr_connect(0, &gSrCdb.connection);
                if (rc != SR_ERR_OK) 
                {
                    std::cout << "SYSREPO_CONNECTION_FAILED" << "\t Attempt -> " << oam_confd_count << std::endl;
                    std::this_thread::sleep_for (std::chrono::seconds(oam_confd_timer));

                    if (0 == oam_confd_count)
                    {
                        oam_confd_count = 1;
                    }

                    continue;
                }

                break;
            }
        }

        /* start session */
        rc = sr_session_start(gSrCdb.connection, SR_DS_RUNNING, &gSrCdb.session);
        if (rc != SR_ERR_OK) {
            sr_disconnect(gSrCdb.connection);
            return oam::ret_t::FAILURE;
        }

        return oam::ret_t::SUCCESS;
    }

    /*!
     * \fn       copy_xml_to_doc
     * \brief    This function is used to merge two XML files
     * \return   Void
     */
    void oam_agent_hdlr::copy_xml_to_doc(tinyxml2::XMLNode* vs_elem, tinyxml2::XMLDocument* dest_doc)
    {
        tinyxml2::XMLNode* vs_elem_dst;

        vs_elem_dst = vs_elem->DeepClone(dest_doc);
        dest_doc->InsertEndChild(vs_elem_dst);
    }

    /*!
     * \fn       merge_xml_to_doc
     * \brief    This function is used to merge two XML files
     * \return   Void
     */
    void oam_agent_hdlr::merge_xml_to_doc(tinyxml2::XMLNode* elem_src, tinyxml2::XMLDocument* dest_doc)
    {
        tinyxml2::XMLNode* vs_elemRoot;
        tinyxml2::XMLNode* vs_elemDstB;

        vs_elemRoot = dest_doc->FirstChildElement();

        if (vs_elemRoot)
        {
            vs_elemRoot = vs_elemRoot->FirstChildElement();

            if (vs_elemRoot)
            {
                elem_src = elem_src->FirstChildElement();

                if (elem_src)
                {
                    elem_src = elem_src->FirstChildElement();

                    while (elem_src)
                    {
                        std::string sValue;
                        char* cValue = (char*)elem_src->Value();
                        sValue = static_cast<std::string>(cValue);

                        if (sValue.compare("id") != 0)
                        {
                            vs_elemDstB = elem_src->DeepClone(dest_doc);
                            vs_elemRoot->InsertEndChild(vs_elemDstB);
                        }

                        if (elem_src->NextSiblingElement())
                        {
                            elem_src = elem_src->NextSiblingElement();
                        }
                        else
                        {
                            break;
                        }
                    }//while
                }
            }
        }
        else
        {
            copy_xml_to_doc(elem_src, dest_doc);
        }
    }

    void build_req_uplane_conf(oam::user_plane_conf& user_plane_conf, sr_change_oper_e op, sr_val_t *val)
    {
      char cmp_str[1024];
      memset(&cmp_str, 0, 1024);
      //TODO xpaht string parser

      /* parser
      *  val->xpath
       *  example xpath : /o-ran-uplane-conf:user-plane-configuration/low-level-tx-endpoints[name='lowl_tx_1']/cp-length
       *
       * val->type
       *  SR_STRING_T,      val->data.string_val
       *  SR_BOOL_T,        val->data.bool_val
       *  SR_UINT32_T,      val->data.uint32_val
       *  SR_ENUM_T,        val->data.enum_val
       */
    }

    /*!
     * \fn       dynamic_cfg_read
     * \brief    This function is handle confd reader dynamic
     * \return   SUCCESS / FAILURE
     */
    oam::ret_t oam_agent_hdlr::dynamic_cfg_read(std::string top_cnt, t_cfg_action action, sr_change_oper_e op, sr_val_t *val)
    {
      oam::ret_t ret = oam::ret_t::FAILURE;
      static oam::config_request* request;

      if (action == T_CFG_CREATE)
      {
        if (nullptr != request)
        {
          //delete request;
          request = nullptr;
        }

        std::cout << "[liboam] request new:" << std::endl;
        request = new oam::config_request();
      }
      else if (action == T_CFG_SEND)
      {
        std::cout << "[liboam] request send:" << std::endl;
        //FORM AND SEND THE MESSAGE IN OAM INTF
        oam::oam_intf_msg* oam_intf_msg = new oam::oam_intf_msg((void*)request,
                sizeof(oam::config_request), oam::oam_intf_msg::msg_type::CONFIG_REQUEST);

        oam::oam_app_intf::get_instance().on_message_received(oam_intf_msg);
        ret = oam::ret_t::SUCCESS;

        if (nullptr != request)
        {
          //delete request;
          request = nullptr;
        }
        ret = oam::ret_t::SUCCESS;
        return ret;
      }
      if (top_cnt.compare(XPATH_ORAN_UPLANE_CONF) == 0)
      {
        oam::user_plane_conf& user_plane_conf = request->get_config_request_info_oran();
        build_req_uplane_conf(user_plane_conf, op, val);
      }

      ret = oam::ret_t::SUCCESS;
      return ret;
    }

    /*!
     * \fn   static_cfg_read
     * \brief    This function is handle confd reader
     * \return   SUCCESS / FAILURE
     */
    oam::ret_t oam_agent_hdlr::static_cfg_read()
    {
        static bool reading = false;
        oam::ret_t ret = oam::ret_t::FAILURE;
        std::cout << "[liboam] static_cfg_read:" << std::endl;
        if (reading == true)
        {
          ret = oam::ret_t::SUCCESS;
          return ret;
        }
        reading = true;

        ret = establish_confd_read_connection();

        if (oam::ret_t::FAILURE == ret)
        {
            std::cout << "oam_agent_hdlr::static_cfg_read FAILED" << std::endl;
            //TODO exit or pthread cancel/join?
            exit(0);
            ret = oam::ret_t::FAILURE;
        }

        oam::config_request* local_config_request = allocated_and_return_config_request();

        if (nullptr == local_config_request)
        {
            reading = false;
            ret = oam::ret_t::FAILURE;
            return ret;
        }

        oam::oran_interfaces& _intf = local_config_request->get_config_request_info_intf();
        oam::processing_elements& _pe = local_config_request->get_config_request_info_pe();
        oam::user_plane_conf& _user_plane_conf = local_config_request->get_config_request_info_oran();
        oam::delay_management& _dm = local_config_request->get_config_request_info_dm();
        _intf.read(confd_read_sock, root_path_oran_intf);
        _pe.read(confd_read_sock, root_path_oran_pe);
        _user_plane_conf.read(confd_read_sock, root_path_oran);
        _dm.read(confd_read_sock, root_path_oran_dm);

        cdb_end_session(confd_read_sock);
        close_confd_read_connection();

        //FORM AND SEND THE MESSAGE IN OAM INTF
        oam::oam_intf_msg* oam_intf_msg = new oam::oam_intf_msg((void*)local_config_request,
                sizeof(oam::config_request), oam::oam_intf_msg::msg_type::CONFIG_REQUEST);

        if (oam_intf_str_logging)
        {
            std::string log_string = {0};
            std::stringstream oam_intf_msg_string(log_string);
            uint32_t indent = 0;

            // config
            _intf.to_string(oam_intf_msg_string, indent);
            _pe.to_string(oam_intf_msg_string, indent);
            _user_plane_conf.to_string(oam_intf_msg_string, indent);
            _dm.to_string(oam_intf_msg_string, indent);
            // capability
            oam::uplane_conf_cap::get_instance()->to_string(oam_intf_msg_string, indent);
            std::cout << oam_intf_msg_string.str();
        }

        oam::oam_app_intf::get_instance().on_message_received(oam_intf_msg);
        ret = oam::ret_t::SUCCESS;

        deallocated_and_reset_config_request();
        //delete oam_intf_msg;

        reading = false;
        ret = oam::ret_t::SUCCESS;
        return ret;
    }
    /*!
     * \fn   confd_connect_and_read
     * \brief    This function is handle confd connect and read
     * \return   SUCCESS / FAILURE
     */
    void oam_agent_hdlr::confd_connect_and_read()
    {
    }

            /*!
             * \fn   init_enum_map
             * \brief    This function construct the map of ENUM string to integer
             * \return   void
             */
    void oam_agent_hdlr::init_enum_map()
    {
enum_map["COMMUNICATIONS_ALARM"] = 2;
enum_map["QUALITY_OF_SERVICE_ALARM"] = 3;
enum_map["PROCESSING_ERROR_ALARM"] = 4;
enum_map["EQUIPMENT_ALARM"] = 5;
enum_map["ENVIRONMENTAL_ALARM"] = 6;
enum_map["INTEGRITY_VIOLATION"] = 7;
enum_map["OPERATIONAL_VIOLATION"] = 8;
enum_map["PHYSICAL_VIOLATIONu"] = 9;
enum_map["SECURITY_SERVICE_OR_MECHANISM_VIOLATION"] = 10;
enum_map["TIME_DOMAIN_VIOLATION"] = 11;
enum_map["CRITICAL"] = 3;
enum_map["MAJOR"] = 4;
enum_map["MINOR"] = 5;
enum_map["WARNING"] = 6;
enum_map["INDETERMINATE"] = 7;
enum_map["CLEARED"] = 8;
enum_map["RRU"] = 0;
enum_map["BBU"] = 1;
enum_map["Wide-area-BS-cabinet"] = 2;
enum_map["Medium-range-BS"] = 3;
enum_map["Indoor"] = 0;
enum_map["Outdoor"] = 1;
enum_map["AC"] = 0;
enum_map["DC"] = 1;
enum_map["INCREASING"] = 0;
enum_map["DECREASING"] = 1;
enum_map["Enabled"] = 0;
enum_map["Disabled"] = 1;
enum_map["Locked"] = 0;
enum_map["Shutdown"] = 1;
enum_map["Unlocked"] = 2;
enum_map["IN TEST"] = 0;
enum_map["FAILED"] = 1;
enum_map["POWER OFF"] = 2;
enum_map["OFF LINE"] = 3;
enum_map["OFF DUTY"] = 4;
enum_map["DEPENDENCY"] = 5;
enum_map["DEGRADED"] = 6;
enum_map["NOT INSTALLED"] = 7;
enum_map["LOG FULL"] = 8;
enum_map["Idle"] = 0;
enum_map["Inactive"] = 1;
enum_map["Active"] = 2;
enum_map["Normal"] = 0;
enum_map["Extended"] = 1;
enum_map["DL"] = 0;
enum_map["UL"] = 1;
enum_map["DL and UL"] = 2;
//enum_map["DL"] = 0;
//enum_map["UL"] = 1;
enum_map["SUL"] = 2;
enum_map["INITIAL"] = 0;
enum_map["OTHER"] = 1;
enum_map["disableNs"] = 0;
enum_map["oamBasedNsCfg"] = 1;
enum_map["sliceMgrBasedNsCfg"] = 2;
enum_map["nTimingAdvance_0"] = 0;
enum_map["nTimingAdvance_25600"] = 1;
enum_map["nTimingAdvance_39936"] = 2;
enum_map["tciInDci_disabled"] = 0;
enum_map["tciInDci_enabled"] = 1;
enum_map["NSA"] = 0;
enum_map["SA"] = 1;
enum_map["NSA_SA"] = 2;
enum_map["TA_TIMER_PRD_INFINITY_MS"] = 0;
enum_map["TA_TIMER_PRD_500MS"] = 1;
enum_map["TA_TIMER_PRD_750MS"] = 2;
enum_map["TA_TIMER_PRD_1280MS"] = 3;
enum_map["TA_TIMER_PRD_1920MS"] = 4;
enum_map["TA_TIMER_PRD_2560MS"] = 5;
enum_map["TA_TIMER_PRD_5120MS"] = 6;
enum_map["TA_TIMER_PRD_10240MS"] = 7;
//manual update
enum_map["am_size12"] = 1;
enum_map["am_size18"] = 2;
enum_map["um_size6"] = 1;
enum_map["um_size12"] = 2;
enum_map["milliseconds5"] = 5;
enum_map["milliseconds10"] = 10;
enum_map["milliseconds15"] = 15;
enum_map["milliseconds20"] = 20;
enum_map["milliseconds25"] = 25;
enum_map["milliseconds30"] = 30;
enum_map["milliseconds35"] = 35;
enum_map["milliseconds40"] = 40;
enum_map["milliseconds45"] = 45;
enum_map["milliseconds50"] = 50;
enum_map["milliseconds55"] = 55;
enum_map["milliseconds60"] = 60;
enum_map["milliseconds65"] = 65;
enum_map["milliseconds70"] = 70;
enum_map["milliseconds75"] = 75;
enum_map["milliseconds80"] = 80;
enum_map["milliseconds85"] = 85;
enum_map["milliseconds90"] = 90;
enum_map["milliseconds95"] = 95;
enum_map["milliseconds100"] = 100;
enum_map["milliseconds105"] = 105;
enum_map["milliseconds110"] = 110;
enum_map["milliseconds115"] = 115;
enum_map["milliseconds120"] = 120;
enum_map["milliseconds125"] = 125;
enum_map["milliseconds130"] = 130;
enum_map["milliseconds135"] = 135;
enum_map["milliseconds140"] = 140;
enum_map["milliseconds145"] = 145;
enum_map["milliseconds150"] = 150;
enum_map["milliseconds155"] = 155;
enum_map["milliseconds160"] = 160;
enum_map["milliseconds165"] = 165;
enum_map["milliseconds170"] = 170;
enum_map["milliseconds175"] = 175;
enum_map["milliseconds180"] = 180;
enum_map["milliseconds185"] = 185;
enum_map["milliseconds190"] = 190;
enum_map["milliseconds195"] = 195;
enum_map["milliseconds200"] = 200;
enum_map["milliseconds205"] = 205;
enum_map["milliseconds210"] = 210;
enum_map["milliseconds215"] = 215;
enum_map["milliseconds220"] = 220;
enum_map["milliseconds225"] = 225;
enum_map["milliseconds230"] = 230;
enum_map["milliseconds235"] = 235;
enum_map["milliseconds240"] = 240;
enum_map["milliseconds245"] = 245;
enum_map["milliseconds250"] = 250;
enum_map["milliseconds300"] = 300;
enum_map["milliseconds350"] = 350;
enum_map["milliseconds400"] = 400;
enum_map["milliseconds450"] = 450;
enum_map["milliseconds500"] = 500;
enum_map["milliseconds800"] = 800;
enum_map["milliseconds1000"] = 1000;
enum_map["milliseconds2000"] = 2000;
enum_map["milliseconds4000"] = 4000;
enum_map["pdu4"] = 4;
enum_map["pdu8"] = 8;
enum_map["pdu16"] = 16;
enum_map["pdu32"] = 32;
enum_map["pdu64"] = 64;
enum_map["pdu128"] = 128;
enum_map["pdu256"] = 256;
enum_map["pdu512"] = 512;
enum_map["pdu1024"] = 1024;
enum_map["pdu2048"] = 2048;
enum_map["pdu4096"] = 4096;
enum_map["pdu6144"] = 6144;
enum_map["pdu8192"] = 8192;
enum_map["pdu12288"] = 12288;
enum_map["pdu16384"] = 16384;
enum_map["pdu20480"] = 20480;
enum_map["pdu24576"] = 24576;
enum_map["pdu28672"] = 28672;
enum_map["pdu32768"] = 32768;
enum_map["pdu40960"] = 40960;
enum_map["pdu49152"] = 49152;
enum_map["pdu57344"] = 57344;
enum_map["pdu65536"] = 65536;
enum_map["infinity"] = -1;
enum_map["kiloByte1"] = 0;
enum_map["kiloByte2"] = 1;
enum_map["kiloByte5"] = 2;
enum_map["kiloByte8"] = 3;
enum_map["kiloByte10"] = 4;
enum_map["kiloByte15"] = 5;
enum_map["kiloByte25"] = 6;
enum_map["kiloByte50"] = 7;
enum_map["kiloByte75"] = 8;
enum_map["kiloByte100"] = 9;
enum_map["kiloByte125"] = 10;
enum_map["kiloByte250"] = 11;
enum_map["kiloByte375"] = 12;
enum_map["kiloByte500"] = 13;
enum_map["kiloByte750"] = 14;
enum_map["kiloByte1000"] = 15;
enum_map["kiloByte1250"] = 16;
enum_map["kiloByte1500"] = 17;
enum_map["kiloByte2000"] = 18;
enum_map["kiloByte3000"] = 19;
enum_map["kiloByte4000"] = 20;
enum_map["kiloByte4500"] = 21;
enum_map["kiloByte5000"] = 22;
enum_map["kiloByte5500"] = 23;
enum_map["kiloByte6000"] = 24;
enum_map["kiloByte6500"] = 25;
enum_map["kiloByte7000"] = 26;
enum_map["kiloByte7500"] = 27;
enum_map["megaByte8"] = 28;
enum_map["megaByte9"] = 29;
enum_map["megaByte10"] = 30;
enum_map["megaByte11"] = 31;
enum_map["megaByte12"] = 32;
enum_map["megaByte13"] = 33;
enum_map["megaByte14"] = 34;
enum_map["megaByte15"] = 35;
enum_map["megaByte16"] = 36;
enum_map["megaByte17"] = 37;
enum_map["megaByte18"] = 38;
enum_map["megaByte20"] = 39;
enum_map["megaByte25"] = 40;
enum_map["megaByte30"] = 41;
enum_map["megaByte40"] = 42;
enum_map["timer1"] = 1;
enum_map["timer2"] = 2;
enum_map["timer3"] = 3;
enum_map["timer4"] = 4;
enum_map["timer6"] = 6;
enum_map["timer8"] = 8;
enum_map["timer16"] = 16;
enum_map["timer32"] = 32;
enum_map["milliseconds1200"] = 1200;
enum_map["milliseconds1600"] = 1600;
enum_map["milliseconds2000"] = 2000;
enum_map["milliseconds2400"] = 2400;
enum_map["FDD"] = 0;
enum_map["TDD"] = 1;
enum_map["scs_KHz15"] = 0;
enum_map["scs_KHz30"] = 1;
enum_map["scs_KHz60"] = 2;
enum_map["scs_KHz120"] = 3;
enum_map["scs_KHz240"] = 4;
enum_map["freqRangeType_FR1_LT_3"] = 0;
enum_map["freqRangeType_FR1_LT_6"] = 1;
enum_map["freqRangeType_FR2"] = 2;
enum_map["cyclicPrefix_NORMAL"] = 0;
enum_map["cyclicPrefix_EXTENDED"] = 1;
enum_map["dmrsTypAPos_pos2"] = 0;
enum_map["dmrsTypAPos_pos3"] = 1;
enum_map["MCS_TBL_64QAM"] = 0;
enum_map["MCS_TBL_256QAM"] = 1;
enum_map["extended"] = 0;
enum_map["slot_1Null"] = 0;
enum_map["slot_2I"] = 1;
enum_map["slot_4I"] = 2;
enum_map["slot_5I"] = 3;
enum_map["slot_8I"] = 4;
enum_map["slot_10I"] = 5;
enum_map["slot_16I"] = 6;
enum_map["slot_20I"] = 7;
enum_map["slot_40I"] = 8;
enum_map["slot_80I"] = 9;
enum_map["slot_160I"] = 10;
enum_map["slot_320I"] = 11;
enum_map["slot_640I"] = 12;
enum_map["slot_1280I"] = 13;
enum_map["slot_2560I"] = 14;
enum_map["CHAN_BW_5MHZ"] = 0;
enum_map["CHAN_BW_10MHZ"] = 1;
enum_map["CHAN_BW_15MHZ"] = 2;
enum_map["CHAN_BW_20MHZ"] = 3;
enum_map["CHAN_BW_25MHZ"] = 4;
enum_map["CHAN_BW_30MHZ"] = 5;
enum_map["CHAN_BW_40MHZ"] = 6;
enum_map["CHAN_BW_50MHZ"] = 7;
enum_map["CHAN_BW_60MHZ"] = 8;
enum_map["CHAN_BW_80MHZ"] = 9;
enum_map["CHAN_BW_100MHZ"] = 10;
enum_map["CHAN_BW_200MHZ"] = 11;
enum_map["CHAN_BW_400MHZ"] = 12;
enum_map["srPeriodicity_sym2"] = 0;
enum_map["srPeriodicity_sym6or7"] = 1;
enum_map["srPeriodicity_sl1"] = 2;
enum_map["srPeriodicity_sl2"] = 3;
enum_map["srPeriodicity_sl4"] = 4;
enum_map["srPeriodicity_sl5"] = 5;
enum_map["srPeriodicity_sl8"] = 6;
enum_map["srPeriodicity_sl10"] = 7;
enum_map["srPeriodicity_sl16"] = 8;
enum_map["srPeriodicity_sl20"] = 9;
enum_map["srPeriodicity_sl40"] = 10;
enum_map["srPeriodicity_sl80"] = 11;
enum_map["srPeriodicity_sl160"] = 12;
enum_map["srPeriodicity_sl320"] = 13;
enum_map["srPeriodicity_sl640"] = 14;
//manual update
//enum_map["csiPeriodicity_slots"] = 0;
enum_map["csiPeriodicity_slots4"] = 4;
enum_map["csiPeriodicity_slots5"] = 5;
enum_map["csiPeriodicity_slots8"] = 8;
enum_map["csiPeriodicity_slots10"] = 10;
enum_map["csiPeriodicity_slots16"] = 16;
enum_map["csiPeriodicity_slots20"] = 20;
enum_map["csiPeriodicity_slots40"] = 40;
enum_map["csiPeriodicity_slots80"] = 80;
enum_map["csiPeriodicity_slots160"] = 160;
enum_map["csiPeriodicity_slots320"] = 320;
enum_map["codeBook"] = 0;
enum_map["nonCodeBook"] = 1;
enum_map["none"] = 2;
enum_map["pucchGroupHopping_neither"] = 0;
enum_map["pucchGroupHopping_enable"] = 1;
enum_map["pucchGroupHopping_disable"] = 2;
// manual update
//enum_map["maxCodeRate_zeroDot0"] = 0;
enum_map["maxCodeRate_zeroDot08"] = 8;
enum_map["maxCodeRate_zeroDot15"] = 15;
enum_map["maxCodeRate_zeroDot25"] = 25;
enum_map["maxCodeRate_zeroDot35"] = 35;
enum_map["maxCodeRate_zeroDot45"] = 45;
enum_map["maxCodeRate_zeroDot60"] = 60;
enum_map["maxCodeRate_zeroDot80"] = 80;
enum_map["preambleTransMax_n3"] = 0;
enum_map["preambleTransMax_n4"] = 1;
enum_map["preambleTransMax_n5"] = 2;
enum_map["preambleTransMax_n6"] = 3;
enum_map["preambleTransMax_n7"] = 4;
enum_map["preambleTransMax_n8"] = 5;
enum_map["preambleTransMax_n10"] = 6;
enum_map["preambleTransMax_n20"] = 7;
enum_map["preambleTransMax_n50"] = 8;
enum_map["preambleTransMax_n100"] = 9;
enum_map["preambleTransMax_n200"] = 10;
enum_map["pwrRampingStep_dB0"] = 0;
enum_map["pwrRampingStep_dB2"] = 1;
enum_map["pwrRampingStep_dB4"] = 2;
enum_map["pwrRampingStep_dB6"] = 3;
// manual update:
//enum_map["rachResponseWindow_sl"] = 0;
enum_map["rachResponseWindow_sl1"] = 1;
enum_map["rachResponseWindow_sl2"] = 2;
enum_map["rachResponseWindow_sl4"] = 4;
enum_map["rachResponseWindow_sl8"] = 8;
enum_map["rachResponseWindow_sl10"] = 10;
enum_map["rachResponseWindow_sl20"] = 20;
enum_map["rachResponseWindow_sl40"] = 40;
enum_map["rachResponseWindow_sl80"] = 80;
//enum_map["msg3SizeGroupA_b5"] = 0;
enum_map["msg3SizeGroupA_b56"] = 56;
enum_map["msg3SizeGroupA_b72"] = 72;
enum_map["msg3SizeGroupA_b208"] = 208;
enum_map["msg3SizeGroupA_b256"] = 256;
enum_map["msg3SizeGroupA_b282"] = 282;
enum_map["msg3SizeGroupA_b480"] = 480;
enum_map["msg3SizeGroupA_b640"] = 640;
enum_map["msg3SizeGroupA_b800"] = 800;
enum_map["msg3SizeGroupA_b1000"] = 1000;
enum_map["SSB_ONE_EIGHT"] = 0;
enum_map["SSB_ONE_FOURTH"] = 1;
enum_map["SSB_ONE_HALF"] = 2;
enum_map["SSB_ONE"] = 3;
enum_map["SSB_TWO"] = 4;
enum_map["SSB_FOUR"] = 5;
enum_map["SSB_EIGHT"] = 6;
enum_map["SSB_SIXTEEN"] = 7;
//manual update
enum_map["contentionResolutionTmr_sf8"] = 8;
enum_map["contentionResolutionTmr_sf16"] = 16;
enum_map["contentionResolutionTmr_sf24"] = 24;
enum_map["contentionResolutionTmr_sf32"] = 32;
enum_map["contentionResolutionTmr_sf40"] = 40;
enum_map["contentionResolutionTmr_sf48"] = 48;
enum_map["contentionResolutionTmr_sf56"] = 56;
enum_map["contentionResolutionTmr_sf64"] = 64;
enum_map["rootSeqId_L839Int"] = 0;
enum_map["rootSeqId_L139Int"] = 1;
enum_map["msg1Scs_KHz15"] = 0;
enum_map["msg1Scs_KHz30"] = 1;
enum_map["msg1Scs_KHz60"] = 2;
enum_map["msg1Scs_KHz120"] = 3;
enum_map["msg1Scs_KHz240"] = 4;
enum_map["RACH_UNREST"] = 0;
enum_map["RACH_REST_TYPE_A"] = 1;
enum_map["RACH_REST_TYPE_B"] = 2;
enum_map["ORAN_RACH_FORMAT_0"] = 0;
enum_map["ORAN_RACH_FORMAT_1"] = 1;
enum_map["ORAN_RACH_FORMAT_2"] = 2;
enum_map["ORAN_RACH_FORMAT_3"] = 3;
enum_map["ORAN_RACH_FORMAT_A1"] = 4;
enum_map["ORAN_RACH_FORMAT_A2"] = 5;
enum_map["ORAN_RACH_FORMAT_A3"] = 6;
enum_map["ORAN_RACH_FORMAT_B1"] = 7;
enum_map["ORAN_RACH_FORMAT_B2"] = 8;
enum_map["ORAN_RACH_FORMAT_B3"] = 9;
enum_map["ORAN_RACH_FORMAT_B4"] = 10;
enum_map["ORAN_RACH_FORMAT_C0"] = 11;
enum_map["ORAN_RACH_FORMAT_C2"] = 12;
// manual update:
enum_map["POWER_ALPHA0"] = 0;
enum_map["POWER_ALPHA4"] = 4;
enum_map["POWER_ALPHA5"] = 5;
enum_map["POWER_ALPHA6"] = 6;
enum_map["POWER_ALPHA7"] = 7;
enum_map["POWER_ALPHA8"] = 8;
enum_map["POWER_ALPHA9"] = 9;
enum_map["POWER_ALPHALL"] = 10;
enum_map["FREQ_SHFT_7P5KHZ_DISBL"] = 0;
enum_map["FREQ_SHFT_7P5KHZ_ENBL"] = 1;
enum_map["sibT_rf8"] = 8;
enum_map["sibT_rf16"] = 16;
enum_map["sibT_rf32"] = 32;
enum_map["sibT_rf64"] = 64;
enum_map["sibT_rf128"] = 128;
enum_map["sibT_rf256"] = 256;
enum_map["sibT_rf512"] = 512;
enum_map["sibType_DU_SIB_TYPE_2"] = 0;
enum_map["sibType_DU_SIB_TYPE_3"] = 1;
enum_map["prachFdm_one"] = 0;
enum_map["prachFdm_two"] = 1;
enum_map["prachFdm_four"] = 2;
enum_map["prachFdm_eight"] = 3;
enum_map["prachSsbRach_oneEight"] = 0;
enum_map["prachSsbRach_oneFourth"] = 1;
enum_map["prachSsbRach_oneHalf"] = 2;
enum_map["prachSsbRach_one"] = 3;
enum_map["prachSsbRach_two"] = 4;
enum_map["prachSsbRach_four"] = 5;
enum_map["prachSsbRach_eight"] = 6;
enum_map["scs15or60"] = 0;
enum_map["scs30or120"] = 1;
enum_map["barred"] = 0;
enum_map["notBarred"] = 1;
enum_map["allowed"] = 0;
enum_map["notAllowed"] = 1;
enum_map["reserved"] = 0;
enum_map["notReserved"] = 1;
enum_map["T300_MS100"] = 0;
enum_map["T300_MS200"] = 1;
enum_map["T300_MS300"] = 2;
enum_map["T300_MS400"] = 3;
enum_map["T300_MS600"] = 4;
enum_map["T300_MS1000"] = 5;
enum_map["T300_MS1500"] = 6;
enum_map["T300_MS2000"] = 7;
enum_map["T301_MS100"] = 0;
enum_map["T301_MS200"] = 1;
enum_map["T301_MS300"] = 2;
enum_map["T301_MS400"] = 3;
enum_map["T301_MS600"] = 4;
enum_map["T301_MS1000"] = 5;
enum_map["T301_MS1500"] = 6;
enum_map["T301_MS2000"] = 7;
enum_map["T310_MS0"] = 0;
enum_map["T310_MS50"] = 1;
enum_map["T310_MS100"] = 2;
enum_map["T310_MS200"] = 3;
enum_map["T310_MS500"] = 4;
enum_map["T310_MS1000"] = 5;
enum_map["T310_MS2000"] = 6;
enum_map["N310_N1"] = 0;
enum_map["N310_N2"] = 1;
enum_map["N310_N3"] = 2;
enum_map["N310_N4"] = 3;
enum_map["N310_N6"] = 4;
enum_map["N310_N8"] = 5;
enum_map["N310_N10"] = 6;
enum_map["N310_N20"] = 7;
enum_map["t311TimerAndConst_T311_MS1000"] = 0;
enum_map["t311TimerAndConst_T311_MS3000"] = 1;
enum_map["t311TimerAndConst_T311_MS5000"] = 2;
enum_map["t311TimerAndConst_T311_MS10000"] = 3;
enum_map["t311TimerAndConst_T311_MS15000"] = 4;
enum_map["t311TimerAndConst_T311_MS20000"] = 5;
enum_map["t311TimerAndConst_T311_MS30000"] = 6;
enum_map["N311_N1"] = 0;
enum_map["N311_N2"] = 1;
enum_map["N311_N3"] = 2;
enum_map["N311_N4"] = 3;
enum_map["N311_N5"] = 4;
enum_map["N311_N6"] = 5;
enum_map["N311_N8"] = 6;
enum_map["N311_N10"] = 7;
enum_map["t319TimerAndConst_T319_MS100"] = 0;
enum_map["t319TimerAndConst_T319_MS200"] = 1;
enum_map["t319TimerAndConst_T319_MS300"] = 2;
enum_map["t319TimerAndConst_T319_MS400"] = 3;
enum_map["t319TimerAndConst_T319_MS600"] = 4;
enum_map["t319TimerAndConst_T319_MS1000"] = 5;
enum_map["t319TimerAndConst_T319_MS1500"] = 6;
enum_map["t319TimerAndConst_T319_MS2000"] = 7;
enum_map["type1"] = 0;
enum_map["type2"] = 1;
enum_map["pos0"] = 0;
enum_map["pos1"] = 1;
enum_map["pos2"] = 2;
enum_map["pos3"] = 3;
enum_map["len1"] = 0;
enum_map["len2"] = 1;
enum_map["disabled"] = 0;
enum_map["enabled"] = 1;
enum_map["NS_RES_ISOLATED"] = 0;
enum_map["NS_RES_SHARED"] = 1;
enum_map["firstPDCCH_Monitoring_sCS15KHZoneT"] = 0;
enum_map["firstPDCCH_Monitoring_sCS30KHZoneT_SCS15KHZhalfT"] = 1;
enum_map["firstPDCCH_Monitoring_sCS60KHZoneT_SCS30KHZhalfT_SCS15KHZquarterT"] = 2;
enum_map["firstPDCCH_Monitoring_sCS120KHZoneT_SCS60KHZhalfT_SCS30KHZquarterT_SCS15KHZoneEighthT"] = 3;
enum_map["firstPDCCH_Monitoring_sCS120KHZhalfT_SCS60KHZquarterT_SCS30KHZoneEighthT_SCS15KHZoneSixteenthT"] = 4;
enum_map["firstPDCCH_Monitoring_sCS120KHZquarterT_SCS60KHZoneEighthT_SCS30KHZoneSixteenthT"] = 5;
enum_map["firstPDCCH_Monitoring_sCS120KHZoneEighthT_SCS60KHZoneSixteenthT"] = 6;
enum_map["firstPDCCH_Monitoring_sCS120KHZoneSixteenthT"] = 7;
enum_map["defaultPagCycle_rf32"] = 0;
enum_map["defaultPagCycle_rf64"] = 1;
enum_map["defaultPagCycle_rf128"] = 2;
enum_map["defaultPagCycle_rf256"] = 3;
enum_map["PAGING_DURATION_ONE_T"] = 0;
enum_map["PAGING_DURATION_HALF_T"] = 1;
enum_map["PAGING_DURATION_QUARTER_T"] = 2;
enum_map["PAGING_DURATION_ONE_EIGHTH_T"] = 3;
enum_map["PAGING_DURATION_ONE_SIXTEENTH_T"] = 4;
enum_map["ns_four"] = 0;
enum_map["ns_two"] = 1;
enum_map["ns_one"] = 2;
enum_map["drx-onDurationTimerSubMilliSeconds"] = 0;
enum_map["drx-onDurationTimerMilliSeconds"] = 1;
//manual
enum_map["NON_XRAN_FDD"] = 3;
enum_map["NON_XRAN_TDD"] = 0;
enum_map["XRAN"] = 4;
enum_map["TIMER"] = 1;
enum_map["DRX_TMR_PRD_0MS"] = 0;
enum_map["DRX_TMR_PRD_1MS"] = 1;
enum_map["DRX_TMR_PRD_2MS"] = 2;
enum_map["DRX_TMR_PRD_3MS"] = 3;
enum_map["DRX_TMR_PRD_4MS"] = 4;
enum_map["DRX_TMR_PRD_5MS"] = 5;
enum_map["DRX_TMR_PRD_6MS"] = 6;
enum_map["DRX_TMR_PRD_8MS"] = 7;
enum_map["DRX_TMR_PRD_10MS"] = 8;
enum_map["DRX_TMR_PRD_20MS"] = 9;
enum_map["DRX_TMR_PRD_30MS"] = 10;
enum_map["DRX_TMR_PRD_40MS"] = 11;
enum_map["DRX_TMR_PRD_50MS"] = 12;
enum_map["DRX_TMR_PRD_60MS"] = 13;
enum_map["DRX_TMR_PRD_80MS"] = 14;
enum_map["DRX_TMR_PRD_100MS"] = 15;
enum_map["DRX_TMR_PRD_200MS"] = 16;
enum_map["DRX_TMR_PRD_300MS"] = 17;
enum_map["DRX_TMR_PRD_500MS"] = 18;
enum_map["DRX_TMR_PRD_750MS"] = 19;
enum_map["DRX_TMR_PRD_1280MS"] = 20;
enum_map["DRX_TMR_PRD_1920MS"] = 21;
enum_map["DRX_TMR_PRD_2560MS"] = 22;
enum_map["DRX_ON_DURATION_TMR_PRD_1MS"] = 0;
enum_map["DRX_ON_DURATION_TMR_PRD_2MS"] = 1;
enum_map["DRX_ON_DURATION_TMR_PRD_3MS"] = 2;
enum_map["DRX_ON_DURATION_TMR_PRD_4MS"] = 3;
enum_map["DRX_ON_DURATION_TMR_PRD_5MS"] = 4;
enum_map["DRX_ON_DURATION_TMR_PRD_6MS"] = 5;
enum_map["DRX_ON_DURATION_TMR_PRD_8MS"] = 6;
enum_map["DRX_ON_DURATION_TMR_PRD_10MS"] = 7;
enum_map["DRX_ON_DURATION_TMR_PRD_20MS"] = 8;
enum_map["DRX_ON_DURATION_TMR_PRD_30MS"] = 9;
enum_map["DRX_ON_DURATION_TMR_PRD_40MS"] = 10;
enum_map["DRX_ON_DURATION_TMR_PRD_50MS"] = 11;
enum_map["DRX_ON_DURATION_TMR_PRD_60MS"] = 12;
enum_map["DRX_ON_DURATION_TMR_PRD_80MS"] = 13;
enum_map["DRX_ON_DURATION_TMR_PRD_100MS"] = 14;
enum_map["DRX_ON_DURATION_TMR_PRD_200MS"] = 15;
enum_map["DRX_ON_DURATION_TMR_PRD_300MS"] = 16;
enum_map["DRX_ON_DURATION_TMR_PRD_400MS"] = 17;
enum_map["DRX_ON_DURATION_TMR_PRD_500MS"] = 18;
enum_map["DRX_ON_DURATION_TMR_PRD_600MS"] = 19;
enum_map["DRX_ON_DURATION_TMR_PRD_800MS"] = 20;
enum_map["DRX_ON_DURATION_TMR_PRD_1000MS"] = 21;
enum_map["DRX_ON_DURATION_TMR_PRD_1200MS"] = 22;
enum_map["DRX_ON_DURATION_TMR_PRD_1600MS"] = 23;
enum_map["SLOT_DRX_TMR_PRD_0Slot"] = 0;
enum_map["SLOT_DRX_TMR_PRD_1Slot"] = 1;
enum_map["SLOT_DRX_TMR_PRD_2Slot"] = 2;
enum_map["SLOT_DRX_TMR_PRD_4Slot"] = 3;
enum_map["SLOT_DRX_TMR_PRD_6Slot"] = 4;
enum_map["SLOT_DRX_TMR_PRD_8Slot"] = 5;
enum_map["SLOT_DRX_TMR_PRD_16Slot"] = 6;
enum_map["SLOT_DRX_TMR_PRD_24Slot"] = 7;
enum_map["SLOT_DRX_TMR_PRD_33Slot"] = 8;
enum_map["SLOT_DRX_TMR_PRD_40Slot"] = 9;
enum_map["SLOT_DRX_TMR_PRD_64Slot"] = 10;
enum_map["SLOT_DRX_TMR_PRD_80Slot"] = 11;
enum_map["SLOT_DRX_TMR_PRD_96Slot"] = 12;
enum_map["SLOT_DRX_TMR_PRD_112Slot"] = 13;
enum_map["SLOT_DRX_TMR_PRD_128Slot"] = 14;
enum_map["SLOT_DRX_TMR_PRD_160Slot"] = 15;
enum_map["SLOT_DRX_TMR_PRD_320Slot"] = 16;
enum_map["DRX_CYCL_PRD_10MS"] = 0;
enum_map["DRX_CYCL_PRD_20MS"] = 1;
enum_map["DRX_CYCL_PRD_32MS"] = 2;
enum_map["DRX_CYCL_PRD_40MS"] = 3;
enum_map["DRX_CYCL_PRD_60MS"] = 4;
enum_map["DRX_CYCL_PRD_64MS"] = 5;
enum_map["DRX_CYCL_PRD_70MS"] = 6;
enum_map["DRX_CYCL_PRD_80MS"] = 7;
enum_map["DRX_CYCL_PRD_128MS"] = 8;
enum_map["DRX_CYCL_PRD_160MS"] = 9;
enum_map["DRX_CYCL_PRD_256MS"] = 10;
enum_map["DRX_CYCL_PRD_320MS"] = 11;
enum_map["DRX_CYCL_PRD_512MS"] = 12;
enum_map["DRX_CYCL_PRD_640MS"] = 13;
enum_map["DRX_CYCL_PRD_1024MS"] = 14;
enum_map["DRX_CYCL_PRD_1280MS"] = 15;
enum_map["DRX_CYCL_PRD_2048MS"] = 16;
enum_map["DRX_CYCL_PRD_2560MS"] = 17;
enum_map["DRX_CYCL_PRD_5120MS"] = 18;
enum_map["DRX_CYCL_PRD_10240MS"] = 19;
enum_map["DRX_SHORT_CYCL_PRD_2MS"] = 0;
enum_map["DRX_SHORT_CYCL_PRD_3MS"] = 1;
enum_map["DRX_SHORT_CYCL_PRD_4MS"] = 2;
enum_map["DRX_SHORT_CYCL_PRD_5MS"] = 3;
enum_map["DRX_SHORT_CYCL_PRD_6MS"] = 4;
enum_map["DRX_SHORT_CYCL_PRD_7MS"] = 5;
enum_map["DRX_SHORT_CYCL_PRD_8MS"] = 6;
enum_map["DRX_SHORT_CYCL_PRD_10MS"] = 7;
enum_map["DRX_SHORT_CYCL_PRD_14MS"] = 8;
enum_map["DRX_SHORT_CYCL_PRD_16MS"] = 9;
enum_map["DRX_SHORT_CYCL_PRD_20MS"] = 10;
enum_map["DRX_SHORT_CYCL_PRD_30MS"] = 11;
enum_map["DRX_SHORT_CYCL_PRD_32MS"] = 12;
enum_map["DRX_SHORT_CYCL_PRD_35MS"] = 13;
enum_map["DRX_SHORT_CYCL_PRD_40MS"] = 14;
enum_map["DRX_SHORT_CYCL_PRD_64MS"] = 15;
enum_map["DRX_SHORT_CYCL_PRD_80MS"] = 16;
enum_map["DRX_SHORT_CYCL_PRD_128MS"] = 17;
enum_map["DRX_SHORT_CYCL_PRD_160MS"] = 18;
enum_map["DRX_SHORT_CYCL_PRD_256MS"] = 19;
enum_map["DRX_SHORT_CYCL_PRD_320MS"] = 20;
enum_map["DRX_SHORT_CYCL_PRD_512MS"] = 21;
enum_map["DRX_SHORT_CYCL_PRD_640MS"] = 22;
enum_map["OAMAG"] = 0;
enum_map["DUMGR"] = 1;
enum_map["DURRM"] = 2;
enum_map["APPUE"] = 3;
enum_map["CODEC"] = 4;
enum_map["EVENT"] = 5;
enum_map["EGTPU"] = 6;
enum_map["DUUDP"] = 7;
enum_map["DUCMN"] = 8;
enum_map["DURLC"] = 9;
enum_map["DUMAC"] = 10;
enum_map["SCHL1"] = 11;
enum_map["SCHL2"] = 12;
enum_map["DUCL"] = 13;
enum_map["DUNS"] = 14;
enum_map["FSPKT"] = 15;
enum_map["NRUP"] = 16;
enum_map["F1AP"] = 17;
enum_map["SCTP_DU"] = 18;
enum_map["CL"] = 19;
enum_map["COMMON_UMM"] = 20;
enum_map["MAC_COMMON"] = 21;
enum_map["MAC_UL"] = 22;
enum_map["MAC_DL"] = 23;
enum_map["SCH"] = 24;
enum_map["RLC_UL"] = 25;
enum_map["RLC_DL"] = 26;
enum_map["RLC_COMMON"] = 27;
enum_map["INVALID_MODULE"] = 0;
enum_map["APP"] = 1;
enum_map["COMMON"] = 2;
enum_map["OAM_AGENT"] = 3;
enum_map["GNB_MGR"] = 4;
enum_map["CU_UP_MGR"] = 5;
enum_map["RM"] = 6;
enum_map["UE_CONN_MGR"] = 7;
enum_map["BEARER_MGR"] = 8;
enum_map["CODEC_COMMON"] = 9;
enum_map["X2AP_CODEC"] = 10;
enum_map["F1AP_CODEC"] = 11;
enum_map["RRC_CODEC"] = 12;
enum_map["NGAP_CODEC"] = 13;
enum_map["XNAP_CODEC"] = 14;
enum_map["E1AP_CODEC"] = 15;
enum_map["SCTP_COMMON"] = 16;
enum_map["SCTP_CNTRL"] = 17;
enum_map["SCTP_TX"] = 18;
enum_map["SCTP_RX"] = 19;
enum_map["EGTPU_COMMON"] = 20;
enum_map["EGTPU_UPPER_TX_CNTRL"] = 21;
enum_map["EGTPU_UPPER_RX_CNTRL"] = 22;
enum_map["EGTPU_UPPER_TX"] = 23;
enum_map["EGTPU_UPPER_RX"] = 24;
enum_map["EGTPU_LOWER_TX_CNTRL"] = 25;
enum_map["EGTPU_LOWER_RX_CNTRL"] = 26;
enum_map["EGTPU_LOWER_TX"] = 27;
enum_map["EGTPU_LOWER_RX"] = 28;
enum_map["PDCP_COMMON"] = 29;
enum_map["PDCP_TX_CNTRL"] = 30;
enum_map["PDCP_RX_CNTRL"] = 31;
enum_map["PDCP_TX"] = 32;
enum_map["PDCP_RX"] = 33;
enum_map["PDCP_C_CNTRL"] = 34;
enum_map["PDCP_C_RX"] = 35;
enum_map["PDCP_C_TX"] = 36;
enum_map["UDP_CNTRL"] = 37;
enum_map["UDP_TX"] = 38;
enum_map["UDP_RX"] = 39;
enum_map["NRUP_CODEC"] = 40;
enum_map["SDAP_COMMON"] = 41;
enum_map["SDAP_TX_CNTRL"] = 42;
enum_map["SDAP_RX_CNTRL"] = 43;
enum_map["SDAP_TX"] = 44;
enum_map["SDAP_RX"] = 45;
enum_map["SDAP_CODEC"] = 46;
enum_map["TIMER"] = 47;
enum_map["EGTPU_TIMER"] = 48;
enum_map["CRYPTO_RX"] = 49;
enum_map["INVALID_MODULE"] = 0;
enum_map["APP"] = 1;
enum_map["COMMON"] = 2;
enum_map["OAM_AGENT"] = 3;
enum_map["GNB_MGR"] = 4;
enum_map["RM"] = 6;
enum_map["UE_CONN_MGR"] = 7;
enum_map["BEARER_MGR"] = 8;
enum_map["CODEC_COMMON"] = 9;
enum_map["X2AP_CODEC"] = 10;
enum_map["F1AP_CODEC"] = 11;
enum_map["RRC_CODEC"] = 12;
enum_map["NGAP_CODEC"] = 13;
enum_map["XNAP_CODEC"] = 14;
enum_map["E1AP_CODEC"] = 15;
enum_map["SCTP_COMMON"] = 16;
enum_map["SCTP_CNTRL"] = 17;
enum_map["SCTP_TX"] = 18;
enum_map["SCTP_RX"] = 19;
enum_map["PDCP_COMMON"] = 29;
enum_map["PDCP_C_CNTRL"] = 34;
enum_map["PDCP_C_RX"] = 35;
enum_map["PDCP_C_TX"] = 36;
enum_map["TIMER"] = 47;
enum_map["CRYPTO_RX"] = 49;
enum_map["INVALID_MODULE"] = 0;
enum_map["APP"] = 1;
enum_map["COMMON"] = 2;
enum_map["OAM_AGENT"] = 3;
enum_map["CU_UP_MGR"] = 5;
enum_map["RM"] = 6;
enum_map["BEARER_MGR"] = 8;
enum_map["CODEC_COMMON"] = 9;
enum_map["E1AP_CODEC"] = 15;
enum_map["SCTP_COMMON"] = 16;
enum_map["SCTP_CNTRL"] = 17;
enum_map["SCTP_TX"] = 18;
enum_map["SCTP_RX"] = 19;
enum_map["EGTPU_COMMON"] = 20;
enum_map["EGTPU_UPPER_TX_CNTRL"] = 21;
enum_map["EGTPU_UPPER_RX_CNTRL"] = 22;
enum_map["EGTPU_UPPER_TX"] = 23;
enum_map["EGTPU_UPPER_RX"] = 24;
enum_map["EGTPU_LOWER_TX_CNTRL"] = 25;
enum_map["EGTPU_LOWER_RX_CNTRL"] = 26;
enum_map["EGTPU_LOWER_TX"] = 27;
enum_map["EGTPU_LOWER_RX"] = 28;
enum_map["PDCP_COMMON"] = 29;
enum_map["PDCP_TX_CNTRL"] = 30;
enum_map["PDCP_RX_CNTRL"] = 31;
enum_map["PDCP_TX"] = 32;
enum_map["PDCP_RX"] = 33;
enum_map["UDP_CNTRL"] = 37;
enum_map["UDP_TX"] = 38;
enum_map["UDP_RX"] = 39;
enum_map["NRUP_CODEC"] = 40;
enum_map["SDAP_COMMON"] = 41;
enum_map["SDAP_TX_CNTRL"] = 42;
enum_map["SDAP_RX_CNTRL"] = 43;
enum_map["SDAP_TX"] = 44;
enum_map["SDAP_RX"] = 45;
enum_map["SDAP_CODEC"] = 46;
enum_map["TIMER"] = 47;
enum_map["EGTPU_TIMER"] = 48;
enum_map["CRYPTO_RX"] = 49;
enum_map["MEM"] = 4096;
enum_map["BUF"] = 4097;
enum_map["STATS"] = 4098;
enum_map["TIMER"] = 4099;
enum_map["STHREAD"] = 4100;
enum_map["CTHREAD"] = 4101;
enum_map["SYS"] = 4102;
enum_map["EXCP"] = 4103;
enum_map["COMM"] = 4104;
enum_map["SCTP"] = 4105;
enum_map["UDP"] = 4106;
enum_map["TCP"] = 4107;
enum_map["MSGQ"] = 4108;
enum_map["PRIOQ"] = 4109;
enum_map["WORKQ"] = 4110;
enum_map["PERF"] = 4111;
enum_map["NIL"] = 0;
enum_map["FATAL"] = 1;
enum_map["ERR"] = 2;
enum_map["WRN"] = 3;
enum_map["INF"] = 4;
enum_map["TRC"] = 5;
enum_map["ETH-INTERFACE"] = 0;
enum_map["UDPIP-INTERFACE"] = 1;
enum_map["ALIASMAC-INTERFACE"] = 2;
enum_map["SHARED-CELL-ETH-INTERFACE"] = 3;
enum_map["POSITIVE"] = 0;
enum_map["NEGATIVE"] = 1;
enum_map["GPS"] = 0;
enum_map["GLONASS"] = 1;
enum_map["GALILEO"] = 2;
enum_map["BEIDOU"] = 3;
enum_map["G_8275_1"] = 0;
enum_map["G_8275_2"] = 1;
enum_map["FORWARDABLE"] = 0;
enum_map["NONFORWARDABLE"] = 1;
enum_map["NORMAL"] = 0;
enum_map["EXTENDED"] = 1;
enum_map["KHZ_15"] = 0;
enum_map["KHZ_30"] = 1;
enum_map["KHZ_60"] = 2;
enum_map["KHZ_120"] = 3;
enum_map["KHZ_240"] = 4;
enum_map["KHZ_1_25"] = 12;
enum_map["KHZ_3_75"] = 13;
enum_map["KHZ_5"] = 14;
enum_map["KHZ_7_5"] = 15;
enum_map["STATIC"] = 0;
enum_map["DYNAMIC"] = 1;
enum_map["NONE"] = 0;
enum_map["PRACH"] = 1;
enum_map["SRS"] = 2;
enum_map["INACTIVE"] = 0;
enum_map["SLEEP"] = 1;
enum_map["ACTIVE"] = 2;
enum_map["NR"] = 0;
enum_map["LTE"] = 1;
enum_map["DSS_LTE_NR"] = 2;
enum_map["GNSS"] = 0;
enum_map["PTP"] = 1;
enum_map["SYNCE"] = 2;
enum_map["EXT_GNSS"] = 3;
enum_map["INTER_CLOCK"] = 4;

    };

} //namespace oam_agent
