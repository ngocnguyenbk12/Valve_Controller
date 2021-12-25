REM ***************************************************************************
REM
REM  Copyright (c) 2001-2012
REM  Sigma Designs, Inc.
REM  All Rights Reserved
REM
REM ---------------------------------------------------------------------------
REM
REM  Description: Windows batch script for setting UDEF and DEF for unifdef
REM               of this sample application.
REM
REM  Author:   Erik Friis Harck
REM
REM  Last Changed By:  $Author: tro $
REM  Revision:         $Revision: 23083 $
REM  Last Changed:     $Date: 2012-07-06 12:45:08 +0200 (fr, 06 jul 2012) $
REM
REM ****************************************************************************

REM Application version placement definition
REM SET APPVERSOURCEFILE=   <nothing> to skip version check
SET APPVERSOURCEFILE=config_app.h
SET APPVERSIONDEFINE=APP_VERSION
SET APPREVISIONDEFINE=APP_REVISION

REM Assume next macros is undefined and remove source code in #ifdef with this macros (leave code in #ifndef/#else)
SET UDEF=-UDEBUG_INTERNAL

REM Assume next macros is defined and leave source code in #ifdef with this macros (remove code in #ifndef/#else)
SET DEF=-D_NONE_

REM Do we need make binaries?
SET MAKE_BINARIES=FALSE
SET MAKE_TF_TARGETS=TRUE

REM Set which variant of Z-Wave library to refer to during build
REM ZWAVELIB_REF=bin for all normal applications. ZWAVELIB_REF=src for applications needing access to lib source.
SET ZWAVELIB_REF=bin
