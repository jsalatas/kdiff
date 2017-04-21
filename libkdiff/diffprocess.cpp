/***************************************************************************
                                diffprocess.cpp
                                ------------------
        begin                   : Sun Mar 4 2001
        Copyright 2001-2005,2009 Otto Bruggeman <bruggie@gmail.com>
        Copyright 2001-2003 John Firebaugh <jfirebaugh@kde.org>
        Copyright 2007-2008 Kevin Kofler   <kevin.kofler@chello.at>
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
***************************************************************************/

#include "diffprocess.h"

#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QTextCodec>
#include <QLoggingCategory>

#include <kcharsets.h>
#include <kio/global.h>

#include "diffsettings.h"

/// TODO: This should be replaced to QDir::relativeFilePath
QString constructRelativePath(const QString& from, const QString& to)
{
    QUrl fromURL(from);
    QUrl toURL(to);
    QUrl root;
    int upLevels = 0;

    // Find a common root.
    root = fromURL;
    while (root.isValid() && !root.isParentOf(toURL)) {
        root = KIO::upUrl(root);
        upLevels++;
    }

    if(!root.isValid()) {return to;}

    QString relative;
    for (; upLevels > 0; upLevels--) {
        relative += "../";
    }

    relative += QString(to).replace(0, root.path().length(), "");
    return relative;
}

DiffProcess::DiffProcess(DiffSettings* diffSettings, Compare::DiffMode diffMode, const QString& source, const QString& destination,  const QString& dir)
        : KProcess()
        , m_diffSettings(diffSettings)
        , m_mode(diffMode)
        , m_customString(0)
        , m_textDecoder(0)
{
    // connect the signal that indicates that the proces has exited
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT (slotFinished(int, QProcess::ExitStatus)));

    setEnv("LANG", "C");

    // Write command and options
    if(m_mode == Compare::Default) {
        writeDefaultCommandLine();
    } else {
        writeCommandLine();
    }

    if(!dir.isEmpty()) {
        setWorkingDirectory(dir);
    }

    // Write file names
    *this << "--";

    *this << constructRelativePath(dir, source);

    *this << constructRelativePath(dir, destination);
}

void DiffProcess::writeDefaultCommandLine()
{
    if(!m_diffSettings || m_diffSettings->m_diffProgram.isEmpty()) {
        *this << "diff" << "-dr";
    } else {
        *this << m_diffSettings->m_diffProgram << "-dr";
    }

    *this << "-U" << QString::number(m_diffSettings->m_linesOfContext);
}

void DiffProcess::writeCommandLine()
{
    // load the executable into the KProcess
    if(m_diffSettings->m_diffProgram.isEmpty()) {
        *this << "diff";
    } else {
        *this << m_diffSettings->m_diffProgram;
    }

    // Always use unified format
    *this << "-U" << QString::number(m_diffSettings->m_linesOfContext);

    if(m_diffSettings->m_largeFiles
// default diff does not have -H on OpenBSD
// so don't pass this option unless the user overrode the default program
#if defined(__OpenBSD__)
        && !m_diffSettings->m_diffProgram.isEmpty()
#endif
            ) {
        *this << "-H";
    }

    if(m_diffSettings->m_ignoreWhiteSpace) {
        *this << "-b";
    }

    if(m_diffSettings->m_ignoreAllWhiteSpace) {
        *this << "-w";
    }

    if(m_diffSettings->m_ignoreEmptyLines) {
        *this << "-B";
    }

    if(m_diffSettings->m_ignoreChangesDueToTabExpansion) {
        *this << "-E";
    }

    if(m_diffSettings->m_createSmallerDiff) {
        *this << "-d";
    }

    if(m_diffSettings->m_ignoreChangesInCase) {
        *this << "-i";
    }

    if(m_diffSettings->m_ignoreRegExp && !m_diffSettings->m_ignoreRegExpText.isEmpty()) {
        *this << "-I" << m_diffSettings->m_ignoreRegExpText;
    }

    if(m_diffSettings->m_showCFunctionChange) {
        *this << "-p";
    }

    if(m_diffSettings->m_convertTabsToSpaces) {
        *this << "-t";
    }

    if(m_diffSettings->m_recursive) {
        *this << "-r";
    }

    if(m_diffSettings->m_newFiles) {
        *this << "-N";
    }

// This option is more trouble than it is worth... please do not ever enable it unless you want really weird crashes
//	if ( m_diffSettings->m_allText )
//	{
//		*this << "-a";
//	}

    if(m_diffSettings->m_excludeFilePattern) {
        Q_FOREACH(const QString& it, m_diffSettings->m_excludeFilePatternList) {
                *this << "-x" << it;
            }
    }

    if(m_diffSettings->m_excludeFilesFile && !m_diffSettings->m_excludeFilesFileURL.isEmpty()) {
        *this << "-X" << m_diffSettings->m_excludeFilesFileURL;
    }
}

DiffProcess::~DiffProcess()
{
    blockSignals(true);
    close();
    delete m_textDecoder;
}

void DiffProcess::setEncoding(const QString& encoding)
{
    if(!encoding.compare("default", Qt::CaseInsensitive)) {
        m_textDecoder = QTextCodec::codecForLocale()->makeDecoder();
    } else {
        m_codec = KCharsets::charsets()->codecForName(encoding.toLatin1());
        if(m_codec) {
            m_textDecoder = m_codec->makeDecoder();
        } else {
            m_codec = QTextCodec::codecForLocale();
            m_textDecoder = m_codec->makeDecoder();
        }
    }
}

void DiffProcess::start()
{
//#ifndef NDEBUG
//    QString cmdLine;
//    QStringList program = KProcess::program();
//    QStringList::ConstIterator it = program.constBegin();
//    QStringList::ConstIterator end = program.constEnd();
//    for (; it != end; ++it)
//        cmdLine += "\"" + (*it) + "\" ";
//
//    qDebug() << cmdLine;
//#endif

    setOutputChannelMode(SeparateChannels);
    setNextOpenMode(QIODevice::ReadWrite);
    KProcess::start();

    //If we have a string to compare against input it now
    if(m_customString) {
        write(m_codec->fromUnicode(*m_customString));
    }
    closeWriteChannel();
}

void DiffProcess::slotFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // add all output to m_stdout/m_stderr
    if(m_textDecoder) {
        m_stdout = m_textDecoder->toUnicode(readAllStandardOutput());
        m_stderr = m_textDecoder->toUnicode(readAllStandardError());
    }

    // exit code of 0: no differences
    //              1: some differences
    //              2: error but there may be differences !
    emit diffHasFinished(exitStatus == NormalExit && exitCode != 0);
}

