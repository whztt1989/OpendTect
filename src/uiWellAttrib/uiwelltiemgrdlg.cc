/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelltiemgrdlg.h"

#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seistrctr.h"
#include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "wavelet.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltransl.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"
#include "welltiegeocalculator.h"
#include "wellreader.h"
#include "welld2tmodel.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiseiswvltsel.h"
#include "uiseparator.h"
#include "uiwaveletextraction.h"
#include "uiwellpropertyrefsel.h"
#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecheckshotedit.h"


namespace WellTie
{

uiTieWinMGRDlg::uiTieWinMGRDlg( uiParent* p, WellTie::Setup& wtsetup )
	: uiDialog(p,uiDialog::Setup("Tie Well To Seismics",
		"Select Data to tie Well to Seismic","107.4.0")
		.savetext("Save as default")
		.savebutton(true)
		.savechecked(false)
		.modal(false))
	, wtsetup_(wtsetup)
	, wllctio_(*mMkCtxtIOObj(Well))
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, seisctio2d_(*uiSeisSel::mkCtxtIOObj(Seis::Line,true))
	, seisctio3d_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,true))
	, seis2dfld_(0)
	, seis3dfld_(0)
	, seislinefld_(0)
	, seisextractfld_(0)
	, typefld_(0)
	, extractwvltdlg_(0)
	, wd_(0)
{
    setCtrlStyle( DoAndStay );

    const IOObjContext wellctxt = mIOObjContext(Well);
    wellfld_ = new uiIOObjSel( this, wellctxt );
    if ( !wtsetup_.wellid_.isEmpty() )
	wellfld_->setInput( wtsetup_.wellid_ );

    wellfld_->selectionDone.notify( mCB(this,uiTieWinMGRDlg,wellSelChg) );

    uiSeparator* sep = new uiSeparator( this, "Well2Seismic Sep" );
    sep->attach( stretchedBelow, wellfld_ );

    uiGroup* seisgrp = new uiGroup( this, "Seismic selection group" );
    seisgrp->attach( ensureBelow, sep );
    seisgrp->attach( alignedBelow, wellfld_ );

    const bool has2d = SI().has2D();
    const bool has3d = SI().has3D();

    if ( has2d && has3d )
    {
	BufferStringSet seistypes;
	seistypes.add( Seis::nameOf(Seis::Line) );
	seistypes.add( Seis::nameOf(Seis::Vol) );
	typefld_ = new uiGenInput( seisgrp, "Seismic",
				   StringListInpSpec( seistypes ) );
	typefld_->setValue( true );
	typefld_->valuechanged.notify( mCB(this,uiTieWinMGRDlg,typeSelChg) );
    }

    if ( has2d )
    {
	uiSeisSel::Setup seis2dfldsetup = uiSeisSel::Setup(Seis::Line);
	seis2dfldsetup.allowsetsurvdefault_ = true;
	seis2dfldsetup.optional_ = true;
	const IOObjContext seis2dctxt =
			uiSeisSel::ioContext( seis2dfldsetup.geom_, true );
	seis2dfld_ = new uiSeisSel( seisgrp, seis2dctxt, seis2dfldsetup );
	seis2dfld_->setChecked( true );
	seis2dfld_->display( !has3d );
	if ( typefld_ )
	    seis2dfld_->attach( alignedBelow, typefld_ );

	seis2dfld_->selectionDone.notify( mCB(this,uiTieWinMGRDlg,seisSelChg) );
	seis2dfld_->optionalChecked.notify(
				      mCB(this,uiTieWinMGRDlg,seis2DCheckChg) );

	seislinefld_ = new uiSeis2DLineNameSel( seisgrp, true );
	seislinefld_->display( !has3d );
	if ( !seis2dfld_->key().isEmpty() )
	    seislinefld_->setLineSet( seis2dfld_->key() );
	else
	    set2DSeis(); //required if survey type is both 2D/3D

	seislinefld_->attach( alignedBelow, seis2dfld_ );
    }

    if ( has3d )
    {
	uiSeisSel::Setup seis3dfldsetup = uiSeisSel::Setup(Seis::Vol);
	seis3dfldsetup.allowsetsurvdefault_ = true;
	seis3dfldsetup.optional_ = true;
	const IOObjContext seis3dctxt =
			uiSeisSel::ioContext( seis3dfldsetup.geom_, true );
	seis3dfld_ = new uiSeisSel( seisgrp, seis3dctxt, seis3dfldsetup );
	seis3dfld_->setChecked( true );
	if ( typefld_ )
	    seis3dfld_->attach( alignedBelow, typefld_ );

	seis3dfld_->selectionDone.notify( mCB(this,uiTieWinMGRDlg,seisSelChg) );
    }

    seisgrp->setHAlignObj( typefld_ ? (uiGroup*)typefld_
				    : (uiGroup*)seis2dfld_ ?
				    seis2dfld_ : seis3dfld_ );

    sep = new uiSeparator( this, "Seismic2Log Sep" );
    sep->attach( stretchedBelow, seisgrp );

    uiGroup* logsgrp = new uiGroup( this, "Log selection group" );
    logsgrp->attach( alignedBelow, wellfld_ );
    logsgrp->attach( ensureBelow, sep );

    logsfld_ = new uiWellElasticPropSel( logsgrp );
    logsfld_->setAltPropRefPreferred( true );

    used2tmbox_ = new uiCheckBox( logsgrp, "Use existing depth/time model");
    used2tmbox_->activated.notify( mCB(this, uiTieWinMGRDlg, d2TSelChg ) );
    used2tmbox_->attach( alignedBelow, logsfld_ );

    const char** corrs = WellTie::Setup::CorrTypeNames();
    cscorrfld_ = new uiLabeledComboBox( logsgrp, corrs,
					WellTie::Setup::sKeyCSCorrType());
    cscorrfld_->attach( alignedBelow, used2tmbox_ );
    logsgrp->setHAlignObj( cscorrfld_ );

    sep = new uiSeparator( this, "Logs2Wavelt Sep" );
    sep->attach( stretchedBelow, logsgrp );

    wvltfld_ = new uiSeisWaveletSel( this, "Reference wavelet" );
    wvltfld_->attach( alignedBelow, wellfld_ );
    wvltfld_->attach( ensureBelow, sep );
    uiPushButton* crwvltbut = new uiPushButton( this, "Extract",
				mCB(this,uiTieWinMGRDlg,extrWvlt), false );
    crwvltbut->attach( rightOf, wvltfld_ );

    postFinalise().notify( mCB(this,uiTieWinMGRDlg,onFinalise) );
}


