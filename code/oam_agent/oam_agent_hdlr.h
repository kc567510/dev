/*!*************************************************************************************************************************
 *\file        oam_agent_hdlr.h
 *
 *\brief        This source file contains functions for oam config file mapper logic
 **************************************************************************************************************************/
#ifndef __OAM_AGENT_HDLR_H__
#define __OAM_AGENT_HDLR_H__

#include <sys/poll.h>
#include "oam_agent_config_mapper.h"
#include "oam_intf.h"
#include "oam_intf_cm_msg_types.h"
#include "oam_confd_types.h"
#include "oam_yang_def.h"
#include "tinyxml2.h"
#include "synKPI_fm.h"


#define ACTION_COMPLETE     0
#define ACTION_IN_PROGRESS  1
#define Enable              1
#define Disable             0


// 2022/04/27, kevin.Chen , support liteon device alarm message.
#define INFO_DEICE_TEMPERATURE              "/sys/class/hwmon/hwmon3/temp2_input"
#define INFO_DEICE_CPU_USAGE                "/proc/stat"
#define THRESHOLD_DEV_TEMPERATURE           60
#define THRESHOLD_PER_CPU_USAGE             90
#define THRESHOLD_PER_FS_FREE_SPACE         10
#define THRESHOLD_PER_DEV_MEMORY            10
#define CORE_NETWORK_INTERFACE              "eth0"
#define CMD_GET_DEICE_MEMORY                "free -m | grep Mem | awk '{print $7}'"

/*!
* \namespace    oam
 * \brief       This namespace is used for LIB OAM AGENT ConfD implementation
*/
namespace oam_agent
{

    enum t_cfg_action
    {
      T_CFG_CREATE,
      T_CFG_APPEND,
      T_CFG_SEND,
    };

    /*!
    * \class    oam_agent_hdlr
    * \brief    This Class defines implementation of LIB OAM AGENT ConfD mapper
    *        to load them.
    */
    class oam_agent_hdlr
    {
        private:
            static oam_agent_hdlr* instance;/*!<Oam Agent ConfD Handler instance */

            oam_agent::oam_agent_config_mapper config_mapper;          /*!< OAM AGENT CFG Handler */
            ipv4_address_t oam_confd_ip_address;  /*!< IP address for the ConfD Daemon */
            uint16_t     oam_confd_port;          /*!< Port number for the ConfD Daemon */
            bool         oam_intf_str_logging;    /*!< Flag to control OAM INTF STR Logging */
            struct       sockaddr_in addr;        /*!< Sock Address for ConfD Connection */
            int          confd_subs_sock;         /*!< FD for ConfD Susbcription Socket Connection */
            int          confd_read_sock;         /*!< FD for ConfD Read Socket Connection */
            int          confd_maapi_sock;        /*!< FD for ConfD maapi Socket Connection */
            int          confd_stream_sock;       /*!< FD for ConfD stream Socket Connection */
            int          subscription_point_3gpp; /*!< subscription point for 3GPP namespace */

            std::string  root_path_oran;          /*!< Container root path for oran uplane conf YANG */
            std::string  root_path_oran_intf;     /*!< Container root path for oran interfaces YANG */
            std::string  root_path_oran_pe;       /*!< Container root path for oran processing element YANG */
            std::string  root_path_oran_sync;     /*!< Container root path for oran sync YANG */
            std::string  root_path_oran_dm;     /*!< Container root path for oran delay management YANG */

            int          namespace_3gpp;          /*!< Confd namespace for 3GPP YANG*/
            oam::config_request* local_config_request;  /*!< Place holder to keep the 3GPP managedElemnt until all VsData decoded*/
            //confd_debug_level    debug_level;       /*!< ConfD Logging Levels*/
            uint16_t     oam_confd_count;         /*!< oam count to run confd connection */
            uint16_t     oam_confd_timer;         /*!< oam confd timer */
            std::map<std::string, int32_t> enum_map; /*!< Convert ENUM string to value for Sysrepo */
            
            int inotifyFD;                        // inotify descriptor
            int triggerWatchFD;                   // watch descriptor for firmware download
            int triggerWatchFD_FM;                // watch descriptor for FM
            int download_in_progress;
            int install_in_progress;
            int activate_in_progress;
            int resetTimer_in_progress;
            int rst_timer;


