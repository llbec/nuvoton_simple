#include "NuMicro.h"

extern volatile uint8_t g_buff0[256];

void TMR0_IRQHandler(void)
{
    //runtime_sync_flag = 1;
    /* clear timer interrupt flag */
    if(g_buff0[0xD] > 0) g_buff0[0xE]++;

    TIMER_ClearIntFlag(TIMER0);
}

/*Timer use for periodic synchronization operations, 0.1ms*/
void MCU_sync_timer_start (void)
{
    /* Set timer frequency to 1/33 = 0.03 s */
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 33);
    TIMER_EnableInt(TIMER0); // Enable timer interrupt
    NVIC_EnableIRQ(TMR0_IRQn);
    TIMER_Start(TIMER0);// Start Timer 0
}
