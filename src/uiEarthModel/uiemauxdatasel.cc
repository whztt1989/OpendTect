/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2010
________________________________________________________________________

-*/

#include "uiemauxdatasel.h"

#include "uidialog.h"
#include "uiiosurface.h"

#include "bufstringset.h"
#include "keystrs.h"
#include "emsurfacetr.h"
#include "dbman.h"
#include "ptrman.h"

uiEMAuxDataSel::uiEMAuxDataSel( uiParent* p, const uiString& lbl,
				const DBKey* hormid, const char* auxdatanm )
    : uiCompoundParSel( p, lbl )
    , hormid_(hormid ? *hormid : DBKey::getInvalid())
    , auxdatanm_(auxdatanm)
{
    butPush.notify( mCB(this,uiEMAuxDataSel,butPushCB) );
}


bool uiEMAuxDataSel::butPushCB( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup(tr("Horizon/Attributes Selection"),
                                        uiStrings::sEmptyString(),mNoHelpKey) );
    PtrMan<uiSurfaceRead> surfacefld = new uiSurfaceRead( &dlg,
		  uiSurfaceRead::Setup(EMHorizon3DTranslatorGroup::sGroupName())
				 .withsectionfld(false)
				 .multiattribsel(false) );
    surfacefld->setInput( hormid_ );
    BufferStringSet attribname;
    attribname.add( auxdatanm_ );
    surfacefld->setSelAttributes( attribname );

    if ( !dlg.go() )
	return false;

    if ( surfacefld->selIOObj() )
	return false;

    hormid_ = surfacefld->selIOObj()->key();
    BufferStringSet selattribs;
    surfacefld->getSelAttributes( selattribs );
    if ( selattribs.size() <=0 )
	return false;

    auxdatanm_ = selattribs.get( 0 );
    return true;
}


const DBKey& uiEMAuxDataSel::getSurfaceID() const
{ return hormid_; }


const char* uiEMAuxDataSel::getAuxDataSel() const
{ return auxdatanm_; }


BufferString uiEMAuxDataSel::getSummary() const
{
    PtrMan<IOObj> ioobj = DBM().get( hormid_ );
    if ( !ioobj )
	return "";

    BufferString summary( ioobj->name() );
    summary.add( "[" ).add( auxdatanm_ ).add( "]" );
    return summary;
}
