/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismpeseedcatcher.cc,v 1.16 2006-12-22 10:45:07 cvsjaap Exp $";

#include "vismpeseedcatcher.h"

#include "attribdataholder.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfacetr.h"
#include "emhorizon2d.h"
#include "survinfo.h"
#include "visdataman.h"
#include "visemobjdisplay.h"
#include "visevent.h"
#include "vishorizon2ddisplay.h"
#include "visseis2ddisplay.h"
#include "vismpe.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "visplanedatadisplay.h"

mCreateFactoryEntry( visSurvey::MPEClickCatcher );


namespace visSurvey
{

MPEClickCatcher::MPEClickCatcher()
    : click( this )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , trackertype_( 0 )
{ }


MPEClickCatcher::~MPEClickCatcher()
{
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );
}


void MPEClickCatcher::setSceneEventCatcher( visBase::EventCatcher* nev )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,MPEClickCatcher,clickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nev;

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,MPEClickCatcher,clickCB));
	eventcatcher_->ref();
    }
}


void MPEClickCatcher::setDisplayTransformation( visBase::Transformation* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();
}


visBase::Transformation* MPEClickCatcher::getDisplayTransformation()
{ return transformation_; }


#define mTrackerEquals( typestr, typekey) \
    ( typestr && !strcmp(typestr,EM##typekey##TranslatorGroup::keyword) )


#define mCheckPlaneDataDisplay( typ, dataobj, plane, yn ) \
    mDynamicCastGet( PlaneDataDisplay*, plane, dataobj ); \
    const bool yn = mTrackerEquals(typ,Horizon) && plane && \
		    plane->getOrientation()!=PlaneDataDisplay::Timeslice; 


#define mCheckMPEDisplay( typ, dataobj, mpedisplay, cs, yn ) \
    mDynamicCastGet( MPEDisplay*, mpedisplay, dataobj ); \
    CubeSampling cs; \
    const bool yn = mTrackerEquals(typ,Horizon) && \
		    mpedisplay && mpedisplay->isDraggerShown() && \
		    mpedisplay->getPlanePosition(cs) && cs.nrZ()!=1; 


#define mCheckSeis2DDisplay( typ, dataobj, seis2ddisplay, yn ) \
    mDynamicCastGet( Seis2DDisplay*, seis2ddisplay, dataobj ); \
    const bool yn = mTrackerEquals(typ,Horizon2D) && seis2ddisplay; 


bool MPEClickCatcher::isClickable( const char* trackertype, int visid )
{
    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
    if ( !dataobj )
	return false;
    
    mCheckPlaneDataDisplay( trackertype, dataobj, plane, planeisclickable );
    if ( planeisclickable )
	return true;
    
    mCheckMPEDisplay( trackertype, dataobj, mpedisplay, cs, mpeisclickable );
    if ( mpeisclickable )
	return true;

    mCheckSeis2DDisplay( trackertype, dataobj, seis2ddisp, seis2disclickable );
    if ( seis2disclickable )
	return true;

    return false;
}


void MPEClickCatcher::setTrackerType( const char* tt )
{ trackertype_ = tt; }


const MPEClickInfo& MPEClickCatcher::info() const
{ return info_; }


MPEClickInfo& MPEClickCatcher::info() 
{ return info_; }


void MPEClickCatcher::clickCB( CallBacker* cb )
{
    if ( eventcatcher_->isEventHandled() || !isOn() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	return;

    if ( eventinfo.mousebutton!=visBase::EventInfo::leftMouseButton() )
	return;

    if ( eventinfo.alt || eventinfo.ctrl && eventinfo.shift )
	return;
    
    info().setCtrlClicked( eventinfo.ctrl );
    info().setShiftClicked( eventinfo.shift );
    info().setPos( eventinfo.pickedpos );

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	if ( !dataobj ) 
	    continue;
	info().setObjID( visid );

	mDynamicCastGet( visSurvey::Horizon2DDisplay*, hor2ddisp, dataobj );
	if ( hor2ddisp )
	{
	    sendUnderlying2DSeis( hor2ddisp, eventinfo );
	    eventcatcher_->eventIsHandled();
	    break;
	}

	mDynamicCastGet( visSurvey::EMObjectDisplay*, emod, dataobj );
	if ( emod )
	{
	    sendUnderlyingPlanes( emod, eventinfo );
	    eventcatcher_->eventIsHandled();
	    break;
	}

	if ( eventinfo.ctrl || eventinfo.shift )
	    continue;
	    
	mCheckPlaneDataDisplay( trackertype_, dataobj, plane, validplaneclick );
	if ( validplaneclick )
	{
	    info().setObjCS( plane->getCubeSampling() );
	    info().setObjData( plane->getCacheVolume(false) );
	    info().setObjDataSelSpec( plane->getSelSpec(0) );
	    click.trigger();
	    eventcatcher_->eventIsHandled();
	    break;
	}

	mCheckMPEDisplay( trackertype_, dataobj, mpedisplay, cs, validmpeclick);
	if ( validmpeclick )
	{
	    info().setObjCS( cs );
	    click.trigger();
	    eventcatcher_->eventIsHandled();
	    break;
	}

	mCheckSeis2DDisplay( trackertype_, dataobj, seis2ddisplay, 
			     validseis2dclick );
	if ( validseis2dclick )
	{
	    RefMan<const Attrib::Data2DHolder> cache =
					    seis2ddisplay->getCache(0);
	    RefMan<Attrib::DataCubes> cube = 0;

	    if ( cache )
	    {
		cube = new Attrib::DataCubes;
		if ( !cache->fillDataCube(*cube) )
		    cube = 0;
	    }

	    info().setObjData( cube );
	    info().setObjDataSelSpec( seis2ddisplay->getSelSpec(0) );
	    info().setObjLineSet( seis2ddisplay->lineSetID() );
	    info().setObjLineName( seis2ddisplay->name() );
	    info().setObjLineData( cache );
	    click.trigger();
	    eventcatcher_->eventIsHandled();
	    break;
	}
    }
    info().clear();
}


void MPEClickCatcher::sendUnderlying2DSeis( 
				    const visSurvey::EMObjectDisplay* emod,
				    const visBase::EventInfo& eventinfo )
{
    const EM::EMObject* emobj = EM::EMM().getObject( emod->getObjectID() );
    if ( !emobj ) 
	return;
    
    const EM::PosID nodepid = emod->getPosAttribPosID( EM::EMObject::sSeedNode,
						       eventinfo.pickedobjids );
    info().setNode( nodepid );

    mDynamicCastGet( const EM::Horizon2D*, hor2d, emobj );
    const int lineidx = RowCol( nodepid.subID() ).r();
    const int lineid = hor2d->geometry().lineID( lineidx );
    const BufferString linenm = hor2d->geometry().lineName( lineid );
    const MultiID& lineset = hor2d->geometry().lineSet( lineid );

    TypeSet<int> seis2dinscene;
    visBase::DM().getIds( typeid(visSurvey::Seis2DDisplay), seis2dinscene ); 

    for ( int idx=0; idx<seis2dinscene.size(); idx++ )
    {
	visBase::DataObject* dataobj = 
				visBase::DM().getObject( seis2dinscene[idx] );
	if ( !dataobj )
	    continue;

	mCheckSeis2DDisplay( trackertype_, dataobj, seis2ddisp, 
			     validseis2dclick );
	if ( !validseis2dclick )
	    continue;

	if ( lineset==seis2ddisp->lineSetID() && linenm==seis2ddisp->name() )	
	{
	    RefMan<const Attrib::Data2DHolder> cache = seis2ddisp->getCache(0);
	    RefMan<Attrib::DataCubes> cube = 0;

	    if ( cache )
	    {
		cube = new Attrib::DataCubes;
		if ( !cache->fillDataCube(*cube) )
		    cube = 0;
	    }

	    info().setObjData( cube );
	    info().setObjDataSelSpec( seis2ddisp->getSelSpec(0) );
	    info().setObjLineSet( seis2ddisp->lineSetID() );
	    info().setObjLineName( seis2ddisp->name() );
	    info().setObjLineData( cache );
	    click.trigger();
	    return;
    	}
    }
}


void MPEClickCatcher::sendUnderlyingPlanes( 
				    const visSurvey::EMObjectDisplay* emod,
				    const visBase::EventInfo& eventinfo )
{
    const EM::EMObject* emobj = EM::EMM().getObject( emod->getObjectID() );
    if ( !emobj ) 
	return;
    
    const EM::PosID nodepid = emod->getPosAttribPosID( EM::EMObject::sSeedNode,
						       eventinfo.pickedobjids );
    Coord3 nodepos = emobj->getPos( nodepid );
    info().setNode( nodepid );
    
    if ( !nodepos.isDefined() )
    {
	 if ( eventinfo.ctrl || eventinfo.shift ) return;

	 Scene* scene = STM().currentScene();
	 const Coord3 disppos = scene->getZScaleTransform()->
					transformBack( eventinfo.pickedpos );
	 nodepos = scene->getUTM2DisplayTransform()->transformBack( disppos );
    }
    const BinID nodebid = SI().transform( nodepos );

    TypeSet<int> mpedisplays;
    visBase::DM().getIds( typeid(visSurvey::MPEDisplay), mpedisplays ); 
    
    for ( int idx=0; idx<mpedisplays.size(); idx++ )
    {
	visBase::DataObject* dataobj = 
				visBase::DM().getObject( mpedisplays[idx] );
	if ( !dataobj )
	    continue;

	mCheckMPEDisplay( trackertype_, dataobj, mpedisplay, cs, validmpeclick);
	if ( validmpeclick && cs.hrg.includes(nodebid) && 
	     cs.zrg.includes(nodepos.z) )
	{
	    info().setObjID( mpedisplay->id() );
	    info().setObjCS( cs );
	    click.trigger();
	    return;
	}
    }

    TypeSet<int> planesinscene;
    visBase::DM().getIds( typeid(visSurvey::PlaneDataDisplay), planesinscene ); 
    
    for ( int idx=0; idx<planesinscene.size(); idx++ )
    {
	visBase::DataObject* dataobj = 
				visBase::DM().getObject( planesinscene[idx] );
	if ( !dataobj )
	    continue;

	mCheckPlaneDataDisplay( trackertype_, dataobj, plane, validplaneclick );
	if ( !validplaneclick )
	    continue;

	const CubeSampling cs = plane->getCubeSampling();
	if ( cs.hrg.includes(nodebid) && cs.zrg.includes(nodepos.z) )
	{
	    info().setObjID( plane->id() );
	    info().setObjCS( cs );
	    info().setObjData( plane->getCacheVolume(false) );
	    info().setObjDataSelSpec( plane->getSelSpec(0) );
	    click.trigger();
	}
    }
}


MPEClickInfo::MPEClickInfo()
{ clear(); }


bool MPEClickInfo::isCtrlClicked() const
{ return ctrlclicked_; }


bool MPEClickInfo::isShiftClicked() const
{ return shiftclicked_; }


const EM::PosID& MPEClickInfo::getNode() const
{ return clickednode_; }


const Coord3& MPEClickInfo::getPos() const
{ return clickedpos_; }


int MPEClickInfo::getObjID() const
{ return clickedobjid_; }


const CubeSampling& MPEClickInfo::getObjCS() const
{ return clickedcs_; }


const Attrib::DataCubes* MPEClickInfo::getObjData() const
{ return attrdata_; }


const Attrib::SelSpec* MPEClickInfo::getObjDataSelSpec() const
{ return attrsel_; }


const MultiID& MPEClickInfo::getObjLineSet() const
{ return lineset_; }


const char* MPEClickInfo::getObjLineName() const
{ return linename_[0] ? (const char*) linename_ : 0; }


const Attrib::Data2DHolder* MPEClickInfo::getObjLineData() const
{ return linedata_; }


void MPEClickInfo::clear()
{
    ctrlclicked_ = false;
    shiftclicked_ = false;
    clickednode_ = EM::PosID( -1, -1, -1 );
    clickedpos_ = Coord3::udf();
    clickedobjid_ = -1;
    clickedcs_.init( false);
    attrsel_ = 0;
    attrdata_ = 0;
    linedata_ = 0;
    lineset_ = MultiID( -1 );
    linename_ = "";
}


void MPEClickInfo::setCtrlClicked( bool yn )
{ ctrlclicked_ = yn; }


void MPEClickInfo::setShiftClicked( bool yn )
{ shiftclicked_ = yn; }


void MPEClickInfo::setNode( const EM::PosID& pid )
{ clickednode_ = pid; }


void MPEClickInfo::setPos(const Coord3& pos )
{ clickedpos_ = pos; }


void MPEClickInfo::setObjID( int visid )
{ clickedobjid_ = visid; }


void MPEClickInfo::setObjCS( const CubeSampling& cs )
{ clickedcs_ = cs; }


void MPEClickInfo::setObjData( const Attrib::DataCubes* ad )
{ attrdata_ = ad; }


void MPEClickInfo::setObjDataSelSpec( const Attrib::SelSpec* as )
{ attrsel_ = as; }


void MPEClickInfo::setObjLineSet( const MultiID& mid )
{ lineset_ = mid; }


void MPEClickInfo::setObjLineName( const char* str )
{ linename_ = str; }


void MPEClickInfo::setObjLineData( const Attrib::Data2DHolder* ad2dh )
{ linedata_ = ad2dh; }



}; //namespce
