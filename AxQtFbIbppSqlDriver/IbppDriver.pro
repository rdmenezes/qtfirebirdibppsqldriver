CONFIG +=   qt \
  plugin \
  release
QT +=   core \
  sql
DEFINES			-= UNICODE 
QT -=   gui # +=   GUI
TEMPLATE =   lib
DESTDIR +=   bin
TARGET =   qsqlfirebird
MOC_DIR +=   build
OBJECTS_DIR +=   build
RCC_DIR +=   build
UI_DIR +=   build
DEFINES +=   QT_NO_CAST_TO_ASCII \
  QT_NO_CAST_FROM_ASCII
HEADERS +=   src/qsql_ibpp.h \
  src/qsqlcachedresult_p.h
SOURCES +=   src/main.cpp \
  src/qsql_ibpp.cpp
include(./ibpp2531/ibpp.pri) # +=   IBPP
contains(QT_CONFIG, reduce_exports):CONFIG+=hide_symbols  # +=   hide_symbols
