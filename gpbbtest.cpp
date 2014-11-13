#include "gpbbtest.h"
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

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define EIM_BASE (0x08000000)
#define EIM_DOUT (0x0010)
#define EIM_DIR (0x0012)
#define EIM_DIN (0x1010)

#define FPGA_I2C_LOOPBACK 0x0
#define FPGA_I2C_ADC_CTL 0x2
#define FPGA_I2C_ADC_DAT_L 0x40
#define FPGA_I2C_ADC_DAT_H 0x41
#define FPGA_I2C_ADC_VALID 0x42
#define FPGA_I2C_V_MIN_L 0xfc
#define FPGA_I2C_V_MIN_H 0xfd
#define FPGA_I2C_V_MAJ_L 0xfe
#define FPGA_I2C_V_MAJ_H 0xff

#define ADC108S022_I2C_ADR (0x3c >> 1) // actually, the whole FPGA sits here
#define DAC101C085_A_I2C_ADR (0x14 >> 1)
#define DAC101C085_B_I2C_ADR (0x12 >> 1)

enum DACenum { DAC_B = 1, DAC_A = 0 };
typedef enum DACenum dacType;

static int adc108s022_write_byte(unsigned char adr, unsigned char data) {
    int i2cfd;
    char i2cbuf[2];
    int slave_address = -1;
    struct i2c_msg msg[2];
    struct i2c_ioctl_rdwr_data {
        struct i2c_msg *msgs; /* ptr to array of simple messages */
        int nmsgs; /* number of messages to exchange */
    } msgst;
    slave_address = ADC108S022_I2C_ADR;
    i2cfd = open("/dev/i2c-2", O_RDWR);
    if( i2cfd < 0 ) {
        perror("Unable to open /dev/i2c-2\n");
        i2cfd = 0;
        return 1;
    }
    if( ioctl( i2cfd, I2C_SLAVE, slave_address) < 0 ) {
        perror("Unable to set I2C slave device\n" );
        printf( "Address: %02x\n", slave_address );
        close( i2cfd );
        return 1;
    }
    i2cbuf[0] = adr; i2cbuf[1] = data;
    // set address for read
    msg[0].addr = slave_address;
    msg[0].flags = 0; // no flag means do a write
    msg[0].len = 2;
    msg[0].buf = i2cbuf;
    #ifdef DEBUG
    dump(i2cbuf, 2);
    #endif
    msgst.msgs = msg;
    msgst.nmsgs = 1;
    if (ioctl(i2cfd, I2C_RDWR, &msgst) < 0) {
        perror("Transaction failed\n" );
        close( i2cfd );
        return -1;
    }
    close( i2cfd );
    return 0;
}

int adc108s022_read_byte(unsigned char adr, unsigned char *data) {
    int i2cfd;
    int slave_address = -1;
    struct i2c_msg msg[2];
    struct i2c_ioctl_rdwr_data {
        struct i2c_msg *msgs; /* ptr to array of simple messages */
        int nmsgs; /* number of messages to exchange */
    } msgst;
    slave_address = ADC108S022_I2C_ADR;
    i2cfd = open("/dev/i2c-2", O_RDWR);
    if( i2cfd < 0 ) {
        perror("Unable to open /dev/i2c-2\n");
        i2cfd = 0;
        return -1;
    }
    if( ioctl( i2cfd, I2C_SLAVE, slave_address) < 0 ) {
        perror("Unable to set I2C slave device\n" );
        printf( "Address: %02x\n", slave_address );
        close( i2cfd );
        return -1;
    }
    // set write address
    msg[0].addr = slave_address;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = (char *) &adr;
    // set readback buffer
    msg[1].addr = slave_address;
    msg[1].flags = I2C_M_NOSTART | I2C_M_RD;
    // msg[1].flags = I2C_M_RD;
    msg[1].len = 1;
    msg[1].buf = (char *) data;
    msgst.msgs = msg;
    msgst.nmsgs = 2;
    if (ioctl(i2cfd, I2C_RDWR, &msgst) < 0){
        perror("Transaction failed\n" );
        close( i2cfd );
        return -1;
    }
    close( i2cfd );
    return 0;
}

static unsigned int adc_read(void)
{
    unsigned char chan;
    unsigned char valid = 0;
    unsigned char data;
    unsigned int retval;
    adc108s022_read_byte( FPGA_I2C_ADC_CTL, &chan );
    adc108s022_write_byte( FPGA_I2C_ADC_CTL, chan | 0x8); // initiate conversion
    while (!valid)
        adc108s022_read_byte( FPGA_I2C_ADC_VALID, &valid);

    adc108s022_read_byte(FPGA_I2C_ADC_DAT_L, &data);
    retval = data;
    adc108s022_read_byte(FPGA_I2C_ADC_DAT_H, &data);
    retval |= (data << 8);
    adc108s022_write_byte(FPGA_I2C_ADC_CTL, chan); // clear initiation bit
    return retval;
}

