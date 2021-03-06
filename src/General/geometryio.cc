/*+
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* AUTHOR   : Salil Agarwal
* DATE     : Dec 2012
-*/


#include "geometryio.h"

#include "ioobjctxt.h"
#include "dbdir.h"
#include "dbman.h"
#include "survgeom2d.h"
#include "posinfo2d.h"
#include "survgeometrytransl.h"
#include "survinfo.h"
#include "executor.h"
#include "keystrs.h"
#include "od_iostream.h"

namespace Survey
{

#define mReturn return ++nrdone_ < totalNr() ? MoreToDo() : Finished();

class GeomFileReader : public Executor
{
public:

    GeomFileReader( const DBDir& dbdir,
		    ObjectSet<Geometry>& geometries, bool updateonly )
	: Executor( "Loading Files" )
	, dbdir_(dbdir)
	, geometries_(geometries)
	, nrdone_(0)
	, updateonly_(updateonly)
    {}


    od_int64 nrDone() const
    { return nrdone_; }


    od_int64 totalNr() const
    { return dbdir_.size(); }

protected:

    bool isLoaded( Geometry::ID geomid ) const
    {
	for ( int idx=0; idx<geometries_.size(); idx++ )
	    if ( geometries_[idx]->getID() == geomid )
		return true;

	return false;
    }

    int nextStep()
    {
	PtrMan<IOObj> ioobj = dbdir_.getEntryByIdx( mCast(int,nrdone_) );
	const Geometry::ID geomid = SurvGeom2DTranslator::getGeomID( *ioobj );
	if ( updateonly_ && isLoaded(geomid) )
	    mReturn

	PtrMan<Translator> transl = ioobj->createTranslator();
	mDynamicCastGet(SurvGeom2DTranslator*,geomtransl,transl.ptr());
	if ( !geomtransl )
	    mReturn

	uiString errmsg;
	Geometry* geom = geomtransl->readGeometry( *ioobj, errmsg );
	if ( geom )
	{
	    geom->ref();
	    geometries_ += geom;
	}

	mReturn
    }

    const DBDir&		dbdir_;
    ObjectSet<Geometry>&	geometries_;
    od_int64			nrdone_;
    bool			updateonly_;

};


void GeometryWriter2D::initClass()
{ GeometryWriter::factory().addCreator( create2DWriter, sKey::TwoD() ); }


bool GeometryWriter2D::write( Geometry& geom, uiString& errmsg,
			      const char* createfromstr ) const
{
    RefMan< Geometry2D > geom2d;
    mDynamicCast( Geometry2D*, geom2d, &geom );
    if ( !geom2d )
	return false;

    PtrMan<IOObj> ioobj = createEntry( geom2d->data().lineName().buf() );
    if ( !ioobj || !ioobj->key().hasValidObjID() )
	return false;

    PtrMan<Translator> transl = ioobj->createTranslator();
    mDynamicCastGet(SurvGeom2DTranslator*,geomtransl,transl.ptr());
    if ( !geomtransl )
	return false;

    const FixedString crfromstr( createfromstr );
    if ( !crfromstr.isEmpty() )
    {
	ioobj->pars().set( sKey::CrFrom(), crfromstr );
	DBM().setEntry( *ioobj );
    }

    return geomtransl->writeGeometry( *ioobj, geom, errmsg );
}


Geometry::ID GeometryWriter2D::createNewGeomID( const char* name ) const
{
    PtrMan<IOObj> geomobj = createEntry( name );
    if ( !geomobj )
	return Survey::GM().cUndefGeomID();
    return SurvGeom2DTranslator::getGeomID( *geomobj );
}


IOObj* GeometryWriter2D::createEntry( const char* name ) const
{
    return SurvGeom2DTranslator::createEntry( name,
					dgbSurvGeom2DTranslator::translKey() );
}


void GeometryWriter3D::initClass()
{
    GeometryWriter::factory().addCreator( create3DWriter, sKey::ThreeD() );
}


bool GeometryReader2D::read( ObjectSet<Geometry>& geometries,
			     TaskRunner* tr ) const
{
    const IOObjContext& ctxt = mIOObjContext(SurvGeom2D);
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( ctxt.getSelDirID() );
    if ( !dbdir )
	return false;

    GeomFileReader gfr( *dbdir, geometries, false );
    return TaskRunner::execute( tr, gfr );
}


bool GeometryReader2D::updateGeometries( ObjectSet<Geometry>& geometries,
					 TaskRunner* tr ) const
{
    const IOObjContext& ctxt = mIOObjContext(SurvGeom2D);
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( ctxt.getSelDirID() );
    if ( !dbdir )
	return false;

    if ( dbdir->size() == geometries.size() )
	return true; //TODO: Update existing geometries if modified.

    GeomFileReader gfr( *dbdir, geometries, true );
    return TaskRunner::execute( tr, gfr );
}


void GeometryReader2D::initClass()
{
    GeometryReader::factory().addCreator( create2DReader, sKey::TwoD() );
}


void GeometryReader3D::initClass()
{
    GeometryReader::factory().addCreator( create3DReader, sKey::ThreeD() );
}

} // namespace Survey
