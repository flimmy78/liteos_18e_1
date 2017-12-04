
#include "boardconfig.h"
#include "kfifo.h"

u8 g_WakeUp_Halt_Flag = 0; //�������ѱ�־

u8 g_Wifi_WakeUp = 0;  //WIFI���ѱ�־
u8 g_KeyPoweroff_Flag = 0; //�����ػ���־
u8 g_Vbus_Check = 0; //VBUS���

u8 g_PIR_Check = 0; //PIR���
u8 g_IsSystemOn = 0; //dv����оƬ��״̬��0�ػ��� 1����
u8 g_Key_Handle_Flag = 0;

u8 g_Wifi_Reg_Det_Flag = 1;

u8 g_Bar_Det_Flag = 0; //wifi����ʱ�ĵ�ؼ��

u8 g_poweroff_check_flg = 0;//����Ƿ�����ϵ������ް�������pir����
u8 g_pir_ok_poweroff = 0; // PIR����ػ�����
u8 g_key_ok_poweroff = 0; // ��������ػ�����
u8 g_host_readly_uart = 0;//��־����оƬuart׼��OK
u16 g_WakeUp_Key_Flag = 0;
u16 USART_Receive_Timeout = 0;
u8 USART_Send_Buf[UF_MAX_LEN];
u8 USART_Receive_Buf[UF_MAX_LEN];
u32 USART_Receive_Flag = 0;
u32 g_time_count = 0;


u8 g_IsDevPwrOn = 0; //device power ��״̬��0 �ػ� ��1 ����
u8 g_IsDevWlanEn = 0; //device wlan ��״̬��0 disable ��1 enable
u8 g_Iswkupmsgsend = 0;
u8 g_wkup_reason = 0;

u8 g_debounce_cnt = 0;
#define GPIO_DEBOUNCE 2
struct kfifo recvfifo;

/*���ò����ʣ�Ĭ��Ϊ9600*/
#define BAUD_RATE 9600

#if (BAUD_RATE == 115200)
#define BAUD_CLK  CLK_SYSCLKDiv_1
#elif (BAUD_RATE == 9600)
#define BAUD_CLK  CLK_SYSCLKDiv_32
#endif


/**********************************************************************
 * �������ƣ�  MDelay
 * ������������ʱ��������λΪms
 * para:
delay_count:  ��ʱʱ��
 ***********************************************************************/
static void MDelay(u32 delay_count)
{
	u32 i, j;

	for (i = 0; i < delay_count; i++)
	{
		for (j = 0; j < 50; j++);
	}
}

/**********************************************************************
 * �������ƣ�  PWR_ON
 * �����������������ϵ�
 * para:
 void
 ***********************************************************************/
void PWR_ON(void)
{
	GPIO_SetBits(PWR_HOLD_GPIO, PWR_HOLD_GPIO_Pin);
	g_IsSystemOn = 1;
}

/**********************************************************************
 * �������ƣ�  PWR_OFF
 * �����������ж����ص�Դ
 * para:
 void
 ***********************************************************************/
void PWR_OFF(void)
{
	GPIO_ResetBits(PWR_HOLD_GPIO, PWR_HOLD_GPIO_Pin);
	GPIO_ResetBits(MCU_GPIO2_GPIO, MCU_GPIO2_GPIO_Pin);
	g_IsSystemOn = 0;
	g_key_ok_poweroff = 0;
	g_pir_ok_poweroff = 0;
	g_host_readly_uart = 0;
}

/**********************************************************************
 * �������ƣ�  XOR_Inverted_Check
 * �������������ݵ����ȡ��У��
 * para:
inBuf: ������ַ���
inLen: ��������ַ�������
 ***********************************************************************/
u8  XOR_Inverted_Check(unsigned char* inBuf, unsigned char inLen)
{
	u8 check = 0, i;

	for (i = 0; i < inLen; i++) {
		check ^= inBuf[i];
	}

	return ~check;
}

/**********************************************************************
 * �������ƣ�  USART_Send_Data
 * �������������ʹ������ݽӿ�
 * para:
Dbuf: ���͵�����
len: �����͵����ݳ���
 ***********************************************************************/
void USART_Send_Data(unsigned char* Dbuf, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++) {
		while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);

		USART_SendData8(USART1, Dbuf[i]);

		while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	}

}

/**********************************************************************
 * �������ƣ�UartHandle
 * ��������������ͨ�����ݴ���
 * para:
 void
 ***********************************************************************/
