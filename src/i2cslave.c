#include "NuMicro.h"

typedef void (*I2C_FUNC)(uint32_t u32Status);
volatile static I2C_FUNC s_I2C0HandlerFn = NULL;

volatile uint8_t g_buff0[256];
volatile uint8_t g_buff1[256];
volatile uint8_t g_buff2_slv_ctrl[256];
volatile uint8_t g_buff3_mst_ctrl[256];

#define g_slvCtrlLen g_buff2_slv_ctrl[0]
#define g_slvAddr g_buff2_slv_ctrl[1]
#define g_slvOffset g_buff2_slv_ctrl[2]

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C Rx Callback Function                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_SlaveRx_cb(uint32_t u32Status)
{
	uint8_t u8Data;
    if(u32Status == 0x60)                       /* Own SLA+W has been receive; ACK has been return */
    {
        g_slvCtrlLen = 0;
        g_slvAddr = (unsigned char) I2C_GET_DATA(I2C0);
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0x80)                 /* Previously address with own SLA address
                                                   Data has been received; ACK has been returned*/
    {
        u8Data = (unsigned char) I2C_GET_DATA(I2C0);
        if (0 == g_slvCtrlLen)
        {
            g_slvOffset = u8Data;
            g_slvCtrlLen++;
        }
        else
        {
            switch(g_slvAddr)
            {
            case 0xA0:
                    g_buff0[g_slvOffset] = u8Data;
                    g_buff0[0xD] = 1;
                    break;
            case 0xA2:
                    g_buff1[g_slvOffset] = u8Data;
                    break;
            case 0xA4:
            case 0xA6:
            default:
                    break;
            }
            g_slvOffset++;
        }

        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0xA0)                 /* A STOP or repeated START has been received while still
                                                   addressed as Slave/Receiver*/
    {
        g_slvCtrlLen = 0;
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0xA8)                  /* Own SLA+R has been receive; ACK has been return */
    {
        switch(g_slvAddr)
        {
        case 0xA0:
            I2C_SET_DATA(I2C0, g_buff0[g_slvOffset]);
            break;
        case 0xA2:
            I2C_SET_DATA(I2C0, g_buff1[g_slvOffset]);
            break;
        case 0xA4:
            I2C_SET_DATA(I2C0, g_buff2_slv_ctrl[g_slvOffset]);
            break;
        case 0xA6:
            I2C_SET_DATA(I2C0, g_buff3_mst_ctrl[g_slvOffset]);
            break;
        default:
            I2C_SET_DATA(I2C0, 0xFF);
            break;
        }
        g_slvOffset++;
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0xC0)                 /* Data byte or last data in I2CDAT has been transmitted
                                                   Not ACK has been received */
    {
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0xB8)                   /* Data byte in I2CDAT has been transmitted ACK has been received */
    {
        switch(g_slvAddr)
        {
        case 0xA0:
            I2C_SET_DATA(I2C0, g_buff0[g_slvOffset]);
            break;
        case 0xA2:
            I2C_SET_DATA(I2C0, g_buff1[g_slvOffset]);
            break;
        case 0xA4:
            I2C_SET_DATA(I2C0, g_buff2_slv_ctrl[g_slvOffset]);
            break;
        case 0xA6:
            I2C_SET_DATA(I2C0, g_buff3_mst_ctrl[g_slvOffset]);
            break;
        default:
            I2C_SET_DATA(I2C0, 0xFF);
            break;
        }
        g_slvOffset++;
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0x88)                 /* Previously addressed with own SLA address; NOT ACK has
                                                   been returned */
    {
        g_slvCtrlLen = 0;
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else
    {
        /* TO DO */
        //printf("Status 0x%x is NOT processed\n", u32Status);
    }
}

void I2C0_Init(void)
{
    /* Open I2C module and set bus clock */
    I2C_Open(I2C0, 100000);

    /* Set I2C 4 Slave Addresses */
    I2C_SetSlaveAddr(I2C0, 0, 0xA0, I2C_GCMODE_DISABLE);   /* Slave Address : 0x15 */
    I2C_SetSlaveAddr(I2C0, 1, 0xA2, I2C_GCMODE_DISABLE);   /* Slave Address : 0x35 */
    I2C_SetSlaveAddr(I2C0, 2, 0xA4, I2C_GCMODE_DISABLE);   /* Slave Address : 0x55 */
    I2C_SetSlaveAddr(I2C0, 3, 0xA6, I2C_GCMODE_DISABLE);   /* Slave Address : 0x75 */

    /* Set I2C0 4 Slave Addresses Mask */
    I2C_SetSlaveAddrMask(I2C0, 0, 0xFF);
    I2C_SetSlaveAddrMask(I2C0, 1, 0xFF);
    I2C_SetSlaveAddrMask(I2C0, 2, 0xFF);
    I2C_SetSlaveAddrMask(I2C0, 3, 0xFF);

    /* Enable I2C interrupt */
    I2C_EnableInt(I2C0);
    NVIC_EnableIRQ(I2C0_IRQn);
	
	/* I2C enter no address SLV mode */
    I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
	
	/* I2C function to Slave receive data */
    s_I2C0HandlerFn = I2C_SlaveRx_cb;
}

void I2C0_Close(void)
{
    /* Disable I2C0 interrupt and clear corresponding NVIC bit */
    I2C_DisableInt(I2C0);
    NVIC_DisableIRQ(I2C0_IRQn);

    /* Disable I2C0 and close I2C0 clock */
    I2C_Close(I2C0);
    CLK_DisableModuleClock(I2C0_MODULE);
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C0 IRQ Handler                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void I2C0_IRQHandler(void)
{
    uint32_t u32Status;

    u32Status = I2C_GET_STATUS(I2C0);

    if(I2C_GET_TIMEOUT_FLAG(I2C0))
    {
        /* Clear I2C0 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C0);
    }
    else
    {
        if(s_I2C0HandlerFn != NULL)
            s_I2C0HandlerFn(u32Status);
    }
}
