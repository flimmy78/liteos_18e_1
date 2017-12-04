/*
 *  osdrv sample
 */
#include "sys/types.h"
#include "sys/time.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/statfs.h"
#include "limits.h"

#include "los_event.h"
#include "los_printf.h"

#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "eth_drv.h"
#include "arch/perf.h"

#include "fcntl.h"
#include "fs/fs.h"

#include "stdio.h"

#include "shell.h"
#include "hisoc/uart.h"
#include "vfs_config.h"
#include "disk.h"

#include "los_cppsupport.h"

#include "linux/fb.h"

#include "adaptsecure.h"

/*Hi1131 modify add */
#include "los_event.h"
#include "wpa_supplicant/wpa_supplicant.h"
#include "hostapd/hostapd_if.h"
#include "hisi_wifi.h"
#include <mmc/host.h>
#include "hisilink_lib.h"
#include "hilink_link.h"
#include "driver_hisi_lib_api.h"
#include <linux/completion.h>

#define WPA_SUPPLICANT_CONFIG_PATH  "/jffs0/etc/hisi_wifi/wifi/wpa_supplicant.conf"

typedef enum
{
    HSL_STATUS_UNCREATE,
    HSL_STATUS_CREATE,
    HSL_STATUS_RECEIVE,
    HSL_STATUS_CONNECT,
    HSL_STATUS_BUTT
}hsl_status_enum;

typedef enum
{
    HILINK_STATUS_UNCREATE,
    HILINK_STATUS_RECEIVE, //hilink���ڽ����鲥�׶�
    HILINK_STATUS_CONNECT, //hilink���ڹ����׶�
    HILINK_STATUS_BUTT
}hilink_status_enum;

extern void mcu_uart_proc();
extern void hisi_wifi_shell_cmd_register(void);

/* ������dhcp�����1���ѯIP�Ƿ��ȡ��30��δ��ȡIPִ��ȥ�������� */
#define             DHCP_CHECK_CNT      30
#define             DHCP_CHECK_TIME     1000
#define             DHCP_IP_SET         0
#define             DHCP_IP_DEL         1

#define WLAN_FILE_STORE_MIN_SIZE             (0)
#define WLAN_FILE_STORE_MID_SIZE             (0x30000)
#define WLAN_FILE_STORE_MAX_SIZE             (0x70000)
#define WLAN_FILE_STORE_BASEADDR             (0x750000)


struct timer_list   hisi_dhcp_timer;
unsigned int        check_ip_loop = 0;

struct netif       *pwifi = NULL;

extern unsigned char hsl_demo_get_status(void);
extern hsl_result_stru* hsl_demo_get_result(void);
extern unsigned char hilink_demo_get_status(void);
extern hsl_result_stru* hilink_demo_get_result(void);
extern void hisi_reset_addr(void);
extern int hilink_demo_online(hilink_s_result* pst_result);
extern int hsl_demo_online(hsl_result_stru* pst_params);
extern void start_dhcps(void);



extern hisi_rf_customize_stru g_st_rf_customize;

/*Hi1131 modify end */

int secure_func_register(void)
{
    int ret;
    STlwIPSecFuncSsp stlwIPSspCbk= {0};
    stlwIPSspCbk.pfMemset_s = Stub_MemSet;
    stlwIPSspCbk.pfMemcpy_s = Stub_MemCpy;
    stlwIPSspCbk.pfStrNCpy_s = Stub_StrnCpy;
    stlwIPSspCbk.pfStrNCat_s = Stub_StrnCat;
    stlwIPSspCbk.pfStrCat_s = Stub_StrCat;
    stlwIPSspCbk.pfMemMove_s = Stub_MemMove;
    stlwIPSspCbk.pfSnprintf_s = Stub_Snprintf;
    stlwIPSspCbk.pfRand = rand;
    ret = lwIPRegSecSspCbk(&stlwIPSspCbk);
    if (ret != 0)
    {
        PRINT_ERR("\n***lwIPRegSecSspCbk Failed***\n");
        return -1;
    }

    PRINTK("\nCalling lwIPRegSecSspCbk\n");
    return ret;
}

extern UINT32 osShellInit(char *);

struct netif *pnetif;

