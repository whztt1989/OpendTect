/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/


#include "uigoogleexpwells.h"
#include "googlexmlwriter.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "oddirs.h"
#include "dbdir.h"
#include "ioobj.h"
#include "strmprov.h"
#include "survinfo.h"
#include "welltransl.h"
#include "welldata.h"
#include "welltrack.h"
#include "wellreader.h"
#include "dbman.h"
#include "latlong.h"
#include "od_ostream.h"
#include "od_helpids.h"


uiGoogleExportWells::uiGoogleExportWells( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport( tr("Wells to KML")),
				 tr("Specify wells to output"),
				 mODHelpKey(mGoogleExportWellsHelpID)))
{
    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Well(s)") );
    selfld_ = new uiListBox( this, su );

    mImplFileNameFld("wells");
    fnmfld_->attach( alignedBelow, selfld_ );

    preFinalise().notify( mCB(this,uiGoogleExportWells,initWin) );
}


uiGoogleExportWells::~uiGoogleExportWells()
{
}


void uiGoogleExportWells::initWin( CallBacker* )
{
    const DBDirEntryList del( WellTranslatorGroup::ioContext() );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	selfld_->addItem( mToUiStringTodo(del.dispName(idx)) );
	wellids_ += del.key( idx );
    }
    selfld_->chooseAll( true );
}


bool uiGoogleExportWells::acceptOK()
{
    mCreateWriter( "Wells", SI().name() );

    wrr.writeIconStyles( "wellpin", 20 );

    for ( int idx=0; idx<selfld_->size(); idx++ )
    {
	if ( !selfld_->isChosen(idx) )
	    continue;

	RefMan<Well::Data> wd = new Well::Data;
	Well::Reader wllrdr( wellids_[idx], *wd );
	Coord coord;
	if ( !wllrdr.getMapLocation(coord) )
	    continue;

	wrr.writePlaceMark( "wellpin", coord, selfld_->textOfItem(idx) );
	if ( !wrr.isOK() )
	    { uiMSG().error(wrr.errMsg()); return false; }
    }

    wrr.close();
    return true;
}
