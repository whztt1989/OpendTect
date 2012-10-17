/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visobject.h"

#include "errh.h"
#include "iopar.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vistransform.h"

#include <osg/Switch>
#include <osg/Material>

namespace visBase
{

const char* VisualObjectImpl::sKeyMaterialID()	{ return "Material ID"; }
const char* VisualObjectImpl::sKeyMaterial()    { return "Material"; }
const char* VisualObjectImpl::sKeyIsOn()	{ return "Is on"; }


VisualObject::VisualObject( bool issel )
    : isselectable(issel)
    , deselnotifier(this)
    , selnotifier(this)
    , rightClick(this)
    , rcevinfo(0)
{}


VisualObject::~VisualObject()
{
    deepUnRef( nodestates_ );
}
    
    
    
void VisualObject::doAddNodeState(visBase::NodeState* ns)
{
    ns->ref();
    nodestates_ += ns;
    osg::ref_ptr<osg::StateSet> stateset = getStateSet();
    if ( !stateset )
    {
	pErrMsg("Setting nodestate on class without stateset.");
    }
    else
	ns->setStateSet( stateset );
}

    
visBase::NodeState* VisualObject::removeNodeState( visBase::NodeState* ns )
{
    const int idx = nodestates_.indexOf( ns );
    if ( nodestates_.validIdx(idx) )
    {
	ns->setStateSet( 0 );
	nodestates_.remove( idx )->unRef();
    }
    
    return ns;
}
    
    
osg::StateSet* VisualObject::getStateSet()
{
    return gtOsgNode() ? gtOsgNode()->getOrCreateStateSet() : 0;
}
    

VisualObjectImpl::VisualObjectImpl( bool issel )
    : VisualObject( issel )
    , osgroot_( new osg::Switch )
    , material_( 0 )
    , righthandsystem_( true )
{
}


VisualObjectImpl::~VisualObjectImpl()
{
    if ( material_ ) material_->unRef();
}


void VisualObjectImpl::setLockable()
{
}


void VisualObjectImpl::readLock()
{
}
	

void VisualObjectImpl::readUnLock()
{
}

bool VisualObjectImpl::tryReadLock()
{
    return false;
}


void VisualObjectImpl::writeLock()
{
}
	

void VisualObjectImpl::writeUnLock()
{
}


bool VisualObjectImpl::tryWriteLock()
{
    return false;
}



void VisualObjectImpl::turnOn( bool yn )
{

    if ( yn )
	osgroot_->setAllChildrenOn();
    else
	osgroot_->setAllChildrenOff();
}


bool VisualObjectImpl::isOn() const
{
    if ( osgroot_->getNumChildren() )
    {
	return osgroot_->getValue( 0 );
    }
    
    return true;
}


void VisualObjectImpl::setMaterial( Material* nm )
{
    if ( material_ )
    {
	material_->setStateSet( 0 );
	material_->unRef();
    }

    material_ = nm;

    if ( material_ )
    {
	material_->ref();
	material_->setStateSet( osgroot_->getOrCreateStateSet() );
    }
}
    
    
Material* VisualObjectImpl::getMaterial()
{
    if ( !material_ )
	setMaterial( new visBase::Material );
    
    return material_;
}


void VisualObjectImpl::removeSwitch()
{
    pErrMsg( "Don't call");
}


osg::Node* VisualObjectImpl::gtOsgNode()
{
    return osgroot_;
}
    
    
void VisualObjectImpl::addChild( SoNode* nn )
{  }


void VisualObjectImpl::insertChild( int pos, SoNode* nn )
{  }


void VisualObjectImpl::removeChild( SoNode* nn )
{  }


int VisualObjectImpl::childIndex( const SoNode* nn ) const
{ return -1;}


int VisualObjectImpl::addChild( osg::Node* nn )
{
    if ( !nn )
	return -1;
    
    return osgroot_->addChild( nn );
}


void VisualObjectImpl::insertChild( int pos, osg::Node* nn )
{
    osgroot_->insertChild( pos, nn );
}


void VisualObjectImpl::removeChild( osg::Node* nn )
{
    osgroot_->removeChild( nn );
}


int VisualObjectImpl::childIndex( const osg::Node* nn ) const
{
    const int idx = osgroot_->getChildIndex(nn);
    if ( idx==osgroot_->getNumChildren() )
	return -1;
    return idx;
}


SoNode* VisualObjectImpl::getChild(int idx)
{ return 0; }


int VisualObjectImpl::usePar( const IOPar& iopar )
{
    if ( material_ )
    {
	PtrMan<IOPar> matpar = iopar.subselect( sKeyMaterial() );
	if ( matpar )
	    material_->usePar( *matpar );
    }
    
    bool isonsw;
    if ( iopar.getYN(sKeyIsOn(),isonsw) )
	turnOn( isonsw );

    return 1;
}


void VisualObjectImpl::fillPar( IOPar& iopar ) const
{
    if ( material_ )
    {
	IOPar materialpar;
	material_->fillPar( materialpar );
	iopar.mergeComp( materialpar, sKeyMaterial() );
	
    }
    
    iopar.setYN( sKeyIsOn(), isOn() );
}


void VisualObject::triggerRightClick( const EventInfo* eventinfo )
{
    rcevinfo = eventinfo;
    rightClick.trigger();
}


bool VisualObject::getBoundingBox( Coord3& minpos, Coord3& maxpos ) const
{
    pErrMsg( "Not impl. Not sure if needed." );
    return false;
    /*
    SbViewportRegion vp;
    SoGetBoundingBoxAction action( vp );
    action.apply( const_cast<SoNode*>(getInventorNode()) );
    const SbBox3f bbox = action.getBoundingBox();

    if ( bbox.isEmpty() )
	return false;

    const SbVec3f min = bbox.getMin();
    const SbVec3f max = bbox.getMax();

    minpos.x = min[0]; minpos.y = min[1]; minpos.z = min[2];
    maxpos.x = max[0]; maxpos.y = max[1]; maxpos.z = max[2];

    const Transformation* trans =
	const_cast<VisualObject*>(this)->getDisplayTransformation();
    if ( trans )
    {
	minpos = trans->transformBack( minpos );
	maxpos = trans->transformBack( maxpos );
    }

    return true;
     */
}


const TypeSet<int>* VisualObject::rightClickedPath() const
{
    return rcevinfo ? &rcevinfo->pickedobjids : 0;
}

}; //namespace