void net_init()
{
    (void)secure_func_register();
    tcpip_init(NULL, NULL);
#ifdef LOSCFG_DRIVERS_HIGMAC
    extern struct los_eth_driver higmac_drv_sc;
    pnetif = &(higmac_drv_sc.ac_if);
    higmac_init();
#endif

#ifdef LOSCFG_DRIVERS_HIETH_SF
    extern struct los_eth_driver hisi_eth_drv_sc;
    pnetif = &(hisi_eth_drv_sc.ac_if);
    hisi_eth_init();
#endif

    dprintf("cmd_startnetwork : DHCP_BOUND finished\n");

    netif_set_up(pnetif);
}

extern unsigned int g_uwFatSectorsPerBlock;
extern unsigned int g_uwFatBlockNums;
#define SUPPORT_FMASS_PARITION
#ifdef SUPPORT_FMASS_PARITION
extern int fmass_register_notify(void(*notify)(void* context, int status), void* context);
extern int fmass_partition_startup(char* path);
void fmass_app_notify(void* conext, int status)
{
    if(status == 1)/*usb device connect*/
    {
        char *path = "/dev/mmcblk0p0";
        //startup fmass access patition
        fmass_partition_startup(path);
    }
}
#endif

#include "board.h"
extern UINT32 g_sys_mem_addr_end;
extern unsigned int g_uart_fputc_en;
void board_config(void)
{
    g_sys_mem_addr_end = SYS_MEM_BASE + SYS_MEM_SIZE_DEFAULT;
    g_uwSysClock = OS_SYS_CLOCK;
    g_uart_fputc_en = 1;
#ifndef TWO_OS
    extern unsigned long g_usb_mem_addr_start;
    extern unsigned long g_usb_mem_size;
    g_usb_mem_addr_start = g_sys_mem_addr_end;
    g_usb_mem_size = 0x20000; //recommend 128K nonCache for usb

    g_uwFatSectorsPerBlock = CONFIG_FS_FAT_SECTOR_PER_BLOCK;
    g_uwFatBlockNums       = CONFIG_FS_FAT_BLOCK_NUMS;
    //different board should set right mode:"rgmii" "rmii" "mii"
    //if you don't set :
	//hi3516a board's default mode is "rgmii"
	//hi3518ev200 board's default mode is "rmii"
	//hi3519 board's default mode is "rgmii"
#if defined(HI3516A) || defined(HI3519) || defined(HI3519V101) || defined(HI3559)
    hisi_eth_set_phy_mode("rgmii");
#endif
#if defined(HI35168EV200)
    hisi_eth_set_phy_mode("rmii");
#endif
    //different board should set right addr:0~31
    //if you don't set ,driver will detect it automatically
    //hisi_eth_set_phy_addr(0);//0~31

#if (defined(HI3518EV200) &&defined(LOSCFG_DRIVERS_EMMC)) || defined(HI3519) || defined(HI3519V101) || defined(HI3559)
    size_t part0_start_sector = 16   * (0x100000/512);
    size_t part0_count_sector = 1024 * (0x100000/512);
    size_t part1_start_sector = 16   * (0x100000/512) + part0_count_sector;
    size_t part1_count_sector = 1024 * (0x100000/512);
    extern struct disk_divide_info emmc;
    add_mmc_partition(&emmc, part0_start_sector, part0_count_sector);
    add_mmc_partition(&emmc, part1_start_sector, part1_count_sector);
#endif
#endif
}

