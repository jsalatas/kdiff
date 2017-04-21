/**************************************************************************
**                             parserbase.cpp
**                             --------------
**      begin                   : Sun Aug  4 15:05:35 2002
**      Copyright 2002-2004,2009 Otto Bruggeman <bruggie@gmail.com>
**      Copyright 2007,2010 Kevin Kofler   <kevin.kofler@chello.at>
***************************************************************************/
/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   ( at your option ) any later version.
**
***************************************************************************/

#include "parserbase.h"

#include <QtCore/QObject>

#include <QLoggingCategory>

#include "diffmodel.h"
#include "diffhunk.h"
#include "difference.h"
#include "modellist.h"

// static
QString ParserBase::unescapePath(QString path)
{
    // If path contains spaces, it is enclosed in quotes
    if(path.startsWith(QLatin1Char('"')) && path.endsWith(QLatin1Char('"'))) {
        path = path.mid(1, path.size() - 2);
    }

    // Unescape quotes
    path.replace(QLatin1String("\\\""), QLatin1String("\""));

#ifndef Q_OS_WIN
    // Unescape backquotes
    path.replace(QLatin1String("\\\\"), QLatin1String("\\"));
#endif

    return path;
}

// static
QString ParserBase::escapePath(QString path)
{
#ifndef Q_OS_WIN
    // Escape backquotes
    path.replace(QLatin1String("\\"), QLatin1String("\\\\"));
#endif

    // Escape quotes
    path.replace(QLatin1String("\""), QLatin1String("\\\""));

    // Enclose in quotes if path contains space
    if(path.contains(QLatin1Char(' '))) {
        path = QStringLiteral("\"%1\"").arg(path);
    }

    return path;
}

ParserBase::ParserBase(const QStringList& diff)
        : m_diffLines(diff)
        , m_currentModel(0)
        , m_models(0)
        , m_diffIterator(m_diffLines.begin())
        , m_singleFileDiff(false)
        , m_malformed(false)
{
    m_models = new DiffModelMap();

    // used in contexthunkheader
    m_contextHunkHeader1.setPattern("\\*{15} ?(.*)\\n"); // capture is for function name
    m_contextHunkHeader2.setPattern("\\*\\*\\* ([0-9]+),([0-9]+) \\*\\*\\*\\*.*\\n");
    // used in contexthunkbody
    m_contextHunkHeader3.setPattern("--- ([0-9]+),([0-9]+) ----\\n");

    m_contextHunkBodyRemoved.setPattern("- (.*)");
    m_contextHunkBodyAdded.setPattern("\\+ (.*)");
    m_contextHunkBodyChanged.setPattern("! (.*)");
    m_contextHunkBodyContext.setPattern("  (.*)");
    m_contextHunkBodyLine.setPattern("[-\\+! ] (.*)");

    // This regexp sucks... i'll see what happens
    m_normalDiffHeader.setPattern("diff (?:(?:-|--)[a-zA-Z0-9=\\\"]+ )*(?:|-- +)(.*) +(.*)\\n");

    m_normalHunkHeaderAdded.setPattern("([0-9]+)a([0-9]+)(|,[0-9]+)(.*)\\n");
    m_normalHunkHeaderRemoved.setPattern("([0-9]+)(|,[0-9]+)d([0-9]+)(.*)\\n");
    m_normalHunkHeaderChanged.setPattern("([0-9]+)(|,[0-9]+)c([0-9]+)(|,[0-9]+)(.*)\\n");

    m_normalHunkBodyRemoved.setPattern("< (.*)");
    m_normalHunkBodyAdded.setPattern("> (.*)");
    m_normalHunkBodyDivider.setPattern("---\\n");

    m_unifiedDiffHeader1.setPattern("--- ([^\\t]+)(?:\\t([^\\t]+)(?:\\t?)(.*))?\\n");
    m_unifiedDiffHeader2.setPattern("\\+\\+\\+ ([^\\t]+)(?:\\t([^\\t]+)(?:\\t?)(.*))?\\n");
    m_unifiedHunkHeader.setPattern("@@ -([0-9]+)(|,([0-9]+)) \\+([0-9]+)(|,([0-9]+)) @@(?: ?)(.*)\\n");
    m_unifiedHunkBodyAdded.setPattern("\\+(.*)");
    m_unifiedHunkBodyRemoved.setPattern("-(.*)");
    m_unifiedHunkBodyContext.setPattern(" (.*)");
    m_unifiedHunkBodyLine.setPattern("([-+ ])(.*)");
}

