    QtFirebirdIBPPSQLDriver - Qt SQL driver for Firebird with IBPP library

    (C) 2006-2009 Alex Wencel <alex.wencel@gmail.com>

Installation
~~~~~~~~~~~


Download is available from http://code.google.com/p/qtfirebirdibppsqldriver/, the official website.

You need :
- Qt (from Nokia) at least version 4.* to be able to compile QtFirebirdIBPPSQLDriver. M
- IBPP, a C++ Client Interface to Firebird Server. http://www.ibpp.org
- Firebird  at least version 1.5.*. http://firebirdsql.org

The simplest way to compile (QtFirebirdIBPPSQLDriver) package is:

1. Extract the source zip.
2. Copy IBPP source code to project/ibpp* dir, 
   or copy ibpp.pri file to IBPP dir and change IbppDriver.pro project file 
   in line contains "include(./ibpp2531/ibpp.pri)" 
3. Copy Firebird library file (fbclient.lib for Win) to project/lib dir, 
   and change ibpp.pri project file.
4. qmake in project directory.
3. Type `make' on Linux or `mingw32-make` on Windows to compile the package.
4. Copy drivers to Qt Sql plugins dir.


Documentation
~~~~~~~~~~~~~
.........

License
~~~~~~~~~~~~~

Copyright (C) 2006-2009 Alex Wencel <alex.wencel@gmail.com>

QtFirebirdIBPPSQLDriver is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
