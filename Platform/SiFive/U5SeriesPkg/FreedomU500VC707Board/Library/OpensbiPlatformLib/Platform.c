/*
 *
 * Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/sifive-uart.h>
#include <sbi_utils/sys/clint.h>
#include <U5Clint.h>

#define U500_HART_COUNT          FixedPcdGet32(PcdHartCount)
#define U500_BOOTABLE_HART_COUNT FixedPcdGet32(PcdBootableHartNumber)
#define U500_HART_STACK_SIZE     FixedPcdGet32(PcdOpenSbiStackSize)
#define U500_BOOT_HART_ID        FixedPcdGet32(PcdBootHartId)

#define U500_SYS_CLK             FixedPcdGet32(PcdU5PlatformSystemClock)

#define U500_PLIC_ADDR              0xc000000
#define U500_PLIC_NUM_SOURCES       0x35
#define U500_PLIC_NUM_PRIORITIES    7

#define U500_UART_ADDR              FixedPcdGet32(PcdU5UartBase)

#define U500_UART_BAUDRATE          115200

/* PRCI clock related macros */
//TODO: Do we need a separate driver for this ?
#define U500_PRCI_BASE_ADDR                 0x10000000
#define U500_PRCI_CLKMUXSTATUSREG           0x002C
#define U500_PRCI_CLKMUX_STATUS_TLCLKSEL    (0x1 << 1)

/* Full tlb flush always */
#define U500_TLB_RANGE_FLUSH_LIMIT		0

unsigned long log2roundup(unsigned long x);

static struct plic_data plic = {
    .addr = U500_PLIC_ADDR,
    .num_src = U500_PLIC_NUM_SOURCES,
};

static struct clint_data clint = {
    .addr = CLINT_REG_BASE_ADDR,
    .first_hartid = 0,
    .hart_count = U500_HART_COUNT,
    .has_64bit_mmio = TRUE,
};

static void U500_modify_dt(void *fdt)
{
    u32 i, size;
    int chosen_offset, err;
    int cpu_offset;
    char cpu_node[32] = "";
    const char *mmu_type;

    for (i = 0; i < U500_HART_COUNT; i++) {
        sbi_sprintf(cpu_node, "/cpus/cpu@%d", i);
        cpu_offset = fdt_path_offset(fdt, cpu_node);
        mmu_type = fdt_getprop(fdt, cpu_offset, "mmu-type", NULL);
        if (mmu_type && (!AsciiStrCmp(mmu_type, "riscv,sv39") ||
            !AsciiStrCmp(mmu_type,"riscv,sv48")))
            continue;
        else
            fdt_setprop_string(fdt, cpu_offset, "status", "masked");
        memset(cpu_node, 0, sizeof(cpu_node));
    }
    size = fdt_totalsize(fdt);
    err = fdt_open_into(fdt, fdt, size + 256);
    if (err < 0)
        sbi_printf("Device Tree can't be expanded to accmodate new node");

    chosen_offset = fdt_path_offset(fdt, "/chosen");
    fdt_setprop_string(fdt, chosen_offset, "stdout-path",
               "/soc/serial@10010000:115200");

    fdt_plic_fixup(fdt, "riscv,plic0");
}

static int U500_final_init(bool cold_boot)
{
    void *fdt;
    struct sbi_scratch *ThisScratch;

    if (!cold_boot)
        return 0;

    fdt = sbi_scratch_thishart_arg1_ptr();
    U500_modify_dt(fdt);
    //
    // Set PMP of firmware regions to R and X. We will lock this in the end of PEI.
    // This region only protects SEC, PEI and Scratch buffer.
    //
    ThisScratch = sbi_scratch_thishart_ptr ();
    pmp_set(0, PMP_R | PMP_X | PMP_W, ThisScratch->fw_start, log2roundup (ThisScratch->fw_size));
    return 0;
}

static int U500_console_init(void)
{
    unsigned long peri_in_freq;

    peri_in_freq = U500_SYS_CLK/2;
    return sifive_uart_init(U500_UART_ADDR, peri_in_freq, U500_UART_BAUDRATE);
}

static int U500_irqchip_init(bool cold_boot)
{
    int rc;
    u32 hartid = current_hartid();

    if (cold_boot) {
        rc = plic_cold_irqchip_init(&plic);
        if (rc)
            return rc;
    }

    return plic_warm_irqchip_init(&plic,
            (hartid) ? (2 * hartid - 1) : 0,
            (hartid) ? (2 * hartid) : -1);
}

static int U500_ipi_init(bool cold_boot)
{
    int rc;

    if (cold_boot) {
        rc = clint_cold_ipi_init(&clint);
        if (rc)
            return rc;

    }

    return clint_warm_ipi_init();
}

static u64 U500_get_tlbr_flush_limit(void)
{
    return U500_TLB_RANGE_FLUSH_LIMIT;
}

static int U500_timer_init(bool cold_boot)
{
    int rc;

    if (cold_boot) {
        rc = clint_cold_timer_init(&clint, NULL);
        if (rc)
            return rc;
    }

    return clint_warm_timer_init();
}
/**
 * The U500 SoC has 4 HARTs, Boot HART ID is determined by
 * PcdBootHartId.
 */
static u32 u500_hart_index2id[U500_BOOTABLE_HART_COUNT] = {0, 1, 2, 3};

static void U500_system_reset(u32 type, u32 second_param)
{
    /* For now nothing to do. */
}

const struct sbi_platform_operations platform_ops = {
    .final_init = U500_final_init,
    .console_putc = sifive_uart_putc,
    .console_getc = sifive_uart_getc,
    .console_init = U500_console_init,
    .irqchip_init = U500_irqchip_init,
    .ipi_send = clint_ipi_send,
    .ipi_clear = clint_ipi_clear,
    .ipi_init = U500_ipi_init,
    .get_tlbr_flush_limit = U500_get_tlbr_flush_limit,
    .timer_value = clint_timer_value,
    .timer_event_stop = clint_timer_event_stop,
    .timer_event_start = clint_timer_event_start,
    .timer_init = U500_timer_init,
    .system_reset = U500_system_reset
};

const struct sbi_platform platform = {
    .opensbi_version    = OPENSBI_VERSION,                      // The OpenSBI version this platform table is built bassed on.
    .platform_version   = SBI_PLATFORM_VERSION(0x0001, 0x0000), // SBI Platform version 1.0
    .name               = "SiFive Freedom U500",
    .features           = SBI_PLATFORM_DEFAULT_FEATURES,
    .hart_count         = U500_BOOTABLE_HART_COUNT,
    .hart_index2id      = u500_hart_index2id,
    .hart_stack_size    = U500_HART_STACK_SIZE,
    .platform_ops_addr  = (unsigned long)&platform_ops
};
