#include "board.h"
#include <string.h>
#include <stdio.h>
#include "utils.h"

//#include "FreeRTOS.h"
//#include "task.h"

uint8_t irq_ringbuf[1024];
uint16_t irq_pendings = 0;
uint16_t irq_pending_ptr = 0;

void bsp_irq_init()
{
    memset(irq_ringbuf, 0xFF, sizeof(irq_ringbuf));
    BF_CLR(ICOLL_CTRL, SFTRST);
    BF_CLR(ICOLL_CTRL, CLKGATE);
    BF_SET(ICOLL_CTRL, SFTRST);
    while (!BF_RD(ICOLL_CTRL, CLKGATE))
    {
        ;
    }
    BF_CLR(ICOLL_CTRL, SFTRST);
    BF_CLR(ICOLL_CTRL, CLKGATE);

    HW_ICOLL_CTRL_CLR(BM_ICOLL_CTRL_BYPASS_FSM | BM_ICOLL_CTRL_NO_NESTING | BM_ICOLL_CTRL_ARM_RSE_MODE);

    HW_ICOLL_CTRL_SET(BM_ICOLL_CTRL_FIQ_FINAL_ENABLE |
                      BM_ICOLL_CTRL_IRQ_FINAL_ENABLE |
                      //BM_ICOLL_CTRL_ARM_RSE_MODE |
                      BM_ICOLL_CTRL_NO_NESTING);

    BF_CS1(ICOLL_VBASE, TABLE_ADDRESS, 0);
    BW_ICOLL_CTRL_VECTOR_PITCH(BV_ICOLL_CTRL_VECTOR_PITCH__BY4);

    HW_ICOLL_CTRL.B.RSRVD1 = 0x00;
}

void bsp_enable_irq(uint32_t IRQNum, uint32_t enable)
{
    //if (IRQNum > 63)
     //   return;
    volatile unsigned int *baseAddress = (unsigned int *)HW_ICOLL_PRIORITYn_ADDR((IRQNum / 4));
    if (enable){
        baseAddress[1] = (0x4 << ((IRQNum % 4) * 8));
    } else {
        baseAddress[2] = (0x4 << ((IRQNum % 4) * 8));
    }
}

static void bsp_ack_irq(uint32_t IRQNum)
{
    BF_SETV(ICOLL_VECTOR, IRQVECTOR, IRQNum);
    BF_SETV(ICOLL_LEVELACK, IRQLEVELACK, 
        1 << ((*((volatile unsigned int *)HW_ICOLL_PRIORITYn_ADDR(HW_ICOLL_STAT.B.VECTOR_NUMBER / 4)) >> (8 * (HW_ICOLL_STAT.B.VECTOR_NUMBER % 4))) & 0x03)); 
}

static void set_as_pending_irq(uint32_t IRQNum )
{
    irq_ringbuf[irq_pending_ptr++] = IRQNum;
    irq_pendings++;
    if(irq_pending_ptr >= sizeof(irq_ringbuf)){
        irq_pending_ptr = 0;
    }
}

static void power_irq(uint32_t nirq)
{

    switch (nirq)
    {
    case HW_IRQ_VDD5V:
        HW_POWER_CTRL_CLR(BM_POWER_CTRL_VDD5V_GT_VDDIO_IRQ);
        INFO("USB 5V Connect\\Disconnect\n");
        break;
    case HW_IRQ_VDD18_BRNOUT:
        INFO("VDD 1.8v Brownout\n");
        break;
    case HW_IRQ_VDDD_BRNOUT:
        INFO("VDDD Brownout\n");
        break;
    case HW_IRQ_VDDIO_BRNOUT:
        INFO("VDDIO Brownout\n");
        break;
    case HW_IRQ_BATT_BRNOUT:
        INFO("BAT Brownout\n");
        break;
    case 63:
        INFO("portPowerIRQ\n");
        break;
    }
        HW_POWER_CTRL.B.DC_OK_IRQ = 0;
        HW_POWER_CTRL.B.VBUSVALID_IRQ = 0;
        HW_POWER_CTRL.B.LINREG_OK_IRQ = 0;
        HW_POWER_CTRL.B.POLARITY_LINREG_OK = 0;
        HW_POWER_CTRL.B.POLARITY_PSWITCH = 0;
        HW_POWER_CTRL.B.POLARITY_VBUSVALID = 0;
        HW_POWER_CTRL.B.POLARITY_VDD5V_GT_VDDIO = 0;
        HW_POWER_CTRL.B.VDDA_BO_IRQ = 0;
        HW_POWER_CTRL.B.VDDD_BO_IRQ = 0;
        HW_POWER_CTRL.B.VDDIO_BO_IRQ = 0;
        HW_POWER_CTRL.B.BATT_BO_IRQ = 0;
}

void dcd_int_handler(uint8_t rhport);

void do_irq()
{
    uint32_t IRQNum = BF_RD(ICOLL_VECTOR, IRQVECTOR) ;
    
    switch (IRQNum)
    {
    case HW_IRQ_TIMER0:
        BF_CLRn(TIMROT_TIMCTRLn, 0, IRQ);
        set_as_pending_irq(IRQNum);
        //print_uart0("tick\r\n");
        
        //if( xTaskIncrementTick() != pdFALSE )
		{
			/* Find the highest priority task that is ready to run. */
		//	vTaskSwitchContext();
		}


        break;
    case HW_IRQ_USB_CTRL: 
        dcd_int_handler(0);
        //set_as_pending_irq(IRQNum);

        break;
    case HW_IRQ_VDD5V:
    case HW_IRQ_VDD5V_DROOP:
    case HW_IRQ_VDD18_BRNOUT:
    case HW_IRQ_VDDD_BRNOUT:
    case HW_IRQ_VDDIO_BRNOUT:
    case HW_IRQ_BATT_BRNOUT:

        power_irq(IRQNum);

        break;
    
    case 63:    //??
        break;
    default:
        printf("ERROR:Unknown IRQ:%ld\r\n", IRQNum);
        break;
    }
    bsp_ack_irq(IRQNum);
    //printf("irq:%d\r\n",IRQNum);

}

