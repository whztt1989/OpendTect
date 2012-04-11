#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODQtUtils.cmake,v 1.14 2012-04-11 07:50:59 cvskris Exp $
#_______________________________________________________________________________

SET(QTDIR "" CACHE PATH "QT Location" )

MACRO(OD_SETUP_QT)
    IF ( QTDIR STREQUAL "" )
	MESSAGE( FATAL_ERROR "QTDIR not set")
    ENDIF()

    SET( ENV{QTDIR} ${QTDIR} )

    FIND_PACKAGE(Qt4 COMPONENTS QtGui QtCore QtSql QtNetwork )
     
    include(${QT_USE_FILE})

    IF(${OD_USEQT} MATCHES "Core" )
	LIST(APPEND OD_MODULE_INCLUDEPATH
            ${QT_QTNETWORK_INCLUDE_DIR}
            ${QT_QTCORE_INCLUDE_DIR} ${QTDIR}/include )
        SET(OD_QT_LIBS ${QT_QTCORE_LIBRARY_RELEASE}
                       ${QT_QTNETWORK_LIBRARY_RELEASE})
    ENDIF()

    IF(${OD_USEQT} MATCHES "Sql" )
	LIST(APPEND OD_MODULE_INCLUDEPATH
            ${QT_QTSQL_INCLUDE_DIR}
            ${QTDIR}/include )
        SET(OD_QT_LIBS ${QT_QTSQL_LIBRARY_RELEASE})
    ENDIF()

    IF(${OD_USEQT} MATCHES "Gui")
	LIST(APPEND OD_MODULE_INCLUDEPATH
            ${QT_QTCORE_INCLUDE_DIR}
            ${QT_QTGUI_INCLUDE_DIR} ${QTDIR}/include )
        SET(OD_QT_LIBS ${QT_QTGUI_LIBRARY_RELEASE})
    ENDIF()

    IF(${OD_USEQT} MATCHES "OpenGL")
	LIST(APPEND OD_MODULE_INCLUDEPATH
            ${QT_QTOPENGL_INCLUDE_DIR} ${QTDIR}/include )
        SET(OD_QT_LIBS ${QT_QTOPENGL_LIBRARY_RELEASE})
    ENDIF()

    IF( QT_MOC_HEADERS )
        FOREACH( HEADER ${QT_MOC_HEADERS} )
            LIST(APPEND QT_MOC_INPUT
                ${OpendTect_SOURCE_DIR}/include/${OD_MODULE_NAME}/${HEADER})
        ENDFOREACH()

        QT4_WRAP_CPP (QT_MOC_OUTFILES ${QT_MOC_INPUT})
    ENDIF( QT_MOC_HEADERS )

    LIST(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_QT_LIBS} )
    IF ( OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
	INSTALL ( FILES ${QT_QTGUI_LIBRARY_RELEASE} ${QT_QTCORE_LIBRARY_RELEASE}
			${QT_QTSQL_LIBRARY_RELEASE} ${QT_QTNETWORK_LIBRARY_RELEASE}
		    DESTINATION ${OD_LIBRARY_INSTALL_PATH} )
    ENDIF()
ENDMACRO(OD_SETUP_QT)
