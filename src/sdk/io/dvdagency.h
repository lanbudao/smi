#ifndef DVD_AGENCY_H
#define DVD_AGENCY_H

#include "sdk/global.h"
#include "sdk/DPTR.h"

NAMESPACE_BEGIN

class MediaIO;
class DVDAgencyPrivate;
class DVDAgency
{
    DPTR_DECLARE_PRIVATE(DVDAgency)
public:
    DVDAgency(MediaIO* io);
    ~DVDAgency();

    int setMouseActive(int x, int y);
    int setMouseSelect(int x, int y);
    int setUpperButtonSelect();
    int setLowerButtonSelect();
    int setLeftButtonSelect();
    int setRightButtonSelect();

private:
    DPTR_DECLARE(DVDAgency)
};

NAMESPACE_END

#endif