#if 0
void app_init(void)
{
    UINT32 uwRet;

    dprintf("uart init ...\n");
    hi_uartdev_init();

    dprintf("shell init ...\n");
    system_console_init(TTY_DEVICE);
    osShellInit(TTY_DEVICE);

    dprintf("spi bus init ...\n");
    hi_spi_init();

    dprintf("dmac init\n");
    hi_dmac_init();

    dprintf("i2c bus init ...\n");
    i2c_dev_init();

    dprintf("random dev init ...\n");
    ran_dev_register();

    dprintf("mem dev init ...\n");
	mem_dev_register();

    dprintf("fb dev init ...\n");
    struct fb_info *info = malloc(sizeof(struct fb_info));
	register_framebuffer(info);

	dprintf("porc fs init ...\n");
    proc_fs_init();

    dprintf("cxx init ...\n");
    extern char __init_array_start__, __init_array_end__;
    LOS_CppSystemInit((unsigned long)&__init_array_start__, (unsigned long)&__init_array_end__, NO_SCATTER);

#ifndef TWO_OS
    dprintf("nand init ...\n");
    if(!nand_init()) {
        add_mtd_partition("nand", 0x200000, 32*0x100000, 0);
        add_mtd_partition("nand", 0x200000 + 32*0x100000, 32*0x100000, 1);
        mount("/dev/nandblk0", "/yaffs0", "yaffs", 0, NULL);
        //mount("/dev/nandblk1", "/yaffs1", "yaffs", 0, NULL);
    }

    dprintf("spi nor flash init ...\n");
    if(!spinor_init()){
        add_mtd_partition("spinor", 0x100000, 2*0x100000, 0);
        add_mtd_partition("spinor", 3*0x100000, 2*0x100000, 1);
#ifndef HI3559
        mount("/dev/spinorblk0", "/jffs0", "jffs", 0, NULL);
#endif
        //mount("/dev/spinorblk1", "/jffs1", "jffs", 0, NULL);
    }

    dprintf("gpio init ...\n");
    hi_gpio_init();

    dprintf("net init ...\n");
    net_init();

    dprintf("sd/mmc host init ...\n");
    SD_MMC_Host_init();
    msleep(2000);
#ifndef SUPPORT_FMASS_PARITION
    uwRet = mount("/dev/mmcblk0p0", "/sd0p0", "vfat", 0, 0);
    if (uwRet)
        dprintf("mount mmcblk0p0 to sd0p0 err %d\n", uwRet);
    uwRet = mount("/dev/mmcblk0p1", "/sd0p1", "vfat", 0, 0);
    if (uwRet)
        dprintf("mount mmcblk0p1 to sd0p1 err %d\n", uwRet);
    uwRet = mount("/dev/mmcblk1p0", "/sd1p0", "vfat", 0, 0);
    if (uwRet)
        dprintf("mount mmcblk1p0 to sd1p0 err %d\n", uwRet);
    uwRet = mount("/dev/mmcblk2p0", "/sd2p0", "vfat", 0, 0);
    if (uwRet)
        dprintf("mount mmcblk2p0 to sd2p0 err %d\n", uwRet);
#endif
    dprintf("usb init ...\n");
    //g_usb_mem_addr_start and g_usb_mem_size must be set in board_config
    //usb_init must behind SD_MMC_Host_init
    uwRet = usb_init();
#ifdef SUPPORT_FMASS_PARITION
    if(!uwRet)
        fmass_register_notify(fmass_app_notify,NULL);
#endif
    msleep(2000);
    uwRet = mount("/dev/sdap0", "/usbp0", "vfat", 0, 0);
    if (uwRet)
        dprintf("mount sdap0 to usbp0 err %d\n", uwRet);
    uwRet = mount("/dev/sdap1", "/usbp1", "vfat", 0, 0);
    if (uwRet)
        dprintf("mount sdap1 to usbp1 err %d\n", uwRet);

#ifdef HI3516A
    dprintf("dvfs init ...\n");
    dvfs_init();
#endif
#endif

    dprintf("g_sys_mem_addr_end=0x%08x,\n",g_sys_mem_addr_end);
    dprintf("done init!\n");
    dprintf("Date:%s.\n", __DATE__);
    dprintf("Time:%s.\n", __TIME__);

#ifdef TWO_OS
    extern int _ipcm_vdd_init(void);
    extern int ipcm_net_init(void);

    dprintf("tcpip init ...\n");
    (void)secure_func_register();
    tcpip_init(NULL, NULL);

    dprintf("_ipcm vdd init ...\n");
    _ipcm_vdd_init();

    dprintf("ipcm net init ...\n");
    ipcm_net_init();

    extern int HI_ShareFs_Client_Init(char *);
    dprintf("share fs init ... \n");
    HI_ShareFs_Client_Init("/liteos");
    dprintf("share fs init done.\n");
#endif

#ifdef __OSDRV_TEST__
#ifndef TWO_OS
    msleep(5000);
#endif
    test_app_init();
#endif

    return;
}

#else