            //timer
            int timer_in_progress;
            struct timespec time_start,time_end;
            /*!
             * \fn   init_enum_map
             * \brief    This function construct the map of ENUM string to integer
             * \return   void
             */
            void init_enum_map();
            /*!
             * \fn   oam_agent_hdlr
             * \brief    This function is the constructor for struct oam_agent_hdlr
             * \return   void
             */
            oam_agent_hdlr()
            {
                oam_confd_ip_address = "127.0.0.1";
                oam_confd_port = 11565;
                oam_intf_str_logging = false;
                oam_confd_count = 100;
                oam_confd_timer = 10;
                const char* address = oam_confd_ip_address.c_str();
                addr.sin_addr.s_addr = inet_addr(address);
                addr.sin_family = AF_INET;
                addr.sin_port = htons(oam_confd_port);
                confd_subs_sock = -1;
                confd_read_sock = -1;
                confd_maapi_sock = -1;
                confd_stream_sock = -1;
                subscription_point_3gpp = 0;
                root_path_oran.clear();
                root_path_oran_intf.clear();
                root_path_oran_pe.clear();
                root_path_oran_sync.clear();
                root_path_oran = "/o-ran-uplane-conf:user-plane-configuration";  //synergy: /user-plane-configuration -->
                root_path_oran_intf = "/ietf-interfaces:interfaces";  //synergy: /interfaces -->
                root_path_oran_pe = "/o-ran-processing-element:processing-elements";  //synergy: /processing-elements -->
                root_path_oran_sync = "/o-ran-sync:sync";  //synergy: /sync -->
                root_path_oran_dm = "/o-ran-delay-management:delay-management";  //synergy: /delay-management -->
                namespace_3gpp = 0;
                local_config_request = nullptr;
                //debug_level = CONFD_DEBUG;
                init_enum_map();
            }
        public:
            /*!
             * \fn         get_instance
             * \brief      This function is used for creating new instance of oam_agent_hdlr class
             * \return     oam_agent_hdlr*
             */
            static oam_agent_hdlr* get_instance()
            {
                if (instance == nullptr)
                {
                    instance = new oam_agent_hdlr;
                }

                return instance;
            }

            /*!
             * \fn       copy_xml_to_doc
             * \brief    This function is used to merge two XML files
             * \return   Void
             */
            void copy_xml_to_doc(tinyxml2::XMLNode* elem_src, tinyxml2::XMLDocument* dest_doc);

            /*!
             * \fn       merge_xml_to_doc
             * \brief    This function is used to merge two XML files
             * \return   Void
             */
            void merge_xml_to_doc(tinyxml2::XMLNode* elem_src, tinyxml2::XMLDocument* dest_doc);

            /*!
             * \fn  close_confd_subscription_connection
             * \brief   This function is used to close the ConfD subscription connection
             * \return  void
             */
            void close_confd_subscription_connection();

            /*!
              * \fn establish_confd_subscription
              * \brief  This function is used to establish ConfD subscription connection
              * \return SUCCESS/FAILURE
              */
            oam::ret_t establish_confd_subscription_connection();

            /*!
             * \fn  close_confd_read_connection
             * \brief   This function is used to close the ConfD read connection
             * \return  void
             */
            void close_confd_read_connection();

            /*!
              * \fn establish_confd_read
              * \brief  This function is used to establish ConfD read connection
              * \return SUCCESS/FAILURE
              */
            oam::ret_t establish_confd_read_connection();

            /*!
             * \fn      dynamic_cfg_read
             * \brief   This function is read dynamic cfg
             * \return  SUCCESS / FAILURE
             */
            oam::ret_t dynamic_cfg_read(std::string top_cnt, t_cfg_action action, sr_change_oper_e op, sr_val_t *val);

            /*!
             * \fn  static_cfg_read
             * \brief   This function is handle confd read 3gpp
             * \return  SUCCESS / FAILURE
             */
            oam::ret_t static_cfg_read();

            /*!
             * \fn  confd_connect_and_read
             * \brief   This function is handle confd connect and read
             * \return  void
             */
            void confd_connect_and_read();

            /*!
             * \fn      allocated_and_return_config_request
             * \brief   This function is used to allocate and store the config request local variable in the class
             * \return  void
             */
            oam::config_request* allocated_and_return_config_request();

            /*!
             * \fn      deallocated_and_reset_config_request
             * \brief   This function is used to free and reset the config request local variable in the class
             * \return  void
             */
            void deallocated_and_reset_config_request();

            /*!
             * \fn	get_enum_map
             * \brief	This function is used to map enum string to integer value
             * \return	void
             */
            int32_t get_enum_map(std::string enum_str)
            {
                return enum_map[enum_str]; 
            }

            oam::ret_t SWM_inotify_initial();
            oam::ret_t SWM_moniter_notify_polling();
            oam::ret_t timer_init();
            oam::ret_t SendTimerNotify();

            void process_timer_check(int value)
            {
                if(value == ACTION_IN_PROGRESS)
                {
                    memset(&time_start, 0, sizeof(struct timespec));
                    clock_gettime(CLOCK_MONOTONIC, &time_start);
                    timer_in_progress = 1;  
                }
                else
                {
                    memset(&time_end, 0, sizeof(struct timespec));
                    timer_in_progress = 0;    
                }   

            }      

            void set_resetTimer_value(int value)
            {
                rst_timer = value;
                process_timer_check(value);
            }

            int get_resetTimer_value()
            {
                return rst_timer;
            }
            void set_resetTimer_flag(int value)
            {
                resetTimer_in_progress = value;
                process_timer_check(value);
            }

            int get_resetTimer_flag()
            {
                return resetTimer_in_progress;
            }

            void set_download_flag(int value)
            {
                download_in_progress = value;
                process_timer_check(value);
            }

            int get_download_flag()
            {
                return download_in_progress;
            }
            
            void set_install_flag(int value)
            {
                install_in_progress = value;
                process_timer_check(value);
            }
            
            int get_install_flag()
            {
                return install_in_progress;
            }

            void set_activate_flag(int value)
            {
                activate_in_progress = value;
                process_timer_check(value);
            }

            int get_activate_flag()
            {
                return activate_in_progress;
            }
    };
}
#endif/*__OAM_AGENT_HDLR_H__*/
