#include "fpgatest.h"
#include <QFile>
#include <QThread>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>

#define EIM_BASE (0x08000000)
#define EIM_DOUT (0x0010)
#define EIM_DIR (0x0012)
#define EIM_DIN (0x1010)

#define DDR3_SIZE (1024 * 1024 * 1) // in words (4 bytes per word)
#define DDR3_FIFODEPTH 64

#define PULSE_GATE_MASK 0x1000

// Any number of errors is bad.  We'll stop reporting them after
// getting this many errors, and abort the test outright.
#define MAX_ERRORS 30

class SleeperThread : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};


int FpgaTest::readKernelMemory(long offset, int virtualized, int size)
{
    int result;

    volatile int *mem_range = (volatile int *)(offset & ~0xFFFF);
    if( mem_range != prev_mem_range ) {
        prev_mem_range = mem_range;

        if(mem_32)
            munmap((void *)mem_32, 0xFFFF);
        if(mem_fd)
            close(mem_fd);

        if(virtualized) {
            mem_fd = open("/dev/kmem", O_RDWR);
            if( mem_fd < 0 ) {
                testError(QString() + "Couldn't open /dev/mem" + strerror(errno));
                mem_fd = 0;
                return -1;
            }
        }
        else {
            mem_fd = open("/dev/mem", O_RDWR);
            if (mem_fd < 0) {
                testError(QString() + "Couldn't open /dev/mem" + strerror(errno));
                mem_fd = 0;
                return -1;
            }
        }

        mem_32 = (qint32 *)mmap(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, offset&~0xFFFF);
        if (-1 == (int)mem_32) {
            testError(QString() + "Couldn't mmap /dev/kmem: " + strerror(errno));

            if (-1 == close(mem_fd))
                testError(QString() + "Also couldn't close /dev/kmem: " + strerror(errno));

            mem_fd=0;
            return -1;
        }
        mem_16 = (volatile short *)mem_32;
        mem_8  = (volatile char  *)mem_32;
    }

    int scaled_offset = (offset-(offset&~0xFFFF));
    if (size == 1)
        result = mem_8[scaled_offset/sizeof(char)];
    else if (size == 2)
        result = mem_16[scaled_offset/sizeof(short)];
    else
        result = mem_32[scaled_offset/sizeof(long)];

    return result;
}

int FpgaTest::writeKernelMemory(long offset, long value, int virtualized, int size)
{
    int old_value = readKernelMemory(offset, virtualized, size);
    int scaled_offset = (offset-(offset&~0xFFFF));
    if (size == 1)
        mem_8[scaled_offset/sizeof(char)]   = value;
    else if (size == 2)
        mem_16[scaled_offset/sizeof(short)] = value;
    else
        mem_32[scaled_offset/sizeof(long)]  = value;
    return old_value;
}


