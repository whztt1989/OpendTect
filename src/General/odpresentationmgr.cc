/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : August 2016
-*/


#include "odpresentationmgr.h"
#include "keystrs.h"
#include "iopar.h"
#include "dbman.h"

const char* OD::sKeyPresentationObj()	{ return "Presentation Obj"; }

static OD::PresentationManager* prman_ = 0;
OD::PresentationManager& OD::PrMan()
{
    if ( !prman_ )
	prman_ = new OD::PresentationManager;
    return *prman_;
}


OD::PresentationManager::PresentationManager()
{
    syncAllViewerTypes();
}


OD::VwrTypePresentationMgr* OD::PresentationManager::getViewerTypeMgr(
	OD::ViewerTypeID vwrtypeid )
{
    const int idx = syncInfoIdx( vwrtypeid );
    if ( idx<0 )
	return 0;

    return vwrtypemanagers_[idx];
}


void OD::PresentationManager::request( OD::ViewerID vwrid,
				     OD::PresentationRequestType req,
				     const IOPar& prinfopar )
{
    for ( int idx=0; idx<vwrtypemanagers_.size(); idx++ )
    {
	OD::VwrTypePresentationMgr* vwrtypemgr = vwrtypemanagers_[idx];
	const SyncInfo& syninfo = vwrtypesyncinfos_[idx];
	const OD::ViewerTypeID vwrtypeid = syninfo.vwrtypeid_;
	if ( !areViewerTypesSynced(vwrid.viewerTypeID(),vwrtypeid) )
	    continue;

	vwrtypemgr->request(
		req, prinfopar, vwrtypeid==vwrid.viewerTypeID()
				    ? vwrid.viewerObjID()
				    : OD::ViewerObjID::get(-1) );
    }
}


int OD::PresentationManager::syncInfoIdx( OD::ViewerTypeID vwrtypeid ) const
{
    for ( int idx=0; idx<vwrtypesyncinfos_.size(); idx++ )
    {
	if ( vwrtypesyncinfos_[idx].vwrtypeid_ == vwrtypeid )
	    return idx;
    }

    return -1;
}


bool OD::PresentationManager::areViewerTypesSynced(
	OD::ViewerTypeID vwr1typeid, OD::ViewerTypeID vwr2typeid ) const
{
    const int vwr1typeidx = syncInfoIdx( vwr1typeid );
    const int vwr2typeidx = syncInfoIdx( vwr2typeid );
    if ( vwr1typeidx<0 || vwr2typeidx<0 )
	return false;

    return vwrtypesyncinfos_[vwr1typeidx].issynced_ &&
	   vwrtypesyncinfos_[vwr2typeidx].issynced_;
}


void OD::PresentationManager::syncAllViewerTypes()
{
    for ( int idx=0; idx<vwrtypesyncinfos_.size(); idx++ )
	vwrtypesyncinfos_[idx].issynced_ = true;
}


void OD::PresentationManager::addViewerTypeManager(
	OD::VwrTypePresentationMgr* vtm )
{
    vwrtypemanagers_ += vtm;
    vwrtypesyncinfos_ += SyncInfo( vtm->viewerTypeID(), true );
}


OD::PresentationManagedViewer::PresentationManagedViewer()
    : ObjAdded(this)
    , ObjOrphaned(this)
    , UnsavedObjLastCall(this)
    , ShowRequested(this)
    , HideRequested(this)
    , VanishRequested(this)
    , viewerobjid_(OD::ViewerObjID::get(-1))
{
}


OD::PresentationManagedViewer::~PresentationManagedViewer()
{
    detachAllNotifiers();
}


void OD::VwrTypePresentationMgr::request( OD::PresentationRequestType req,
					const IOPar& prinfopar,
					OD::ViewerObjID skipvwrid )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	OD::PresentationManagedViewer* vwr = viewers_[idx];
	if ( vwr->viewerObjID()==skipvwrid )
	    continue;

	switch ( req )
	{
	    case OD::Add:
		vwr->ObjAdded.trigger( prinfopar );
		break;
	    case OD::Vanish:
		vwr->VanishRequested.trigger( prinfopar );
		break;
	    case OD::Show:
		vwr->ShowRequested.trigger( prinfopar );
		break;
	    case OD::Hide:
		vwr->HideRequested.trigger( prinfopar );
		break;
	}
    }
}


