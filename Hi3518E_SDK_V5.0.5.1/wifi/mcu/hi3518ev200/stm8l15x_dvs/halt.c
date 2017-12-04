#include "halt.h"
#include "boardconfig.h"

/******************************************************************
*�������ƣ�InitEnterHaltMode
*
*��������������͹���ģʽǰ������
* para:
	    void
*******************************************************************/
void InitEnterHaltMode(void)
{
	/*�ر��ж�*/
    disableInterrupts();
#if 0
    /*�����ػ��ܽ���Ϊ�ж�ģʽ*/
    GPIO_Init(SW_KEY_GPIO,SW_KEY_GPIO_Pin,GPIO_Mode_In_PU_IT);

    /*Ȼ�������ж�1Ϊ�½��ص͵�ƽ����*/
    EXTI_SetPinSensitivity(EXTI_Pin_1, EXTI_Trigger_Falling);

    /*�����жϵ����ȼ�*/
    ITC_SetSoftwarePriority(EXTI1_IRQn, ITC_PriorityLevel_1);
#endif
    GPIO_Init(D2H_WAK_GPIO,D2H_WAK_GPIO_Pin,GPIO_Mode_In_FL_IT);

    /*Ȼ�������ж�1Ϊ�½��ص͵�ƽ����*/
    EXTI_SetPinSensitivity(EXTI_Pin_0, EXTI_Trigger_Rising);

    /*�����жϵ����ȼ�*/
    ITC_SetSoftwarePriority(EXTI0_IRQn, ITC_PriorityLevel_1);


 	/*�رն�ʱ���ж�*/
    TIM3_DeInit();
    /*�رմ���*/
    GPIO_Init(USART_RX_GPIO,USART_RX_GPIO_Pin,GPIO_Mode_Out_PP_Low_Slow);
    GPIO_Init(USART_TX_GPIO,USART_TX_GPIO_Pin,GPIO_Mode_Out_PP_Low_Slow);

    GPIO_ResetBits(USART_TX_GPIO, USART_TX_GPIO_Pin);
    GPIO_ResetBits(USART_RX_GPIO, USART_RX_GPIO_Pin);

    USART_Cmd(USART1,DISABLE);
    USART_DeInit(USART1);

	/*�ر�ADC����*/
    ADC_Deinit_adc0();
    
    /*���迪��PIR�жϻ��ѣ�������PIRΪ�ж�ģʽ*/
    extern u8 g_Wifi_Reg_Det_Flag;
    if(g_Wifi_Reg_Det_Flag==0) {
      GPIO_Init(PIR_GPIO, PIR_GPIO_Pin, GPIO_Mode_In_FL_IT);
    } else {
      GPIO_Init(PIR_GPIO, PIR_GPIO_Pin, GPIO_Mode_Out_OD_HiZ_Slow);
    }
    /*�����ж�*/
    enableInterrupts();
}

/******************************************************************
*�������ƣ�InitExitHaltMode
*
*�����������˳��͹���ģʽ�������
* para:
	    void
*******************************************************************/
void InitExitHaltMode(void)
{
  disableInterrupts();

  GPIO_Init(PIR_GPIO,PIR_GPIO_Pin,GPIO_Mode_In_FL_No_IT);

  TIM3_Config();

  ADC_Init_adc0();

  Usart_Config();

  enableInterrupts();
}

/******************************************************************
*�������ƣ�GPIO_init
*
*����������GPIO��ʼ��
* para:
	    void
*******************************************************************/
void GPIO_init(void)
{
  GPIO_DeInit(GPIOA);

  GPIO_DeInit(GPIOB);

  GPIO_DeInit(GPIOC);

  GPIO_DeInit(GPIOD);

/* Port A in output push-pull 0 */
  GPIO_Init(GPIOA,GPIO_Pin_All,GPIO_Mode_Out_PP_Low_Slow);

/* Port B in output push-pull 0 */
  GPIO_Init(GPIOB, GPIO_Pin_All, GPIO_Mode_Out_PP_Low_Slow);

/* Port C in output push-pull 0 */
   GPIO_Init(GPIOC,GPIO_Pin_All, GPIO_Mode_Out_PP_Low_Slow);

/* Port D in output push-pull 0 */
  GPIO_Init(GPIOD,GPIO_Pin_All, GPIO_Mode_Out_PP_Low_Slow);
}

