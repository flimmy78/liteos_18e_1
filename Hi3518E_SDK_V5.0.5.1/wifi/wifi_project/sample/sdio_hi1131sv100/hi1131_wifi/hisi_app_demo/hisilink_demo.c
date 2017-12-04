/******************************************************************************

                  ��Ȩ���� (C), 2001-2011, ��Ϊ�������޹�˾

 ******************************************************************************
  �� �� ��   : hisilink_demo.c
  �� �� ��   : ����
  ��    ��   : 
  ��������   : 2016��11��29��
  ����޸�   :
  ��������   : HSL_config���Ĵ���
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2016��11��29��
    ��    ��   : 
    �޸�����   : �����ļ�

******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif
#endif
#include "hisilink_adapt.h"
#include "hostapd/hostapd_if.h"
#include "wpa_supplicant/wpa_supplicant.h"
#include "src/common/defs.h"
#include "los_task.h"
#include "sys/prctl.h"
#include <pthread.h>
#include "lwip/ip_addr.h"
#include "driver_hisi_lib_api.h"
#include "hisilink_lib.h"
#include "hisilink_ext.h"
#include <linux/completion.h>

#define HSL_PERIOD_CHANNEL 50     //���ŵ�����
#define HSL_PERIOD_TIMEOUT 70000 //hisilink��ʱʱ��

typedef enum
{
    HSL_STATUS_UNCREATE,
    HSL_STATUS_CREATE,
    HSL_STATUS_RECEIVE, //hisi_link���ڽ����鲥�׶�
    HSL_STATUS_CONNECT, //hisi_link���ڹ����׶�
    HSL_STATUS_BUTT
}hsl_status_enum;
typedef unsigned char hsl_status_enum_type ;

unsigned int         gul_hsl_taskid;
hsl_context_stru     st_context;
hsl_result_stru      gst_hsl_params;
hsl_status_enum_type guc_hsl_status   = 0;
extern unsigned char hsl_receive_flag;
/*****************************************************************************
                                  ��������
*****************************************************************************/
int           hsl_demo_init(void);
int           hsl_demo_main(void);
void          hsl_demo_task_channel_change(void);
int           hsl_demo_connect(hsl_result_stru* pst_params);
int           hsl_demo_connect_prepare(void);
unsigned char hsl_demo_get_status(void);
int           hsl_demo_set_status(hsl_status_enum_type uc_type);
hsl_result_stru* hsl_demo_get_result(void);

/*****************************************************************************
 �� �� ��  : hsl_demo_prepare
 ��������  : hsl demo����hostapdΪ�����鲥��׼��
 �������  : ��
 �������  : ��
 �� �� ֵ  : �ɹ�����HISI_SUCC,�쳣����-HISI_EFAIL
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��25��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
int hsl_demo_prepare(void)
{
    int            l_ret;
    char           ac_ssid[33];
    unsigned char  auc_password[65];
    unsigned int   ul_ssid_len = 0;
    unsigned int   ul_pass_len = 0;
    unsigned char  auc_manufacturer_id[3] = {'0','0','3'};
    unsigned char  auc_device_id[3]       = {'0','0','1'};
    struct netif  *pst_netif;
    ip_addr_t      st_gw;
    ip_addr_t      st_ipaddr;
    ip_addr_t      st_netmask;
    struct hostapd_conf hapd_conf;

    /* ����ɾ��wpa_supplicant��hostapd */
    l_ret = wpa_supplicant_stop();
    if (0 != l_ret)
    {
        HISI_PRINT_WARNING("%s[%d]:wpa_supplicant stop fail",__func__,__LINE__);
    }
    pst_netif = netif_find("wlan0");
    if (NULL != pst_netif)
    {
        dhcps_stop(pst_netif);
    }
    l_ret = hostapd_stop();
    if (0 != l_ret)
    {
        HISI_PRINT_WARNING("%s[%d]:hostapd stop fail",__func__,__LINE__);
    }
    memset(ac_ssid, 0, sizeof(char)*33);
    /* ��ȡ����AP��Ҫ��SSID������ */
    l_ret = hsl_get_ap_params(auc_manufacturer_id, auc_device_id, ac_ssid, &ul_ssid_len, auc_password, &ul_pass_len);
    if (HSL_SUCC != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:get ap params fail",__func__,__LINE__);
        return -HSL_FAIL;
    }

    /* ����AP���� */
    hsl_memset(&hapd_conf, 0, sizeof(struct hostapd_conf));
    hsl_memcpy(hapd_conf.ssid, ac_ssid, ul_ssid_len);
    hsl_memcpy(hapd_conf.key, auc_password, ul_pass_len);
    hsl_memcpy(hapd_conf.ht_capab, "[HT40+]", strlen("[HT40+]"));
    hapd_conf.channel_num           = 6;
    hapd_conf.wpa_key_mgmt          = WPA_KEY_MGMT_PSK;
    hapd_conf.wpa                   = 2;
    hapd_conf.authmode              = HOSTAPD_SECURITY_WPAPSK;
    hapd_conf.wpa_pairwise          = WPA_CIPHER_TKIP | WPA_CIPHER_CCMP;
    hsl_memcpy(hapd_conf.driver, "hisi", 5);