uiTieWinMGRDlg::~uiTieWinMGRDlg()
{
    delWins();
    if ( extractwvltdlg_ )
	delete extractwvltdlg_;

    delete &wtsetup_;
    delete wllctio_.ioobj; delete &wllctio_;
    delete wvltctio_.ioobj; delete &wvltctio_;
    delete seisctio3d_.ioobj; delete &seisctio3d_;
    delete seisctio2d_.ioobj; delete &seisctio2d_;
}


void uiTieWinMGRDlg::onFinalise( CallBacker* )
{
    wellSelChg( 0 );
}


void uiTieWinMGRDlg::delWins()
{
    deepErase( welltiedlgsetcpy_ );
}


bool uiTieWinMGRDlg::selIs2D() const
{
    if ( typefld_ )
	return !typefld_->getIntValue();

    return SI().has2D();
}


void uiTieWinMGRDlg::wellSelChg( CallBacker* cb )
{
    const IOObj* wellobj = wellfld_->ioobj( true );
    if ( !wellobj ) return;

    const char* wllfilenm = Well::IO::getMainFileName( *wellobj );
    const MultiID& wellid = wellobj->key();
    logsfld_->wellid_ = wellid;

    wd_ = Well::MGR().get( wellid, false );
    if ( !wd_ || !logsfld_->setLogs(wd_->logs()) )
    {
	if ( !cb ) // Probably the default well at startup.
	    wellfld_->setEmpty();
	else
	{
	    BufferString errmsg = "This well has no valid log to use as input";
	    errmsg += "\n";
	    errmsg += "Use well manager to either import or create your logs";
	    uiMSG().error( errmsg );
	}

	return;
    }

    const bool canuseexistingd2t = wd_->d2TModel() && !mIsUnvalidD2TM((*wd_));
    used2tmbox_->display( canuseexistingd2t );
    used2tmbox_->setChecked( canuseexistingd2t );

    getSetup( wllfilenm );
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return; }

