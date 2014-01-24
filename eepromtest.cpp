#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QNetworkReply>
#include <QEventLoop>

#if QT_VERSION >= 0x050000
#include <QJsonObject>
#include <QJsonParseError>
#else
#include <qjson/parser.h>
#endif

#include <stdint.h>
#ifdef linux
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <ctype.h>


#define EEPROM_ADDRESS (0xac >> 1)
#define I2C_BUS "/dev/i2c-2"
#define NOVENA_SIGNATURE "Novena"
#define NOVENA_VERSION 1

#endif
#include "eepromtest.h"

struct novena_eeprom_data {
        uint8_t         signature[6];   /* 'Novena' */
        uint8_t         version;        /* 1 */
        uint8_t         reserved1;
        uint32_t        serial;
        uint8_t         mac[6];
        uint16_t        features;
} __attribute__((__packed__));

struct feature {
        uint32_t        flags;
        const char      *name;
        const char      *descr;
};

struct feature features[] = {
        {
                /*.flags =*/ 0x01,
                /*.name  =*/ "es8328",
                /*.descr =*/ "ES8328 audio codec",
        },
        {
                /*.flags =*/ 0x02,
                /*.name  =*/ "senoko",
                /*.descr =*/ "Senoko battery board",
        },
        {
                /*.flags =*/ 0x04,
                /*.name  =*/ "retina",
                /*.descr =*/ "Retina-class dual-LVDS display",
        },
        {
                /*.flags =*/ 0x08,
                /*.name  =*/ "pixelqi",
                /*.descr =*/ "PixelQi LVDS display",
        },
        {
                /*.flags =*/ 0x10,
                /*.name  =*/ "pcie",
                /*.descr =*/ "PCI Express support",
        },
        {
                /*.flags =*/ 0x20,
                /*.name  =*/ "gbit",
                /*.descr =*/ "Gigabit Ethernet",
        },
        {
                /*.flags =*/ 0x40,
                /*.name  =*/ "hdmi",
                /*.descr =*/ "HDMI Output",
        },
        {0, 0, 0} /* Sentinal */
};

struct eeprom_dev {
    /* File handle to I2C bus */
    int				fd;

    /* I2C address of the EEPROM */
    int				addr;

    /* True, if we've read the contents of eeprom */
    int				cached;

    /* Contents of the EEPROM */
    struct novena_eeprom_data 	data;
};


static struct eeprom_dev *dev;
static QString *g_serial;

static int parse_features(const char *str) {
    char *ctx;
    const char *sep = ",";
    char *word;
    uint16_t flags = 0;
    char mystr[strlen(str)+1];

    strcpy(mystr, str);

    for (word = strtok_r(mystr, sep, &ctx);
         word;
         word = strtok_r(NULL, sep, &ctx)) {

        struct feature *feature = features;
        while (feature->name) {
            if (!strcmp(feature->name, word)) {
                flags |= feature->flags;
                break;
            }
            feature++;
        }
        if (!feature->name) {
            return -1;
        }
    }
    return flags;
}

static int parse_mac(const char *str, void *out) {
    int i;
    char *mac = (char *)out;
    for (i=0; i<6; i++) {
        if (!isdigit(str[0]) &&
            (tolower(str[0]) < 'a' || tolower(str[0]) > 'f')) {
            return 1;
        }
        if (!isdigit(str[1]) &&
            (tolower(str[1]) < 'a' || tolower(str[1]) > 'f')) {
            return 1;
        }

        *mac = strtoul(str, NULL, 16);
        mac++;
        str+=2;
        if (*str == '-' || *str == ':' || *str == '.')
            str++;
    }
    if (*str) {
        return 1;
    }
    return 0;
}

static int eeprom_read_i2c(struct eeprom_dev *dev, int addr, void *data, int count) {
    struct i2c_rdwr_ioctl_data session;
    struct i2c_msg messages[2];
    uint8_t set_addr_buf[2];

    memset(set_addr_buf, 0, sizeof(set_addr_buf));
    memset(data, 0, count);

    set_addr_buf[0] = addr>>8;
    set_addr_buf[1] = addr;

    messages[0].addr = dev->addr;
    messages[0].flags = 0;
    messages[0].len = sizeof(set_addr_buf);
    messages[0].buf = set_addr_buf;

    messages[1].addr = dev->addr;
    messages[1].flags = I2C_M_RD;
    messages[1].len = count;
    messages[1].buf = (unsigned char *)data;

    session.msgs = messages;
    session.nmsgs = 2;

    if(ioctl(dev->fd, I2C_RDWR, &session) < 0) {
        return 1;
    }

    return 0;
}

static int eeprom_write_i2c(struct eeprom_dev *dev, int addr, void *data, int count) {
    struct i2c_rdwr_ioctl_data session;
    struct i2c_msg messages[1];
    uint8_t data_buf[2+count];

    data_buf[0] = addr>>8;
    data_buf[1] = addr;
    memcpy(&data_buf[2], data, count);

    messages[0].addr = dev->addr;
    messages[0].flags = 0;
    messages[0].len = sizeof(data_buf);
    messages[0].buf = data_buf;

    session.msgs = messages;
    session.nmsgs = 1;

    if(ioctl(dev->fd, I2C_RDWR, &session) < 0) {
        return 1;
    }

    return 0;
}