#ifdef HISI_CONNETIVITY_PATCH
    hapd_conf.ignore_broadcast_ssid = 0;
#endif
    l_ret = hostapd_start(NULL, &hapd_conf);
    if (HSL_SUCC != l_ret)
    {
       HISI_PRINT_ERROR("HSL_start_hostapd:start failed[%d]",l_ret);
       return -HSL_FAIL;
    }

    pst_netif = netif_find("wlan0");
    if (HSL_NULL == pst_netif)
    {
        HISI_PRINT_ERROR("HSL_start_dhcps:pst_netif null");
        return -HSL_ERR_NULL;
    }
    /* ����������Ϣ */
    IP4_ADDR(&st_gw, 192, 168, 43, 1);
    IP4_ADDR(&st_ipaddr, 192, 168, 43, 1);
    IP4_ADDR(&st_netmask, 255, 255, 255, 0);
    netif_set_addr(pst_netif, &st_ipaddr, &st_netmask, &st_gw);
    netif_set_up(pst_netif);
    l_ret = dhcps_start(pst_netif);
    if (HSL_SUCC != l_ret)
    {
        HISI_PRINT_ERROR("HSL_start_dhcps:DHCP Server start fail");
        return -HSL_FAIL;
    }
    return HSL_SUCC;
}

