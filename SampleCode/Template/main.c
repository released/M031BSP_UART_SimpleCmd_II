/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "project_config.h"


/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;
volatile uint32_t counter_systick = 0;

#if defined (ENABLE_TICK_EVENT)
typedef void (*sys_pvTimeFunPtr)(void);   /* function pointer */
typedef struct timeEvent_t
{
    uint8_t             active;
    unsigned long       initTick;
    unsigned long       curTick;
    sys_pvTimeFunPtr    funPtr;
} TimeEvent_T;

#define TICKEVENTCOUNT                                 (8)                   
volatile  TimeEvent_T tTimerEvent[TICKEVENTCOUNT];
volatile uint8_t _sys_uTimerEventCount = 0;             /* Speed up interrupt reponse time if no callback function */
#endif

enum{
	fmt_head ,      // 0
 	fmt_code ,      // 1

 	fmt_data_0 ,    // 2
 	fmt_data_1 ,    // 3
 	fmt_data_2 ,    // 4
 	fmt_data_3 ,    // 5

 	fmt_cs ,        // 6
 	fmt_tail ,      // 7
}cmd_fmt_index;

enum{
	error_head , 
	error_wrong_code ,

	error_wrong_data0 ,   
    error_wrong_data1 ,  
    error_wrong_data2 ,  
    error_wrong_data3 , 

	error_cs ,  
	error_tail , 

	error_timeout,  
	error_not_define ,       

}err_fmt_index;

#define _T(x) x

#define UART_RX_RCV_LEN                         (8)

#define UART_CMD_FMT_HEAD                       (0x5A)
#define UART_CMD_FMT_TAIL                       (0xA5)

#define UART_CMD_FMT_CODE1                      (0x01)
#define UART_CMD_FMT_CODE2                      (0x02)
#define UART_CMD_FMT_CODE3                      (0x03)
#define UART_CMD_FMT_CODE4                      (0x04)

unsigned char RXBUFFER[UART_RX_RCV_LEN] = {0};
unsigned char uart_rcv_cnt = 0;
volatile uint32_t u32rcvtick = 0;

const uint8_t CRC8TAB[256] = 
{ 
	//0
	0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97, 
	0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E, 
	//1
	0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4,
	0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D,
	//2
	0x86, 0xB7, 0xE4, 0xD5, 0x42, 0x73, 0x20, 0x11,
	0x3F, 0x0E, 0x5D, 0x6C, 0xFB, 0xCA, 0x99, 0xA8,
	//3
	0xC5, 0xF4, 0xA7, 0x96, 0x01, 0x30, 0x63, 0x52, 
	0x7C, 0x4D, 0x1E, 0x2F, 0xB8, 0x89, 0xDA, 0xEB,
	//4 
	0x3D, 0x0C, 0x5F, 0x6E, 0xF9, 0xC8, 0x9B, 0xAA,
	0x84, 0xB5, 0xE6, 0xD7, 0x40, 0x71, 0x22, 0x13,
	//5
	0x7E, 0x4F, 0x1C, 0x2D, 0xBA, 0x8B, 0xD8, 0xE9,
	0xC7, 0xF6, 0xA5, 0x94, 0x03, 0x32, 0x61, 0x50,
	//6
	0xBB, 0x8A, 0xD9, 0xE8, 0x7F, 0x4E, 0x1D, 0x2C,
	0x02, 0x33, 0x60, 0x51, 0xC6, 0xF7, 0xA4, 0x95,
	//7
	0xF8, 0xC9, 0x9A, 0xAB, 0x3C, 0x0D, 0x5E, 0x6F,
	0x41, 0x70, 0x23, 0x12, 0x85, 0xB4, 0xE7, 0xD6,
	//8
	0x7A, 0x4B, 0x18, 0x29, 0xBE, 0x8F, 0xDC, 0xED,
	0xC3, 0xF2, 0xA1, 0x90, 0x07, 0x36, 0x65, 0x54,
	//9
	0x39, 0x08, 0x5B, 0x6A, 0xFD, 0xCC, 0x9F, 0xAE,
	0x80, 0xB1, 0xE2, 0xD3, 0x44, 0x75, 0x26, 0x17,
	//A
	0xFC, 0xCD, 0x9E, 0xAF, 0x38, 0x09, 0x5A, 0x6B,
	0x45, 0x74, 0x27, 0x16, 0x81, 0xB0, 0xE3, 0xD2,
	//B
	0xBF, 0x8E, 0xDD, 0xEC, 0x7B, 0x4A, 0x19, 0x28,
	0x06, 0x37, 0x64, 0x55, 0xC2, 0xF3, 0xA0, 0x91,
	//C
	0x47, 0x76, 0x25, 0x14, 0x83, 0xB2, 0xE1, 0xD0,
	0xFE, 0xCF, 0x9C, 0xAD, 0x3A, 0x0B, 0x58, 0x69,
	//D
	0x04, 0x35, 0x66, 0x57, 0xC0, 0xF1, 0xA2, 0x93,
	0xBD, 0x8C, 0xDF, 0xFE, 0x79, 0x48, 0x1B, 0x2A,
	//E
	0xC1, 0xF0, 0xA3, 0x92, 0x05, 0x34, 0x67, 0x56,
	0x78, 0x49, 0x1A, 0x2B, 0xBC, 0x8D, 0xDE, 0xEF,
	//F
	0x82, 0xB3, 0xE0, 0xD1, 0x46, 0x77, 0x24, 0x15,
	0x3B, 0x0A, 0x59, 0x68, 0xFF, 0xCE, 0x9D, 0xAC
}; 

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

