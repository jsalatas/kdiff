/***************************************************************************
                                info.h  -  description
                                -------------------
        begin                   : Sun Mar 4 2001
        Copyright 2001-2003 Otto Bruggeman <otto.bruggeman@home.nl>
        Copyright 2001-2003 John Firebaugh <jfirebaugh@kde.org>
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
***************************************************************************/

#ifndef INFO_H
#define INFO_H

#include <QUrl>

#include "kdiffparser_export.h"

// Forward declaration needed
class QTemporaryDir;

namespace Compare
{
enum Format
{
    Context = 0,
    Ed = 1,
    Normal = 2,
    RCS = 3,
    Unified = 4,
    SideBySide = 5,
    UnknownFormat = -1
};

enum Generator
{
    CVSDiff = 0,
    Diff = 1,
    Perforce = 2,
    SubVersion = 3,
    Reserved2 = 4,
    Reserved3 = 5,
    Reserved4 = 6,
    Reserved5 = 7,
    Reserved6 = 8,
    Reserved7 = 9,
    UnknownGenerator = -1
};

enum CompareMode
{
    ComparingFiles,      // compareFiles
    ComparingDirs,       // compareDirs
    ShowingDiff,         // openDiff
    BlendingDir,         // openDirAndDiff
    BlendingFile,        // openFileAndDiff
    UnknownMode          // Used to initialize the Infoi struct
};

enum DiffMode
{
    Default,
    Custom,
    UnknownDiffMode // Use to initialize the ComparisonInfo struct
};

enum Status
{
    RunningDiff,
    Parsing,
    FinishedParsing,
    FinishedWritingDiff,
    ReRunningDiff    // When a change has been detected after diff has run
};

enum Target
{
    Source,
    Destination
};

struct KDIFFPARSER_EXPORT ComparisonInfo
{
    ComparisonInfo(
            enum CompareMode _mode = UnknownMode,
            enum DiffMode _diffMode = UnknownDiffMode,
            enum Format _format = UnknownFormat,
            enum Generator _generator = UnknownGenerator,
            QUrl _source = QUrl(),
            QUrl _destination = QUrl(),
            QString _localSource = "",
            QString _localDestination = "",
            QTemporaryDir* _sourceQTempDir = 0,
            QTemporaryDir* _destinationQTempDir = 0,
            uint _depth = 0,
            bool _applied = true
    );
    void swapSourceWithDestination();

    enum CompareMode mode;
    enum DiffMode diffMode;
    enum Format format;
    enum Generator generator;
    QUrl source;
    QUrl destination;
    QString localSource;
    QString localDestination;
    QTemporaryDir* sourceQTempDir;
    QTemporaryDir* destinationQTempDir;
    uint depth;
    bool applied;
};
}

#endif