int FpgaTest::prepEim(void)
{
    int i;

    // set up pads to be mapped to EIM
    for (i = 0; i < 16; i++) {
        writeKernelMemory(0x20e0114 + i*4, 0x0, 0, 4);  // mux mapping
        writeKernelMemory(0x20e0428 + i*4, 0xb0b1, 0, 4); // pad strength config'd for a 100MHz rate 
    }

    // mux mapping
    writeKernelMemory(0x20e046c - 0x314, 0x0, 0, 4); // BCLK
    writeKernelMemory(0x20e040c - 0x314, 0x0, 0, 4); // CS0
    writeKernelMemory(0x20e0410 - 0x314, 0x0, 0, 4); // CS1
    writeKernelMemory(0x20e0414 - 0x314, 0x0, 0, 4); // OE
    writeKernelMemory(0x20e0418 - 0x314, 0x0, 0, 4); // RW
    writeKernelMemory(0x20e041c - 0x314, 0x0, 0, 4); // LBA
    writeKernelMemory(0x20e0468 - 0x314, 0x0, 0, 4); // WAIT
    writeKernelMemory(0x20e0408 - 0x314, 0x0, 0, 4); // A16
    writeKernelMemory(0x20e0404 - 0x314, 0x0, 0, 4); // A17
    writeKernelMemory(0x20e0400 - 0x314, 0x0, 0, 4); // A18

    // pad strength
    writeKernelMemory(0x20e046c, 0xb0b1, 0, 4); // BCLK
    writeKernelMemory(0x20e040c, 0xb0b1, 0, 4); // CS0
    writeKernelMemory(0x20e0410, 0xb0b1, 0, 4); // CS1
    writeKernelMemory(0x20e0414, 0xb0b1, 0, 4); // OE
    writeKernelMemory(0x20e0418, 0xb0b1, 0, 4); // RW
    writeKernelMemory(0x20e041c, 0xb0b1, 0, 4); // LBA
    writeKernelMemory(0x20e0468, 0xb0b1, 0, 4); // WAIT
    writeKernelMemory(0x20e0408, 0xb0b1, 0, 4); // A16
    writeKernelMemory(0x20e0404, 0xb0b1, 0, 4); // A17
    writeKernelMemory(0x20e0400, 0xb0b1, 0, 4); // A18

    writeKernelMemory(0x020c4080, 0xcf3, 0, 4); // ungate eim slow clocks

    // rework timing for sync use
    // 0011 0  001 1   001    0   001 00  00  1  011  1    0   1   1   1   1   1   1
    // PSZ  WP GBC AUS CSREC  SP  DSZ BCS BCD WC BL   CREP CRE RFL WFL MUM SRD SWR CSEN
    //
    // PSZ = 0011  64 words page size
    // WP = 0      (not protected)
    // GBC = 001   min 1 cycles between chip select changes
    // AUS = 0     address shifted according to port size
    // CSREC = 001 min 1 cycles between CS, OE, WE signals
    // SP = 0      no supervisor protect (user mode access allowed)
    // DSZ = 001   16-bit port resides on DATA[15:0]
    // BCS = 00    0 clock delay for burst generation
    // BCD = 00    divide EIM clock by 0 for burst clock
    // WC = 1      write accesses are continuous burst length
    // BL = 011    32 word memory wrap length
    // CREP = 1    non-PSRAM, set to 1
    // CRE = 0     CRE is disabled
    // RFL = 1     fixed latency reads
    // WFL = 1     fixed latency writes
    // MUM = 1     multiplexed mode enabled
    // SRD = 1     synch reads
    // SWR = 1     synch writes
    // CSEN = 1    chip select is enabled

    //  writeKernelMemory( 0x21b8000, 0x5191C0B9, 0, 4 );
    writeKernelMemory(0x21b8000, 0x31910BBF, 0, 4);

    // EIM_CS0GCR2   
    //  MUX16_BYP_GRANT = 1
    //  ADH = 1 (1 cycles)
    //  0x1001
    writeKernelMemory(0x21b8004, 0x1000, 0, 4);


    // EIM_CS0RCR1   
    // 00 000101 0 000   0   000   0 000 0 000 0 000 0 000
    //    RWSC     RADVA RAL RADVN   OEA   OEN   RCSA  RCSN
    // RWSC 000101    5 cycles for reads to happen
    //
    // 0000 0111 0000   0011   0000 0000 0000 0000
    //  0    7     0     3      0  0    0    0
    // 0000 0101 0000   0000   0 000 0 000 0 000 0 000
    writeKernelMemory(0x21b8008, 0x05000000, 0, 4);
    writeKernelMemory(0x21b8008, 0x0A024000, 0, 4);
    writeKernelMemory(0x21b8008, 0x09014000, 0, 4);

    // EIM_CS0RCR2  
    // 0000 0000 0   000 00 00 0 010  0 001 
    //           APR PAT    RL   RBEA   RBEN
    // APR = 0   mandatory because MUM = 1
    // PAT = XXX because APR = 0
    // RL = 00   because async mode
    // RBEA = 000  these match RCSA/RCSN from previous field
    // RBEN = 000
    // 0000 0000 0000 0000 0000  0000
    writeKernelMemory(0x21b800c, 0x00000000, 0, 4);

    // EIM_CS0WCR1
    // 0   0    000100 000   000   000  000  010 000 000  000
    // WAL WBED WWSC   WADVA WADVN WBEA WBEN WEA WEN WCSA WCSN
    // WAL = 0       use WADVN
    // WBED = 0      allow BE during write
    // WWSC = 000100 4 write wait states
    // WADVA = 000   same as RADVA
    // WADVN = 000   this sets WE length to 1 (this value +1)
    // WBEA = 000    same as RBEA
    // WBEN = 000    same as RBEN
    // WEA = 010     2 cycles between beginning of access and WE assertion
    // WEN = 000     1 cycles to end of WE assertion
    // WCSA = 000    cycles to CS assertion
    // WCSN = 000    cycles to CS negation
    // 1000 0111 1110 0001 0001  0100 0101 0001
    // 8     7    E    1    1     4    5    1
    // 0000 0111 0000 0100 0000  1000 0000 0000
    // 0      7    0   4    0     8    0     0
    // 0000 0100 0000 0000 0000  0100 0000 0000
    //  0    4    0    0     0    4     0    0

    writeKernelMemory(0x21b8010, 0x09080800, 0, 4);
    //  writeKernelMemory( 0x21b8010, 0x02040400, 0, 4 );

    // EIM_WCR
    // BCM = 1   free-run BCLK
    // GBCD = 0  don't divide the burst clock
    writeKernelMemory(0x21b8090, 0x701, 0, 4);

    // EIM_WIAR 
    // ACLK_EN = 1
    writeKernelMemory(0x21b8094, 0x10, 0, 4);

    /* Dummy reads to fix a first-byte-invalid bug */
    eimGet(fpga_r_ddr3_v_minor);
    eimGet(fpga_r_ddr3_v_major);
    return 0;
}