uint32_t get_systick(void)
{
	return (counter_systick);
}

void set_systick(uint32_t t)
{
	counter_systick = t;
}

void systick_counter(void)
{
	counter_systick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void tick_counter(void)
{
	counter_tick++;
    if (get_tick() >= 60000)
    {
        set_tick(0);
    }
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    #if 1
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}
    #else
    if (memcmp(src, des, nBytes))
    {
        printf("\nMismatch!! - %d\n", nBytes);
        for (i = 0; i < nBytes; i++)
            printf("0x%02x    0x%02x\n", src[i], des[i]);
        return -1;
    }
    #endif

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

// void delay_ms(uint16_t ms)
// {
// 	TIMER_Delay(TIMER0, 1000*ms);
// }


#if defined (ENABLE_TICK_EVENT)
void TickCallback_processB(void)
{
    printf("%s test \r\n" , __FUNCTION__);
}

void TickCallback_processA(void)
{
    printf("%s test \r\n" , __FUNCTION__);
}

void TickClearTickEvent(uint8_t u8TimeEventID)
{
    if (u8TimeEventID > TICKEVENTCOUNT)
        return;

    if (tTimerEvent[u8TimeEventID].active == TRUE)
    {
        tTimerEvent[u8TimeEventID].active = FALSE;
        _sys_uTimerEventCount--;
    }
}

signed char TickSetTickEvent(unsigned long uTimeTick, void *pvFun)
{
    int  i;
    int u8TimeEventID = 0;

    for (i = 0; i < TICKEVENTCOUNT; i++)
    {
        if (tTimerEvent[i].active == FALSE)
        {
            tTimerEvent[i].active = TRUE;
            tTimerEvent[i].initTick = uTimeTick;
            tTimerEvent[i].curTick = uTimeTick;
            tTimerEvent[i].funPtr = (sys_pvTimeFunPtr)pvFun;
            u8TimeEventID = i;
            _sys_uTimerEventCount += 1;
            break;
        }
    }

    if (i == TICKEVENTCOUNT)
    {
        return -1;    /* -1 means invalid channel */
    }
    else
    {
        return u8TimeEventID;    /* Event ID start from 0*/
    }
}

void TickCheckTickEvent(void)
{
    uint8_t i = 0;

    if (_sys_uTimerEventCount)
    {
        for (i = 0; i < TICKEVENTCOUNT; i++)
        {
            if (tTimerEvent[i].active)
            {
                tTimerEvent[i].curTick--;

                if (tTimerEvent[i].curTick == 0)
                {
                    (*tTimerEvent[i].funPtr)();
                    tTimerEvent[i].curTick = tTimerEvent[i].initTick;
                }
            }
        }
    }
}

void TickInitTickEvent(void)
{
    uint8_t i = 0;

    _sys_uTimerEventCount = 0;

    /* Remove all callback function */
    for (i = 0; i < TICKEVENTCOUNT; i++)
        TickClearTickEvent(i);

    _sys_uTimerEventCount = 0;
}
#endif 

void SysTick_Handler(void)
{

    systick_counter();

    if (get_systick() >= 0xFFFFFFFF)
    {
        set_systick(0);      
    }

    // if ((get_systick() % 1000) == 0)
    // {
       
    // }

    #if defined (ENABLE_TICK_EVENT)
    TickCheckTickEvent();
    #endif    
}

void SysTick_delay(unsigned long delay)
{  
    
    uint32_t tickstart = get_systick(); 
    uint32_t wait = delay; 

    while((get_systick() - tickstart) < wait) 
    { 
    } 

}

void SysTick_enable(int ticks_per_second)
{
    set_systick(0);
    if (SysTick_Config(SystemCoreClock / ticks_per_second))
    {
        /* Setup SysTick Timer for 1 second interrupts  */
        printf("Set system tick error!!\n");
        while (1);
    }

    #if defined (ENABLE_TICK_EVENT)
    TickInitTickEvent();
    #endif
}

uint8_t CRC8( uint8_t *buf, uint16_t len)     
{               
	uint8_t  crc=0;

	while ( len-- )     
	{   
		crc = CRC8TAB[crc^*buf]; 

		buf++;   
	}     

	return crc;     
}  

void stop_timer0(void)
{
    TIMER_ClearIntFlag(TIMER0);    
    TIMER_DisableInt(TIMER0); 
    TIMER_Stop(TIMER0);        
}

void start_timer0(void)
{
    TIMER_EnableInt(TIMER0);
    TIMER_Start(TIMER0);
}


void put_rc(uint8_t rc)
{
    
    const char *p =
        _T("error_head\0error_wrong_code\0error_wrong_data0\0error_wrong_data1\0error_wrong_data2\0error_wrong_data3\0")
        _T("error_cs\0error_tail\0error_timeout\0");

    uint32_t i;
    for (i = 0; (i != (unsigned int)rc) && *p; i++)
    {
        while(*p++) ;
    }
    printf(_T("rc=%u result:%s\r\n"), (unsigned int)rc, p);
}

void fmt_err(uint8_t idx)
{
    uint8_t i = 0;

    printf("cmd parsing error(0x%2X)\r\n",idx);
    put_rc(idx);
    for(i = 0 ; i <= (UART_RX_RCV_LEN-1) ; i++)
    {
        printf("(%d):0x%2X",i , RXBUFFER[i]);
        if (i == idx)
        {
            printf("   ***");
        }
        printf("\r\n");
        // if ((i+1)%8 ==0)
        // {
        //     printf("\r\n");
        // }            
    }
    printf("\r\n\r\n");

    uart_rcv_cnt = 0;
    set_flag(flag_uart_rx_rcv_timeout_10000ms ,DISABLE);
    stop_timer0();

    for(i = 0 ; i <= (UART_RX_RCV_LEN-1) ; i++)
    {
        RXBUFFER[i] = 0x00;
    }
}

void fmt_parsing(void)
{
    uint8_t cal = 0;

    if (is_flag_set(flag_uart_rx_rcv_ready))                        //Uart receives completion and starts parsing.
    {
        if (RXBUFFER[fmt_head] != UART_CMD_FMT_HEAD )               //Input format parsing. check header
        {
            fmt_err(error_head);
        }
        else
        {
            cal = CRC8(RXBUFFER, (UART_RX_RCV_LEN - 2 ));
            if (RXBUFFER[fmt_cs] != cal)                            //Input format parsing. check checksum
            {
               fmt_err(error_cs);  
            }
            else 
            {
                if (RXBUFFER[fmt_tail] != UART_CMD_FMT_TAIL)        //Input format parsing. check tail
                {
                    fmt_err(error_tail);
                }
                else
                {
                    if ((RXBUFFER[fmt_code] == UART_CMD_FMT_CODE1) || 
                        (RXBUFFER[fmt_code] == UART_CMD_FMT_CODE2) || 
                        (RXBUFFER[fmt_code] == UART_CMD_FMT_CODE3) ||
                        (RXBUFFER[fmt_code] == UART_CMD_FMT_CODE4) )
                    {
                        set_flag(flag_uart_rx_rcv_timeout_10000ms ,DISABLE);

                        printf("process OK !\r\n");

                        dump_buffer(RXBUFFER , UART_RX_RCV_LEN);
                    }
                    else
                    {
                        fmt_err(error_wrong_code);
                    }                        
                }
            }
        }         

        set_flag(flag_uart_rx_rcv_ready ,DISABLE); 
    }
}

void rx_rcv_irq(uint8_t c)
{    
    if (c == 0x0D /*|| c == UART_CMD_FMT_TAIL*/)                        //hit enter or tail judged that the input is completed
    {
        u32rcvtick = 0;
        set_flag(flag_uart_rx_rcv_timeout_10000ms ,ENABLE);
        stop_timer0();
    }

    if (uart_rcv_cnt < (UART_RX_RCV_LEN-1))
    {
        if (c == 0x7F || c == 0x08)                                 //Delete or Backspace
        {
            uart_rcv_cnt--;
        }
        else
        {
            RXBUFFER[uart_rcv_cnt] = c;
            uart_rcv_cnt++;
        }

        if (uart_rcv_cnt == 1)
        {
            start_timer0();
        }

    }
    else if (uart_rcv_cnt ==  (UART_RX_RCV_LEN-1))
    {
        if (c == 0x7F || c == 0x08)                                 //Delete or Backspace
        {
            uart_rcv_cnt--;
        }
        else
        {
            RXBUFFER[uart_rcv_cnt] = c;
            set_flag(flag_uart_rx_rcv_ready ,ENABLE); 
            uart_rcv_cnt = 0;
            stop_timer0();
        }
    }    
}



void TMR0_IRQHandler(void)
{	
    if(TIMER_GetIntFlag(TIMER0) == 1)
    {
        u32rcvtick++;
        
		if (u32rcvtick == 1000)
		{
            u32rcvtick = 0;
            set_flag(flag_uart_rx_rcv_timeout_10000ms ,ENABLE);
            stop_timer0();
		}   

		set_flag(flag_uart_rx_rcv_ready ,DISABLE); 
    }
}

void TIMER0_Init(void)
{
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 10);

    NVIC_EnableIRQ(TMR0_IRQn);	
}

