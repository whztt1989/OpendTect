/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bruno
 * DATE     : Sept 2010
-*/



#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "ascstream.h"
#include "separstr.h"

static const char* sKeyStratTree = "Stratigraphic Tree";
static const char* sKeyLith = "Lithology";
static const char* sKeyContents = "Contents";
static const char* sKeyAppearance = "Appearance";


Strat::RefTree::RefTree()
    : NodeOnlyUnitRef(0,"","Contains all units")
    , deleteNotif(this)
    , unitAdded(this)
    , unitChanged(this)
    , unitToBeDeleted(this)
    , notifun_(0)
    , udfleaf_(*new LeafUnitRef(this,-1,"Undef unit"))
{
    udfleaf_.setColor( Color::LightGrey() );
    initTree();
}


void Strat::RefTree::initTree()
{
    src_ = Repos::Temp;
    mAttachCB( Strat::eLVLS().objectChanged(), Strat::RefTree::levelSetChgCB );
}


Strat::RefTree::~RefTree()
{
    detachAllNotifiers();
    deleteNotif.trigger();
    delete &udfleaf_;
}


void Strat::RefTree::reportChange( const Strat::UnitRef* un, bool isrem )
{
    notifun_ = un;
    if ( un )
    {
	(isrem ? unitToBeDeleted : unitChanged).trigger();
	notifun_ = 0;
    }
}


void Strat::RefTree::reportAdd( const Strat::UnitRef* un )
{
    notifun_ = un;
    if ( un )
    {
	unitAdded.trigger();
	notifun_ = 0;
    }
}


bool Strat::RefTree::addLeavedUnit( const char* fullcode, const char* dumpstr )
{
    if ( !fullcode || !*fullcode )
	return false;

    CompoundKey ck( fullcode );
    UnitRef* par = find( ck.upLevel().buf() );
    if ( !par || par->isLeaf() )
	return false;

    const BufferString newcode( ck.key( ck.nrKeys()-1 ) );
    NodeUnitRef* parnode = (NodeUnitRef*)par;
    UnitRef* newun = new LeavedUnitRef( parnode, newcode );

    newun->use( dumpstr );
    parnode->refs_ += newun;
    return true;
}


void Strat::RefTree::setToActualTypes()
{
    UnitRefIter it( *this );
    ObjectSet<LeavedUnitRef> chrefs;
    while ( it.next() )
    {
	LeavedUnitRef* un = (LeavedUnitRef*)it.unit();
	if ( un->levelID().isInvalid() || !un->hasChildren() )
	    chrefs += un;
    }

    ObjectSet<LeavedUnitRef> norefs;
    for ( int idx=0; idx<chrefs.size(); idx++ )
    {
	LeavedUnitRef* un = chrefs[idx];
	NodeUnitRef* par = un->upNode();
	if ( un->hasChildren() )
	    { norefs += un; continue; }
	LeafUnitRef* newun = new LeafUnitRef( par, -1, un->description() );
	IOPar iop; un->putPropsTo( iop ); newun->getPropsFrom( iop );
	delete par->replace( par->indexOf(un), newun );
    }
    for ( int idx=0; idx<norefs.size(); idx++ )
    {
	LeavedUnitRef* un = norefs[idx];
	if ( un->ref(0).isLeaf() )
	    continue;
	NodeUnitRef* par = un->upNode();
	NodeOnlyUnitRef* newun = new NodeOnlyUnitRef( par, un->code(),
						    un->description() );
	newun->takeChildrenFrom( un );
	IOPar iop; un->putPropsTo( iop ); newun->getPropsFrom( iop );
	delete par->replace( par->indexOf(un), newun );
    }
}


bool Strat::RefTree::read( od_istream& strm )
{
    deepErase( refs_ ); liths_.setEmpty(); deepErase( contents_ );
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyStratTree) )
	{ initTree(); return false; }

    while ( !atEndOfSection( astrm.next() ) )
    {
	BufferString keyw( astrm.keyWord() );
	if ( keyw == sKeyLith )
	{
	    const BufferString nm( astrm.value() );
	    Lithology* lith = new Lithology(astrm.value());
	    if ( lith->id() < 0 || nm == Lithology::undef().name() )
		delete lith;
	    else
		liths_.add( lith );
	}
	else if ( keyw.startsWith(sKeyContents) )
	{
	    if ( keyw == sKeyContents )
	    {
		FileMultiString fms( astrm.value() );
		const int nrcont = fms.size();
		for ( int idx=0; idx<nrcont; idx++ )
		    contents_ += new Content( fms[idx] );
	    }
	    else if ( keyw.count('.') > 1 )
	    {
		char* contnm = keyw.find( '.' ) + 1;
		char* contkeyw = firstOcc( contnm, '.' );
		*contkeyw = '\0'; contkeyw++;
		Content* c = contents_.getByName( contnm );
		if ( c )
		{
		    if ( BufferString(contkeyw) == sKeyAppearance )
			c->getApearanceFrom( astrm.value() );
		}
	    }
	}
    }

    if ( !astrm.isOK() )
	{ initTree(); return false; }

    astrm.next(); // Read away the line: 'Units'
    while ( !atEndOfSection( astrm.next() ) )
	addLeavedUnit( astrm.keyWord(), astrm.value() );
    setToActualTypes();

    const int propsforlen = FixedString(sKeyPropsFor()).size();
    while ( !atEndOfSection( astrm.next() ) )
    {
	IOPar iop; iop.getFrom( astrm );
	const char* iopnm = iop.name().buf();
	if ( *iopnm != 'P' || FixedString(iopnm).size() < propsforlen )
	    break;

	BufferString unnm( iopnm + propsforlen );
	UnitRef* un = unnm == sKeyTreeProps() ? this : find( unnm.buf() );
	if ( un )
	    un->getPropsFrom( iop );
    }

    if ( refs_.isEmpty() )
	initTree();
    return true;
}


