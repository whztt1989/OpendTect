#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.20 2004-07-16 15:35:25 bert Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
#include "multiid.h"
#include "binidvalset.h"
#include "bufstringset.h"

class Color;
class PickSet;
class BinIDRange;
class SurfaceInfo;
class PickSetGroup;
class RandLocGenPars;


/*! \brief Service provider for application level - seismics */

class uiPickPartServer : public uiApplPartServer
{
public:
				uiPickPartServer(uiApplService&);
				~uiPickPartServer();

    const char*			name() const		{ return "Picks"; }

				// Services
    void			importPickSet();
    void			exportPickSet();
    bool			fetchPickSets();
    bool			storePickSets();

    static const int		evGetAvailableSets;
    static const int		evFetchPicks;
    static const int		evGetHorInfo;
    static const int		evGetHorDef;

				// Interaction stuff
    PickSetGroup&		group()			{ return psg; }
    				//!< 1) Result of fetchPickSets()
    				//!< 2) Must be filled on evFetchPicks
    BufferStringSet&		availableSets()		{ return avsets; }
    const BoolTypeSet& 		selectedSets() const	{ return selsets; }
    MultiID&			psgID()			{ return psgid; }
    const Color&		getPickColor()		{ return pickcolor; }
    bool			storeSinglePickSet(PickSet*);
    void			renamePickset(const char*,BufferString&);
    void			setMisclassSet(const BinIDValueSet&);

    BinIDValueSet&		genDef() 		{ return gendef; }
    ObjectSet<SurfaceInfo>& 	horInfos()		{ return hinfos; }
    const ObjectSet<MultiID>&	selHorIDs() const	{ return selhorids; }
    const BinIDRange*		selBinIDRange() const	{ return selbr; }

protected:

    PickSetGroup&		psg;
    MultiID			psgid;
    BufferStringSet		avsets;
    BoolTypeSet			selsets;
    Color&			pickcolor;

    BinIDValueSet 		gendef;
    ObjectSet<SurfaceInfo> 	hinfos;
    ObjectSet<MultiID>		selhorids;
    const BinIDRange*		selbr;

    bool			mkRandLocs(PickSet&,const RandLocGenPars&);
};


#endif
