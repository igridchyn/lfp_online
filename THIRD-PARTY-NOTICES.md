Third-party notices
The MIT license in LICENSE covers the lfp_online source code. The files listed below are third-party code redistributed in this repository under their own terms, and are not licensed under the MIT license.

Intan Technologies RHD2000 Rhythm Interface API
Files: lfp_online/intan/rhd2000evalboard.{cpp,h}, lfp_online/intan/rhd2000datablock.{cpp,h}, lfp_online/intan/rhd2000registers.{cpp,h}

Copyright (c) 2013-2014 Intan Technologies LLC

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any applications that use Intan Technologies integrated circuits, and to alter it and redistribute it freely.

Note that this permission is limited to applications that use Intan Technologies integrated circuits. See http://www.intantech.com.

Opal Kelly FrontPanel DLL import source
Files: lfp_online/intan/okFrontPanelDLL.{cpp,h}

Copyright (c) 2005-2010 Opal Kelly Incorporated

These files are the import shim distributed with the Opal Kelly FrontPanel SDK and are redistributed here for convenience of building the Intan acquisition path. They are not covered by this repository's license. Users who need this path should obtain the FrontPanel SDK from Opal Kelly under its own terms.

inpout32
Files: lfp_online/inpout32.{cpp,h}

Windows parallel-port access shim, used by the TTL trigger path on Windows. Derived from the inpout32 driver interface; redistributed under its original terms.

Build-time dependencies (not redistributed here)
Linked at build time and installed separately — Boost (BSL-1.0), Armadillo (Apache-2.0), SDL2 and SDL2_ttf (Zlib), mlpack (BSD-3-Clause), OpenCV (Apache-2.0/BSD-3-Clause), ANN (LGPL), libdc1394 (LGPL-2.1). None of these are copied into this repository, so they place no license requirement on the lfp_online source itself; packagers distributing binaries should honour the LGPL terms for ANN and libdc1394.