/*****************************************************************************
 �� �� ��  : hsl_demo_init
 ��������  : ��ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  : �ɹ�����HISI_SUCC,�쳣����-HISI_EFAIL
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��25��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
int hsl_demo_init(void)
{
    int l_ret;

    memset(&gst_hsl_params, 0, sizeof(hsl_result_stru));
    l_ret = hsl_os_init(&st_context);
    if (HSL_SUCC != l_ret)
    {
        hsl_demo_set_status(HSL_STATUS_UNCREATE);
        return -HSL_FAIL;
    }
    l_ret = hisi_hsl_adapt_init();
    if (HISI_SUCC != l_ret)
    {
        hsl_demo_set_status(HSL_STATUS_UNCREATE);
        HISI_PRINT_ERROR("%s[%d]:hisi_HSL_adapt_init fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
    return HSL_SUCC;
}
/*****************************************************************************
 �� �� ��  : hsl_demo_task_channel_change
 ��������  : hsl demo�����ŵ��̴߳�����
 �������  : ��
 �������  : ��
 �� �� ֵ  : ��
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��25��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
void hsl_demo_task_channel_change(void)
{
    int             l_ret;
    unsigned int    ul_ret;
    unsigned int    ul_time      = 0;
    unsigned int    ul_time_temp = 0;
    unsigned int    ul_wait_time = 0;
    unsigned char  *puc_macaddr;
    unsigned char   uc_status;
    struct netif   *pst_netif;

    ul_time_temp = hsl_get_time();
    ul_time      = ul_time_temp;
    uc_status    = hsl_demo_get_status();
    while((HSL_STATUS_RECEIVE == uc_status) && (HSL_PERIOD_TIMEOUT >= ul_wait_time))
    {
        if (RECEIVE_FLAG_ON == hsl_receive_flag)
        {
            ul_time_temp = hsl_get_time();
            if (HSL_PERIOD_CHANNEL < (ul_time_temp - ul_time))
            {
                ul_wait_time += (ul_time_temp - ul_time);
                ul_time = ul_time_temp;
                l_ret = hsl_get_lock_status();
                if (HSL_CHANNEL_UNLOCK == l_ret)
                {
                    l_ret = hisi_hsl_change_channel();
                    if (HISI_SUCC != l_ret)
                    {
                        HISI_PRINT_WARNING("%s[%d]:change channel fail",__func__,__LINE__);
                    }
                    hsl_os_reset();
                }
            }
        }
        l_ret = hisi_hsl_process_data();
        if (HSL_DATA_STATUS_FINISH == l_ret)
        {
            LOS_TaskLock();
            hsl_receive_flag = RECEIVE_FLAG_OFF;
            LOS_TaskUnlock();
            hsl_demo_set_status(HSL_STATUS_CONNECT);
            break;
        }
        msleep(1);
        uc_status = hsl_demo_get_status();
    }

    hisi_hsl_adapt_deinit();
    if (HSL_PERIOD_TIMEOUT >= ul_wait_time)
    {
        hsl_memset(&gst_hsl_params, 0, sizeof(hsl_result_stru));
        l_ret = hsl_get_result(&gst_hsl_params);
        if (HSL_SUCC == l_ret)
        {
            puc_macaddr = hisi_wlan_get_macaddr();
            if (HSL_NULL != puc_macaddr)
            {
                if ((puc_macaddr[4] == gst_hsl_params.auc_flag[0]) && (puc_macaddr[5] == gst_hsl_params.auc_flag[1]))
                {
                    l_ret = hsl_demo_connect_prepare();
                    if (0 != l_ret)
                    {
                        HISI_PRINT_ERROR("hsl_demo_task_channel_change:connect prepare fail[%d]\n",l_ret);
                    }
                }
                else
                {
                    HISI_PRINT_ERROR("This device is not intended to be associated:%02x %02x\n",gst_hsl_params.auc_flag[0],gst_hsl_params.auc_flag[1]);
                }
            }
        }
    }
    else
    {
        HISI_PRINT_INFO("hsl timeout\n");

        /* ɾ��APģʽ */
        pst_netif = netif_find("wlan0");
        if (HSL_NULL != pst_netif)
        {
            dhcps_stop(pst_netif);
        }
        l_ret = hostapd_stop();
        if (0 != l_ret)
        {
            HISI_PRINT_ERROR("%s[%d]:stop hostapd fail",__func__,__LINE__);
        }
    }
    hsl_demo_set_status(HSL_STATUS_UNCREATE);
    ul_ret = LOS_TaskDelete(gul_hsl_taskid);
    if (0 != ul_ret)
    {
        HISI_PRINT_WARNING("%s[%d]:delete task fail[%d]",__func__,__LINE__,ul_ret);
    }
}
extern struct completion  dhcp_complet;
/*****************************************************************************
 �� �� ��  : hsl_demo_connect_prepare
 ��������  : �л���wpa׼������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��29��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
hsl_int32 hsl_demo_connect_prepare(void)
{
    int                      l_ret;
    unsigned int             ul_ret;
    hsl_result_stru         *pst_result;
    struct netif            *pst_netif;

    /* ɾ��APģʽ */
    pst_netif = netif_find("wlan0");
    if (HSL_NULL != pst_netif)
    {
        dhcps_stop(pst_netif);
    }
    l_ret = hostapd_stop();
    if (0 != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:stop hostapd fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
    init_completion(&dhcp_complet);
#ifdef CONFIG_NO_CONFIG_WRITE
    l_ret = wpa_supplicant_start("wlan0", "hisi", HSL_NULL);
#else
    l_ret = wpa_supplicant_start("wlan0", "hisi", WPA_SUPPLICANT_CONFIG_PATH);
#endif
    if (0 != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:start wpa_supplicant fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
    ul_ret = wait_for_completion_timeout(&dhcp_complet, LOS_MS2Tick(40000));//40s��ʱ
    if (0 == ul_ret)
    {
        HISI_PRINT_ERROR("hsl_demo_connect_prepare:cannot get ip\n");
        return -HSL_FAIL;
    }
    pst_result = hsl_demo_get_result();
    if (HSL_NULL != pst_result)
    {
        hsl_demo_online(pst_result);
    }
    return HSL_SUCC;
}
/*****************************************************************************
 �� �� ��  : hsl_demo_connect
 ��������  : ���û�ȡ���Ĳ�����������
 �������  : pst_params�����ȡ���Ĳ���
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��29��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
int hsl_demo_connect(hsl_result_stru* pst_params)
{
    struct wpa_assoc_request wpa_assoc_req;
    if (HSL_NULL == pst_params)
    {
        HISI_PRINT_ERROR("%s[%d]:pst_params is null",__func__,__LINE__);
        return -HSL_ERR_NULL;
    }

    hsl_memset(&wpa_assoc_req , 0 ,sizeof(struct wpa_assoc_request));
    wpa_assoc_req.hidden_ssid     = 1;
    hsl_memcpy(wpa_assoc_req.ssid, pst_params->auc_ssid, pst_params->uc_ssid_len);
    wpa_assoc_req.auth            = pst_params->en_auth_mode;

    if (HSL_AUTH_TYPE_OPEN != wpa_assoc_req.auth)
    {
        hsl_memcpy(wpa_assoc_req.key, pst_params->auc_pwd, pst_params->uc_pwd_len);
    }
    /* ��ʼ�������� */
    wpa_connect_interface(&wpa_assoc_req);
    return HSL_SUCC;
}
/*****************************************************************************
 �� �� ��  : hsl_demo_get_status
 ��������  : ���û�ȡ��ǰhisilink��״̬
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��29��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
unsigned char hsl_demo_get_status(void)
{
    unsigned char uc_type;
    LOS_TaskLock();
    uc_type = guc_hsl_status;
    LOS_TaskUnlock();
    return uc_type;
}
/*****************************************************************************
 �� �� ��  : hsl_demo_set_status
 ��������  : ����hisilink��״̬
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��29��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
int hsl_demo_set_status(hsl_status_enum_type uc_type)
{
    LOS_TaskLock();
    guc_hsl_status = uc_type;
    LOS_TaskUnlock();
    return 0;
}
/*****************************************************************************
 �� �� ��  : hsl_demo_get_result
 ��������  : ��ȡ���ܽ��ָ��
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��29��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
hsl_result_stru* hsl_demo_get_result(void)
{
    hsl_result_stru *pst_result;
    pst_result = &gst_hsl_params;
    return pst_result;
}

/*****************************************************************************
 �� �� ��  : hsl_demo_online
 ��������  : �ɹ���ȡIP��������֪ͨ
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��29��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
int hsl_demo_online(hsl_result_stru* pst_params)
{
    if (HSL_NULL == pst_params)
    {
        HISI_PRINT_ERROR("%s[%d]:pst_params is null",__func__,__LINE__);
        return -HSL_ERR_NULL;
    }
    hisi_hsl_online_notice(pst_params);
    return HSL_SUCC;
}
extern int hilink_demo_get_status(void);
/*****************************************************************************
 �� �� ��  : hsl_demo_main
 ��������  : hsl demo���������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��29��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
hsl_int32 hsl_demo_main(void)
{
    unsigned int        ul_ret;
    int                 l_ret;
    unsigned char       uc_status;
    TSK_INIT_PARAM_S    st_hsl_task;

    uc_status = hilink_demo_get_status();
    if (0 != uc_status)
    {
        HISI_PRINT_ERROR("hilink already start, cannot start hisilink");
        return -HSL_FAIL;
    }
    uc_status = hsl_demo_get_status();
    if (HSL_STATUS_UNCREATE != uc_status)
    {
        HISI_PRINT_ERROR("hsl already start,cannot start again");
        return -HSL_FAIL;
    }
    hsl_demo_set_status(HSL_STATUS_RECEIVE);
    l_ret = hsl_demo_prepare();
    if (0 != l_ret)
    {
        hsl_demo_set_status(HSL_STATUS_UNCREATE);
        HISI_PRINT_ERROR("%s[%d]:demo init fail",__func__,__LINE__);
        return -HSL_FAIL;
    }

   /* �������ŵ��߳�,�߳̽������Զ��ͷ� */
    memset(&st_hsl_task, 0, sizeof(TSK_INIT_PARAM_S));
    st_hsl_task.pfnTaskEntry = (TSK_ENTRY_FUNC)hsl_demo_task_channel_change;

    st_hsl_task.uwStackSize  = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    st_hsl_task.pcName       = "hsl_thread";
    st_hsl_task.usTaskPrio   = 8;
    st_hsl_task.uwResved     = LOS_TASK_STATUS_DETACHED;
    ul_ret = LOS_TaskCreate(&gul_hsl_taskid, &st_hsl_task);
    if(0 != ul_ret)
    {
       hsl_demo_set_status(HSL_STATUS_UNCREATE);
       HISI_PRINT_ERROR("%s[%d]:create task fail[%d]",__func__,__LINE__,ul_ret);
       return -HSL_FAIL;
    }
    return HSL_SUCC;
}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