volatile quint16 *FpgaTest::eimOpen(unsigned int type)
{
    int range = (type >> 16) & 7;

    if (ranges[range])
        return ((volatile quint16 *) (((volatile quint8 *)ranges[range])+(type&0xffff)));

    fd = open("/dev/mem", O_RDWR);
    if (fd == -1) {
        testError(QString() + "Couldn't open /dev/mem" + strerror(errno));
        return NULL;
    }

    ranges[range] = (volatile quint16 *)mmap(NULL, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, fd, EIM_BASE+(type&0xffff0000));
    if (ranges[range] == ((quint16 *)-1)) {
        testError(QString() + "Couldn't mmap EIM region: " + strerror(errno));
        return NULL;
    }

    return eimOpen(type);
}

quint16 FpgaTest::eimGet(unsigned int type)
{
    return *eimOpen(type);
}

quint16 FpgaTest::eimSet(unsigned int type, quint16 value)
{
    volatile quint16 *ptr = eimOpen(type);
    quint16 old = *ptr;
    *ptr = value;
    return old;
}

int FpgaTest::loadFpga(const QString &bitpath)
{
    QFile exportFile("/sys/class/gpio/export");
    if (!exportFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO export file: " + exportFile.errorString());
        return 1;
    }
    exportFile.write("135");
    exportFile.close();

    QFile directionFile("/sys/class/gpio/gpio135/direction");
    if (!directionFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO135 direction file: " + directionFile.errorString());
        return 1;
    }
    directionFile.write("out\n");
    directionFile.close();

    QFile valueFile("/sys/class/gpio/gpio135/value");
    if (!valueFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO135 value file: " + valueFile.errorString());
        return 1;
    }
    valueFile.write("0\n");
    valueFile.close();

    SleeperThread::msleep(200);

    if (!valueFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO135 value file: " + valueFile.errorString());
        return 1;
    }
    valueFile.write("1\n");
    valueFile.close();
    SleeperThread::msleep(200);

    QFile bitstreamFile(bitpath);
    if (!bitstreamFile.open(QIODevice::ReadOnly)) {
        testError(QString() + "Unable to open bitstream file " + bitpath + ": " + bitstreamFile.errorString());
        return 1;
    }

    QFile spiDev("/dev/spidev2.0");
    if (!spiDev.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open spi dev: " + spiDev.errorString());
        return 1;
    }

    while(1) {
        QByteArray bytes = bitstreamFile.read(128);
        if (bytes.length() == 0)
            break;
        spiDev.write(bytes);
        spiDev.flush();
    }

    bitstreamFile.close();
    spiDev.close();

    return 0;
}