void Uart_Handle(void)
{
	unsigned char check = 0;
	u8 cmdstr[CMD_LEN];
	u8 len = 0;
	u8 rec_len = __kfifo_len(&recvfifo);
	if(rec_len >= MIN_CMD_LEN)
	{
		if(CMDFLAG_START == recvfifo.buffer[(recvfifo.out + UF_START) & (recvfifo.size - 1)])
		{
			len = recvfifo.buffer[(recvfifo.out + UF_LEN) & (recvfifo.size - 1)];
			if(rec_len >= len)
			{
				__kfifo_get(&recvfifo, cmdstr, len);

				check = XOR_Inverted_Check(cmdstr, len - 1);
				if(check == cmdstr[len - 1])
				{
					if (cmdstr[UF_CMD] < 0x80)
						Response_CMD_Handle();
					else
						Request_CMD_Handle(cmdstr, len);
				}

			}
		}
		else
		{
			__kfifo_get(&recvfifo, cmdstr, 1);//�����ǰλ�ò���������ʼ��־������
		}
	}
}

void dev_pwr_on(void)
{
	g_IsDevPwrOn = 1;
	GPIO_SetBits(DEV_PWR_GPIO,DEV_PWR_GPIO_Pin);
}


void dev_pwr_off(void)
{
	g_IsDevPwrOn = 0;
	GPIO_ResetBits(DEV_PWR_GPIO, DEV_PWR_GPIO_Pin);
}

void dev_wlan_enable(void)
{
	g_IsDevWlanEn = 1;
	GPIO_SetBits(WL_EN_GPIO, WL_EN_GPIO_Pin);
}


void dev_wlan_disable(void)
{
	g_IsDevWlanEn = 0;
	GPIO_ResetBits(WL_EN_GPIO, WL_EN_GPIO_Pin);
}

void dev_wlan_reset_handle(u8 val)
{
	if  (val && (!g_IsDevWlanEn))
	{
		dev_wlan_enable();
	}
	else if ((!val) && g_IsDevWlanEn)
	{
		dev_wlan_disable();
	}
}

void dev_wlan_power_handle(u8 val)
{
	if  (val && (!g_IsDevPwrOn))
	{
		dev_pwr_on();
	}
	else if ((!val) && g_IsDevPwrOn)
	{
		dev_pwr_off();
	}
}

void host_clr_wkup_reason(void)
{
	GPIO_ResetBits(MCU_GPIO1_GPIO, MCU_GPIO1_GPIO_Pin);
	g_wkup_reason = 0;
}

void dev_wkup_host(void)
{
#if 1
	GPIO_SetBits(MCU_GPIO2_GPIO, MCU_GPIO2_GPIO_Pin);
	MDelay(1);
	GPIO_ResetBits(MCU_GPIO2_GPIO, MCU_GPIO2_GPIO_Pin);
#else
	USART_Send_Buf[UF_START] = 0x7B;
	USART_Send_Buf[UF_LEN] = 0x4;
	USART_Send_Buf[UF_CMD] = 0xbb;
	USART_Send_Buf[USART_Send_Buf[UF_LEN] - 1] = XOR_Inverted_Check(USART_Send_Buf, USART_Send_Buf[UF_LEN] - 1);
	USART_Send_Data(USART_Send_Buf, USART_Send_Buf[UF_LEN]);
#endif
}

void host_set_wkup_reason(u8 val)
{
	if(1 & val)
	{
		GPIO_SetBits(MCU_GPIO1_GPIO, MCU_GPIO1_GPIO_Pin);
	}
	else
	{
		GPIO_ResetBits(MCU_GPIO1_GPIO, MCU_GPIO1_GPIO_Pin);
	}
}

/******************************************************************
 * �������ƣ� Request_CMD_Handle
 *
 * �������������մ���ָ���
 * para:
 void
 *******************************************************************/
