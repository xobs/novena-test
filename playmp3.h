#ifndef PLAYMP3_H
#define PLAYMP3_H
#include "novenatest.h"

class PlayMP3 : public NovenaTest
{
public:
    PlayMP3(const QString &path);
    void runTest();

private:
    QString _path;
};

#endif // PLAYMP3_H