void hi_rf_customize_init(void)
{
    /*11b scaling ���ʣ���4�ֽڣ����ֽڵ����ֽڶ�Ӧ1m��2m��5.5m��11m��ֵԽ��11bģʽ���书��Խ��*/
    g_st_rf_customize.l_11b_scaling_value              = 0x9c9c9c9c;
    /*11g scaling ���ʣ���4�ֽڣ����ֽڵ����ֽڶ�Ӧ6m��9m��12m��18m��ֵԽ��11gģʽ���书��Խ��*/
    g_st_rf_customize.l_11g_u1_scaling_value           = 0x6c6c6c6c;
    /*11g scaling ���ʣ���4�ֽڣ����ֽڵ����ֽڶ�Ӧ24m��36m��48m��54m��ֵԽ��11gģʽ���书��Խ��*/
    g_st_rf_customize.l_11g_u2_scaling_value           = 0x666c6c6c;
    /*11n 20m scaling ���ʣ���4�ֽڣ����ֽڵ����ֽڶ�Ӧmcs4��mcs5��mcs6��mcs7��ֵԽ��11nģʽ���书��Խ��*/
    g_st_rf_customize.l_11n_20_u1_scaling_value        = 0x57575757;
    /*11n 20m scaling ���ʣ���4�ֽڣ����ֽڵ����ֽڶ�Ӧmcs0��mcs1��mcs2��mcs3��ֵԽ��11nģʽ���书��Խ��*/
    g_st_rf_customize.l_11n_20_u2_scaling_value        = 0x5c5c5757;
    /*11n 40m scaling ���ʣ���4�ֽڣ����ֽڵ����ֽڶ�Ӧmcs4��mcs5��mcs6��mcs7��ֵԽ��11nģʽ���书��Խ��*/
    g_st_rf_customize.l_11n_40_u1_scaling_value        = 0x5a5a5a5a;
    /*11n 40m scaling ���ʣ���4�ֽڣ����ֽڵ����ֽڶ�Ӧmcs0��mcs1��mcs2��mcs3��ֵԽ��11nģʽ���书��Խ��*/
    g_st_rf_customize.l_11n_40_u2_scaling_value        = 0x5d5d5a5a;

    /*1-4�ŵ�upc���ʵ�����ֵԽ�󣬸��ŵ�����Խ��*/
    g_st_rf_customize.l_ban1_ref_value                  = 55;
    /*5-9�ŵ�upc���ʵ�����ֵԽ�󣬸��ŵ�����Խ��*/
    g_st_rf_customize.l_ban2_ref_value                  = 55;
    /*10-13�ŵ�upc���ʵ�����ֵԽ�󣬸��ŵ�����Խ��*/
    g_st_rf_customize.l_ban3_ref_value                  = 56;

    /*40M����bypass����1����40M����д0ʹ��40M����*/
    g_st_rf_customize.l_disable_bw_40                   = 1;
    /*�͹��Ŀ��أ�д1�򿪵͹��ģ�д0�رյ͹���*/
    g_st_rf_customize.l_pm_switch                       = 1;
    /*dtim����, ��Χ1-10*/
    g_st_rf_customize.l_dtim_setting                    = 1;


    /*���趨�ƻ�������1����������0*/
    g_st_rf_customize.l_customize_enable                = 1;

}
void hi_wifi_register_init(void)
{
    unsigned int value = 0;

    /*���ùܽŸ���*/
    if (0 == WIFI_SDIO_INDEX)
    {
        value = 0x0001;
        writel(value, REG_MUXCTRL_SDIO0_CLK_MAP);
        //writel(value, REG_MUXCTRL_SDIO0_DETECT_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CWPR_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CDATA1_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CDATA0_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CDATA3_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CCMD_MAP);
        writel(value, REG_MUXCTRL_SDIO0_POWER_EN_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CDATA2_MAP);
    }
    else if (1 == WIFI_SDIO_INDEX)
    {
        value = 0X0004;
        writel(value, REG_MUXCTRL_SDIO1_CLK_MAP);
        //writel(value, REG_MUXCTRL_SDIO1_DETECT_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CWPR_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CDATA1_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CDATA0_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CDATA3_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CCMD_MAP);
        writel(value, REG_MUXCTRL_SDIO1_POWER_EN_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CDATA2_MAP);
    }

    value = 0x0;
    writel(value, REG_MUXCTRL_WIFI_DATA_INTR_GPIO_MAP);
    writel(value, REG_MUXCTRL_HOST_WAK_DEV_GPIO_MAP);
    writel(value, REG_MUXCTRL_WIFI_WAK_FLAG_GPIO_MAP);
    writel(value, REG_MUXCTRL_DEV_WAK_HOST_GPIO_MAP);

}