void Request_CMD_Handle(u8* cmdstr, u8 len)
{
	u16 adcvalue = 0;
	int ret;

	USART_Send_Buf[UF_START] = 0x7B;

	switch (cmdstr[UF_CMD]) 
	{
		/*����֪ͨMCU�ɽ��йػ�����*/
		case 0x80:

			GPIO_ResetBits(MCU_GPIO1_GPIO, MCU_GPIO1_GPIO_Pin);
			PWR_OFF();
			MDelay(100);
			g_wkup_reason = 1;
			g_WakeUp_Key_Flag = 0;
			break;
			/*���ؿ���wifiʹ�ܹ���IO*/

		case 0x8a:
			dev_wlan_power_handle(cmdstr[UF_DATA]);
			break;

		case 0x8c:
			host_clr_wkup_reason();
			break;
		case 0x84:
			if(cmdstr[UF_DATA] == 0)
			{
				g_Wifi_Reg_Det_Flag = 0;
			} else {
				g_Wifi_Reg_Det_Flag = 1;
				GPIO_ResetBits(MCU_GPIO2_GPIO, MCU_GPIO2_GPIO_Pin);
			}



			USART_Send_Buf[UF_LEN] = 0x4;
			USART_Send_Buf[UF_CMD] = 0x85;
			USART_Send_Buf[USART_Send_Buf[UF_LEN] - 1] = XOR_Inverted_Check(USART_Send_Buf, USART_Send_Buf[UF_LEN] - 1);
			USART_Send_Data(USART_Send_Buf, USART_Send_Buf[UF_LEN]);

			break;
			/*δ֪*/
		case 0x86:
			ret = RTC_Alarm_Duration_Check(cmdstr[UF_DATA + 1], cmdstr[UF_DATA + 2]);
			if (!ret) {
				RTC_AlarmCmd(DISABLE);
				RTC_ITConfig(RTC_IT_ALRA, DISABLE);
				RTC_ClearITPendingBit(RTC_IT_ALRA);
				g_Alarm_Event.flag = cmdstr[UF_DATA];
				g_Alarm_Event.hours = cmdstr[UF_DATA + 1];
				g_Alarm_Event.minutes = cmdstr[UF_DATA + 2];
				USART_Send_Buf[UF_DATA] = 0;
			} else {
				USART_Send_Buf[UF_DATA] = 1;
			}

			USART_Send_Buf[UF_LEN] = 0x5;
			USART_Send_Buf[UF_CMD] = 0x87;
			USART_Send_Buf[USART_Send_Buf[UF_LEN] - 1] = XOR_Inverted_Check(USART_Send_Buf, USART_Send_Buf[UF_LEN] - 1);
			USART_Send_Data(USART_Send_Buf, USART_Send_Buf[UF_LEN]);

			break;
			/*����������йػ����*/
		case 0x88:
			dev_wlan_reset_handle(cmdstr[UF_DATA]);

			break;
			/*��������ADC����ֵ*/
		case 0x90:
			adcvalue = ADC_GetBatVal();
			USART_Send_Buf[UF_LEN] = 0x6;
			USART_Send_Buf[UF_LEN] = 0x6;
			USART_Send_Buf[UF_CMD] = 0x91;
			USART_Send_Buf[UF_DATA] = adcvalue>>8;
			USART_Send_Buf[UF_DATA+1] = adcvalue&0xff;
			USART_Send_Buf[USART_Send_Buf[UF_LEN] - 1] = XOR_Inverted_Check(USART_Send_Buf, USART_Send_Buf[UF_LEN] - 1);
			USART_Send_Data(USART_Send_Buf, USART_Send_Buf[UF_LEN]);
			break;
		case 0xee:
			g_host_readly_uart = 1;
			break;
		default:
			break;
	}
}

/******************************************************************
 *�������ƣ�Response_CMD_Handle
 *
 *��������������ָ��������
 * para:
 void
 *******************************************************************/
void Response_CMD_Handle(void)
{}

/******************************************************************
 *�������ƣ�uart_send_sure_power
 *
 *��������������ȷ�Ϲػ�����ָ��
 * para:
 void
 *******************************************************************/
void uart_send_cmd(u8 cmd)
{
	USART_Send_Buf[UF_START] = 0x7B;
	USART_Send_Buf[UF_LEN] = 0x4;
	USART_Send_Buf[UF_CMD] = cmd;
	USART_Send_Buf[USART_Send_Buf[UF_LEN] - 1] = XOR_Inverted_Check(USART_Send_Buf, USART_Send_Buf[UF_LEN] - 1);
	USART_Send_Data(USART_Send_Buf, USART_Send_Buf[UF_LEN]);
}

/******************************************************************
 *�������ƣ�CLK_Config
 *
 *���������������շ�ʱ�ӹ����ʼ��
 * para:
 void
 *******************************************************************/
