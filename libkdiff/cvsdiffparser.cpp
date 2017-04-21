/**************************************************************************
**                              cvsdiffparser.cpp
**                              -----------------
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

#include "cvsdiffparser.h"

#include <QtCore/QRegExp>

#include <QLoggingCategory>

#include "modellist.h"

CVSDiffParser::CVSDiffParser(const QStringList& diff)
        : ParserBase(diff)
{
    // The regexps needed for context cvs diff parsing, the rest is the same as in parserbase.cpp
    // third capture in header1 is non optional for cvs diff, it is the revision
    m_contextDiffHeader1.setPattern("\\*\\*\\* ([^\\t]+)\\t([^\\t]+)\\t(.*)\\n");
    m_contextDiffHeader2.setPattern("--- ([^\\t]+)\\t([^\\t]+)(|\\t(.*))\\n");

    m_normalDiffHeader.setPattern("Index: (.*)\\n");
}

CVSDiffParser::~CVSDiffParser()
{
}

enum Compare::Format CVSDiffParser::determineFormat()
{
    QRegExp normalRE("[0-9]+[0-9,]*[acd][0-9]+[0-9,]*");
    QRegExp unifiedRE("^--- [^\\t]+\\t");
    QRegExp contextRE("^\\*\\*\\* [^\\t]+\\t");
    QRegExp rcsRE("^[acd][0-9]+ [0-9]+");
    QRegExp edRE("^[0-9]+[0-9,]*[acd]");

    QStringList::ConstIterator it = m_diffLines.begin();

    while (it != m_diffLines.end()) {
        if((*it).indexOf(normalRE, 0) == 0) {
            return Compare::Normal;
        } else if((*it).indexOf(unifiedRE, 0) == 0) {
            return Compare::Unified;
        } else if((*it).indexOf(contextRE, 0) == 0) {
            return Compare::Context;
        } else if((*it).indexOf(rcsRE, 0) == 0) {
            return Compare::RCS;
        } else if((*it).indexOf(edRE, 0) == 0) {
            return Compare::Ed;
        }
        ++it;
    }
    return Compare::UnknownFormat;
}

bool CVSDiffParser::parseNormalDiffHeader()
{
    bool result = false;

    QStringList::ConstIterator diffEnd = m_diffLines.end();

    while (m_diffIterator != diffEnd) {
        if(m_normalDiffHeader.exactMatch(*m_diffIterator)) {
            m_currentModel = new DiffModel();
            m_currentModel->setSourceFile(m_normalDiffHeader.cap(1));
            m_currentModel->setDestinationFile(m_normalDiffHeader.cap(1));

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


bool CVSDiffParser::parseEdDiffHeader()
{
    return false;
}

bool CVSDiffParser::parseRCSDiffHeader()
{
    return false;
}

bool CVSDiffParser::parseEdHunkHeader()
{
    return false;
}

bool CVSDiffParser::parseRCSHunkHeader()
{
    return false;
}

bool CVSDiffParser::parseEdHunkBody()
{
    return false;
}

bool CVSDiffParser::parseRCSHunkBody()
{
    return false;
}