int FpgaTest::ddr3Load(const QByteArray &bytes)
{
    unsigned int *buf;
    unsigned int totalBytes;
    unsigned int burstaddr = 0;
    unsigned int arg = 0;
    int offset;
    int i;

    // clear burst mode
    eimSet(fpga_w_ddr3_p3_cmd, eimGet(fpga_w_ddr3_p3_cmd) & 0x7fff);

    buf = (unsigned int *)bytes.constData();
    totalBytes = bytes.size() / 4;

    testInfo(QString() + "Writing " + QString::number(totalBytes) + " 32-bit words");
    if (totalBytes % 64)
        testDebug("Warning: number of bytes being written is not divisible by 64");


    testDebug("Writing data to RAM");
    // dummy writes to clear any previous data in the queue -- caution, writes to "wherever"!
    while ( !(eimGet(fpga_r_ddr3_p2_stat) & 4)) {
        eimSet(fpga_w_ddr3_p2_cmd, 0x008 | PULSE_GATE_MASK);
        eimSet(fpga_w_ddr3_p2_cmd, 0x000 | PULSE_GATE_MASK);
    }
    // dummy reads to clear any previous data in the queue
    while ( !(eimGet(fpga_r_ddr3_p3_stat) & 4)) {
        eimSet(fpga_w_ddr3_p3_ren, 0x010);
        eimSet(fpga_w_ddr3_p3_ren, 0x000);
    }

    offset = 0;
    burstaddr = 0;
    eimSet(fpga_w_ddr3_p2_ladr + offset, ((burstaddr * 4) & 0xFFFF));
    eimSet(fpga_w_ddr3_p2_hadr + offset, ((burstaddr * 4) >> 16) & 0xFFFF);

    //////////////////// // dummy loop to clear bad config state
    // wait for write queue to be empty
    while (!(eimGet(fpga_r_ddr3_p2_stat) & 4))
        ;

    for (i = 0; i < DDR3_FIFODEPTH; i++) {
        eimSet(fpga_w_ddr3_p2_ldat, (buf[burstaddr + i] & 0xFFFF));
        eimSet(fpga_w_ddr3_p2_hdat, (buf[burstaddr + i] >> 16) & 0xFFFF);
    }

    arg = ((DDR3_FIFODEPTH - 1) << 4);
    eimSet(fpga_w_ddr3_p2_cmd, arg | PULSE_GATE_MASK);
    arg |= 8;
    eimSet(fpga_w_ddr3_p2_cmd, arg | PULSE_GATE_MASK);
    eimSet(fpga_w_ddr3_p2_cmd, 0x000 | PULSE_GATE_MASK);
    burstaddr += DDR3_FIFODEPTH;
    eimSet(fpga_w_ddr3_p2_ladr + offset, ((burstaddr * 4) & 0xFFFF));
    eimSet(fpga_w_ddr3_p2_hadr + offset, ((burstaddr * 4) >> 16) & 0xFFFF);

    // dummy writes to clear any previous data in the queue -- caution, writes to "wherever"!
    while (!(eimGet(fpga_r_ddr3_p2_stat) & 4)) {
        eimSet(fpga_w_ddr3_p2_cmd, 0x008 | PULSE_GATE_MASK);
        eimSet(fpga_w_ddr3_p2_cmd, 0x000 | PULSE_GATE_MASK);
    }
    // dummy reads to clear any previous data in the queue
    while (!(eimGet(fpga_r_ddr3_p3_stat) & 4)) {
        eimSet(fpga_w_ddr3_p3_ren, 0x010);
        eimSet(fpga_w_ddr3_p3_ren, 0x000);
    }

    offset = 0;
    burstaddr = 0;
    eimSet(fpga_w_ddr3_p2_ladr + offset, ((burstaddr * 4) & 0xFFFF));
    eimSet(fpga_w_ddr3_p2_hadr + offset, ((burstaddr * 4) >> 16) & 0xFFFF);
    /////////////////////////// end dummy code

    while (burstaddr < totalBytes) {
        // wait for write queue to be empty
        while (!(eimGet(fpga_r_ddr3_p2_stat) & 4))
            ;

        for (i = 0; i < DDR3_FIFODEPTH; i++) {
            eimSet(fpga_w_ddr3_p2_ldat, (buf[burstaddr + i] & 0xFFFF));
            eimSet(fpga_w_ddr3_p2_hdat, (buf[burstaddr + i] >> 16) & 0xFFFF);
        }
        if ((eimGet(fpga_r_ddr3_p2_stat) >> 8) != DDR3_FIFODEPTH) {
            testDebug("FIFO depth is weird");
            //printf( "z%d\n", cs0[FPGA_MAP(FPGA_R_DDR3_P2_STAT)] >> 8 );
            //putchar('z'); fflush(stdout);
        }
        arg = ((DDR3_FIFODEPTH - 1) << 4);
        eimSet(fpga_w_ddr3_p2_cmd, arg | PULSE_GATE_MASK);
        arg |= 8;
        eimSet(fpga_w_ddr3_p2_cmd, arg | PULSE_GATE_MASK);
        eimSet(fpga_w_ddr3_p2_cmd, 0x000 | PULSE_GATE_MASK);
        burstaddr += DDR3_FIFODEPTH;
        eimSet(fpga_w_ddr3_p2_ladr + offset, ((burstaddr * 4) & 0xFFFF));
        eimSet(fpga_w_ddr3_p2_hadr + offset, ((burstaddr * 4) >> 16) & 0xFFFF);
    }
    return 0;
}