void uiTieWinMGRDlg::typeSelChg( CallBacker* )
{
    const bool is2d = selIs2D();
    if ( seis3dfld_ )
	seis3dfld_->display( !is2d );

    if ( seis2dfld_ )
	seis2dfld_->display( is2d );

    if ( seislinefld_ )
	seislinefld_->display( is2d );
}


void uiTieWinMGRDlg::seisSelChg( CallBacker* )
{
    const bool is2d = selIs2D();
    mDynamicCastGet( uiSeisSel*, seisfld, is2d ? seis2dfld_ : seis3dfld_ );
    if ( !seisfld )
	mErrRet( "Please select a seismic type" )

    if ( seisfld->isChecked() )
    {
	const MultiID& seisid = seisfld->key();
	if ( seisid.isEmpty() )
	    mErrRet("Please select the input seimic data")

	if ( is2d && seislinefld_ )
	    seislinefld_->setLineSet( seisid );
    }
}


void uiTieWinMGRDlg::seis2DCheckChg( CallBacker* )
{
    seislinefld_->display( seis2dfld_->isChecked() );
}


void uiTieWinMGRDlg::d2TSelChg( CallBacker* )
{
    const bool useexistingmdl = used2tmbox_->isChecked();
    const bool havecs = wd_ && wd_->haveCheckShotModel();
    cscorrfld_->display( !useexistingmdl && havecs );
}


void uiTieWinMGRDlg::extrWvlt( CallBacker* )
{
    if ( !extractwvltdlg_ )
	extractwvltdlg_ = new uiWaveletExtraction( 0, selIs2D() );
    extractwvltdlg_->extractionDone.notify(
				mCB(this,uiTieWinMGRDlg,extractWvltDone) );
    extractwvltdlg_->show();
}


void uiTieWinMGRDlg::extractWvltDone( CallBacker* )
{
    wvltfld_->setInput( extractwvltdlg_->storeKey() );
}

#undef mErrRet
#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }


void uiTieWinMGRDlg::getSetup( const char* nm )
{
    WellTie::Reader wtr( nm );
    wtr.getWellTieSetup( wtsetup_ );
    getSeismicInSetup();
    getVelLogInSetup();
    getDenLogInSetup();

    if ( !wtsetup_.wvltid_.isEmpty() )
	wvltfld_->setInput( wtsetup_.wvltid_ );

    if ( !wtsetup_.useexistingd2tm_ )
    {
	used2tmbox_->setChecked(false);
	const bool havecs = wd_->haveCheckShotModel();
	cscorrfld_->display( havecs );
	cscorrfld_->box()->setCurrentItem( wtsetup_.corrtype_ );
    }

    d2TSelChg(0);
}


bool uiTieWinMGRDlg::getSeismicInSetup()
{
    if ( !wtsetup_.seisid_.isEmpty() )
    {
	const bool idinsetupis2d = !seisIDIs3D( wtsetup_.seisid_ );
	const bool surveyhastype = idinsetupis2d ? SI().has2D() : SI().has3D();
	if ( !surveyhastype )
	{
	    BufferString errmsg;
	    errmsg  = "Stored setup contains seismic of another type\n";
	    errmsg += "than the survey.\n";
	    errmsg += "Change the survey type to 2D/3D.\n";
	    errmsg += "Or select a new dataset.";
	    mErrRet( errmsg );
	}

	if ( typefld_ )
	{
	    typefld_->setValue( !idinsetupis2d );
	    typeSelChg(0);
	}

	mDynamicCastGet( uiSeisSel*, seisfld,
			 idinsetupis2d ? seis2dfld_ : seis3dfld_ );
	if ( seisfld )
	{
	    seisfld->setInput( wtsetup_.seisid_ );
	    if ( seisfld->key().isEmpty() )
	    {
		seisfld->setEmpty();
		mErrRet("Cannot restore the seismic data from the setup")
	    }

	    if ( idinsetupis2d && seislinefld_ &&
		 !wtsetup_.linekey_.lineName().isEmpty() )
	    {
		seislinefld_->setLineSet( wtsetup_.seisid_ );
		seislinefld_->setInput( wtsetup_.linekey_.lineName() );
	    }
	}
    }

    return true;
}