/**********************************************************************
* �������ƣ�  ALL_PWR_ON
* ���������������й��������ϵ磬�������غ�wifi
* para:
		void
***********************************************************************/
void ALL_PWR_ON(void)
{

    GPIO_SetBits(PWR_EN_GPIO, PWR_EN_GPIO_Pin);
    //MDelay(10);
    GPIO_SetBits(PWR_HOLD_GPIO, PWR_HOLD_GPIO_Pin);

    g_IsSystemOn = 1;

}
/**********************************************************************
* �������ƣ�  ALL_PWR_OFF
* �������������غ�wifi�жϵ�Դ��ֻ����MCU�͹��ģ�
				     ����߽��ܵ͹���ģʽ
* para:
		void
***********************************************************************/
void ALL_PWR_OFF(void)
{
    GPIO_ResetBits(PWR_HOLD_GPIO, PWR_HOLD_GPIO_Pin);

    g_IsSystemOn = 0;
}

/******************************************************************
*�������ƣ�BAR_Detect
*
*��������������״̬�µ�ص������͹ر�WIFI��������������
* para:
	    void
*******************************************************************/
void BAR_Detect(void)
{
     u8 key_val = 0;

    /*WIFI�����е�ص����� ���*/
    key_val = GPIO_ReadInputDataBit(BAT_LOW_DET_GPIO, BAT_LOW_DET_GPIO_Pin); //��⵽Ϊ��

    if(g_Bar_Det_Flag||(!key_val))
    {
        ALL_PWR_OFF();
        g_Bar_Det_Flag=0;
    }
}

/******************************************************************
*�������ƣ�Change_Mode
*
*��������������ģʽ�͵͹���ģʽ�л�
* para:
	  void
*******************************************************************/
void Change_Mode(void)
{
	/*����⵽��ص���< 3v������ȴ���*/
	//BAR_Detect();

	/*  normal mode enter halt mode*/
	/*��ʱ�����ã���������ʱ��ʹMCU����͹��� */
    if(!g_Wifi_WakeUp)
    {
        if(g_IsSystemOn==0)	{
        /*�����޳������»��ް�������*/
            if(!g_WakeUp_Key_Flag) {
                /*�������IOģʽ*/
                InitEnterHaltMode();
                /*����͹���ģʽ*/
                Halt_Mode();
            }
		}
    }
	// halt mode enter normal mode
	/*�����ж�*/
	if(g_WakeUp_Halt_Flag||g_Vbus_Check) {
		/*ȷ���ǹػ�״̬���˳�haltģʽ*/
		if(g_IsSystemOn==0) {
			/*�˳��͹��ĵ��������*/
			InitExitHaltMode();
			/*��������*/
			g_WakeUp_Halt_Flag=0;
			/*����Ƿ�������°�����ʶ*/
			g_WakeUp_Key_Flag = 10;
		}
	}
}


/******************************************************************
*�������ƣ�Halt_Mode
*
*�����������͹���ģʽ�ӿ�
* para:
	  void
*******************************************************************/
void Halt_Mode(void)
{
	/*ENTER ACTIVE HALT CLOSE the main voltage regulator is powered off*/
	CLK->ICKCR|=CLK_ICKCR_SAHALT;

	/* Set STM8 in low power */
	PWR->CSR2 = 0x2;

	/*  low power fast wake up disable*/
	PWR_FastWakeUpCmd(DISABLE);

	CLK_RTCClockConfig(CLK_RTCCLKSource_Off, CLK_RTCCLKDiv_1);
	CLK_PeripheralClockConfig(CLK_Peripheral_RTC, DISABLE);

	CLK_LSICmd(DISABLE);
	while ((CLK->ICKCR & 0x04) != 0x00);

	CLK_PeripheralClockConfig(CLK_Peripheral_TIM1, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_TIM5, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_I2C1, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_SPI1, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_USART2, DISABLE);
	CLK_PeripheralClockConfig(CLK_Peripheral_USART3, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_BEEP, DISABLE);
	CLK_PeripheralClockConfig(CLK_Peripheral_DAC, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_LCD, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_DMA1, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_BOOTROM, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_COMP, DISABLE);
	CLK_PeripheralClockConfig(CLK_Peripheral_AES, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_SPI2, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_CSSLSE, DISABLE);


	CLK_PeripheralClockConfig(CLK_Peripheral_TIM3, DISABLE);

	CLK_PeripheralClockConfig(CLK_Peripheral_USART1, DISABLE);

	enableInterrupts();

	halt(); //system go to halt mode;
}