static int eeprom_read(struct eeprom_dev *dev) {
    int ret;

    if (dev->cached)
        return 0;

    ret = eeprom_read_i2c(dev, 0, &dev->data, sizeof(dev->data));
    if (ret)
        return ret;

    dev->cached = 1;
    return 0;
}

static int eeprom_write(struct eeprom_dev *dev) {
    dev->cached = 1;
    return eeprom_write_i2c(dev, 0, &dev->data, sizeof(dev->data));
}

static struct eeprom_dev *eeprom_open(const char *path, int addr) {
    struct eeprom_dev *dev;

    dev = (struct eeprom_dev *)malloc(sizeof(*dev));
    if (!dev) {
        goto malloc_err;
    }

    memset(dev, 0, sizeof(*dev));

    dev->fd = open(path, O_RDWR);
    if (dev->fd == -1) {
        goto open_err;
    }

    dev->addr = addr;

    return dev;

open_err:
    free(dev);
malloc_err:
    return NULL;
}

static int eeprom_close(struct eeprom_dev **dev) {
    if (!dev || !*dev)
        return 0;
    close((*dev)->fd);
    free(*dev);
    *dev = NULL;
    return 0;
}


EEPROMStart::EEPROMStart(QString url, QString features)
{
    name = "EEPROM checkout";
    _url = url;
    _features = features;
}

void EEPROMStart::runTest()
{
    int ret;
    int feat;
    uint8_t new_mac[6];

    testInfo("Trying to open I2C");
    dev = eeprom_open(I2C_BUS, EEPROM_ADDRESS);
    if (!dev) {
        testError(QString("Unable to communicate over I2C: %1").arg(strerror(errno)));
        return;
    }

    ret = eeprom_read(dev);
    if (ret) {
        testError(QString("Unable to communicate with EEPROM: %1").arg(strerror(errno)));
        return;
    }

    feat = parse_features(_features.toUtf8());
    if (feat == -1) {
        testError("Invalid feature string");
        return;
    }


    testInfo(QString() + "Checking out serial and MAC address from " + _url);

    QNetworkAccessManager *manager = new QNetworkAccessManager(this->parent());

    testDebug("Creating network request");
    QNetworkRequest request((QUrl(_url)));

    testDebug("Making GET request");
    QNetworkReply *reply = manager->get(request);

    // execute an event loop to process the request (nearly-synchronous)
    QEventLoop eventLoop;
    // also dispose the event loop after the reply has arrived
    testDebug("Connecting event loop");
    connect(manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    testDebug("Running event loop");
    eventLoop.exec();

    testDebug("Waiting for data...");
    QByteArray data = reply->readAll();

    if (data.length() < 5) {
        testError("No reply from server");
        return;
    }

    testDebug(QString("JSON data: %1").arg(QString(data)));

#if QT_VERSION >= 0x050000
    QJsonParseError *err = new QJsonParseError();
    QJsonDocument doc = QJsonDocument::fromJson(data, err);
    if (err->error != 0) {
        testError(QString("Unable to communicate with server: %1").arg(err->errorString()));
        return;
    }

    if (doc.isNull()) {
        testError("Server returned an empty document");
        return;
    }

    if (!doc.isObject()) {
        testError("Server returned an unexpected JSON object");
        return;
    }

    QJsonObject obj = doc.object();
    QVariantMap map = obj.toVariantMap();
#else
    QJson::Parser parser;
    bool ok;
    QVariantMap map = parser.parse(data, &ok).toMap();
#endif

    testDebug(QString("Serial number: %1  MAC address: %2  Status: %3")
                .arg(map["Serial"].toString())
                .arg(map["MAC"].toString())
                .arg(map["Status"].toString())
            );

    testDebug("Document fetched successfully");

    parse_mac(map["MAC"].toByteArray(), (void *)new_mac);
    g_serial = new QString(map["Serial"].toString());

    memcpy(&dev->data.mac, new_mac, sizeof(new_mac));
    dev->data.serial = map["Serial"].toInt();
    dev->data.features = feat;
    memcpy(&dev->data.signature, NOVENA_SIGNATURE, sizeof(dev->data.signature));
    dev->data.version = NOVENA_VERSION;


    delete reply;
    delete manager;
}



EEPROMFinish::EEPROMFinish(QString url)
{
    name = "EEPROM verify";
    _url = url;
}

void EEPROMFinish::runTest()
{
    testInfo("Confirming serial and MAC address");
    testInfo("Checking out serial and MAC address");

    QNetworkAccessManager *manager = new QNetworkAccessManager(this->parent());

    testDebug("Creating network request");
    QNetworkRequest request((QUrl(_url.arg(*g_serial))));

    testDebug("Making GET request");
    QNetworkReply *reply = manager->get(request);

    // execute an event loop to process the request (nearly-synchronous)
    QEventLoop eventLoop;
    // also dispose the event loop after the reply has arrived
    testDebug("Connecting event loop");
    connect(manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    testDebug("Running event loop");
    eventLoop.exec();

    testDebug("Waiting for data...");
    QByteArray data = reply->readAll();
    if (data.length() < 5) {
        testError("No reply from server");
        return;
    }

    testDebug(QString("JSON data: %1").arg(QString(data)));


    testDebug("Writing EEPROM data out");
    if (eeprom_write(dev)) {
        testError(QString("Unable to write EEPROM data: %1").arg(strerror(errno)));
        return;
    }

    eeprom_close(&dev);
    testInfo("Done");
}
