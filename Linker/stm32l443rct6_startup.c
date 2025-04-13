#include <stdint.h>

/* Forward declare main function */
int main(void);

/* Memory definitions */
#define SRAM_BASE 0x20000000U
#define SRAM_SIZE (128 * 1024U) /* 128KB */
#define SRAM_END (SRAM_BASE + SRAM_SIZE)

#define SRAM1_BASE SRAM_BASE
#define SRAM1_SIZE (90 * 1024U)
#define SRAM2_BASE (SRAM_BASE + SRAM1_SIZE)
#define SRAM2_SIZE (32 * 1024U)

#define STACK_BASE (SRAM_END)

extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _stack_data;


/* Function prototypes for interrupt handlers */
/* Forward declarations for interrupt handlers */
void Default_Handler(void);
void Reset_Handler(void);
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));
void WWDG_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PVD_PVM_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_TAMP_STAMP_CSS_LSE_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_CH1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_CH2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_CH3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_CH4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_CH5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_CH6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_CH7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC1_2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN1_TX_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN1_RX0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN1_RX1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN1_SCE_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_TIM15_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_UP_TIM16_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_TRG_COM_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C2_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C2_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI15_10_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_ALARM_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SDMMC1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM6_DACUNDER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_CH1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_CH2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_CH3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_CH4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_CH5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DFSDM1_FLT0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DFSDM1_FLT1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void COMP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void LPTIM1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void LPTIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USB_FS_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_CH6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_CH7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void LPUART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void QUADSPI_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C3_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C3_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SAI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWPMI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TSC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void LCD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void AES_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RNG_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FPU_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CRS_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C4_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C4_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