static void adc_chan(unsigned int chan)
{
    unsigned char data;
    unsigned char chan_change = 0;
    if (chan > 0x7) {
        printf( "Warning: channel out of range; aborting.\n" );
        return;
    }
    adc108s022_read_byte(FPGA_I2C_ADC_CTL, &data);
    if ((data & 7) != (chan & 7))
        chan_change = 1;

    adc108s022_write_byte(FPGA_I2C_ADC_CTL, (unsigned char) (chan & 0x7));
    if (chan_change)
        adc_read(); // throw away result
}

static qint32 adc_read_chan(unsigned int chan)
{
    adc_chan(chan);
    return adc_read();
}


static int dac101c085_write_byte(unsigned short data, dacType dac) {
    int i2cfd;
    char i2cbuf[2];
    int slave_address = -1;
    struct i2c_msg msg[2];
    struct i2c_ioctl_rdwr_data {
        struct i2c_msg *msgs; /* ptr to array of simple messages */
        int nmsgs; /* number of messages to exchange */
    } msgst;

    if (dac == DAC_A)
        slave_address = DAC101C085_A_I2C_ADR;
    else if (dac == DAC_B)
        slave_address = DAC101C085_B_I2C_ADR;
    else
        slave_address = -1;

    i2cfd = open("/dev/i2c-1", O_RDWR);

    if (i2cfd < 0) {
        perror("Unable to open /dev/i2c-1\n");
        i2cfd = 0;
        return 1;
    }

    if (ioctl( i2cfd, I2C_SLAVE, slave_address) < 0) {
        perror("Unable to set I2C slave device\n");
        printf("Address: %02x\n", slave_address);
        close(i2cfd);
        return 1;
    }
    i2cbuf[0] = ((data & 0xFF00) >> 8); i2cbuf[1] = (data & 0xFF);

    // set address for read
    msg[0].addr = slave_address;
    msg[0].flags = 0; // no flag means do a write
    msg[0].len = 2;
    msg[0].buf = i2cbuf;

    msgst.msgs = msg;
    msgst.nmsgs = 1;
    if (ioctl(i2cfd, I2C_RDWR, &msgst) < 0) {
        perror("Transaction failed\n");
        close(i2cfd);
        return -1;
    }
    close(i2cfd);
    return 0;
}

static int dac101c085_read_byte(unsigned short *data, dacType dac) {
    int i2cfd;
    int slave_address = -1;
    struct i2c_msg msg[2];
    struct i2c_ioctl_rdwr_data {
        struct i2c_msg *msgs; /* ptr to array of simple messages */
        int nmsgs; /* number of messages to exchange */
    } msgst;

    if (dac == DAC_A)
        slave_address = DAC101C085_A_I2C_ADR;
    else if (dac == DAC_B)
        slave_address = DAC101C085_B_I2C_ADR;
    else
        slave_address = -1;

    i2cfd = open("/dev/i2c-1", O_RDWR);
    if (i2cfd < 0) {
        perror("Unable to open /dev/i2c-1\n");
        i2cfd = 0;
        return -1;
    }

    if (ioctl(i2cfd, I2C_SLAVE, slave_address) < 0) {
        perror("Unable to set I2C slave device\n");
        printf("Address: %02x\n", slave_address);
        close(i2cfd);
        return -1;
    }

    // set readback buffer
    msg[0].addr = slave_address;
    msg[0].flags = I2C_M_NOSTART | I2C_M_RD;
    msg[0].len = 2;
    msg[0].buf = (char *) data;
    msgst.msgs = msg;
    msgst.nmsgs = 1;
    if (ioctl(i2cfd, I2C_RDWR, &msgst) < 0) {
        perror("Transaction failed\n");
        close(i2cfd);
        return -1;
    }
    close(i2cfd);
    return 0;
}

static void dac_a_set(unsigned int code) {
    code *= 4; // Two dummy bits
    if (code > 0xFFF) {
        printf( "Warning: code larger than 0xFFF; truncating.\n" );
        code = 0xFFF;
    }
    dac101c085_write_byte((unsigned short) (code & 0xFFFF), DAC_A);
}

static void dac_b_set(unsigned int code) {
    code *= 4; // Two dummy bits
    if (code > 0xFFF) {
        printf( "Warning: code larger than 0xFFF; truncating.\n" );
        code = 0xFFF;
    }
    dac101c085_write_byte((unsigned short) (code & 0xFFFF), DAC_B);
}

class SleeperThread : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};


