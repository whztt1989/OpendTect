/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2007
_______________________________________________________________________

-*/

#include "uiseisamplspectrum.h"

#include "arrayndimpl.h"
#include "seisdatapack.h"


void uiSeisAmplSpectrum::setDataPackID( DataPack::ID dpid,
					DataPackMgr::ID dmid )
{
    uiAmplSpectrum::setDataPackID( dpid, dmid );

    if ( dmid == DataPackMgr::SeisID() )
    {
	ConstRefMan<DataPack> datapack = DPM(dmid).get( dpid );
	mDynamicCastGet(const SeisDataPack*,dp,datapack.ptr());
	if ( dp )
	{
	    setup_.nyqvistspspace_ = dp->getZRange().step;
	    setData( dp->data() );
	}
    }
}