ParserBase::~ParserBase()
{
    if(m_models) {
        m_models = 0;
    } // do not delete this, i pass it around...
}

enum Compare::Format ParserBase::determineFormat()
{
    // Write your own format detection routine damn it :)
    return Compare::UnknownFormat;
}

DiffModelMap* ParserBase::parse(bool* malformed)
{
    DiffModelMap* result;
    switch (determineFormat()) {
        case Compare::Context :
            result = parseContext();
            break;
        case Compare::Ed :
            result = parseEd();
            break;
        case Compare::Normal :
            result = parseNormal();
            break;
        case Compare::RCS :
            result = parseRCS();
            break;
        case Compare::Unified :
            result = parseUnified();
            break;
        default: // Unknown and SideBySide for now
            result = 0;
            break;
    }

    // *malformed is set to true if some hunks or parts of hunks were
    // probably missed due to a malformed diff
    if(malformed) {
        *malformed = m_malformed;
    }

    return result;
}

bool ParserBase::parseContextDiffHeader()
{
    bool result = false;

    while (m_diffIterator != m_diffLines.end()) {
        if(!m_contextDiffHeader1.exactMatch(*(m_diffIterator)++)) {
            continue;
        }
        if(m_diffIterator != m_diffLines.end() && m_contextDiffHeader2.exactMatch(*m_diffIterator)) {
            m_currentModel = new DiffModel(unescapePath(m_contextDiffHeader1.cap(1)), unescapePath(m_contextDiffHeader2.cap(1)));

            ++m_diffIterator;
            result = true;

            break;
        } else {
            // We're screwed, second line does not match or is not there...
            break;
        }
        // Do not inc the Iterator because the second line might be the first line of
        // the context header and the first hit was a fluke (impossible imo)
        // maybe we should return false here because the diff is broken ?
    }

    return result;
}

bool ParserBase::parseEdDiffHeader()
{
    return false;
}

bool ParserBase::parseNormalDiffHeader()
{
    bool result = false;

    while (m_diffIterator != m_diffLines.end()) {
        if(m_normalDiffHeader.exactMatch(*m_diffIterator)) {
            m_currentModel = new DiffModel();
            m_currentModel->setSourceFile(unescapePath(m_normalDiffHeader.cap(1)));
            m_currentModel->setDestinationFile(unescapePath(m_normalDiffHeader.cap(2)));

            result = true;

            ++m_diffIterator;
            break;
        }
        ++m_diffIterator;
    }

    if(result == false) {
        // Set this to the first line again and hope it is a single file diff
        m_diffIterator = m_diffLines.begin();
        m_currentModel = new DiffModel();
        m_singleFileDiff = true;
    }

    return result;
}

bool ParserBase::parseRCSDiffHeader()
{
    return false;
}

bool ParserBase::parseUnifiedDiffHeader()
{
    bool result = false;

    while (m_diffIterator != m_diffLines.end()) // do not assume we start with the diffheader1 line
    {
        if(!m_unifiedDiffHeader1.exactMatch(*m_diffIterator)) {
            ++m_diffIterator;
            continue;
        }
        ++m_diffIterator;
        if(m_diffIterator != m_diffLines.end() && m_unifiedDiffHeader2.exactMatch(*m_diffIterator)) {
            m_currentModel = new DiffModel(unescapePath(m_unifiedDiffHeader1.cap(1)), unescapePath(m_unifiedDiffHeader2.cap(1)));

            ++m_diffIterator;
            result = true;

            break;
        } else {
            // We're screwed, second line does not match or is not there...
            break;
        }
    }

    return result;
}

bool ParserBase::parseContextHunkHeader()
{
    if(m_diffIterator == m_diffLines.end()) {
        return false;
    }

    if(!m_contextHunkHeader1.exactMatch(*(m_diffIterator))) {
        return false;
    } // big fat trouble, aborting...

    ++m_diffIterator;

    if(m_diffIterator == m_diffLines.end())
        return false;

    if(!m_contextHunkHeader2.exactMatch(*(m_diffIterator)))
        return false; // big fat trouble, aborting...

    ++m_diffIterator;

    return true;
}

bool ParserBase::parseEdHunkHeader()
{
    return false;
}

bool ParserBase::parseNormalHunkHeader()
{
    if(m_diffIterator != m_diffLines.end()) {
        if(m_normalHunkHeaderAdded.exactMatch(*m_diffIterator)) {
            m_normalDiffType = Difference::Insert;
        } else if(m_normalHunkHeaderRemoved.exactMatch(*m_diffIterator)) {
            m_normalDiffType = Difference::Delete;
        } else if(m_normalHunkHeaderChanged.exactMatch(*m_diffIterator)) {
            m_normalDiffType = Difference::Change;
        } else {
            return false;
        }

        ++m_diffIterator;
        return true;
    }

    return false;
}