void CLK_Config(void)
{
	CLK_DeInit();

	CLK_HSICmd(ENABLE);
	CLK_SYSCLKDivConfig(BAUD_CLK);//CLK_SYSCLKDiv_32

	while (((CLK->ICKCR) & 0x02) != 0x02); //HSI׼������

	CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_HSI);
	CLK_SYSCLKSourceSwitchCmd(ENABLE);

	while (((CLK->SWCR) & 0x01) == 0x01); //�л����
}

/******************************************************************
 *�������ƣ�TIM3_Config
 *
 *������������ʱ�����ã����������16Khz
 * para:
 void
 *******************************************************************/
void TIM3_Config(void)
{
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM3, ENABLE);/* Enable TIM3 CLK */
	/* Time base configuration */

	TIM3_TimeBaseInit(TIM3_Prescaler_16, TIM3_CounterMode_Up, 125); //20MS 2480------------4ms 499   // 0.5M 4ms 625  1ms - 125

	/* Clear TIM4 update flag */
	TIM3_ClearFlag(TIM3_FLAG_Update);
	/* Enable update interrupt */
	TIM3_ITConfig(TIM3_IT_Update, ENABLE);
	/* Enable TIM4 */
	TIM3_Cmd(ENABLE);
}

/******************************************************************
 *�������ƣ�Usart_Config
 *
 *����������uart��ʼ�����ã�������Ĭ��Ϊ9600
 * para:
 void
 *******************************************************************/
void Usart_Config(void)
{
	GPIO_Init(USART_RX_GPIO, USART_RX_GPIO_Pin, GPIO_Mode_In_PU_No_IT);
	GPIO_Init(USART_TX_GPIO, USART_TX_GPIO_Pin, GPIO_Mode_Out_PP_Low_Fast);
	CLK_PeripheralClockConfig(CLK_Peripheral_USART1, ENABLE);
	USART_Init(USART1, BAUD_RATE, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No, (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));
	USART_ClearITPendingBit(USART1, USART_IT_RXNE); //����USART1->SR
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  //����USART1->CR2��RIENλ
	USART_Cmd(USART1, ENABLE);
}

/******************************************************************
 *�������ƣ�Pin_Det_Handle
 *
 *����������wifi�жϻ��Ѵ����Լ�wifi����ʹ��IO����
 * para:
 void
 *******************************************************************/
void Pin_Det_Handle(void)
{
	u8 val;

#if 0
	/* wlan wake detect handle */
	if (!g_IsSystemOn) {
		/*  wifi �жϻ��ѱ�־ */
		if (g_Wifi_WakeUp == 1) {
			/* �������IO���ã��˳��͹���ģʽ */
			InitExitHaltMode();

			PWR_ON();
			/* ����wifi��������IO�ţ����� */
			GPIO_SetBits(MCU_GPIO2_GPIO, MCU_GPIO2_GPIO_Pin);

			g_Wifi_WakeUp = 0;
		}
	}
	else {
		g_Wifi_WakeUp = 0;
	}


	/* ���ѿ�������wifi�Ĵ�����־����1 */
	if (g_IsSystemOn && g_Wifi_Reg_Det_Flag) {

		/* ���ؿ���wifi�Ĵ����Ƿ������ã��ߵ�ƽ����������*/
		val = !!GPIO_ReadInputDataBit(REG_ON_DET_GPIO, REG_ON_DET_GPIO_Pin);

		/*����wifiʹ�ܹ���IO*/
		if (val) {
			GPIO_SetBits(WIFI_REG_ON_GPIO, WIFI_REG_ON_GPIO_Pin);
		} else {
			GPIO_ResetBits(WIFI_REG_ON_GPIO, WIFI_REG_ON_GPIO_Pin);
		}
	}
#endif
	// DEV WAK HOST DETECT
	if(!g_IsSystemOn)
	{
		val = !!GPIO_ReadInputDataBit(D2H_WAK_GPIO, D2H_WAK_GPIO_Pin);
		if(val)
		{
			g_debounce_cnt++;
			if(GPIO_DEBOUNCE < g_debounce_cnt)
			{
				PWR_ON();
				MDelay(2);
				host_set_wkup_reason(g_wkup_reason);
				InitExitHaltMode();
			}
		}
		else
		{
			g_Wifi_WakeUp = 0;
			g_debounce_cnt = 0;
		}
	}
	else
	{
		val = !!GPIO_ReadInputDataBit(D2H_WAK_GPIO, D2H_WAK_GPIO_Pin);
		if(val)
		{
			if(!g_Iswkupmsgsend)
			{
				dev_wkup_host();
				g_Iswkupmsgsend = 1;
			}
		}
		else
		{
			g_Wifi_WakeUp = 0;
			g_Iswkupmsgsend = 0;
		}
	}

}