bool Strat::RefTree::write( od_ostream& strm ) const
{
    ascostream astrm( strm );
    astrm.putHeader( sKeyStratTree );
    astrm.put( "General" );
    for ( int idx=0; idx<liths_.size(); idx++ )
    {
	BufferString bstr; liths_.getLith(idx).fill( bstr );
	astrm.put( sKeyLith, bstr );
    }
    FileMultiString fms;
    for ( int idx=0; idx<contents_.size(); idx++ )
	fms += contents_[idx]->name();
    if ( !fms.isEmpty() )
	astrm.put( sKeyContents, fms );
    for ( int idx=0; idx<contents_.size(); idx++ )
    {
	BufferString valstr; contents_[idx]->putAppearanceTo(valstr);
	BufferString keystr( sKeyContents, ".", contents_[idx]->name() );
	keystr.add( "." ).add( sKeyAppearance );
	astrm.put( keystr, valstr );
    }

    astrm.newParagraph();
    astrm.put( "Units" );
    UnitRefIter it( *this );
    ObjectSet<const UnitRef> unitrefs;
    unitrefs += it.unit();

    while ( it.next() )
    {
	const UnitRef& un = *it.unit();
	BufferString bstr; un.fill( bstr );
	astrm.put( un.fullCode(), bstr );
	unitrefs += &un;
    }
    astrm.newParagraph();

    for ( int idx=0; idx<unitrefs.size(); idx++ )
    {
	IOPar iop;
	const UnitRef& un = *unitrefs[idx]; un.putPropsTo( iop );
	iop.putTo( astrm );
    }

    return astrm.isOK();
}


void Strat::RefTree::levelSetChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    if ( chgdata.changeType() != Strat::LevelSet::cLevelToBeRemoved() )
	return;

    const Level::ID::IDType idnr = (Level::ID::IDType)chgdata.ID();
    Strat::LeavedUnitRef* lur = getByLevel( Level::ID::get(idnr) );
    if ( lur )
	lur->setLevelID( Level::ID::getInvalid() );
}


void Strat::RefTree::getStdNames( BufferStringSet& nms )
{
    return Strat::LevelSet::getStdNames( nms );
}


Strat::RefTree* Strat::RefTree::createStd( const char* nm )
{
    RefTree* ret = new RefTree;
    if ( nm && *nm )
    {
	const BufferString fnm( getStdFileName(nm,"Tree") );
	od_istream strm( fnm );
	if ( strm.isOK() )
	    ret->read( strm );
    }
    return ret;
}


void Strat::RefTree::createFromLevelSet( const Strat::LevelSet& ls )
{
    setEmpty();
    if ( ls.isEmpty() )
	return;

    NodeOnlyUnitRef* ndun = new NodeOnlyUnitRef( this, "Above",
						"Layers above all markers" );
    const Level lvl0 = ls.first();
    ndun->add( new LeavedUnitRef( ndun, lvl0.name(),
				BufferString("Above ",lvl0.name()) ) );
    add( ndun );

    MonitorLock ml( ls );
    ndun = new NodeOnlyUnitRef( this, "Below", "Layers below a marker" );
    for ( int ilvl=0; ilvl<ls.size(); ilvl++ )
    {
	const Level lvl = ls.getByIdx( ilvl );
	LeavedUnitRef* lur = new LeavedUnitRef( ndun, lvl.name(),
					BufferString("Below ",lvl.name()) );
	lur->setLevelID( lvl.id() );
	ndun->add( lur );
    }
    add( ndun );

    UnitRefIter it( *this, UnitRefIter::LeavedNodes );
    while ( it.next() )
    {
	LeavedUnitRef* lur = (LeavedUnitRef*)it.unit();
	lur->add( new LeafUnitRef(lur) );
    }
}


const Strat::LeavedUnitRef* Strat::RefTree::getLevelSetUnit(
						const char* lvlnm ) const
{
    if ( isEmpty() ) return 0;

    UnitRefIter it( *this, UnitRefIter::LeavedNodes );
    const LeavedUnitRef* first = 0;
    while ( it.next() )
    {
	const LeavedUnitRef* lur = (const LeavedUnitRef*)it.unit();
	if ( !first )
	    first = lur;
	else if ( lur->code() == lvlnm )
	    return lur;
    }
    return first;
}


Strat::LeavedUnitRef* Strat::RefTree::getByLevel( Level::ID lvlid ) const
{
    UnitRefIter it( *this, UnitRefIter::LeavedNodes );
    while ( it.next() )
    {
	LeavedUnitRef* lur = (LeavedUnitRef*)it.unit();
	if ( lur && lur->levelID() == lvlid )
	    return lur;
    }
    return 0;
}
