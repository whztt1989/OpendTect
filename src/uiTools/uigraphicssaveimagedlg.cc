/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uigraphicssaveimagedlg.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uigraphicsscene.h"
#include "uimain.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uispinbox.h"

#include "iopar.h"
#include "filepath.h"
#include "settings.h"


uiGraphicsSaveImageDlg::uiGraphicsSaveImageDlg( uiParent* p,
	uiGraphicsScene* scene )
    : uiSaveImageDlg(p)
    , scene_(scene)
{
    screendpi_ = mCast( float, uiMain::getDPI() );
    createGeomInpFlds( cliboardselfld_ );
    fileinputfld_->attach( alignedBelow, dpifld_ );

    PtrMan<IOPar> ctiopar;
    getSettingsPar( ctiopar, BufferString("2D") );
    if ( ctiopar )
    {
	if ( !usePar(*ctiopar) )
	{
	    useparsfld_->setValue( false );
	    setFldVals( 0 );
	    updateSizes();
	}
    }
    else
    {
	useparsfld_->setValue( false );
	setFldVals( 0 );
    }

    postFinalise().notify( mCB(this,uiGraphicsSaveImageDlg,setAspectRatio) );
    updateFilter();
    unitChg(0);
    lockfld_->setChecked( true );
    lockfld_->setSensitive( false );
}


void uiGraphicsSaveImageDlg::getSupportedFormats( const char** imagefrmt,
						  const char** frmtdesc,
						  BufferString& filters )
{
    BufferStringSet supportedformats;
    supportedImageFormats( supportedformats );
    int idy = 0;
    while ( imagefrmt[idy] )
    {
	if ( supportedformats.isPresent( imagefrmt[idy] ) )
	{
	    if ( !filters.isEmpty() ) filters += ";;";
	    filters += frmtdesc[idy];
	}
	idy++;
    }

    filters += ";;PDF (*.pdf);;PostScript (*.ps)";
    filters_ = filters;
}


const char* uiGraphicsSaveImageDlg::getExtension()
{
    FilePath fp( fileinputfld_->fileName() );
    const BufferString ext( fp.extension() );
    if ( ext == "pdf"
      || FixedString(fileinputfld_->selectedFilter())
			.startsWith("PDF",CaseInsensitive) )
	return "pdf";

    return uiSaveImageDlg::getExtension();
}


void uiGraphicsSaveImageDlg::setAspectRatio( CallBacker* )
{ aspectratio_ = (float) ( scene_->width() / scene_->height() ); }


bool uiGraphicsSaveImageDlg::acceptOK( CallBacker* )
{
    if ( cliboardselfld_->isChecked() )
    {
	scene_->copyToClipBoard();
	return true;
    }

    if ( !filenameOK() ) return false;
    BufferString ext( getExtension() );
    if ( ext == "pdf" )
	scene_->saveAsPDF(fileinputfld_->fileName(),(int)sizepix_.width(),
			  (int)sizepix_.height(),dpifld_->box()->getValue());
    else if ( ext == "ps" || ext == "eps" )
	scene_->saveAsPS( fileinputfld_->fileName(),(int)sizepix_.width(),
			  (int)sizepix_.height(),dpifld_->box()->getValue());
    else
	scene_->saveAsImage( fileinputfld_->fileName(), (int)sizepix_.width(),
			     (int)sizepix_.height(),dpifld_->box()->getValue());

    if ( saveButtonChecked() )
	writeToSettings();
    return true;
}


void uiGraphicsSaveImageDlg::writeToSettings()
{
    IOPar iopar;
    fillPar( iopar, true );
    settings_.mergeComp( iopar, "2D" );
    if ( !settings_.write() )
	uiMSG().error( tr("Cannot write settings") );
}


void uiGraphicsSaveImageDlg::setFldVals( CallBacker* cb )
{
    if ( useparsfld_->getBoolValue() )
    {
	PtrMan<IOPar> ctiopar;
	getSettingsPar( ctiopar, BufferString("2D") );
	if ( ctiopar.ptr() )
	{
	    if ( !usePar(*ctiopar) )
		useparsfld_->setValue( false );
	}
	aspectratio_ = (float) widthfld_->box()->getFValue() /
			       heightfld_->box()->getFValue();
    }
    else
    {
	aspectratio_ = (float) ( scene_->width() / scene_->height() );
	dpifld_->box()->setValue( screendpi_ );
	setSizeInPix( (int)scene_->width(), (int)scene_->height() );
    }
}