void TMR1_IRQHandler(void)
{
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();


		if ((get_tick() % 1000) == 0)
		{
            set_flag(flag_timer_period_1000ms ,ENABLE);
		}          

		if ((get_tick() % 50) == 0)
		{
            
		}	
    }
}

void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void CRC_CAL_SAMPLE(void)
{   
    uint8_t buf[6] = {0};
    uint8_t res = 0 ;
    uint8_t i = 0;

    buf[fmt_head] = UART_CMD_FMT_HEAD;
    buf[fmt_code] = UART_CMD_FMT_CODE4;
    buf[fmt_data_0] = 0x11;
    buf[fmt_data_1] = 0x22;
    buf[fmt_data_2] = 0x33;
    buf[fmt_data_3] = 0x44;
    
    for(i = 0 ; i < 6 ; i++)
    {
        printf("%d:0x%2X\r\n" , i , buf[i]);
    }

    res = CRC8(buf,6);
    printf("CRC8 : 0x%2X\r\n",res);

/*
    CRC8 

    TEST cmd receive CORRECT
    HEAD :0x5A + CODE :0x01 + 0x01 + 0x02 + 0x03 + 0x04 = 0xAA
    HEAD :0x5A + CODE :0x02 + 0x20 + 0x30 + 0x40 + 0x50 = 0x4A
    HEAD :0x5A + CODE :0x03 + 0x7F + 0x7E + 0x7D + 0x7C = 0xC3
    HEAD :0x5A + CODE :0x04 + 0x11 + 0x22 + 0x33 + 0x44 = 0xA8


    TEST cmd receive CODE INCORRECT
    HEAD :0x5A + CODE :0x05 + 0x20 + 0x30 + 0x40 + 0x50 = 0x11

    TEST cmd receive CS INCORRECT
    HEAD :0x5A + CODE :0x02 + 0x20 + 0x30 + 0x40 + 0x50

    TEST cmd receive HEAD INCORRECT
    HEAD :0x5C + CODE :0x02 + 0x20 + 0x30 + 0x40 + 0x50 

    TEST cmd receive HEAD INCORRECT
    HEAD :0x5A + CODE :0x02 + 0x20 + 0x30 + 0x40 + 0x50 


*/

}

