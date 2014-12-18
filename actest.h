#ifndef ACTEST_H
#define ACTEST_H
#include "novenatest.h"

class QFile;

class ACTest : public NovenaTest
{
public:
    enum direction {
      PlugIn,
      UnPlug,
    };

    ACTest(enum direction _dir);
    void runTest();

private:
    enum direction dir;
    int senoko_fd;
};

#endif // ACTEST_H