/* Interrupt Vector Table - Contains addresses of all interrupt handlers */
uint32_t vector_table[] __attribute__((section(".isr_vector"))) = {
    /* Stack pointer and core exceptions */
    STACK_BASE,                                /* 0x000: Reserved */
    (uint32_t)Reset_Handler,                   /* 0x004: Reset - Fixed priority -3 */
    (uint32_t)NMI_Handler,                     /* 0x008: NMI - Fixed priority -2 - RCC Clock Security System */
    (uint32_t)HardFault_Handler,               /* 0x00C: Hard fault - Fixed priority -1 */
    (uint32_t)MemManage_Handler,               /* 0x010: Memory management - Priority 0 */
    (uint32_t)BusFault_Handler,                /* 0x014: Bus fault - Priority 1 */
    (uint32_t)UsageFault_Handler,              /* 0x018: Usage fault - Priority 2 */
    0,                                         /* 0x01C: Reserved */
    0,                                         /* 0x020: Reserved */
    0,                                         /* 0x024: Reserved */
    0,                                         /* 0x028: Reserved */
    (uint32_t)SVC_Handler,                     /* 0x02C: SVCall - Priority 3 */
    (uint32_t)DebugMon_Handler,                /* 0x030: Debug monitor - Priority 4 */
    0,                                         /* 0x034: Reserved */
    (uint32_t)PendSV_Handler,                  /* 0x038: PendSV - Priority 5 */
    (uint32_t)SysTick_Handler,                 /* 0x03C: SysTick - Priority 6 */
    
    /* External interrupts - Priority 7 and above */
    (uint32_t)WWDG_IRQHandler,                 /* 0x040: Window Watchdog interrupt - Priority 7 */
    (uint32_t)PVD_PVM_IRQHandler,              /* 0x044: PVD/PVM1/PVM2/PVM3/PVM4 through EXTI lines 16/35/36/37/38 - Priority 8 */
    (uint32_t)RTC_TAMP_STAMP_CSS_LSE_IRQHandler,/* 0x048: RTC Tamper/TimeStamp/CSS on LSE through EXTI line 19 - Priority 9 */
    (uint32_t)RTC_WKUP_IRQHandler,             /* 0x04C: RTC Wakeup timer through EXTI line 20 - Priority 10 */
    (uint32_t)FLASH_IRQHandler,                /* 0x050: Flash global interrupt - Priority 11 */
    (uint32_t)RCC_IRQHandler,                  /* 0x054: RCC global interrupt - Priority 12 */
    (uint32_t)EXTI0_IRQHandler,                /* 0x058: EXTI Line0 interrupt - Priority 13 */
    (uint32_t)EXTI1_IRQHandler,                /* 0x05C: EXTI Line1 interrupt - Priority 14 */
    (uint32_t)EXTI2_IRQHandler,                /* 0x060: EXTI Line2 interrupt - Priority 15 */
    (uint32_t)EXTI3_IRQHandler,                /* 0x064: EXTI Line3 interrupt - Priority 16 */
    (uint32_t)EXTI4_IRQHandler,                /* 0x068: EXTI Line4 interrupt - Priority 17 */
    (uint32_t)DMA1_CH1_IRQHandler,             /* 0x06C: DMA1 channel 1 interrupt - Priority 18 */
    (uint32_t)DMA1_CH2_IRQHandler,             /* 0x070: DMA1 channel 2 interrupt - Priority 19 */
    (uint32_t)DMA1_CH3_IRQHandler,             /* 0x074: DMA1 channel 3 interrupt - Priority 20 */
    (uint32_t)DMA1_CH4_IRQHandler,             /* 0x078: DMA1 channel 4 interrupt - Priority 21 */
    (uint32_t)DMA1_CH5_IRQHandler,             /* 0x07C: DMA1 channel 5 interrupt - Priority 22 */
    (uint32_t)DMA1_CH6_IRQHandler,             /* 0x080: DMA1 channel 6 interrupt - Priority 23 */
    (uint32_t)DMA1_CH7_IRQHandler,             /* 0x084: DMA1 channel 7 interrupt - Priority 24 */
    (uint32_t)ADC1_2_IRQHandler,               /* 0x088: ADC1 and ADC2 global interrupt - Priority 25 */
    (uint32_t)CAN1_TX_IRQHandler,              /* 0x08C: CAN1_TX interrupts - Priority 26 */
    (uint32_t)CAN1_RX0_IRQHandler,             /* 0x090: CAN1_RX0 interrupts - Priority 27 */
    (uint32_t)CAN1_RX1_IRQHandler,             /* 0x094: CAN1_RX1 interrupt - Priority 28 */
    (uint32_t)CAN1_SCE_IRQHandler,             /* 0x098: CAN1_SCE interrupt - Priority 29 */
    (uint32_t)EXTI9_5_IRQHandler,              /* 0x09C: EXTI Line[9:5] interrupts - Priority 30 */
    (uint32_t)TIM1_BRK_TIM15_IRQHandler,       /* 0x0A0: TIM1 Break/TIM15 global interrupts - Priority 31 */
    (uint32_t)TIM1_UP_TIM16_IRQHandler,        /* 0x0A4: TIM1 Update/TIM16 global interrupts - Priority 32 */
    (uint32_t)TIM1_TRG_COM_IRQHandler,         /* 0x0A8: TIM1 trigger and commutation interrupt - Priority 33 */
    (uint32_t)TIM1_CC_IRQHandler,              /* 0x0AC: TIM1 capture compare interrupt - Priority 34 */
    (uint32_t)TIM2_IRQHandler,                 /* 0x0B0: TIM2 global interrupt - Priority 35 */
    (uint32_t)TIM3_IRQHandler,                 /* 0x0B4: TIM3 global interrupt - Priority 36 */
    0,                                         /* 0x0B8: Reserved - Priority 37 */
    (uint32_t)I2C1_EV_IRQHandler,              /* 0x0BC: I2C1 event interrupt - Priority 38 */
    (uint32_t)I2C1_ER_IRQHandler,              /* 0x0C0: I2C1 error interrupt - Priority 39 */
    (uint32_t)I2C2_EV_IRQHandler,              /* 0x0C4: I2C2 event interrupt - Priority 40 */
    (uint32_t)I2C2_ER_IRQHandler,              /* 0x0C8: I2C2 error interrupt - Priority 41 */
    (uint32_t)SPI1_IRQHandler,                 /* 0x0CC: SPI1 global interrupt - Priority 42 */
    (uint32_t)SPI2_IRQHandler,                 /* 0x0D0: SPI2 global interrupt - Priority 43 */
    (uint32_t)USART1_IRQHandler,               /* 0x0D4: USART1 global interrupt - Priority 44 */
    (uint32_t)USART2_IRQHandler,               /* 0x0D8: USART2 global interrupt - Priority 45 */
    (uint32_t)USART3_IRQHandler,               /* 0x0DC: USART3 global interrupt - Priority 46 */
    (uint32_t)EXTI15_10_IRQHandler,            /* 0x0E0: EXTI Line[15:10] interrupts - Priority 47 */
    (uint32_t)RTC_ALARM_IRQHandler,            /* 0x0E4: RTC alarms through EXTI line 18 interrupts - Priority 48 */
    0,                                         /* 0x0E8: Reserved - Priority 49 */
    0,                                         /* 0x0EC: Reserved - Priority 50 */
    0,                                         /* 0x0F0: Reserved - Priority 51 */
    0,                                         /* 0x0F4: Reserved - Priority 52 */
    0,                                         /* 0x0F8: Reserved - Priority 53 */
    0,                                         /* 0x0FC: Reserved - Priority 54 */
    0,                                         /* 0x100: Reserved - Priority 55 */
    (uint32_t)SDMMC1_IRQHandler,               /* 0x104: SDMMC1 global interrupt - Priority 56 */
    0,                                         /* 0x108: Reserved - Priority 57 */
    (uint32_t)SPI3_IRQHandler,                 /* 0x10C: SPI3 global interrupt - Priority 58 */
    (uint32_t)UART4_IRQHandler,                /* 0x110: UART4 global interrupt - Priority 59 */
    0,                                         /* 0x114: Reserved - Priority 60 */
    (uint32_t)TIM6_DACUNDER_IRQHandler,        /* 0x118: TIM6 global and DAC1 underrun interrupts - Priority 61 */
    (uint32_t)TIM7_IRQHandler,                 /* 0x11C: TIM7 global interrupt - Priority 62 */
    (uint32_t)DMA2_CH1_IRQHandler,             /* 0x120: DMA2 channel 1 interrupt - Priority 63 */
    (uint32_t)DMA2_CH2_IRQHandler,             /* 0x124: DMA2 channel 2 interrupt - Priority 64 */
    (uint32_t)DMA2_CH3_IRQHandler,             /* 0x128: DMA2 channel 3 interrupt - Priority 65 */
    (uint32_t)DMA2_CH4_IRQHandler,             /* 0x12C: DMA2 channel 4 interrupt - Priority 66 */
    (uint32_t)DMA2_CH5_IRQHandler,             /* 0x130: DMA2 channel 5 interrupt - Priority 67 */
    (uint32_t)DFSDM1_FLT0_IRQHandler,          /* 0x134: DFSDM1_FLT0 global interrupt - Priority 68 */
    (uint32_t)DFSDM1_FLT1_IRQHandler,          /* 0x138: DFSDM1_FLT1 global interrupt - Priority 69 */
    0,                                         /* 0x13C: Reserved - Priority 70 */
    (uint32_t)COMP_IRQHandler,                 /* 0x140: COMP1/COMP2 through EXTI lines 21/22 interrupts - Priority 71 */
    (uint32_t)LPTIM1_IRQHandler,               /* 0x144: LPTIM1 global interrupt - Priority 72 */
    (uint32_t)LPTIM2_IRQHandler,               /* 0x148: LPTIM2 global interrupt - Priority 73 */
    (uint32_t)USB_FS_IRQHandler,               /* 0x14C: USB event through EXTI line 17 - Priority 74 */
    (uint32_t)DMA2_CH6_IRQHandler,             /* 0x150: DMA2 channel 6 interrupt - Priority 75 */
    (uint32_t)DMA2_CH7_IRQHandler,             /* 0x154: DMA2 channel 7 interrupt - Priority 76 */
    (uint32_t)LPUART1_IRQHandler,              /* 0x158: LPUART1 global interrupt - Priority 77 */
    (uint32_t)QUADSPI_IRQHandler,              /* 0x15C: QUADSPI global interrupt - Priority 78 */
    (uint32_t)I2C3_EV_IRQHandler,              /* 0x160: I2C3 event interrupt - Priority 79 */
    (uint32_t)I2C3_ER_IRQHandler,              /* 0x164: I2C3 error interrupt - Priority 80 */
    (uint32_t)SAI1_IRQHandler,                 /* 0x168: SAI1 global interrupt - Priority 81 */
    0,                                         /* 0x16C: Reserved - Priority 82 */
    (uint32_t)SWPMI1_IRQHandler,               /* 0x170: SWPMI1 global interrupt - Priority 83 */
    (uint32_t)TSC_IRQHandler,                  /* 0x174: TSC global interrupt - Priority 84 */
    (uint32_t)LCD_IRQHandler,                  /* 0x178: LCD global interrupt - Priority 85 */
    (uint32_t)AES_IRQHandler,                  /* 0x17C: AES global interrupt - Priority 86 */
    (uint32_t)RNG_IRQHandler,                  /* 0x180: RNG global interrupt - Priority 87 */
    (uint32_t)FPU_IRQHandler,                  /* 0x184: FPU global interrupt - Priority 88 */
    (uint32_t)CRS_IRQHandler,                  /* 0x188: CRS interrupt - Priority 89 */
    (uint32_t)I2C4_EV_IRQHandler,              /* 0x18C: I2C4 event interrupt - Priority 90 */
    (uint32_t)I2C4_ER_IRQHandler              /* 0x190: I2C4 error interrupt - Priority 91 */
};

void Default_Handler(void)
{
    while(1); 
}

void Reset_Handler(void)
{
    /* Copy data from flash to RAM */
    uint32_t data_size = (uint32_t)&_edata - (uint32_t)&_sdata;
    uint8_t *pDest = (uint8_t *)&_sdata;
    uint8_t *pSrc = (uint8_t *)&_stack_data;
    for (uint32_t i = 0; i < data_size; i++) {
        *pDest++ = *pSrc++;
    }

    /* Clear BSS section */
    uint32_t bss_size = (uint32_t)&_ebss - (uint32_t)&_sbss;
    pDest = (uint8_t *)&_sbss;
    for (uint32_t i = 0; i < bss_size; i++) {
        *pDest++ = 0;
    }

    /* Set stack pointer */
    // __set_MSP(*(uint32_t *)SRAM_BASE);

    /* Jump to main */
    main();
}

