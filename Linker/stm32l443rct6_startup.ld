/* Entry point */
ENTRY(Reset_Handler)

/* Memory regions definition */
MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 256K   /* 256KB FLASH */
    SRAM (rwx)      : ORIGIN = 0x20000000, LENGTH = 64K    /* 64KB SRAM */
}

/* Sections definition */
SECTIONS
{
    /* Vector table and code section */
    .text :
    {
        KEEP(*(.isr_vector))   /* Vector table */
        *(.text)                /* Program code */
        *(.text*)               /* Program code */
        *(.rodata)              /* Read-only data */
        *(.rodata*)             /* Read-only data */
        . = ALIGN(4);
        _etext = .;             /* End of .text section */
    } >FLASH

    /* Load stack data address */
    _stack_data = LOADADDR(.data);

    /* Initialized data section */
    .data :
    {
        _sdata = .;            /* Start of .data section */
        *(.data)                /* Initialized data */
        *(.data*)               /* Initialized data */
        . = ALIGN(4);
        _edata = .;             /* End of .data section */
    } >SRAM AT>FLASH

    /* Uninitialized data section */
    .bss :
    {
        _sbss = .;             /* Start of .bss section */
        *(.bss)                 /* Uninitialized data */
        *(.bss*)                /* Uninitialized data */
        *(COMMON)               /* Common data */
        . = ALIGN(4);
        _ebss = .;              /* End of .bss section */
        __bss_end__ = _ebss;
    } >SRAM

    /* Stack section */
    .stack_dummy (NOLOAD) :
    {
        . = ALIGN(8);
        *(.stack*)              /* Stack section */
    } >SRAM

    /* Set stack top to end of RAM */
    __StackTop = ORIGIN(SRAM) + LENGTH(SRAM);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    /* Check for RAM overflow */
    ASSERT(__StackLimit >= _ebss, "Error: RAM overflow!")
}