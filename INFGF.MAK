###############################################################################
#####   This Makefile generates the INFGF.DLL for the Gunfighter Add-on   #####
###############################################################################

.autodepend
.silent
.path.c=\bbsv6\src
.path.h=\bbsv6\src
.path.dll=\bbsv6
.path.msg=\bbsv6
.path.obj=\bbsv6\phobj

infgf.dll: \
     infgfmsg.h                                                            \
     infgf.obj                                                             \
     \bbsv6\dlib\galimp.lib      \bbsv6\dlib\gsblimp.lib                   \
     \run286\bc3\lib\phapi.lib   \bbsv6\dlib\nodef.def
     ltdll infgf

###############################################################################
#####                 Build Objects For INFGF DLL                         #####
###############################################################################

infgf.obj: infgf.c
     ctdll infgf

###############################################################################
#####               Generate Headers for Compilation                      #####
###############################################################################

infgfmsg.h: infgfmsg.msg
     cd \bbsv6
     bbsmsx infgfmsg -osrc
     cd \bbsv6\src