/******************************************************************
 *�������ƣ�Key_Handle
 *
 *������������������:
 �ϵ�״̬����: ����
 ����״̬: ֪ͨ���ط�����������
 * para:
 void
 *******************************************************************/
void Key_Handle(void)
{
	/*������⴦��*/
	if (1 == g_Key_Handle_Flag) {
		if (g_IsSystemOn == 0) {
			InitExitHaltMode();
			PWR_ON();
			if(0 == g_Wifi_Reg_Det_Flag) {
				GPIO_SetBits(MCU_GPIO2_GPIO, MCU_GPIO2_GPIO_Pin);
			}
			g_KeyPoweroff_Flag = 0;
			g_Key_Handle_Flag = 0;
		} else {
			/*���ѿ�������֪ͨ�����ٴη���������*/
			if(g_host_readly_uart && (g_time_count % 30 == 0)) {
				uart_send_cmd(0x93);
				g_Key_Handle_Flag = 0;
			}
		}
	} else if(2 == g_Key_Handle_Flag) {

		if(g_host_readly_uart)
			uart_send_cmd(0x95);//reset wifi info

		g_Key_Handle_Flag = 0;
	}
}

/******************************************************************
 *�������ƣ�PWR_Detect_Handle
 *
 *��������������豸�Ƿ���USB���翪��������ֱ�ӿ���
 * para:
 void
 *******************************************************************/
void PWR_Detect_Handle()
{
	/*�Ƿ��⵽��USB����*/
	if (g_IsSystemOn == 0) {
		if (g_Vbus_Check == 1){
			PWR_ON();
			g_Vbus_Check = 0;
		}
	}
}

/******************************************************************
 *�������ƣ�PIR_Detect_Handle
 *
 *����������PIR��⴦��
 * para:
 void
 *******************************************************************/
void PIR_Detect_Handle()
{
	/*�����⴦��*/
	if (g_PIR_Check == 1) {

		/*���ͨ���������ػ����޷�ͨ��PIR�ϵ�*/
		if (g_IsSystemOn == 0&&g_KeyPoweroff_Flag!=1) {
			InitExitHaltMode();
			PWR_ON();
			GPIO_SetBits(MCU_GPIO2_GPIO, MCU_GPIO2_GPIO_Pin);
		}

		g_PIR_Check = 0;
	}
}

/******************************************************************
 *�������ƣ�Fun_Init
 *
 *��������������ģ���ʼ��
 * para:
 void
 *******************************************************************/
void Fun_Init(void)
{
	/*��ʱ����ʼ��*/
	TIM3_Config();

	/*���ڳ�ʼ��*/
	Usart_Config();

	/*ADC������ʼ��*/
	ADC_Init_adc0();
}

/******************************************************************
 *�������ƣ�Board_Init
 *
 *���������� �״��ϵ磬������GPIO��ʼ��
 * para:
 void
 *******************************************************************/
