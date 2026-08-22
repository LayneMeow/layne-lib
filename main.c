#include <ahci.h>
#include <ata.h>
#include <block.h>
#include <console.h>
#include <interrupts.h>
#include <keyboard.h>
#include <limine.h>
#include <memory.h>
#include <nvme.h>
#include <pci.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ufs.h>

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

static size_t unescape_newlines(char *text) {
    char *read = text;
    char *write = text;

    while (*read != '\0') {
        if (read[0] == '\\' && (read[1] == 'n' || read[1] == 'N')) {
            *write++ = '\n';
            read += 2;
        } else {
            *write++ = *read++;
        }
    }

    *write = '\0';
    return (size_t)(write - text);
}

static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

__attribute__((noreturn))
static void panic(void) {
    printf("\Kettenkrad panic!\n");
    asm volatile ("ud2");
    __builtin_unreachable();
}

static void heading(const char *title) {
    console_set_color(CONSOLE_CYAN, CONSOLE_BLACK);
    printf("[%s]\n", title);
    console_set_color(CONSOLE_WHITE, CONSOLE_BLACK);
}

static void ksetup(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    if (!console_init(framebuffer, 1)) {
        hcf();
    }

    bool real_heap = memory_init();

    bool irqs = interrupts_init();
    bool keyboard = keyboard_init();

    interrupts_enable();

    uint32_t tsc_lo, tsc_hi;
    __asm__ volatile ("rdtsc" : "=a"(tsc_lo), "=d"(tsc_hi));
    srand(tsc_lo ^ tsc_hi);

    console_set_color(CONSOLE_BRIGHT, CONSOLE_BLACK);
}

void kmain(void) {
    ksetup();

    list_disks();
    mount_filesystem();
    heading("Layne-lib");
    console_set_color(CONSOLE_YELLOW, CONSOLE_BLACK);
    printf("\nHello World\n");

    hcf();
}