void OD::ObjPresentationInfo::fillPar( IOPar& par ) const
{
    par.set( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
	     objtypekey_ );
}


bool OD::ObjPresentationInfo::usePar( const IOPar& par )
{
    return par.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		    objtypekey_ );
}


bool OD::ObjPresentationInfo::isSameObj(
	const  OD::ObjPresentationInfo& prinfo ) const
{ return objtypekey_ == prinfo.objTypeKey(); }


OD::ObjPresentationInfo* OD::ObjPresentationInfo::clone() const
{
    IOPar prinfopar;
    fillPar( prinfopar );
    return OD::PRIFac().create( prinfopar );
}


uiString OD::SaveableObjPresentationInfo::getName() const
{
    return toUiString( DBM().nameOf(storedid_) );
}


void OD::SaveableObjPresentationInfo::fillPar( IOPar& par ) const
{
    OD::ObjPresentationInfo::fillPar( par );
    par.set( IOPar::compKey(sKey::Stored(),sKey::ID()), storedid_ );
}


bool OD::SaveableObjPresentationInfo::usePar( const IOPar& par )
{
    if ( !OD::ObjPresentationInfo::usePar(par) )
	return false;

    return par.get( IOPar::compKey(sKey::Stored(),sKey::ID()), storedid_ );
}


bool OD::SaveableObjPresentationInfo::isSameObj(
	const  OD::ObjPresentationInfo& prinfo ) const
{
    if ( !OD::ObjPresentationInfo::isSameObj(prinfo) )
	return false;

    mDynamicCastGet(const OD::SaveableObjPresentationInfo*,saveableprinfo,
		    &prinfo);
    if ( !saveableprinfo )
	return false;

    return storedid_.isInvalid() ? true : storedid_==saveableprinfo->storedID();
}


bool OD::ObjPresentationInfoSet::isPresent(
	const OD::ObjPresentationInfo& prinfo ) const
{
    for ( int idx=0; idx<prinfoset_.size(); idx++ )
    {
	if ( prinfoset_[idx]->isSameObj(prinfo) )
	    return true;
    }

    return false;
}


bool OD::ObjPresentationInfoSet::add( OD::ObjPresentationInfo* prinfo )
{
    if ( !prinfo || isPresent(*prinfo) )
	return false;

    prinfoset_ += prinfo;
    return true;
}


OD::ObjPresentationInfo* OD::ObjPresentationInfoSet::get( int idx )
{
    if ( !prinfoset_.validIdx(idx) )
	return 0;

    return prinfoset_[idx];
}


const OD::ObjPresentationInfo* OD::ObjPresentationInfoSet::get( int idx ) const
{
    return const_cast<OD::ObjPresentationInfoSet*>(this)->get( idx );
}


OD::ObjPresentationInfo* OD::ObjPresentationInfoSet::remove( int idx )
{
    if ( !prinfoset_.validIdx(idx) )
	return 0;

    return prinfoset_.removeSingle( idx );
}


static OD::ObjPresentationInfoFactory* dispinfofac_ = 0;

OD::ObjPresentationInfoFactory& OD::PRIFac()
{
    if ( !dispinfofac_ )
	dispinfofac_ = new OD::ObjPresentationInfoFactory;
    return *dispinfofac_;
}


void OD::ObjPresentationInfoFactory::addCreateFunc( CreateFunc crfn,
						const char* key )
{
    const int keyidx = keys_.indexOf( key );
    if ( keyidx >= 0 )
    {
	createfuncs_[keyidx] = crfn;
	return;
    }

    createfuncs_ += crfn;
    keys_.add( key );
}


OD::ObjPresentationInfo* OD::ObjPresentationInfoFactory::create(
	const IOPar& par )
{
    BufferString keystr;
    if ( !par.get(IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		  keystr) )
	return 0;

    const int keyidx = keys_.indexOf( keystr );
    if ( keyidx < 0 )
	return 0;

    return (*createfuncs_[keyidx])( par );
}
