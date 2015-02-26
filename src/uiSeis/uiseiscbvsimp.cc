/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Jun 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseiscbvsimp.h"

#include "ctxtioobj.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "ptrman.h"
#include "scaler.h"
#include "seiscbvs.h"
#include "seisread.h"
#include "seisselection.h"
#include "seisselectionimpl.h"
#include "seissingtrcproc.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "veldesc.h"
#include "velocitycalc.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseisioobjinfo.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiseistransf.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p )
	: uiDialog(p,Setup("Import CBVS cube",
			   "Specify import parameters",
			   mODHelpKey(mSeisImpCBVSHelpID) ))
	, initialinpioobj_(0)
	, outioobj_(0)
	, modefld_(0)
	, sstp_(0)
{
    setCtrlStyle( RunAndClose );

    init( false );
    modeSel(0);
}


uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p, const IOObj* ioobj )
	: uiDialog(p,Setup("Copy cube data",
			   "Specify copy parameters",
			   mODHelpKey(mSeisImpCBVSCopyHelpID) ))
	, initialinpioobj_(ioobj ? ioobj->clone() : 0)
	, outioobj_(0)
	, modefld_(0)
	, sstp_(0)
{
    setCtrlStyle( RunAndClose );

    init( true );
    oinpSel(0);
}


void uiSeisImpCBVS::init( bool fromioobj )
{
    finpfld_ = 0; modefld_ = typefld_ = 0; oinpfld_ = 0; convertfld_ = 0;
    compfld_ = 0;
    ismc_ = false;
    setTitleText( fromioobj ? "Specify transfer parameters"
			    : "Create CBVS cube definition" );
    tmpid_ = "100010."; tmpid_ += IOObj::tmpID();

    uiSeisTransfer::Setup sts( Seis::Vol );
    uiGroup* attobj = 0;
    if ( fromioobj )
    {
	IOObjContext inctxt( uiSeisSel::ioContext( Seis::Vol, true ) );
	uiSeisSel::Setup sssu( Seis::Vol );
	sssu.steerpol( uiSeisSel::Setup::InclSteer );
	oinpfld_ = new uiSeisSel( this, inctxt, sssu );
	oinpfld_->selectionDone.notify( mCB(this,uiSeisImpCBVS,oinpSel) );
	compfld_ = new uiLabeledComboBox( this, "Component(s)" );
	attobj = compfld_;
	compfld_->attach( alignedBelow, oinpfld_ );
	if ( initialinpioobj_ )
	{
	    oinpfld_->setInput( initialinpioobj_->key() );
	    SeisIOObjInfo oinf( *initialinpioobj_ );
	    sts.zdomkey_ = oinf.zDomainDef().key();
	    if ( sts.zdomkey_ != ZDomain::SI().key() )
		oinpfld_->setSensitive( false );
	}
    }
    else
    {
	uiFileInput::Setup fisu( uiFileDialog::Gen );
	fisu.filter("CBVS (*.cbvs)").defseldir( GetBaseDataDir() );
	finpfld_ = new uiFileInput( this, "(First) CBVS file name", fisu );
	finpfld_->valuechanged.notify( mCB(this,uiSeisImpCBVS,finpSel) );

	StringListInpSpec spec;
	spec.addString( "Input data cube" );
	spec.addString( "Generated attribute cube" );
	spec.addString( "SteeringCube" );
	typefld_ = new uiGenInput( this, "Cube type", spec );
	typefld_->attach( alignedBelow, finpfld_ );
	typefld_->valuechanged.notify( mCB(this,uiSeisImpCBVS,typeChg) );

	modefld_ = new uiGenInput( this, "Import mode",
			  BoolInpSpec(false,"Copy the data","Use in-place") );
	modefld_->attach( alignedBelow, typefld_ );
	modefld_->valuechanged.notify( mCB(this,uiSeisImpCBVS,modeSel) );
	attobj = modefld_;

	convertfld_ = new uiCheckBox( this,
		"Convert underscores to spaces in Output Cube name",
		mCB(this,uiSeisImpCBVS,convertSel) );
    }

    sts.withnullfill(fromioobj).withstep(true).onlyrange(false)
				.fornewentry(true);
    transffld_ = new uiSeisTransfer( this, sts );
    transffld_->attach( alignedBelow, attobj );

    uiSeisSel::Setup sssu( Seis::Vol );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );
    IOObjContext outctxt( uiSeisSel::ioContext( Seis::Vol, false ) );
    IOM().to( outctxt.getSelKey() );
    if ( !fromioobj )
    {
	sssu.enabotherdomain( true );
	outctxt.fixTranslator( CBVSSeisTrcTranslator::translKey() );
    }
    outfld_ = new uiSeisSel( this, outctxt, sssu );

    if ( convertfld_ )
    {
	convertfld_->attach( ensureBelow, transffld_ );
	convertfld_->attach( leftAlignedAbove, outfld_ );
    }

    outfld_->attach( alignedBelow, transffld_ );
}


