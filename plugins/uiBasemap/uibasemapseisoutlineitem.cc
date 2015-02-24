/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		November 2014
 RCS:		$Id$
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibasemapseisoutlineitem.h"

#include "basemapseisoutline.h"

#include "uimenu.h"
#include "uisellinest.h"
#include "uistrings.h"
#include "draw.h"
#include "seistrctr.h"


// uiBasemapOutlineGroup
uiBasemapSeisOutlineGroup::uiBasemapSeisOutlineGroup( uiParent* p, bool isadd )
    : uiBasemapIOObjGroup(p,*Seis::getIOObjContext(Seis::Vol,true),isadd)
{
    uiSelLineStyle::Setup stu; stu.drawstyle( false );
    LineStyle lst;
    lsfld_ = new uiSelLineStyle( this, lst, stu );
    lsfld_->attach( alignedBelow, uiBasemapIOObjGroup::lastObject() );

    addNameField();
}


uiBasemapSeisOutlineGroup::~uiBasemapSeisOutlineGroup()
{
}


bool uiBasemapSeisOutlineGroup::acceptOK()
{
    const bool res = uiBasemapIOObjGroup::acceptOK();
    return res;
}


bool uiBasemapSeisOutlineGroup::fillItemPar( int idx, IOPar& par ) const
{
    const bool res = uiBasemapIOObjGroup::fillItemPar( idx, par );

    BufferString lsstr;
    lsfld_->getStyle().toString( lsstr );
    par.set( sKey::LineStyle(), lsstr );

    return res;
}


bool uiBasemapSeisOutlineGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapIOObjGroup::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls; ls.fromString( lsstr );
    lsfld_->setStyle( ls );

    return res;
}


uiObject* uiBasemapSeisOutlineGroup::lastObject()
{ return lsfld_->attachObj(); }



// uiBasemapSeisOutlineTreeItem
uiBasemapSeisOutlineTreeItem::uiBasemapSeisOutlineTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{
}


uiBasemapSeisOutlineTreeItem::~uiBasemapSeisOutlineTreeItem()
{
}


bool uiBasemapSeisOutlineTreeItem::usePar( const IOPar& par )
{
    uiBasemapTreeItem::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );

    int nrobjs = 0;
    par.get( uiBasemapGroup::sKeyNrObjs(), nrobjs );

    while ( nrobjs < basemapobjs_.size() )
	delete removeBasemapObject( *basemapobjs_[0] );

    for ( int idx=0; idx<nrobjs; idx++ )
    {
	MultiID mid;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    continue;

	if ( basemapobjs_.validIdx(idx) )
	{
	    mDynamicCastGet(Basemap::SeisOutlineObject*,obj,basemapobjs_[idx])
	    if ( obj )
	    {
		obj->setMultiID( mid );
		obj->setLineStyle( ls );
		obj->updateGeometry();
	    }
	}
	else
	{
	    Basemap::SeisOutlineObject* obj =
		new Basemap::SeisOutlineObject( mid );
	    obj->setLineStyle( ls );
	    addBasemapObject( *obj );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapSeisOutlineTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapSeisOutlineTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
	BMM().edit( getFamilyID(), ID() );
    else
	return false;

    return true;
}



// uiBasemapSeisOutlineItem
const char* uiBasemapSeisOutlineItem::iconName() const
{ return "cube_z"; }

uiBasemapGroup* uiBasemapSeisOutlineItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapSeisOutlineGroup( p, isadd ); }

uiBasemapTreeItem* uiBasemapSeisOutlineItem::createTreeItem( const char* nm )
{ return new uiBasemapSeisOutlineTreeItem( nm ); }