void hi_wifi_power_set(unsigned char val)
{
    HI_HAL_MCUHOST_WiFi_Power_Set(val);
}
void hi_wifi_rst_set(unsigned char val)
{
    HI_HAL_MCUHOST_WiFi_Rst_Set(val);
}

void himci_wifi_sdio_detect_trigger(void)
{
    unsigned int reg_value = 0;

    struct mmc_host *mmc = NULL;
    mmc = get_mmc_host(WIFI_SDIO_INDEX);
    struct himci_host *host = (struct himci_host *)mmc->priv;

    if (0 == WIFI_SDIO_INDEX)
    {
        writel(0x0001, REG_MUXCTRL_SDIO0_DETECT_MAP);
    }
    else if (1 == WIFI_SDIO_INDEX)
    {
        reg_value = readl(REG_MUXCTRL_SDIO1_DETECT_MAP);
        if (0X0004 == reg_value)
        {
            writel(0x0, REG_MUXCTRL_SDIO1_DETECT_MAP);
            msleep(500);//wait sdio card remove
        }
        writel(0X0004, REG_MUXCTRL_SDIO1_DETECT_MAP);
    }


    hi_mci_pre_detect_card(host);
    hi_mci_pad_ctrl_cfg(host, SIGNAL_VOLT_1V8);


}

void hi_wifi_board_info_register(void)
{
    BOARD_INFO *wlan_board_info = get_board_info();
    if (NULL == wlan_board_info)
    {
        dprintf("wifi_board_info is NULL!\n");
        return;
    }

    wlan_board_info->wlan_irq = WIFI_IRQ;

    wlan_board_info->wifi_data_intr_gpio_group = WIFI_DATA_INTR_GPIO_GROUP;
    wlan_board_info->wifi_data_intr_gpio_offset = WIFI_DATA_INTR_GPIO_OFFSET;

    wlan_board_info->dev_wak_host_gpio_group = DEV_WAK_HOST_GPIO_GROUP;
    wlan_board_info->dev_wak_host_gpio_offset = DEV_WAK_HOST_GPIO_OFFSET;

    wlan_board_info->host_wak_dev_gpio_group = HOST_WAK_DEV_GPIO_GROUP;
    wlan_board_info->host_wak_dev_gpio_offset = HOST_WAK_DEV_GPIO_OFFSET;

    wlan_board_info->wifi_power_set = hi_wifi_power_set;
    wlan_board_info->wifi_rst_set = hi_wifi_rst_set;
    wlan_board_info->wifi_sdio_detect = himci_wifi_sdio_detect_trigger;
}

void hi_wifi_check_wakeup_flag(void)
{
    int wak_flag = 0;
    gpio_dir_config(WIFI_WAK_FLAG_GPIO_GROUP, WIFI_WAK_FLAG_GPIO_OFFSET, 0);
    wak_flag = gpio_read(WIFI_WAK_FLAG_GPIO_GROUP, WIFI_WAK_FLAG_GPIO_OFFSET);
    HI_HAL_MCUHOST_WiFi_Clr_Flag();
    wlan_resume_state_set(wak_flag);
}

struct databk_addr_info *hi_wifi_databk_info_cb(void)
{
    /* ��ʱд������Ӧ�òಹ�� */
    struct databk_addr_info *wlan_databk_addr_info = NULL;
    wlan_databk_addr_info = malloc(sizeof(struct databk_addr_info));
    if (NULL == wlan_databk_addr_info)
    {
        dprintf("hi_wifi_databk_info_cb:wlan_databk_addr_info is NULL!\n");
        return NULL;
    }
    memset(wlan_databk_addr_info, 0, sizeof(struct databk_addr_info));
    wlan_databk_addr_info->databk_addr = 0x7d0000;
    wlan_databk_addr_info->databk_length = 0x20000;
    wlan_databk_addr_info->get_databk_info = NULL;
    return wlan_databk_addr_info;
}

void hi_wifi_register_backup_addr(void)
{
    struct databk_addr_info *wlan_databk_addr_info = hisi_wlan_get_databk_addr_info();
    if (NULL == wlan_databk_addr_info)
    {
        dprintf("hi_wifi_register_backup_addr:wlan_databk_addr_info is NULL!\n");
        return NULL;
    }
    /* ��ʱд������Ӧ�òಹ�� */
    wlan_databk_addr_info->databk_addr = 0x7d0000;
    wlan_databk_addr_info->databk_length = 0x10000;
    wlan_databk_addr_info->get_databk_info = hi_wifi_databk_info_cb;
}

