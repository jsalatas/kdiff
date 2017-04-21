/***************************************************************************
                                diffhunk.cpp
                                ------------
        begin                   : Sun Mar 4 2001
        Copyright 2001-2004,2009 Otto Bruggeman <bruggie@gmail.com>
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

#include "diffhunk.h"

#include "difference.h"

#include <QLoggingCategory>

DiffHunk::DiffHunk(int sourceLine, int destinationLine, QString function, Type type)
        :
        m_sourceLine(sourceLine)
        , m_destinationLine(destinationLine)
        , m_function(function)
        , m_type(type)
{
}

DiffHunk::~DiffHunk()
{
}

void DiffHunk::add(Difference* diff)
{
    m_differences.append(diff);
}

int DiffHunk::sourceLineCount() const
{
    auto diffIt = m_differences.begin();
    auto dEnd = m_differences.end();

    int lineCount = 0;

    for (; diffIt != dEnd; ++diffIt)
        lineCount += (*diffIt)->sourceLineCount();

    return lineCount;
}

int DiffHunk::destinationLineCount() const
{
    auto diffIt = m_differences.begin();
    auto dEnd = m_differences.end();

    int lineCount = 0;

    for (; diffIt != dEnd; ++diffIt)
        lineCount += (*diffIt)->destinationLineCount();

    return lineCount;
}

void DiffHunk::swap()
{
    int tmp = m_sourceLine;
    m_sourceLine = m_destinationLine;
    m_destinationLine = tmp;

    //TODO: maybe I don't need it. Seems that every different is in a hunk as well
    // See m_swapped property in difference class
    auto diffIt = m_differences.begin();
    auto dEnd = m_differences.end();
    for (; diffIt != dEnd; ++diffIt) {
        (*diffIt)->swap();
    }
}