void loop(void)
{
	// static uint32_t LOG1 = 0;
	// static uint32_t LOG2 = 0;

    if ((get_systick() % 1000) == 0)
    {
        // printf("%s(systick) : %4d\r\n",__FUNCTION__,LOG2++);    
    }

    if (is_flag_set(flag_uart_rx_rcv_timeout_10000ms))
    {
        set_flag(flag_uart_rx_rcv_timeout_10000ms ,DISABLE);
        fmt_err(error_timeout);     
    }

    fmt_parsing();

    if (is_flag_set(flag_timer_period_1000ms))
    {
        set_flag(flag_timer_period_1000ms ,DISABLE);

        // printf("%s(timer) : %4d\r\n",__FUNCTION__,LOG1++);
        PB14 ^= 1;        
    }
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

    #if 1
    rx_rcv_irq(res);
    #else
	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1':
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
    #endif
}

void UART02_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
    UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
    NVIC_EnableIRQ(UART02_IRQn);
	
	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	
	#endif	

}

void GPIO_Init (void)
{
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB14MFP_Msk)) | (SYS_GPB_MFPH_PB14MFP_GPIO);
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB15MFP_Msk)) | (SYS_GPB_MFPH_PB15MFP_GPIO);

    GPIO_SetMode(PB, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PB, BIT15, GPIO_MODE_OUTPUT);	

}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);
    
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);	

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);	

    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    CLK_EnableModuleClock(TMR0_MODULE);
  	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, 0);

    CLK_EnableModuleClock(TMR1_MODULE);
  	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);

    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk)) |
                    (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

   /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M031 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	GPIO_Init();
	UART0_Init();
 	TIMER0_Init();   
	TIMER1_Init();

    SysTick_enable(1000);
    #if defined (ENABLE_TICK_EVENT)
    TickSetTickEvent(1000, TickCallback_processA);  // 1000 ms
    TickSetTickEvent(5000, TickCallback_processB);  // 5000 ms
    #endif

    // CRC_CAL_SAMPLE();

    /* Got no where to go, just loop forever */
    while(1)
    {
        loop();

    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