void hi_wifi_no_fs_init(void)
{
    hisi_wlan_no_fs_config(WLAN_FILE_STORE_BASEADDR, WLAN_FILE_STORE_MIN_SIZE);
}

void hi_wifi_pre_proc()
{
    hi_wifi_register_init();
    hi_wifi_board_info_register();
    hi_wifi_check_wakeup_flag();
    hi_wifi_register_backup_addr();
    hi_wifi_no_fs_init();
    hi_rf_customize_init();
}
struct completion  dhcp_complet;

int hi_check_dhcp_success(void)
{
    int             ret    = 0;
    unsigned int     ipaddr = 0;

    ret = dhcp_is_bound(pwifi);
    if (0 == ret)
    {
        /* IP��ȡ�ɹ���֪ͨwifi���� */
        printf("\n\n DHCP SUCC\n\n");
        hisi_wlan_ip_notify(ipaddr, DHCP_IP_SET);
        if ((HSL_STATUS_UNCREATE != hsl_demo_get_status())
            || (HILINK_STATUS_UNCREATE != hilink_demo_get_status()))
        {
            complete(&dhcp_complet);
        }
        del_timer(&hisi_dhcp_timer);
        return 0;
    }

    if (check_ip_loop++ > DHCP_CHECK_CNT)
    {
        /* IP��ȡʧ��ִ��ȥ���� */
        printf("\n\n DHCP FAILED\n\n");
        wpa_cli_disconnect();
        del_timer(&hisi_dhcp_timer);
        return 0;
    }

    /* ������ѯ��ʱ�� */
    add_timer(&hisi_dhcp_timer);
    return 0;
}

extern int hsl_demo_connect(hsl_result_stru* pst_params);
extern int hilink_demo_connect(hilink_s_result* pst_result);
void hisi_wifi_event_cb(enum wpa_event event)
{
    unsigned char    uc_status;
    unsigned int     ipaddr = 0;
    hsl_result_stru *pst_hsl_result;
    hilink_s_result *pst_hilink_result;
    printf("wifi_event_cb,event:%d\n",event);

    if(pwifi == 0)
        return ;

    switch(event) {
        case WPA_EVT_SCAN_RESULTS:
            printf("Scan results available\n");
        break;
        case WPA_EVT_CONNECTED:
            printf("WiFi: Connected\n");
            /* ����dhcp��ȡIP */
            netifapi_dhcp_stop(pwifi);
            netifapi_dhcp_start(pwifi);

            /* ��ѯIP�Ƿ��ȡ */
            check_ip_loop = 0;
            init_timer(&hisi_dhcp_timer);
            hisi_dhcp_timer.expires = LOS_MS2Tick(DHCP_CHECK_TIME);
            hisi_dhcp_timer.function = hi_check_dhcp_success;
            add_timer(&hisi_dhcp_timer);
            msleep(500);
            break;
        case WPA_EVT_DISCONNECTED:
            printf("WiFi: disconnect\n");
            netifapi_dhcp_stop(pwifi);
            hisi_reset_addr();
            hisi_wlan_ip_notify(ipaddr, DHCP_IP_DEL);
            break;
        case WPA_EVT_ADDIFACE:
            //wpa_supplicant�����ɹ�
            /* �ж��Ƿ�Ϊhisi_link������WPA�ɹ� */
            uc_status = hsl_demo_get_status();
            printf("hsl_status=%d\n",uc_status);
            if (HSL_STATUS_CONNECT == uc_status)
            {
                pst_hsl_result = hsl_demo_get_result();
                if (NULL != pst_hsl_result)
                {
                    hsl_demo_connect(pst_hsl_result);
                    break;
                }
            }
            /* �ж��Ƿ�Ϊhilink������WPA�ɹ� */
            uc_status = hilink_demo_get_status();
            printf("hilink_status=%d\n",uc_status);
            if (HILINK_STATUS_CONNECT == uc_status)
            {
               pst_hilink_result = hilink_demo_get_result();
               if (NULL != pst_hilink_result)
               {
                     hilink_demo_connect(pst_hilink_result);
               }
            }
            break;
        default:
            break;
    }
}

