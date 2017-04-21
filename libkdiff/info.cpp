/***************************************************************************
 *                                info.h  -  description
 *                                -------------------
 *        begin                   : Sun Mar 4 2001
 *        Copyright 2001-2003 Otto Bruggeman <otto.bruggeman@home.nl>
 *        Copyright 2001-2003 John Firebaugh <jfirebaugh@kde.org>
 ****************************************************************************/

/***************************************************************************
 * *
 **   This program is free software; you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation; either version 2 of the License, or
 **   (at your option) any later version.
 **
 ***************************************************************************/

#include "info.h"

Compare::ComparisonInfo::ComparisonInfo(Compare::CompareMode _mode, Compare::DiffMode _diffMode, Compare::Format _format, Compare::Generator _generator, QUrl _source, QUrl _destination, QString _localSource,
                    QString _localDestination, QTemporaryDir* _sourceQTempDir, QTemporaryDir* _destinationQTempDir, uint _depth, bool _applied)
        : mode(_mode)
        , diffMode(_diffMode)
        , format(_format)
        , generator(_generator)
        , source(_source)
        , destination(_destination)
        , localSource(_localSource)
        , localDestination(_localDestination)
        , sourceQTempDir(_sourceQTempDir)
        , destinationQTempDir(_destinationQTempDir)
        , depth(_depth)
        , applied(_applied)
{
}


void Compare::ComparisonInfo::swapSourceWithDestination()
{
    QUrl url = source;
    source = destination;
    destination = url;

    QString string = localSource;
    localSource = localDestination;
    localDestination = string;

    QTemporaryDir* tmpDir = sourceQTempDir;
    sourceQTempDir = destinationQTempDir;
    destinationQTempDir = tmpDir;
}
