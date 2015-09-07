/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#include <cstdio>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

#include "WinRing0.h"
#include "StringUtils.h"

#include <inttypes.h>

using std::exception;
using std::string;

uint32_t ReadPciConfig(uint32_t device, uint32_t function, uint32_t regAddress) {
    uint32_t result;
    char path[255]= "\0";
    sprintf(path, "/proc/bus/pci/00/%x.%x", device, function);
    fprintf(stdout,"!! Reading: %s ... ", path);

    int pci = open(path, O_RDONLY);
    if (pci == -1) {
        perror("Failed to open pci device for reading");
        exit(-1);
    }
    pread(pci, &result, sizeof(result), regAddress);
    close(pci);
    fprintf(stdout," done.\n");

    return result;
}

void WritePciConfig(uint32_t device, uint32_t function, uint32_t regAddress, uint32_t value) {
    char path[255]= "\0";
    sprintf(path, "/proc/bus/pci/00/%x.%x", device, function);
    fprintf(stdout,"!! Writing: %s dev:%x func:%x regAddr:%x val:%x... ", path, device, function, regAddress, value);

    int pci = open(path, O_WRONLY);
    if (pci == -1) {
        perror("Failed to open pci device for writing");
        exit(-1);
    }
    if(pwrite(pci, &value, sizeof(value), regAddress) != sizeof(value)) {
        perror("Failed to write to pci device");
    }
    close(pci);
    fprintf(stdout," done.\n");
}


uint64_t Rdmsr(uint32_t index) {
    uint64_t result;

    fprintf(stdout,"!! Rdmsr: %x ... ", index);
    int msr = open("/dev/cpu/0/msr", O_RDONLY);
    if (msr == -1) {
        perror("Failed to open msr device for reading");
        exit(-1);
    }
    pread(msr, &result, sizeof(result), index);
    close(msr);
    fprintf(stdout," done.\n");

    return result;
}

int get_num_cpu() {
    CpuidRegs regs = Cpuid(0x80000008);
    return 1 + (regs.ecx&0xff);
}

void Wrmsr(uint32_t index, const uint64_t& value) {
    char path[255]= "\0";

    for (int i = 0; i < get_num_cpu(); i++) {
        sprintf(path, "/dev/cpu/%d/msr", i);
        //fprintf(stdout,"!! Wrmsr: %s idx:%"PRIu32" val:%"PRIu64"\n", path, index, value);
        fprintf(stdout,"!! Wrmsr: %s idx:%x val:%"PRIu64" ... ", path, index, value);
        int msr = open(path, O_WRONLY);
        if (msr == -1) {
            perror("Failed to open msr device for writing");
            exit(-1);
        }
        if(pwrite(msr, &value, sizeof(value), index) != sizeof(value)) {
            perror("Failed to write to msr device");
        }
        close(msr);
        fprintf(stdout," done.\n");
    }
}


CpuidRegs Cpuid(uint32_t index) {
    CpuidRegs result;

    fprintf(stdout,"!! cpuid: /dev/cpu/0/cpuid %x ... ", index);
    FILE* cpuid = fopen("/dev/cpu/0/cpuid", "r");
    if (cpuid == NULL) {
        perror("Failed to open cpuid device for reading");
        exit(-1);
    }
    /*
     * _IOFBF Full buffering: On output, data is written once the buffer is full (or flushed). On Input, the buffer is filled when an input operation is requested and the buffer is empty.
     * _IOLBF Line buffering: On output, data is written when a newline character is inserted into the stream or when the buffer is full (or flushed), whatever happens first. On Input, the buffer is filled up to the next newline character when an input operation is requested and the buffer is empty.
     * _IONBF No buffering: No buffer is used. Each I/O operation is written as soon as possible. In this case, the buffer and size parameters are ignored.
     */
    setvbuf(cpuid, NULL, _IOFBF, 16);//see kernel's: ./arch/x86/kernel/cpuid.c
    fseek(cpuid, index, SEEK_SET);
//    result.eax=0xFFFFFFFF;
//    fprintf(stderr, "!! sizeof = %lu eax=%x\n", sizeof(result.eax), result.eax); //4, I knew it! so, how come this works:
    fread(&(result.eax), sizeof(result.eax), 1, cpuid);//shouldn't it fail since it's not 16 bytes?(4*1=4)
    //count is 1, so if (count % 16) return -EINVAL ... well? see kernel's: ./arch/x86/kernel/cpuid.c
    //it doesn't fail because the buffer is 8192 (if I remember correctly; that's 512*16)
//    fprintf(stderr, "!! after fread, eax=%x\n", result.eax);
    fread(&(result.ebx), sizeof(result.ebx), 1, cpuid);
    fread(&(result.ecx), sizeof(result.ecx), 1, cpuid);
    fread(&(result.edx), sizeof(result.edx), 1, cpuid);
    fclose(cpuid);
    fprintf(stdout," done.\n");

    return result;
}

