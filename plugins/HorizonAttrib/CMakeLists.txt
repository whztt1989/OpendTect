#_______________________Pmake___________________________________________________
#
#	Makefile : 	HorizonAttrib plugin
# 	March 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS  Basic Algo AttributeEngine PreStackProcessing
		    Attributes EarthModel MPEEngine)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	horizonattrib.cc
	horattribpi.cc
	stratamp.cc
	isopachmaker.cc)
set( OD_MODULE_BATCHPROGS od_isopach.cc od_stratamp.cc )
set( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