uiSeisImpCBVS::~uiSeisImpCBVS()
{
    delete initialinpioobj_;
    delete outioobj_;
}


IOObj* uiSeisImpCBVS::getfInpIOObj( const char* inp ) const
{
    IOStream* iostrm = new IOStream( "_tmp", tmpid_ );
    iostrm->setGroup( mTranslGroupName(SeisTrc) );
    iostrm->setTranslator( CBVSSeisTrcTranslator::translKey() );
    iostrm->setDirName( "Seismics" );
    iostrm->setFileName( inp );
    return iostrm;
}


void uiSeisImpCBVS::modeSel( CallBacker* )
{
    if ( modefld_ )
	transffld_->display( modefld_->getBoolValue() );
}


void uiSeisImpCBVS::typeChg( CallBacker* )
{
    bool issteer = typefld_ ? typefld_->getIntValue() == 2 : false;
    if ( oinpfld_ )
    {
	const IOObj* inioobj = oinpfld_->ioobj( true );
	if ( !inioobj ) return;
	const char* res = inioobj->pars().find( "Type" );
	issteer = res && *res == 'S';
    }

    transffld_->setSteering( issteer );
}


void uiSeisImpCBVS::oinpSel( CallBacker* cb )
{
    if ( !oinpfld_ ) return;
    const IOObj* inioobj = oinpfld_->ioobj( true );
    ismc_ = false;
    if ( inioobj )
    {
	transffld_->updateFrom( *inioobj );
	SeisIOObjInfo oinf( *inioobj );
	ismc_ = oinf.isOK() && oinf.nrComponents() > 1;
	compfld_->display( ismc_ );
	if ( ismc_ )
	{
	    BufferStringSet cnms; oinf.getComponentNames( cnms );
	    compfld_->box()->setEmpty();
	    compfld_->box()->addItem( "<All>" );
	    compfld_->box()->addItems( cnms );
	}
    }
    typeChg( cb );
}


void uiSeisImpCBVS::finpSel( CallBacker* )
{
    BufferString inp = finpfld_->text();
    if ( inp.isEmpty() ) return;

    if ( !File::isEmpty(inp) )
    {
	PtrMan<IOObj> ioobj = getfInpIOObj( inp );
	transffld_->updateFrom( *ioobj );
    }

    getOutputName( inp );
    outfld_->setInputText( inp );
}


void uiSeisImpCBVS::convertSel( CallBacker* )
{
    BufferString inp = finpfld_->text();
    getOutputName( inp );
    outfld_->setInputText( inp );
}


void uiSeisImpCBVS::getOutputName( BufferString& inp ) const
{
    inp = FilePath( inp ).fileName();
    if ( inp.isEmpty() ) return;

    char* ptr = inp.getCStr();
    if ( convertfld_->isChecked() )
    {
	// convert underscores to spaces
	while ( *ptr )
	{
	    if ( *ptr == '_' ) *ptr = ' ';
	    ptr++;
	}
    }

    // remove .cbvs extension
    ptr = inp.find( '.' );
    if ( ptr && *(ptr+1) == 'c' && *(ptr+2) == 'b' && *(ptr+3) == 'v'
	 && *(ptr+4) == 's' )
	*(ptr) = '\0';

}


#define rmTmpIOObj() IOM().permRemove( MultiID(tmpid_.buf()) );

