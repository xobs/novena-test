#include "wifitest.h"
#include <Qt/QtNetwork>


WifiTest::WifiTest()
{
}


void WifiTest::runTest() {
    QList<QNetworkInterface> everything = QNetworkInterface::allInterfaces();
    int i;
    for (i=0; i<everything.size(); i++) {
        qDebug() << "Wifi item " << i << ": " << everything.at(i).humanReadableName();
    }

    QNetworkConfigurationManager *manager = new QNetworkConfigurationManager();
    QList<QNetworkConfiguration> configs = manager->allConfigurations();
    qDebug() << "Listing configs:";
    for (i=0; i<configs.size(); i++) {
        qDebug() << "Config item " << i << ": " << configs.at(i).name() << configs.at(i).bearerTypeName();
    }
}