void hisi_reset_addr(void)
{
    ip_addr_t        st_gw;
    ip_addr_t        st_ipaddr;
    ip_addr_t        st_netmask;
    struct netif    *pst_lwip_netif;

    IP4_ADDR(&st_gw, 0, 0, 0, 0);
    IP4_ADDR(&st_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&st_netmask, 0, 0, 0, 0);

    pst_lwip_netif = netif_find("wlan0");
    if (HISI_NULL == pst_lwip_netif)
    {
        HISI_PRINT_ERROR("cmd_start_hapd::Null param of netdev");
        return;
    }

    /* wpa_stop����������netif�����غ�mac��ַ */
    netif_set_addr(pst_lwip_netif, &st_ipaddr, &st_netmask, &st_gw);
}

extern int hsl_demo_init(void);
extern int hilink_demo_init(void);
void hisi_wifi_hostapd_event_cb(enum hostapd_event event)
{
    unsigned char uc_status;

    printf("hostapd_event=%d\n",event);
    switch(event)
    {
        case HOSTAPD_EVT_ENABLED:
            //hostapd�����ɹ�
            start_dhcps();
            /* �ж��Ƿ���hisilink */
            uc_status = hsl_demo_get_status();
            printf("hsl_status=%d\n",uc_status);
            if (HSL_STATUS_RECEIVE == uc_status)
            {
                hsl_demo_init();
            }
            /* �ж��Ƿ���hilink */
            uc_status = hilink_demo_get_status();
            printf("hilink_status=%d\n",uc_status);
            if (HILINK_STATUS_RECEIVE == uc_status)
            {
                hilink_demo_init();
            }
            break;
        case HOSTAPD_EVT_DISABLED:
            //hostapdɾ���ɹ�
            hisi_reset_addr();
            break;
        case HOSTAPD_EVT_CONNECTED:
            //�û������ɹ�
            break;
        case HOSTAPD_EVT_DISCONNECTED:
            //�û�ȥ����
            break;
        default:
            break;
    }
}

void app_init(void){
    UINT32 uwRet;

    dprintf("random init ...\n");
    ran_dev_register();

    dprintf("uart init ...\n");
    uart_dev_init();


    dprintf("shell init ...\n");
    system_console_init(TTY_DEVICE);
    osShellInit(TTY_DEVICE);

    dprintf("hisi_wifi_shell_cmd_register init ...\n");
    hisi_wifi_shell_cmd_register();

    dprintf("spi nor falsh init ...\n");
    if (!spinor_init())
    {
#if 0
        add_mtd_partition("spinor", 0x100000, 1*0x100000, 0);
        mount("/dev/spinorblk0", "/jffs0", "jffs", 0, NULL);
#endif
    }

    dprintf("gpio init ...\n");
    gpio_dev_init();

    dprintf("net init ...\n");
    net_init();

#ifdef _PRE_FEATURE_USB
    dprintf("usb init ...\n");
    usb_init();
#endif

    dprintf("sd/mmc host init ...\n");
    SD_MMC_Host_init();

    mcu_uart_proc();
    msleep(500);

    /*check system bootup or wakeup*/
    hi_wifi_pre_proc();

    dprintf("porc fs init ...\n");
    proc_fs_init();

    dprintf("hi1131 wifi start \n");

#ifdef Hi1131_SDK
    char *mac_addr[] = {"3E","22","15","05","31","35"};
    cmd_set_macaddr(6,mac_addr);
#endif
    wpa_register_event_cb(hisi_wifi_event_cb);
    hostapd_register_event_cb(hisi_wifi_hostapd_event_cb);
    uwRet = hisi_wlan_wifi_init(&pwifi);
    if(0 != uwRet)
    {
        dprintf("fail to start hi1131 wifi\n");
        if(pwifi != NULL)
            hisi_wlan_wifi_deinit();
    }
#ifdef Hi1131_SDK

    if(0 == hisi_get_resume_wifi_mode())
    {
        uwRet = wpa_supplicant_start("wlan0", "hisi", WPA_SUPPLICANT_CONFIG_PATH);
        if(0 != uwRet)
        {
            dprintf("fail to start wpa_supplicant\n");
        }

        wpa_register_event_cb(wifi_event_cb);


        int argc = 4;
        char *argv[] = {"0","sdk_test","wpa","12345678"};

        uwRet = cmd_wpa_connect(argc, argv);
        if(0 != uwRet)
        {
            dprintf("fail to send wpa connect command");
        }

    }
#endif

    return;

}


#endif