int GPBBTest::readKernelMemory(long offset, int virtualized, int size)
{
    int result;

    int *mem_range = (int *)(offset & ~0xFFFF);
    if( mem_range != prev_mem_range ) {
        prev_mem_range = mem_range;

        if(mem_32)
            munmap(mem_32, 0xFFFF);
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
        mem_16 = (short *)mem_32;
        mem_8  = (char  *)mem_32;
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

int GPBBTest::writeKernelMemory(long offset, long value, int virtualized, int size)
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


int GPBBTest::prepEim(void)
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


volatile quint16 *GPBBTest::eimOpen(unsigned int type)
{
    int range = (type >> 16) & 7;

    if (ranges[range])
        return ((quint16 *) (((quint8 *)ranges[range])+(type&0xffff)));

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

quint16 GPBBTest::eimGet(unsigned int type)
{
    return *eimOpen(type);
}

quint16 GPBBTest::eimSet(unsigned int type, quint16 value)
{
    volatile quint16 *ptr = eimOpen(type);
    quint16 old = *ptr;
    *ptr = value;
    return old;
}

int GPBBTest::loadFpga(const QString &bitpath)
{
    testDebug("Exporting GPIO 135");
    QFile exportFile("/sys/class/gpio/export");
    if (!exportFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO export file: " + exportFile.errorString());
        return 1;
    }
    exportFile.write("135");
    exportFile.close();

    testDebug("Making GPIO135 an output");
    QFile directionFile("/sys/class/gpio/gpio135/direction");
    if (!directionFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO135 direction file: " + directionFile.errorString());
        return 1;
    }
    directionFile.write("out\n");
    directionFile.close();

    testDebug("Putting FPGA into reset");
    QFile valueFile("/sys/class/gpio/gpio135/value");
    if (!valueFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO135 value file: " + valueFile.errorString());
        return 1;
    }
    valueFile.write("0\n");
    valueFile.close();

    SleeperThread::msleep(200);

    testDebug("Taking FPGA out of reset");
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

    testDebug("Writing bitstream to FPGA");
    while(1) {
        QByteArray bytes = bitstreamFile.read(1024);
        if (bytes.length() == 0)
            break;
        spiDev.write(bytes);
        spiDev.flush();
    }

    bitstreamFile.close();
    spiDev.close();

    testDebug("Done programming FPGA");

    return 0;
}

GPBBTest::GPBBTest()
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

void GPBBTest::runTest()
{
    qint32 ver_major, ver_minor;
    qint32 adc_val;

    testInfo("Loading GPBB firmware");
    if (loadFpga("/factory/gpbb_fpga.bit"))
        return;

    errno = 0;
    testDebug("Turning on clock to FPGA");
    writeKernelMemory(0x020c8160, 0x00000D2B, 0, 4);
    if (errno)
        return;

    if (prepEim())
        return;

    testDebug("Reading FPGA Major / Minor versions");
    /* Dummy reads required because the first read is always corrupt */
    eimGet(fpga_r_ddr3_v_major);
    eimGet(fpga_r_ddr3_v_minor);
    SleeperThread::msleep(200);
    eimGet(fpga_r_ddr3_v_major);
    eimGet(fpga_r_ddr3_v_minor);
    SleeperThread::msleep(200);

    ver_major = eimGet(fpga_r_ddr3_v_major);
    ver_minor = eimGet(fpga_r_ddr3_v_minor);
    testDebug(QString() + "GPBB Firmware version "
                        + QString::number(ver_major) + "."
                        + QString::number(ver_minor));

    if ((ver_major != 3) || (ver_minor != 11)) {
        testError("Unexpected version number");
        return;
    }

    testInfo("Disabling 5V");
    eimSet(fpga_w_gpbb_ctl, eimGet(fpga_r_gpbb_ctl) & ~0x8000);

    testInfo("Setting DAC A to 0");
    dac_a_set(0);

    adc_val = adc_read_chan(6);
    testInfo(QString("ADC Channel 6: %1").arg(adc_val));
    if (adc_val > 8) {
        testError("ADC Channel 6 is out of range!  Should be below 8");
        return;
    }

    adc_val = adc_read_chan(7);
    testInfo(QString("ADC Channel 7: %1").arg(adc_val));
    if ((adc_val < 335) || (adc_val > 382)) {
        testError("ADC Channel 7 is out of range!  Should be between 335 and 382");
        return;
    }

    testInfo("Enabling 5V");
    eimSet(fpga_w_gpbb_ctl, eimGet(fpga_r_gpbb_ctl) | 0x8000);

    adc_val = adc_read_chan(7);
    testInfo(QString("ADC Channel 7: %1").arg(adc_val));
    if ((adc_val < 516) || (adc_val > 572)) {
        testError("ADC Channel 7 is out of range!  Should be between 516 and 572");
        return;
    }

    testInfo("Setting DAC A to 512");
    dac_a_set(512);

    adc_val = adc_read_chan(6);
    testInfo(QString("ADC Channel 6: %1").arg(adc_val));
    if ((adc_val < 340) || (adc_val > 376)) {
        testError("ADC Channel 6 is ound of range!  Should be between 340 and 376");
        return;
    }

    testInfo("Setting DAC A to 1023");
    dac_a_set(1023);

    adc_val = adc_read_chan(6);
    testInfo(QString("ADC Channel 6: %1").arg(adc_val));
    if ((adc_val < 681) || (adc_val > 753)) {
        testError("ADC Channel 6 is ound of range!  Should be between 681 and 753");
        return;
    }

    testInfo("Resetting FPGA");
    QFile valueFile("/sys/class/gpio/gpio135/value");
    if (!valueFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO135 value file: " + valueFile.errorString());
        return;
    }
    valueFile.write("0\n");
    valueFile.close();

    testInfo("All GPBB tests passed");
}