bool ParserBase::parseRCSHunkHeader()
{
    return false;
}

bool ParserBase::parseUnifiedHunkHeader()
{
    if(m_diffIterator != m_diffLines.end() && m_unifiedHunkHeader.exactMatch(*m_diffIterator)) {
        ++m_diffIterator;
        return true;
    } else {
        return false;
    }
}

bool ParserBase::parseContextHunkBody()
{
    // Storing the src part of the hunk for later use
    QStringList oldLines;
    for (; m_diffIterator != m_diffLines.end() && m_contextHunkBodyLine.exactMatch(*m_diffIterator); ++m_diffIterator) {
        oldLines.append(*m_diffIterator);
    }

    if(!m_contextHunkHeader3.exactMatch(*m_diffIterator)) {
        return false;
    }

    ++m_diffIterator;

    // Storing the dest part of the hunk for later use
    QStringList newLines;
    for (; m_diffIterator != m_diffLines.end() && m_contextHunkBodyLine.exactMatch(*m_diffIterator); ++m_diffIterator) {
        newLines.append(*m_diffIterator);
    }

    QString function = m_contextHunkHeader1.cap(1);
    int linenoA = m_contextHunkHeader2.cap(1).toInt();
    int linenoB = m_contextHunkHeader3.cap(1).toInt();

    DiffHunk* hunk = new DiffHunk(linenoA, linenoB, function);

    m_currentModel->addHunk(hunk);

    QStringList::Iterator oldIt = oldLines.begin();
    QStringList::Iterator newIt = newLines.begin();

    Difference* diff;
    while (oldIt != oldLines.end() || newIt != newLines.end()) {
        if(oldIt != oldLines.end() && m_contextHunkBodyRemoved.exactMatch(*oldIt)) {
            diff = new Difference(linenoA, linenoB);
            diff->setType(Difference::Delete);
            m_currentModel->addDiff(diff);
            hunk->add(diff);
            for (; oldIt != oldLines.end() && m_contextHunkBodyRemoved.exactMatch(*oldIt); ++oldIt) {
                diff->addSourceLine(m_contextHunkBodyRemoved.cap(1));
                linenoA++;
            }
        } else if(newIt != newLines.end() && m_contextHunkBodyAdded.exactMatch(*newIt)) {
            diff = new Difference(linenoA, linenoB);
            diff->setType(Difference::Insert);
            m_currentModel->addDiff(diff);
            hunk->add(diff);
            for (; newIt != newLines.end() && m_contextHunkBodyAdded.exactMatch(*newIt); ++newIt) {
                diff->addDestinationLine(m_contextHunkBodyAdded.cap(1));
                linenoB++;
            }
        } else if((oldIt == oldLines.end() || m_contextHunkBodyContext.exactMatch(*oldIt)) &&
                  (newIt == newLines.end() || m_contextHunkBodyContext.exactMatch(*newIt))) {
            diff = new Difference(linenoA, linenoB);
            // Do not add this diff with addDiff to the model... no unchanged differences allowed in there...
            diff->setType(Difference::Unchanged);
            hunk->add(diff);
            while ((oldIt == oldLines.end() || m_contextHunkBodyContext.exactMatch(*oldIt)) &&
                   (newIt == newLines.end() || m_contextHunkBodyContext.exactMatch(*newIt)) &&
                   (oldIt != oldLines.end() || newIt != newLines.end())) {
                QString l;
                if(oldIt != oldLines.end()) {
                    l = m_contextHunkBodyContext.cap(1);
                    ++oldIt;
                }
                if(newIt != newLines.end()) {
                    l = m_contextHunkBodyContext.cap(1);
                    ++newIt;
                }
                diff->addSourceLine(l);
                diff->addDestinationLine(l);
                linenoA++;
                linenoB++;
            }
        } else if((oldIt != oldLines.end() && m_contextHunkBodyChanged.exactMatch(*oldIt)) ||
                  (newIt != newLines.end() && m_contextHunkBodyChanged.exactMatch(*newIt))) {
            diff = new Difference(linenoA, linenoB);
            diff->setType(Difference::Change);
            m_currentModel->addDiff(diff);
            hunk->add(diff);
            while (oldIt != oldLines.end() && m_contextHunkBodyChanged.exactMatch(*oldIt)) {
                diff->addSourceLine(m_contextHunkBodyChanged.cap(1));
                linenoA++;
                ++oldIt;
            }
            while (newIt != newLines.end() && m_contextHunkBodyChanged.exactMatch(*newIt)) {
                diff->addDestinationLine(m_contextHunkBodyChanged.cap(1));
                linenoB++;
                ++newIt;
            }
        } else {
            return false;
        }
        diff->determineInlineDifferences();
    }

    return true;
}

