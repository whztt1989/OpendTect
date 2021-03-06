/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/


#include "emhorizonpreload.h"

#include "bufstring.h"
#include "emmanager.h"
#include "emobject.h"
#include "executor.h"
#include "dbman.h"
#include "dbkey.h"
#include "ptrman.h"


namespace EM
{

HorizonPreLoader& HPreL()
{
    mDefineStaticLocalObject( PtrMan<HorizonPreLoader>, hpl,
                              (new HorizonPreLoader) );

    return *hpl;
}


HorizonPreLoader::HorizonPreLoader()
{
    DBM().surveyToBeChanged.notify( mCB(this,HorizonPreLoader,surveyChgCB) );
}


HorizonPreLoader::~HorizonPreLoader()
{
    DBM().surveyToBeChanged.remove( mCB(this,HorizonPreLoader,surveyChgCB) );
}


bool HorizonPreLoader::load( const DBKeySet& newmids, TaskRunner* tr )
{
    errmsg_ = "";
    if ( newmids.isEmpty() )
	return false;

    BufferString msg1( "The selected horizon(s):\n" );
    BufferString msg2( "Could not pre-load:\n" );
    int nralreadyloaded = 0;
    int nrproblems = 0;
    PtrMan<ExecutorGroup> execgrp = new ExecutorGroup("Pre-loading horizons");
    ObjectSet<EM::EMObject> emobjects;
    for ( int idx=0; idx<newmids.size(); idx++ )
    {
	const int selidx = loadedmids_.indexOf( newmids[idx] );
	if ( selidx > -1 )
	{
	    msg1.add( " '" ).add( loadednms_.get(selidx) ).add( "' " );
	    nralreadyloaded++;
	    continue;
	}

	EM::ObjectID emid = EM::EMM().getObjectID( newmids[idx] );
	EM::EMObject* emobj = EM::EMM().getObject( emid );
	if ( !emobj || !emobj->isFullyLoaded() )
	{
	    Executor* exec = EM::EMM().objectLoader( newmids[idx] );
	    if ( !exec )
	    {
		BufferString name( EM::EMM().objectName(newmids[idx]) );
		msg2.add( " '" ).add( name ).add( "' " );
		nrproblems++;
		continue;
	    }

	    execgrp->add( exec );
	}

	emid = EM::EMM().getObjectID( newmids[idx] );
	emobj = EM::EMM().getObject( emid );
	emobjects += emobj;
    }

    if ( nralreadyloaded > 0 )
    {
	msg1.add( " already pre-loaded" );
	errmsg_.add( msg1 );
    }

    if ( nrproblems > 0 )
	errmsg_.add( "\n" ).add( msg2 );

    if ( execgrp->nrExecutors()!=0 &&  !TaskRunner::execute( tr, *execgrp) )
	return false;

    for ( int idx=0; idx<emobjects.size(); idx++ )
    {
	loadedmids_ += emobjects[idx]->dbKey();
	loadednms_.add( emobjects[idx]->name() );
	emobjects[idx]->ref();
    }

    return true;
}


const DBKeySet& HorizonPreLoader::getPreloadedIDs() const
{ return loadedmids_; }

const BufferStringSet& HorizonPreLoader::getPreloadedNames() const
{ return loadednms_; }


DBKey HorizonPreLoader::getDBKey( const char* horname ) const
{
    const int mididx = loadednms_.indexOf( horname );
    return mididx < 0 ? DBKey::getInvalid() : loadedmids_[mididx];
}


void HorizonPreLoader::unload( const BufferStringSet& hornames )
{
    if ( hornames.isEmpty() )
	return;

    errmsg_ = "";
    for ( int hidx=0; hidx<hornames.size(); hidx++ )
    {
	const int selidx = loadednms_.indexOf( hornames.get(hidx) );
	if ( selidx < 0 )
	    continue;

	const DBKey mid = loadedmids_[selidx];
	EM::ObjectID emid = EM::EMM().getObjectID( mid );
	EM::EMObject* emobj = EM::EMM().getObject( emid );
	if ( emobj )
	    emobj->unRef();

	loadedmids_.removeSingle( selidx );
	loadednms_.removeSingle( selidx );
    }
}


void HorizonPreLoader::surveyChgCB( CallBacker* )
{
    unload( loadednms_ );
    loadedmids_.erase();
    loadednms_.erase();
    errmsg_ = "";
}

} // namespace EM
