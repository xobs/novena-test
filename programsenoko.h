#ifndef PROGRAMSENOKO_H
#define PROGRAMSENOKO_H
#include "novenatest.h"

class ProgramSenoko : public NovenaTest
{
public:
    ProgramSenoko(const QString _firmware);
    void runTest();

private:
    QString firmware;
};

#endif // PROGRAMSENOKO_H