bool ParserBase::parseEdHunkBody()
{
    return false;
}

bool ParserBase::parseNormalHunkBody()
{
    QString type;

    int linenoA = 0, linenoB = 0;

    if(m_normalDiffType == Difference::Insert) {
        linenoA = m_normalHunkHeaderAdded.cap(1).toInt();
        linenoB = m_normalHunkHeaderAdded.cap(2).toInt();
    } else if(m_normalDiffType == Difference::Delete) {
        linenoA = m_normalHunkHeaderRemoved.cap(1).toInt();
        linenoB = m_normalHunkHeaderRemoved.cap(3).toInt();
    } else if(m_normalDiffType == Difference::Change) {
        linenoA = m_normalHunkHeaderChanged.cap(1).toInt();
        linenoB = m_normalHunkHeaderChanged.cap(3).toInt();
    }

    DiffHunk* hunk = new DiffHunk(linenoA, linenoB);
    m_currentModel->addHunk(hunk);
    Difference* diff = new Difference(linenoA, linenoB);
    hunk->add(diff);
    m_currentModel->addDiff(diff);

    diff->setType(m_normalDiffType);

    if(m_normalDiffType == Difference::Change || m_normalDiffType == Difference::Delete) {
        for (; m_diffIterator != m_diffLines.end() && m_normalHunkBodyRemoved.exactMatch(*m_diffIterator); ++m_diffIterator) {
            diff->addSourceLine(m_normalHunkBodyRemoved.cap(1));
        }
    }

    if(m_normalDiffType == Difference::Change) {
        if(m_diffIterator != m_diffLines.end() && m_normalHunkBodyDivider.exactMatch(*m_diffIterator)) {
            ++m_diffIterator;
        } else {
            return false;
        }
    }

    if(m_normalDiffType == Difference::Insert || m_normalDiffType == Difference::Change) {
        for (; m_diffIterator != m_diffLines.end() && m_normalHunkBodyAdded.exactMatch(*m_diffIterator); ++m_diffIterator) {
            diff->addDestinationLine(m_normalHunkBodyAdded.cap(1));
        }
    }

    return true;
}

bool ParserBase::parseRCSHunkBody()
{
    return false;
}

bool ParserBase::matchesUnifiedHunkLine(QString line) const
{
    static const QChar context(' ');
    static const QChar added('+');
    static const QChar removed('-');

    QChar first = line[0];

    return (first == context || first == added || first == removed);
}

bool ParserBase::parseUnifiedHunkBody()
{

    int linenoA = 0, linenoB = 0;
    bool wasNum;

    // Fetching the stuff we need from the hunkheader regexp that was parsed in parseUnifiedHunkHeader();
    linenoA = m_unifiedHunkHeader.cap(1).toInt();
    int lineCountA = 1, lineCountB = 1; // an ommitted line count in the header implies a line count of 1
    if(!m_unifiedHunkHeader.cap(3).isEmpty()) {
        lineCountA = m_unifiedHunkHeader.cap(3).toInt(&wasNum);
        if(!wasNum) {
            return false;
        }

        // If a hunk is an insertion or deletion with no context, the line number given
        // is the one before the hunk. this isn't what we want, so increment it to fix this.
        if(lineCountA == 0) {
            linenoA++;
        }
    }
    linenoB = m_unifiedHunkHeader.cap(4).toInt();
    if(!m_unifiedHunkHeader.cap(6).isEmpty()) {
        lineCountB = m_unifiedHunkHeader.cap(6).toInt(&wasNum);
        if(!wasNum) {
            return false;
        }

        if(lineCountB == 0) { // see above
            linenoB++;
        }
    }
    QString function = m_unifiedHunkHeader.cap(7);

    DiffHunk* hunk = new DiffHunk(linenoA, linenoB, function);
    m_currentModel->addHunk(hunk);

    const QStringList::ConstIterator m_diffLinesEnd = m_diffLines.end();

    const QString context = QString(" ");
    const QString added = QString("+");
    const QString removed = QString("-");

    while (m_diffIterator != m_diffLinesEnd && matchesUnifiedHunkLine(*m_diffIterator) && (lineCountA || lineCountB)) {
        Difference* diff = new Difference(linenoA, linenoB);
        hunk->add(diff);

        if((*m_diffIterator).startsWith(context)) {    // context
            for (; m_diffIterator != m_diffLinesEnd && (*m_diffIterator).startsWith(context) && (lineCountA || lineCountB); ++m_diffIterator) {
                QString line = QString(*m_diffIterator);
                line = line.remove(0, 1);
                diff->addSourceLine(line);
                diff->addDestinationLine(line);
                linenoA++;
                linenoB++;
                --lineCountA;
                --lineCountB;
            }
        } else {    // This is a real difference, not context
            for (; m_diffIterator != m_diffLinesEnd && (*m_diffIterator).startsWith(removed) && (lineCountA || lineCountB); ++m_diffIterator) {
                diff->addSourceLine(QString(*m_diffIterator).remove(0, 1));
                linenoA++;
                --lineCountA;
            }
            for (; m_diffIterator != m_diffLinesEnd && (*m_diffIterator).startsWith(added) && (lineCountA || lineCountB); ++m_diffIterator) {
                diff->addDestinationLine(QString(*m_diffIterator).remove(0, 1));
                linenoB++;
                --lineCountB;
            }
            if(diff->sourceLineCount() == 0) {
                diff->setType(Difference::Insert);
            } else if(diff->destinationLineCount() == 0) {
                diff->setType(Difference::Delete);
            } else {
                diff->setType(Difference::Change);
            }
            diff->determineInlineDifferences();
            m_currentModel->addDiff(diff);
        }
    }

    return true;
}

