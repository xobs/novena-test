#ifndef SENOKOSCRIPT_H
#define SENOKOSCRIPT_H
#include "novenatest.h"

#include <QFile>
#include <QStringList>

class SenokoScript : public NovenaTest
{
public:
    SenokoScript(const QStringList _commands);
    void runTest();

private:
    QStringList commands;
    QFile serial;

    const QString runCommand(const QString command);
};

#endif // SENOKOSCRIPT_H