#define mPwaveIdx 1
bool uiTieWinMGRDlg::getVelLogInSetup() const
{
    if ( !wtsetup_.vellognm_.isEmpty() )
    {
	Well::Log* vp = wd_->logs().getLog( wtsetup_.vellognm_ );
	if ( !vp )
	{
	    BufferString errmsg = "Cannot retrieve the velocity log ";
	    errmsg += wtsetup_.vellognm_;
	    errmsg += " stored in the setup.";
	    mErrRet( errmsg );
	}

	const UnitOfMeasure* velpuom = vp->unitOfMeasure();
	const PropertyRef::StdType tp = PropertyRef::Vel;
	const bool reverted = wtsetup_.issonic_;
	logsfld_->setLog( tp, wtsetup_.vellognm_, reverted, velpuom, mPwaveIdx);
    }

    return true;
}


#define mDensityIdx 0
bool uiTieWinMGRDlg::getDenLogInSetup() const
{
    if ( !wtsetup_.denlognm_.isEmpty() )
    {
	Well::Log* den = wd_->logs().getLog( wtsetup_.denlognm_ );
	if ( !den )
	{
	    BufferString errmsg = "Cannot retrieve the density log ";
	    errmsg += wtsetup_.denlognm_;
	    errmsg += " stored in the setup.";
	    mErrRet( errmsg );
	}

	const UnitOfMeasure* denuom = den->unitOfMeasure();
	const PropertyRef::StdType tp = PropertyRef::Den;
	const bool reverted = false;
	logsfld_->setLog( tp, wtsetup_.denlognm_, reverted, denuom,mDensityIdx);
    }

    return true;
}


void uiTieWinMGRDlg::saveWellTieSetup( const MultiID& key,
				      const WellTie::Setup& wts ) const
{
    WellTie::Writer wtr( Well::IO::getMainFileName(key) );
    if ( !wtr.putWellTieSetup( wts ) )
	uiMSG().error( "Could not write parameters" );
}



#undef mErrRet
#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }


bool uiTieWinMGRDlg::initSetup()
{
    if ( !wellfld_->commitInput() )
	mErrRet("Please select a valid well")

    const MultiID& wellid = wellfld_->ctxtIOObj().ioobj->key();
    wd_ = Well::MGR().get( wellid, false );
    if ( !wd_ )
	mErrRet("Cannot read the well data")

    for ( int idx=0; idx<welltiedlgset_.size(); idx++ )
    {
	uiTieWin* win = welltiedlgset_[idx];
	if ( win->Setup().wellid_ == wellid )
	    mErrRet( "A window with this well is already opened" )
    }
    wtsetup_.wellid_ = wellid;

    const bool is2d = selIs2D();
    mDynamicCastGet( uiSeisSel*, seisfld, is2d ? seis2dfld_ : seis3dfld_ );
    if ( !seisfld )
	mErrRet( "Please select a seismic type" )

    if ( seisfld->isChecked() )
    {
	const MultiID& seisid = seisfld->key();
	if ( seisid.isEmpty() )
	    mErrRet("Please select the input seimic data")
	    // msg required because the seismic is optional

	wtsetup_.seisid_ = seisid;
	if ( is2d )
	{
	    wtsetup_.linekey_.setAttrName( seis2dfld_->attrNm() );
	    wtsetup_.linekey_.setLineName( seislinefld_->getInput() );
	}
	else
	    wtsetup_.linekey_.setEmpty();
    }
    else
    {
	wtsetup_.seisid_ = 0;
	if ( is2d )
	    wtsetup_.linekey_.setEmpty();
    }

    if ( !logsfld_->isOK() )
	mErrRet( "Cannot select appropriate logs" )

    uiPropSelFromList* psflden = logsfld_->
				 getPropSelFromListByIndex( mDensityIdx );
    if ( !psflden )
	mErrRet( "Cannot find the density in the log selection list" )

    Well::Log* den = wd_->logs().getLog( psflden->text() );
    if ( !den )
	mErrRet( "Could not extract this density log" )

    if ( !psflden->uom() )
	mErrRet( "Please select a unit for the density log" )

    den->setUnitMeasLabel( psflden->uom()->symbol() );
    wtsetup_.denlognm_ = psflden->text();

    uiPropSelFromList* psflvp = logsfld_->
				getPropSelFromListByIndex( mPwaveIdx );
    if ( !psflvp )
	mErrRet( "Cannot find the Pwave in the log selection list" )

    Well::Log* vp = wd_->logs().getLog( psflvp->text() );
    if ( !vp )
	mErrRet( "Could not extract this velocity log" )

    if ( !psflvp->uom() )
	mErrRet( "Please select a unit for the velocity log" )

    vp->setUnitMeasLabel( psflvp->uom()->symbol() );
    wtsetup_.vellognm_ = psflvp->text();
    wtsetup_.issonic_  = psflvp->isUseAlternate();

    wtsetup_.useexistingd2tm_ = used2tmbox_->isChecked();
    WellTie::Setup::parseEnumCorrType( cscorrfld_->box()->text(),
				       wtsetup_.corrtype_ );

    if ( !wvltfld_->getWavelet() )
	mErrRet("Please select a valid wavelet")

    wtsetup_.wvltid_ = wvltfld_->getID();

    wtsetup_.commitDefaults();
    if ( saveButtonChecked() )
	saveWellTieSetup( wtsetup_.wellid_, wtsetup_ );

    return true;
}


