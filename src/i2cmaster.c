#include "NuMicro.h"

extern volatile uint8_t g_buff1[256];
extern volatile uint8_t g_buff3_mst_ctrl[256];

typedef void (*I2C_FUNC)(uint32_t u32Status);
volatile static I2C_FUNC s_I2C1HandlerFn = NULL;

#define g_MstDeviceAddr g_buff3_mst_ctrl[0]
#define g_MstOffset g_buff3_mst_ctrl[1]
#define g_MstLen g_buff3_mst_ctrl[2]
#define g_MstCount g_buff3_mst_ctrl[3]
#define g_MstEndFlag g_buff3_mst_ctrl[4]
#define g_MstDebugIdx g_buff3_mst_ctrl[5]

void I2C1_IRQHandler(void)
{
    uint32_t u32Status;

    u32Status = I2C_GET_STATUS(I2C1);

    g_buff3_mst_ctrl[g_MstDebugIdx] = u32Status;
    g_MstDebugIdx++;

    if(I2C_GET_TIMEOUT_FLAG(I2C1))
    {
        /* Clear I2C1 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C1);
    }
    else
    {
        if(s_I2C1HandlerFn != NULL)
            s_I2C1HandlerFn(u32Status);
    }
}

void I2C1_Init(void)
{
    /* Open I2C1 module and set bus clock */
    I2C_Open(I2C1, 400000);

    /* Set I2C 4 Slave Addresses */
    I2C_SetSlaveAddr(I2C1, 0, 0x15, 0);   /* Slave Address : 0x15 */
    I2C_SetSlaveAddr(I2C1, 1, 0x35, 0);   /* Slave Address : 0x35 */
    I2C_SetSlaveAddr(I2C1, 2, 0x55, 0);   /* Slave Address : 0x55 */
    I2C_SetSlaveAddr(I2C1, 3, 0x75, 0);   /* Slave Address : 0x75 */

    /* Enable I2C1 interrupt */
    I2C_EnableInt(I2C1);
    NVIC_EnableIRQ(I2C1_IRQn);
}

void I2C1_Close(void)
{
    /* Disable I2C1 interrupt and clear corresponding NVIC bit */
    I2C_DisableInt(I2C1);
    NVIC_DisableIRQ(I2C1_IRQn);

    /* Disable I2C1 and close I2C1 clock */
    I2C_Close(I2C1);
    CLK_DisableModuleClock(I2C1_MODULE);
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C1 Master Tx Callback Function                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_MasterTx(uint32_t u32Status)
{
    if(u32Status == 0x08)                       /* START has been transmitted */
    {
        g_MstCount = 0;
        I2C_SET_DATA(I2C1, g_MstDeviceAddr);    /* Write SLA+W to Register I2CDAT */
        I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);
    }
    else if(u32Status == 0x18)                  /* SLA+W has been transmitted and ACK has been received */
    {
        I2C_SET_DATA(I2C1, g_MstOffset);
        I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);
    }
    else if(u32Status == 0x20)                  /* SLA+W has been transmitted and NACK has been received */
    {
        I2C_STOP(I2C1);
        //I2C_START(I2C1);
        g_MstEndFlag = 1;
    }
    else if(u32Status == 0x28)                  /* DATA has been transmitted and ACK has been received */
    {
        if(g_MstCount < g_MstLen)
        {
            I2C_SET_DATA(I2C1, g_buff1[g_MstOffset + g_MstCount]);
            g_MstCount++;
            I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);
        }
        else
        {
            I2C_SET_CONTROL_REG(I2C1, I2C_CTL_STO_SI);
            g_MstEndFlag = 1;
        }
    }
    else
    {
        /* TO DO */
        I2C_STOP(I2C1);
        g_MstEndFlag = 1;
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C1 Master Rx Callback Function                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_MasterRx(uint32_t u32Status)
{
    if(u32Status == 0x08)                       /* START has been transmitted and prepare SLA+W */
    {
        g_MstCount = 0;
        I2C_SET_DATA(I2C1, g_MstDeviceAddr);    /* Write SLA+W to Register I2CDAT */
        I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);
    }
    else if(u32Status == 0x18)                  /* SLA+W has been transmitted and ACK has been received */
    {
        I2C_SET_DATA(I2C1, g_MstOffset);
        I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);
    }
    else if(u32Status == 0x20)                  /* SLA+W has been transmitted and NACK has been received */
    {
        I2C_STOP(I2C1);
        g_MstEndFlag = 1;
    }
    else if(u32Status == 0x28)                  /* DATA has been transmitted and ACK has been received */
    {
        I2C_SET_CONTROL_REG(I2C1, I2C_CTL_STA_SI); // Send repeat START
    }
    else if(u32Status == 0x10)                  /* Repeat START has been transmitted and prepare SLA+R */
    {
        I2C_SET_DATA(I2C1, (g_MstDeviceAddr + 1));   /* Write SLA+R to Register I2CDAT */
        I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0x40)                  /* SLA+R has been transmitted and ACK has been received */
    {
        I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0x50)                  /* DATA has been received and ACK has been returned */
    {
        g_buff1[g_MstOffset + g_MstCount] = (unsigned char) I2C_GET_DATA(I2C1);
        g_MstCount++;
        if (g_MstCount < g_MstLen) I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI_AA);
        else I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);
    }
    else if(u32Status == 0x58)                  /* DATA has been received and NACK has been returned */
    {
        I2C_SET_CONTROL_REG(I2C1, I2C_CTL_STO_SI);
        g_MstEndFlag = 1;
    }
    else
    {
        /* TO DO */
        I2C_STOP(I2C1);
        g_MstEndFlag = 1;
    }
}

/* action: 0 - write; 1 - read */
void I2C_Master_Start(uint8_t u8Act, uint8_t u8Slave, uint8_t u8Offset, uint8_t u8Len)
{
    uint8_t i;

    g_MstEndFlag = 0;
    g_MstDeviceAddr = u8Slave;
    g_MstOffset = u8Offset;
    g_MstLen = u8Len;

    g_MstDebugIdx = 0x10;
    for (i = 0x10; i < 255; i++)
    {
        g_buff3_mst_ctrl[i] = 0xFF;
    }

    /* I2C1 function to write / read data to slave */
    if (u8Act == 0) s_I2C1HandlerFn = (I2C_FUNC)I2C_MasterTx;
    else s_I2C1HandlerFn = (I2C_FUNC)I2C_MasterRx;

    /* I2C1 as master sends START signal */
    I2C_SET_CONTROL_REG(I2C1, I2C_CTL_STA);

    /* Wait I2C1 Tx Finish */
    while(g_MstEndFlag == 0);

}
