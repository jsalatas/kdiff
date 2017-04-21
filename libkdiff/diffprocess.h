/***************************************************************************
                                diffprocess.h
                                ----------------
        begin                   : Sun Mar 4 2001
        Copyright 2001-2003 Otto Bruggeman <otto.bruggeman@home.nl>
        Copyright 2001-2003 John Firebaugh <jfirebaugh@kde.org>
        Copyright 2008      Kevin Kofler   <kevin.kofler@chello.at>
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
***************************************************************************/

#ifndef DIFFPROCESS_H
#define DIFFPROCESS_H

#include <kprocess.h>

#include "info.h"

class QTextDecoder;

class DiffSettings;

class DiffProcess : public KProcess
{
Q_OBJECT

public:
    DiffProcess(DiffSettings* diffSettings, enum Compare::DiffMode mode, const QString& source,
                   const QString& destination, const QString& directory = QString());
    ~DiffProcess();

    void start();

    QString diffOutput()
    {return m_stdout;}
    QString stdOut()
    {return m_stdout;}
    QString stdErr()
    {return m_stderr;}

    void setEncoding(const QString& encoding);

signals:
    void diffHasFinished(bool finishedNormally);

protected:
    void writeDefaultCommandLine();
    void writeCommandLine();

protected slots:
    void slotFinished(int, QProcess::ExitStatus);

private:
    DiffSettings* m_diffSettings;
    enum Compare::DiffMode m_mode;
    const QString* m_customString; // Used when a comparison between a file and a string is requested
    QString m_stdout;
    QString m_stderr;
    QTextDecoder* m_textDecoder;
    QTextCodec* m_codec;
};

#endif
