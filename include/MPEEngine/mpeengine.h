#ifndef mpeengine_h
#define mpeengine_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: mpeengine.h,v 1.21 2005-09-27 15:21:02 cvskris Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "callback.h"
#include "color.h"
#include "cubesampling.h"
#include "emposid.h"
#include "trackplane.h"

class BufferStringSet;
class Executor;
class CubeSampling;
class MultiID;

namespace Attrib { class SelSpec; class SliceSet; }
namespace EM { class EMObject; };
namespace Geometry { class Element; };

namespace MPE
{

class EMTracker;
class TrackerFactory;
class EditorFactory;
class TrackPlane;
class ObjectEditor;

class Engine : public CallBackClass
{
    friend Engine&		engine();

public:
    				Engine();
    virtual			~Engine();

    const CubeSampling&		activeVolume() const;
    void			setActiveVolume(const CubeSampling&);
    static CubeSampling		getDefaultActiveVolume();
    Notifier<Engine>		activevolumechange;

    const TrackPlane&		trackPlane() const;
    bool			setTrackPlane(const TrackPlane&,bool track);
    void			setTrackMode(TrackPlane::TrackMode);
    TrackPlane::TrackMode	getTrackMode()
    				{ return trackplane.getTrackMode(); }
    Notifier<Engine>		trackplanechange;

    bool			trackAtCurrentPlane();
    Executor*			trackInVolume();

    void			getAvaliableTrackerTypes(BufferStringSet&)const;

    int				highestTrackerID() const;
    const EMTracker*		getTracker(int idx) const;
    EMTracker*			getTracker(int idx);
    int				getTrackerByObject(const EM::ObjectID&) const;
    int				getTrackerByObject(const char*) const;
    int				addTracker(EM::EMObject*);
    void			removeTracker(int idx);

    				/*Attribute stuff */
    void			getNeededAttribs(
				    ObjectSet<const Attrib::SelSpec>&) const;
    CubeSampling		getAttribCube(const Attrib::SelSpec&) const;
    				/*!< Returns the cube that is needed for
				     this attrib, given that the activearea
				     should be tracked. */
    const Attrib::SliceSet*	getAttribCache(const Attrib::SelSpec&) const;
    bool			setAttribData( const Attrib::SelSpec&,
					       const Attrib::SliceSet*);

    				/*Editors */
    ObjectEditor*		getEditor(const EM::ObjectID&,bool create);
    void			removeEditor(const EM::ObjectID&);

    				/*Factories */
    void			addTrackerFactory(TrackerFactory*);
    void			addEditorFactory(EditorFactory*);

    const char*			errMsg() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    int				getFreeID();

    BufferString		errmsg;
    CubeSampling		activevolume;
    TrackPlane			trackplane;

    ObjectSet<EMTracker>	trackers;
    ObjectSet<ObjectEditor>	editors;

    ObjectSet<const Attrib::SliceSet>	attribcache;
    ObjectSet<Attrib::SelSpec>		attribcachespecs;

    ObjectSet<TrackerFactory>	trackerfactories;
    ObjectSet<EditorFactory>	editorfactories;

    static const char*		nrtrackersStr()	{ return "Nr Trackers"; }
    static const char*		objectidStr()	{ return "ObjectID"; }
    static const char*		enabledStr()	{ return "Is enabled"; }
};


void initStandardClasses();


};

#endif

