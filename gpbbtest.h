#ifndef GPBBTEST_H
#define GPBBTEST_H
#include "novenatest.h"

class GPBBTest : public NovenaTest
{
public:
    GPBBTest();
    void runTest();

    enum eimType {

        fpga_w_test0 = 0x40000,
        fpga_w_test1 = 0x40002,

        fpga_w_cpu_to_dut = 0x40010,
        fpga_w_gpbb_ctl = 0x40012,

        fpga_r_test0 = 0x41000,
        fpga_r_test1 = 0x41002,

        fpga_r_cpu_to_dut = 0x41010,
        fpga_r_gpbb_ctl = 0x41012,

        fpga_r_ddr3_v_major = 0x41ffc,
        fpga_r_ddr3_v_minor = 0x41ffe,

        fpga_romulator_base = 0,
    };

private:
    int readKernelMemory(long offset, int virtualized, int size);
    int writeKernelMemory(long offset, long value, int virtualized, int size);
    int prepEim(void);
    int loadFpga(const QString &bitpath);
    volatile quint16 *eimOpen(unsigned int type);
    quint16 eimGet(unsigned int type);
    quint16 eimSet(unsigned int type, quint16 value);

    volatile quint16 *ranges[8];
    volatile int   *mem_32;
    volatile short *mem_16;
    volatile char  *mem_8;
    int   mem_fd;
    int   fd;
    volatile int   *prev_mem_range;

    quint8 cached_dout;
    quint8 cached_dir;
};

#endif // GPBBTEST_H
