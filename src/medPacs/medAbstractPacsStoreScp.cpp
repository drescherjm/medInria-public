#include "medAbstractPacsStoreScp.h"

#include <dtkCore/dtkGlobal.h>

#include <QtCore>

medAbstractPacsStoreScp::medAbstractPacsStoreScp( void )
{

}

medAbstractPacsStoreScp::~medAbstractPacsStoreScp( void )
{

}

int medAbstractPacsStoreScp::start( const char* ourTitle, const char* ourIP, unsigned int ourPort )
{
    DTK_UNUSED(ourTitle);
    DTK_UNUSED(ourIP);
    DTK_UNUSED(ourPort);

    DTK_DEFAULT_IMPLEMENTATION;

    return 0;
}