bool uiTieWinMGRDlg::acceptOK( CallBacker* )
{
    if ( !initSetup() )
	return false;

    Server* server = new Server( wtsetup_ );
    if ( server->errMSG() )
	{ uiMSG().error( server->errMSG() ); delete server; return false; }

    if ( wtsetup_.corrtype_ == WellTie::Setup::UserDefined )
    {
	uiCheckShotEdit dlg( this, *server );
	if ( !dlg.go() )
	    { delete server; return false; }
    }

    WellTie::uiTieWin* wtdlg = new WellTie::uiTieWin( this, *server );
    welltiedlgset_ += wtdlg;
    welltiedlgsetcpy_ += wtdlg;
    //windows are stored in a an ObjectSet to be deleted in the destructor
    wtdlg->windowClosed.notify( mCB(this,uiTieWinMGRDlg,wellTieDlgClosed) );

    PtrMan<IOObj> ioobj = IOM().get( wtsetup_.wellid_ );
    if ( !ioobj ) return false;

    const BufferString fname( ioobj->fullUserExpr(true) );
    WellTie::Reader wtr( fname );
    IOPar* par= wtr.getIOPar( uiTieWin::sKeyWinPar() );
    if ( par ) wtdlg->usePar( *par );
    delete par;

    return false;
}


void uiTieWinMGRDlg::wellTieDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(WellTie::uiTieWin*,win,cb);
    const int idx = welltiedlgset_.indexOf( win );
    if ( !win || idx<0 ) return;

    WellTie::Writer wtr( Well::IO::getMainFileName(win->Setup().wellid_) );
    IOPar par; win->fillPar( par );
    wtr.putIOPar( par, uiTieWin::sKeyWinPar() );

    welltiedlgset_.removeSingle( idx );
}

#define mGetPar(key) \
    defpars->find(SeisTrcTranslatorGroup::key())

void uiTieWinMGRDlg::set2DSeis() const
{
    if ( !seis2dfld_->isEmpty() )
	return;

    PtrMan<IOPar> defpars = SI().pars().subselect( sKey::Default() );
    const FixedString lsid = defpars ? mGetPar( sKeyDefault2D ) : 0;
    const MultiID& outid = lsid;

    BufferString lineidstr;
    PtrMan<IOObj> lsobj = IOM().get( MultiID(outid) );
    if ( !lsobj )
	return;

    BufferString attrnm = defpars ? mGetPar( sKeyDefaultAttrib ) : 0;
    if ( lsobj && attrnm.isEmpty() )
    {
	SeisIOObjInfo seisinfo( lsobj );
	BufferStringSet attrnms;
	SeisIOObjInfo::Opts2D o2d; o2d.steerpol_ = 0;
	seisinfo.getAttribNames( attrnms, o2d );
	if ( !attrnms.isEmpty() )
	    attrnm = attrnms.get(0);
    }

    lineidstr = LineKey( outid, attrnm );
    const MultiID& seisid = lineidstr.buf();
    seis2dfld_->setInput( seisid );
    if ( seis2dfld_->key().isEmpty() )
	return;

    seislinefld_->setLineSet( seisid );
}


bool uiTieWinMGRDlg::seisIDIs3D( MultiID seisid ) const
{
    PtrMan<IOObj> ioobj = IOM().get( seisid );
    SeisTrcReader rdr( ioobj );

    return !rdr.is2D();
}

}; //namespace