int FpgaTest::ddr3Verify(const QByteArray &bytes)
{
    unsigned int *buf;
    unsigned int totalBytes;
    unsigned int burstaddr = 0;
    unsigned int arg = 0;
    unsigned int rv;
    unsigned int data;
    unsigned int readback[DDR3_FIFODEPTH];
    int offset;
    int i;
    int errorCount = 0;

    // clear burst mode
    eimSet(fpga_w_ddr3_p3_cmd, eimGet(fpga_w_ddr3_p3_cmd) & 0x7fff);

    buf = (unsigned int *)bytes.constData();
    totalBytes = bytes.size() / 4;

    testInfo(QString() + "Verifying " + QString::number(totalBytes) + " 32-bit words");
    if (totalBytes % 64)
        testDebug("Warning: number of bytes being written is not divisible by 64");

    offset = 0x10; // accessing port 3 (read port)
    burstaddr = 0;
    eimSet(fpga_w_ddr3_p2_ladr + offset, ((burstaddr * 4) & 0xFFFF));
    eimSet(fpga_w_ddr3_p2_hadr + offset, ((burstaddr * 4) >> 16) & 0xFFFF);

    while (burstaddr < totalBytes) {
        if ((burstaddr % (1024 * 1024)) == 0) {
            //printf( "." );
            //fflush(stdout);
        }
        arg = ((DDR3_FIFODEPTH - 1) << 4) | 1;
        eimSet(fpga_w_ddr3_p3_cmd, arg | PULSE_GATE_MASK);
        arg |= 0x8;
        eimSet(fpga_w_ddr3_p3_cmd, arg | PULSE_GATE_MASK);
        arg &= ~0x8;
        eimSet(fpga_w_ddr3_p3_cmd, arg | PULSE_GATE_MASK);
        for (i = 0; i < DDR3_FIFODEPTH; i++) {
            while (eimGet(fpga_r_ddr3_p3_stat) & 4)
                ; // wait for queue to become full before reading
            rv = eimGet(fpga_r_ddr3_p3_ldat);
            data = ((unsigned int) rv) & 0xFFFF;
            rv = eimGet(fpga_r_ddr3_p3_hdat);
            data |= (rv << 16);
            readback[i] = data;
        }
        while (!(eimGet(fpga_r_ddr3_p3_stat) & 0x4)) {
            testDebug("Read error: Should be empty now");
            eimSet(fpga_w_ddr3_p3_ren, 0x10);
            eimSet(fpga_w_ddr3_p3_ren, 0x00);
        }
        for (i = 0; i < DDR3_FIFODEPTH; i++) {
            if (buf[burstaddr + i] != readback[i]) {
                testError(QString() + "Value read back was not what we wrote. " 
                        + "Expected 0x" + QString::number(buf[burstaddr + i], 16) 
                        + ", got 0x" + QString::number(readback[i], 16)
                        + " at offset 0x" + QString::number((burstaddr + i) * 4));
                errorCount++;

                if (errorCount > MAX_ERRORS) {
                    testError("Aborting test because too many errors were encountered");
                    return 1;
                }
            }
        }
        burstaddr += DDR3_FIFODEPTH;
        eimSet(fpga_w_ddr3_p2_ladr + offset, ((burstaddr * 4) & 0xFFFF));
        eimSet(fpga_w_ddr3_p2_hadr + offset, ((burstaddr * 4) >> 16) & 0xFFFF);
    }

    return errorCount;
}


