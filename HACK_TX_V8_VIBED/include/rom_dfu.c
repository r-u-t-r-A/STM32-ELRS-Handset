/*
 * Jump from the running firmware into the MCU's ROM USB DFU bootloader,
 * same as holding BOOT0 during reset. Only for boards whose system memory
 * bootloader has USB DFU; they define BOARD_ROM_DFU_ADDR in board.h.
 *
 * Follows ST's "jump to system bootloader from application code" sequence:
 * put peripherals and clocks back to reset state, silence every interrupt,
 * map system memory at 0, then load the ROM's stack pointer and reset vector.
 * The ROM bootloader only leaves on reset / power cycle or dfu-util :leave.
 */

#ifdef BOARD_ROM_DFU_ADDR

static void rom_dfu_jump(void) {
    HAL_DeInit();        /* reset APB/AHB peripherals (UARTs, I2C, ADC, timers) */
    HAL_RCC_DeInit();    /* back to HSI, PLL off; needs SysTick still running */

    __disable_irq();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    for (uint8_t i = 0; i < sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]); i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    /* Nothing is enabled any more; the ROM expects PRIMASK clear */
    __enable_irq();

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

    const uint32_t rom_sp = *(volatile uint32_t *)BOARD_ROM_DFU_ADDR;
    void (*rom_reset)(void) = (void (*)(void))(*(volatile uint32_t *)(BOARD_ROM_DFU_ADDR + 4));
    __set_MSP(rom_sp);
    rom_reset();
    while (1) {
    }
}

#endif
