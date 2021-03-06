/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "rangeposprovider.h"
#include "mathproperty.h"
#include "dbman.h"
#include "coordsystem.h"
#include "elasticpropseltransl.h"
#include "mathformulatransl.h"
#include "ioobjselectiontransl.h"
#include "preloads.h"
#include "pickset.h"
#include "geometryio.h"
#include "survgeometrytransl.h"
#include "surveysectionprinfo.h"


mDefSimpleTranslators(IOObjSelection,"Object selection",od,Misc);

class GeneralModuleIniter { public: GeneralModuleIniter(); };

mDefModInitFn(General)
{
    mIfNotFirstTime( return );
    GeneralModuleIniter initer;
}


GeneralModuleIniter::GeneralModuleIniter()
{
    ElasticPropSelectionTranslatorGroup::initClass();
    MathFormulaTranslatorGroup::initClass();
    IOObjSelectionTranslatorGroup::initClass();
    PreLoadsTranslatorGroup::initClass();
    PreLoadSurfacesTranslatorGroup::initClass();

    dgbPreLoadsTranslator::initClass();
    dgbPreLoadSurfacesTranslator::initClass();
    odElasticPropSelectionTranslator::initClass();
    odMathFormulaTranslator::initClass();
    odIOObjSelectionTranslator::initClass();

    Pick::SetPresentationInfo::initClass();
    SurveySectionPresentationInfo::initClass();
    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
    ValueProperty::initClass();
    RangeProperty::initClass();
    MathProperty::initClass();

    Survey::GeometryWriter2D::initClass();
    Survey::GeometryWriter3D::initClass();
    Survey::GeometryReader3D::initClass();
    Survey::GeometryReader2D::initClass();
    SurvGeom2DTranslatorGroup::initClass();
    dgbSurvGeom2DTranslator::initClass();

    DBM().initFirst(); //Trigger creation & reading of geometries

    //After DBM is created
    Coords::PositionSystem::initRepository( &DBM().afterSurveyChange );
}
