/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 20/05/29 3:27p $
 * @brief
 *           Show how a slave receives data from a master in GC (General Call) mode.
 *           This sample code needs to work with I2C_GCMode_Master.
 * @note
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include "NuMicro.h"


extern void I2C0_Init(void);
extern void I2C1_Init(void);
extern void MCU_sync_timer_start (void);
extern volatile uint8_t g_buff0[256];
extern void I2C_Master_Start(uint8_t u8Act, uint8_t u8Slave, uint8_t u8Offset, uint8_t u8Len);

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/


void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable HIRC clock (Internal RC 48MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable Timer0 clock */
    CLK_EnableModuleClock(TMR0_MODULE);

    /* Select Timer0 clock source */
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, 0);

    /* Enable I2C0 clock */
    CLK_EnableModuleClock(I2C0_MODULE);

    /* Enable I2C1 clock */
    CLK_EnableModuleClock(I2C1_MODULE);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and cyclesPerUs automatically. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set PF multi-function pins for I2C0_SDA(PF.2), I2C0_SCL(PF.3) slave*/
    SYS->GPF_MFPL = (SYS->GPF_MFPL & ~(SYS_GPF_MFPL_PF2MFP_Msk | SYS_GPF_MFPL_PF3MFP_Msk)) |
                    (SYS_GPF_MFPL_PF2MFP_I2C0_SDA | SYS_GPF_MFPL_PF3MFP_I2C0_SCL);

    /* Set PA multi-function pins for I2C1_SDA(PA.2), I2C1_SCL(PA.3) master*/
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA3MFP_Msk)) |
                    (SYS_GPA_MFPL_PA2MFP_I2C1_SDA | SYS_GPA_MFPL_PA3MFP_I2C1_SCL);

    /* Lock protected registers */
    SYS_LockReg();
}

int main()
{
    SYS_Init();

    /* Init I2C0 */
    I2C0_Init();
    I2C1_Init();

    MCU_sync_timer_start();

    g_buff0[0x1F] = 0x92;
    g_buff0[0x20] = 255;
    g_buff0[0x21] = 1;
    g_buff0[0x10] = 0;
    g_buff0[0x11] = 0x3F;

    // GPIO Output
    // PIN9 PA.1 TXDIS_CDR default:1
    // PIN16 PB.8 HIPWR_EN default:0
    // PIN17 PB.7 INTL default:1
    // PIN23 PC.1 RESET_TOSA default:1
    GPIO_SetMode(PA, BIT1, GPIO_MODE_OUTPUT);
    PA1 = 1;
    GPIO_SetMode(PB, BIT7|BIT8, GPIO_MODE_OUTPUT);
    PB7 = 1;
    PB8 = 0;
    GPIO_SetMode(PC, BIT1, GPIO_MODE_OUTPUT);
    PC1 = 1;

    while(1)
    {
        g_buff0[0xF]++;
        if(g_buff0[0xE] > 3)
        {
            g_buff0[0xE] = 0;
            g_buff0[0xD] = 0;
            g_buff0[0xC]++;

            switch (g_buff0[0])
            {
            case 1:
                I2C_Master_Start(1, g_buff0[0x1F], g_buff0[0x10], g_buff0[0x11]);
                break;
            case 2:
                I2C_Master_Start(0, g_buff0[0x1F], g_buff0[0x20], g_buff0[0x21]);
                break;
            case 3:
                // 1 HIPWR_EN PB.8 => 1
                PB8 = 1;
                CLK_SysTickDelay(100000);
                // 2 RESET_TOSA PC.1 => 0
                PC1 = 0;
                CLK_SysTickDelay(100000);
                // 3 RESET_TOSA PC.1 => 1
                PC1 = 1;
                CLK_SysTickDelay(100000);
                // 4 TXDIS_CDR PA.1 => 0
                PA1 = 0;
                break;
            case 4:
                // 1 TXDIS_CDR PA.1 => 1
                PA1 = 1;
                CLK_SysTickDelay(10000);
                // 2 HIPWR_EN PB.8 => 0
                PB8 = 0;
                break;
            default:
                break;
            }
            g_buff0[0] = 0;
        }
    }

}

/*** (C) COPYRIGHT 2020 Nuvoton Technology Corp. ***/
