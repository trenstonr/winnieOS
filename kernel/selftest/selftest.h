#ifndef SELFTEST_H
#define SELFTEST_H

#include <stdint.h>

void banner(void);
void ok(const char *name, const char *desc);

void selftest_pmm(void);
void selftest_vmm(void);

void status_bar(uint64_t total_frames);

#endif