bool uiSeisImpCBVS::acceptOK( CallBacker* )
{
    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj )
	return false;

    outioobj_ = ioobj->clone();
    PtrMan<IOObj> inioobj = 0;
    const bool dolink = modefld_ && !modefld_->getBoolValue();

    if ( oinpfld_ )
    {
	ioobj = oinpfld_->ioobj();
	if ( !ioobj )
	    return false;
	inioobj = ioobj->clone();
	outioobj_->pars() = inioobj->pars();
    }
    else
    {
	BufferString fname = finpfld_->text();
	if ( !fname.str() )
	{
	    uiMSG().error( "Please select the input filename" );
	    return false;
	}

	if ( dolink )
	{
	    //Check if it's under the survey dir
	    FilePath inputfile( fname );
	    inputfile.makeCanonical();

	    mDynamicCastGet(IOStream*,iostrm,outioobj_)
	    if ( iostrm )
	    {
		FilePath seismicsdir( iostrm->fullDirName() );
		seismicsdir.makeCanonical();

		if ( inputfile.makeRelativeTo( seismicsdir ) )
		    fname = inputfile.fullPath();
	    }
	}

	const int seltyp = typefld_->getIntValue();
	if ( !seltyp )
	    outioobj_->pars().removeWithKey( "Type" );
	else
	    outioobj_->pars().set( sKey::Type(), seltyp == 1 ?
				        sKey::Attribute() : sKey::Steering() );

	outioobj_->setTranslator( CBVSSeisTrcTranslator::translKey() );
	if ( !dolink )
	    inioobj = getfInpIOObj( fname );
	else
	{
	    mDynamicCastGet(IOStream*,iostrm,outioobj_);
	    iostrm->setFileName( fname );
	}
    }

    if ( !IOM().commitChanges(*outioobj_) )
    {
	uiMSG().error( "Cannot write new file\nSee log file for details" );
	return false;
    }

    if ( dolink )
    {
	uiMSG().message( "Import successful" );
	return false;
    }

    uiSeisIOObjInfo ioobjinfo( *outioobj_, true );
    if ( !ioobjinfo.checkSpaceLeft(transffld_->spaceInfo()) )
	{ rmTmpIOObj(); return false; }

    const char* titl = oinpfld_ ? "Copying seismic data"
			       : "Importing CBVS seismic cube";
    PtrMan<Executor> stp = transffld_->getTrcProc( *inioobj,
				*outioobj_, titl, "Loading data" );
    if ( !stp )
	{ rmTmpIOObj(); return false; }

    mDynamicCastGet(SeisSingleTraceProc*,sstp,stp.ptr())
    sstp_ = sstp;

    if ( inioobj->pars().isTrue( VelocityDesc::sKeyIsVelocity() ) )
	sstp_->proctobedone_.notify( mCB(this,uiSeisImpCBVS,procToBeDoneCB) );

    if ( ismc_ )
    {
	SeisTrcReader& rdr = const_cast<SeisTrcReader&>( *sstp_->reader(0) );
	rdr.setComponent( compfld_->box()->currentItem() - 1 );
    }

    uiTaskRunner dlg( this );
    const bool rv mUnusedVar = TaskRunner::execute( &dlg, *stp ) &&
			!ioobjinfo.is2D() && ioobjinfo.provideUserInfo();
    rmTmpIOObj();
    return false;
}


void uiSeisImpCBVS::procToBeDoneCB( CallBacker* c )
{
    SeisTrc& trc = sstp_->getTrace();
    const SeisTrc intrc = sstp_->getInputTrace();
    const FixedString typestr =
		outioobj_->pars().find( VelocityDesc::sKeyVelocityType() );
    if ( typestr.isEmpty() ) return;

    const int compnr = compfld_ && compfld_->box()->visible() ?
		compfld_->box()->currentItem() - 1 : 0;
    TypeSet<float> trcvals, timevals;
    const int sizein = intrc.data().size(compnr);
    const float sampstep = intrc.info().sampling.step;
    for ( int idx=0; idx<sizein; idx++ )
    {
	trcvals += intrc.get( idx, compnr );
	timevals += intrc.startPos() + idx * sampstep;
    }

    const float* vin = trcvals.arr();
    const float* tin = timevals.arr();
    const int sizeout = trc.data().size(compnr);
    const SamplingData<double> sdout = trc.info().sampling;
    mAllocVarLenArr( float, vout, sizeout );
    if ( !mIsVarLenArrOK(vout) ) return;

    if ( typestr == VelocityDesc::TypeNames()[VelocityDesc::Interval] )
	sampleVint( vin, tin, sizein, sdout, vout, sizeout );
    else if ( typestr == VelocityDesc::TypeNames()[VelocityDesc::RMS] )
	sampleVrms( vin, 0, 0, tin, sizein, sdout, vout, sizeout );
    else if ( typestr == VelocityDesc::TypeNames()[VelocityDesc::Avg] )
	sampleVavg( vin, tin, sizein, sdout, vout, sizeout );

    for ( int idx=0; idx<sizeout; idx++ )
	trc.set( idx, vout[idx], compnr );
}