void ParserBase::checkHeader(const QRegExp& header)
{
    if(m_diffIterator != m_diffLines.end()
       && !header.exactMatch(*m_diffIterator)
       && !m_diffIterator->startsWith("Index: ") /* SVN diff */
       && !m_diffIterator->startsWith("diff ") /* concatenated diff */
       && !m_diffIterator->startsWith("-- ") /* git format-patch */) {
           m_malformed = true;
    }
}

DiffModelMap* ParserBase::parseContext()
{
    while (parseContextDiffHeader()) {
        while (parseContextHunkHeader())
            parseContextHunkBody();
        if(m_currentModel->differenceCount() > 0) {
            m_models->append(m_currentModel);
        }
        checkHeader(m_contextDiffHeader1);
    }

    if(m_models->count() > 0) {
        return m_models;
    } else {
        delete m_models;
        return 0L;
    }
}

DiffModelMap* ParserBase::parseEd()
{
    while (parseEdDiffHeader()) {
        while (parseEdHunkHeader())
            parseEdHunkBody();
        if(m_currentModel->differenceCount() > 0) {
            m_models->append(m_currentModel);
        }
    }

    if(m_models->count() > 0) {
        return m_models;
    } else {
        delete m_models;
        return 0L;
    }
}

DiffModelMap* ParserBase::parseNormal()
{
    while (parseNormalDiffHeader()) {
        while (parseNormalHunkHeader())
            parseNormalHunkBody();
        if(m_currentModel->differenceCount() > 0) {
            m_models->append(m_currentModel);
        }
        checkHeader(m_normalDiffHeader);
    }

    if(m_singleFileDiff) {
        while (parseNormalHunkHeader())
            parseNormalHunkBody();
        if(m_currentModel->differenceCount() > 0) {
            m_models->append(m_currentModel);
        }
        if(m_diffIterator != m_diffLines.end()) {
            m_malformed = true;
        }
    }

    if(m_models->count() > 0) {
        return m_models;
    } else {
        delete m_models;
        return 0L;
    }
}

DiffModelMap* ParserBase::parseRCS()
{
    while (parseRCSDiffHeader()) {
        while (parseRCSHunkHeader())
            parseRCSHunkBody();
        if(m_currentModel->differenceCount() > 0) {
            m_models->append(m_currentModel);
        }
    }

    if(m_models->count() > 0) {
        return m_models;
    } else {
        delete m_models;
        return 0L;
    }
}

DiffModelMap* ParserBase::parseUnified()
{
    while (parseUnifiedDiffHeader()) {
        while (parseUnifiedHunkHeader())
            parseUnifiedHunkBody();
        if(m_currentModel->differenceCount() > 0) {
            m_models->append(m_currentModel);
        }
        checkHeader(m_unifiedDiffHeader1);
    }

    if(m_models->count() > 0) {
        return m_models;
    } else {
        delete m_models;
        return 0L;
    }
}