FpgaTest::FpgaTest()
{
    name = "FPGA Test";
    memset(ranges, 0, sizeof(ranges));
    mem_32 = 0;
    mem_16 = 0;
    mem_8 = 0;
    mem_fd = 0;
    fd = 0;
    prev_mem_range = 0;

    cached_dout = 0;
    cached_dir = 0;
}

void FpgaTest::runTest()
{
    testInfo("Loading FPGA firmware");

    bool success = false;
    int tries;
    quint16 ver_major;
    quint16 ver_minor;

    for (tries = 0; tries < 50; tries++) {
        if (loadFpga("/factory/novena_fpga-1.22.bit"))
            return;

        errno = 0;
        testDebug("Turning on clock to FPGA");
        writeKernelMemory(0x020c8160, 0x00000D2B, 0, 4);
        if (errno)
            return;

        if (prepEim())
            return;

        ver_major = eimGet(fpga_r_ddr3_v_major);
        ver_minor = eimGet(fpga_r_ddr3_v_minor);

        if ((ver_major != 0) && (ver_minor != 0) && (ver_major != 65535) && (ver_minor != 65535)) {
            success = true;
            break;
        }
    }

    testDebug(QString() + "FPGA Firmware version "
                        + QString::number(ver_major) + "."
                        + QString::number(ver_minor));

    if (!success) {
        testError("Unrecognized version number");
        return;
    }

    /* Fill an array of values from 0 to 256, to use as test data */
    QByteArray dataToTest;
    QByteArray initial;
    for (int i = 0; i < 256; i++)
        initial.append(i);
    for (int i = 0; i < 8192; i++)
        dataToTest.append(initial);
    testDebug(QString() + "Test data is " + QString::number(dataToTest.size()) + " bytes long");

    if (ddr3Load(dataToTest))
        return;

    if (ddr3Verify(dataToTest))
        return;
}

