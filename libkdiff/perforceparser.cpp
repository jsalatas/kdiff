/**************************************************************************
**                              perforceparser.cpp
**                              ------------------
**      begin                   : Sun Aug  4 15:05:35 2002
**      Copyright 2002-2004 Otto Bruggeman <otto.bruggeman@home.nl>
***************************************************************************/
/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   ( at your option ) any later version.
**
***************************************************************************/

#include "perforceparser.h"

#include <QtCore/QRegExp>

#include <QLoggingCategory>

PerforceParser::PerforceParser(const QStringList& diff)
        : ParserBase(diff)
{
    m_contextDiffHeader1.setPattern("==== (.*) - (.*) ====\\n");
    m_contextDiffHeader1.setMinimal(true);
    m_normalDiffHeader.setPattern("==== (.*) - (.*) ====\\n");
    m_normalDiffHeader.setMinimal(true);
    m_rcsDiffHeader.setPattern("==== (.*) - (.*) ====\\n");
    m_rcsDiffHeader.setMinimal(true);
    m_unifiedDiffHeader1.setPattern("==== (.*) - (.*) ====\\n");
    m_unifiedDiffHeader1.setMinimal(true);
}

PerforceParser::~PerforceParser()
{
}

enum Compare::Format PerforceParser::determineFormat()
{
    QRegExp unifiedRE("^@@");
    QRegExp contextRE("^\\*{15}");
    QRegExp normalRE("^\\d+(|,\\d+)[acd]\\d+(|,\\d+)");
    QRegExp rcsRE("^[acd]\\d+ \\d+");
    // Summary is not supported since it gives no useful parsable info

    QStringList::ConstIterator it = m_diffLines.begin();

    while (it != m_diffLines.end()) {
        if(it->indexOf(unifiedRE, 0) == 0) {
            return Compare::Unified;
        } else if(it->indexOf(contextRE, 0) == 0) {
            return Compare::Context;
        } else if(it->indexOf(normalRE, 0) == 0) {
            return Compare::Normal;
        } else if(it->indexOf(rcsRE, 0) == 0) {
            return Compare::RCS;
        }
        ++it;
    }
    return Compare::UnknownFormat;
}

bool PerforceParser::parseContextDiffHeader()
{
    bool result = false;

    QStringList::ConstIterator itEnd = m_diffLines.end();

    QRegExp sourceFileRE("([^\\#]+)#(\\d+)");
    QRegExp destinationFileRE("([^\\#]+)#(|\\d+)");

    while (m_diffIterator != itEnd) {
        if(m_contextDiffHeader1.exactMatch(*(m_diffIterator)++)) {
            m_currentModel = new DiffModel();
            sourceFileRE.exactMatch(m_contextDiffHeader1.cap(1));
            destinationFileRE.exactMatch(m_contextDiffHeader1.cap(2));
            m_currentModel->setSourceFile(sourceFileRE.cap(1));
            m_currentModel->setDestinationFile(destinationFileRE.cap(1));

            result = true;

            break;
        }
    }

    return result;
}

bool PerforceParser::parseNormalDiffHeader()
{
    bool result = false;

    QStringList::ConstIterator itEnd = m_diffLines.end();

    QRegExp sourceFileRE("([^\\#]+)#(\\d+)");
    QRegExp destinationFileRE("([^\\#]+)#(|\\d+)");

    while (m_diffIterator != itEnd) {
        if(m_normalDiffHeader.exactMatch(*(m_diffIterator)++)) {
            m_currentModel = new DiffModel();
            sourceFileRE.exactMatch(m_normalDiffHeader.cap(1));
            destinationFileRE.exactMatch(m_normalDiffHeader.cap(2));
            m_currentModel->setSourceFile(sourceFileRE.cap(1));
            m_currentModel->setDestinationFile(destinationFileRE.cap(1));

            result = true;

            break;
        }
    }

    return result;
}

bool PerforceParser::parseRCSDiffHeader()
{
    return false;
}

bool PerforceParser::parseUnifiedDiffHeader()
{
    bool result = false;

    QStringList::ConstIterator itEnd = m_diffLines.end();

    QRegExp sourceFileRE("([^\\#]+)#(\\d+)");
    QRegExp destinationFileRE("([^\\#]+)#(|\\d+)");

    while (m_diffIterator != itEnd) {
        if(m_unifiedDiffHeader1.exactMatch(*(m_diffIterator)++)) {
            m_currentModel = new DiffModel();
            sourceFileRE.exactMatch(m_unifiedDiffHeader1.cap(1));
            destinationFileRE.exactMatch(m_unifiedDiffHeader1.cap(2));
            m_currentModel->setSourceFile(sourceFileRE.cap(1));
            m_currentModel->setDestinationFile(destinationFileRE.cap(1));

            result = true;

            break;
        }
    }

    return result;
}