void Board_Init(void)
{
	/*�����ϵ��־λ*/
	g_IsSystemOn = 0;
	/*�������±�־λ*/
	g_Key_Handle_Flag = 0;

	/*vbus*/
	g_Vbus_Check = 0;

	/*���ڽ���ʱ��*/
	USART_Receive_Timeout = 0;
	/*���ڽ��ձ�־*/
	USART_Receive_Flag = 0;

	/*��������ʼ��*/
	GPIO_Init(SW_KEY_GPIO, SW_KEY_GPIO_Pin, GPIO_Mode_In_PU_No_IT); //�������
	/*���������ϵ�IO��*/
	GPIO_Init(PWR_EN_GPIO, PWR_EN_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow); //Ĭ��3·��ԴΪ��
	/*�������е�ԴIO��*/
	GPIO_Init(PWR_HOLD_GPIO, PWR_HOLD_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow); //Ĭ��all��ԴΪ��
	/*��������wifi�Ĵ�����־IO�ڳ�ʼ��*/
	GPIO_Init(REG_ON_DET_GPIO, REG_ON_DET_GPIO_Pin, GPIO_Mode_In_FL_No_IT);

#if 0
	/*MCU����wifiʹ�ܹ���IO��ʼ��*/
	GPIO_Init(WIFI_REG_ON_GPIO, WIFI_REG_ON_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow);
	/*MCU�����Ƿ�������IO��ʼ��*/
	GPIO_Init(MCU_GPIO2_GPIO, MCU_GPIO2_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow);
#endif

	/*Ԥ��IO������������*/
	GPIO_Init(MCU_GPIO3_GPIO, MCU_GPIO3_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow);
	/*��������GPIO��ʼ��*/
	GPIO_Init(BT_HOST_WAKE_GPIO, BT_HOST_WAKE_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow);
	/*����ʹ�ܹ���IO��ʼ��*/
	GPIO_Init(BT_REG_ON_GPIO, BT_REG_ON_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow);
	/*wifi����MCU IO��ʼ��*/
	GPIO_Init(WIFI_GPIO2_GPIO, WIFI_GPIO2_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow);
	/*VBUS IO��ʼ��*/
	//VBAUS�ػ�״̬�»���MCU
	GPIO_Init(VBUS_DETECT, VBUS_DETECT_GPIO_Pin, GPIO_Mode_In_FL_No_IT);

	/*����WIFI���Ѽ���Ϊ�ж�ģʽ*/
	GPIO_Init(WIFI_GPIO1_GPIO, WIFI_GPIO1_GPIO_Pin, GPIO_Mode_In_FL_IT);


	/*wifi�ϵ�IO��ʼ��*/
	GPIO_Init(DEV_PWR_GPIO, DEV_PWR_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow);
	/*wifi reset IO��ʼ��*/
	GPIO_Init(WL_EN_GPIO, WL_EN_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow);

	GPIO_Init(MCU_GPIO1_GPIO, MCU_GPIO1_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow);
	GPIO_Init(MCU_GPIO2_GPIO, MCU_GPIO2_GPIO_Pin, GPIO_Mode_Out_PP_Low_Slow);
	/*wifi������ƽ̨IO��ʼ��*/
	GPIO_Init(D2H_WAK_GPIO, D2H_WAK_GPIO_Pin, GPIO_Mode_In_FL_No_IT);



	EXTI_SetPinSensitivity(EXTI_Pin_0, EXTI_Trigger_Rising);
	ITC_SetSoftwarePriority(EXTI0_IRQn, ITC_PriorityLevel_1);

	/*���õ�ص�������Ϊ�ж�ģʽ*/
	GPIO_Init(BAT_LOW_DET_GPIO, BAT_LOW_DET_GPIO_Pin, GPIO_Mode_In_FL_IT);
	/*Ȼ�������ж�1Ϊ�½��ص͵�ƽ����*/
	EXTI_SetPinSensitivity(EXTI_Pin_2, EXTI_Trigger_Falling);
	/*�����жϵ����ȼ�*/
	ITC_SetSoftwarePriority(EXTI2_IRQn, ITC_PriorityLevel_1);

#if 0
	/*PIR*/
	GPIO_Init(PIR_GPIO, PIR_GPIO_Pin, GPIO_Mode_In_FL_No_IT);
	//GPIO_Init(PIR_GPIO, PIR_GPIO_Pin, GPIO_Mode_In_PU_IT);
	/*Ȼ�������ж�1Ϊ��������*/
	EXTI_SetPinSensitivity(EXTI_Pin_0, EXTI_Trigger_Rising);
	/*�����жϵ����ȼ�*/
	ITC_SetSoftwarePriority(EXTI0_IRQn, ITC_PriorityLevel_2);
#endif
	CLK_Config();
}

/******************************************************************
 *�������ƣ�check_power_action
 *
 *��������������豸�Ƿ�����ϵ�Ҫ��
 �ް�����������pir����
 * para:
 void
 *******************************************************************/
void check_power_action(void)
{
	if(g_poweroff_check_flg) {
		if(g_key_ok_poweroff || g_pir_ok_poweroff){
			uart_send_cmd(0x00);
			g_poweroff_check_flg = 0;
		}
	}
}

/******************************************************************
 *�������ƣ�main
 *
 *�������������س���
 * para:
 void
 *******************************************************************/
void main(void)
{
	GPIO_init();

	Board_Init();

	PWR_ON();

	Fun_Init();

	enableInterrupts();

	kfifo_init(&recvfifo);
	while (1)
	{

		//     Key_Handle();

		Uart_Handle();

		Pin_Det_Handle();

		//PIR_Detect_Handle();//PIR���

		Change_Mode();

		//    check_power_action();

		MDelay(1);

		g_time_count++;

		//GPIO_SetBits(WIFI_REG_ON_GPIO, WIFI_REG_ON_GPIO_Pin);
	}


}

