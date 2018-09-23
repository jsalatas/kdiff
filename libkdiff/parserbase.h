/**************************************************************************
**                             parserbase.h
**                             -------------
**      begin                   : Tue Jul 30 23:53:52 2002
**      Copyright 2002-2004 Otto Bruggeman <otto.bruggeman@home.nl>
**      Copyright 2010 Kevin Kofler   <kevin.kofler@chello.at>
***************************************************************************/
/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   ( at your option ) any later version.
**
***************************************************************************/

#ifndef DIFF2_PARSERBASE_H
#define DIFF2_PARSERBASE_H

#include <QtCore/QRegExp>

#include "info.h"
#include "difference.h"
#include "diffmodelmap.h"
#include "diffmodel.h"

class QStringList;

class QString;

class ModelList;

class ParserBase
{
public:
    ParserBase(const QStringList& diff);
    virtual ~ParserBase();

    static QString escapePath(QString path);
    static QString unescapePath(QString path);

public:
    enum Compare::Format format()
    {return determineFormat();};
    DiffModelMap* parse(bool* malformed = 0);

protected:
    virtual bool parseContextDiffHeader();
    virtual bool parseEdDiffHeader();
    virtual bool parseNormalDiffHeader();
    virtual bool parseRCSDiffHeader();
    virtual bool parseUnifiedDiffHeader();

    virtual bool parseContextHunkHeader();
    virtual bool parseEdHunkHeader();
    virtual bool parseNormalHunkHeader();
    virtual bool parseRCSHunkHeader();
    virtual bool parseUnifiedHunkHeader();

    virtual bool parseContextHunkBody();
    virtual bool parseEdHunkBody();
    virtual bool parseNormalHunkBody();
    virtual bool parseRCSHunkBody();
    virtual bool parseUnifiedHunkBody();

    virtual DiffModelMap* parseContext();
    virtual DiffModelMap* parseEd();
    virtual DiffModelMap* parseNormal();
    virtual DiffModelMap* parseRCS();
    virtual DiffModelMap* parseUnified();

protected: // Helper methods to speed things up
    bool matchesUnifiedHunkLine(QString line) const;
    void checkHeader(const QRegExp& header);

protected:
    /** What is format of the diff */
    virtual enum Compare::Format determineFormat();

protected:
    // Regexps for context parsing
    QRegExp m_contextDiffHeader1;
    QRegExp m_contextDiffHeader2;

    QRegExp m_contextHunkHeader1;
    QRegExp m_contextHunkHeader2;
    QRegExp m_contextHunkHeader3;

    QRegExp m_contextHunkBodyRemoved;
    QRegExp m_contextHunkBodyAdded;
    QRegExp m_contextHunkBodyChanged;
    QRegExp m_contextHunkBodyContext;
    QRegExp m_contextHunkBodyLine; // Added for convenience

    // Regexps for normal parsing
    QRegExp m_normalDiffHeader;

    QRegExp m_normalHunkHeaderAdded;
    QRegExp m_normalHunkHeaderRemoved;
    QRegExp m_normalHunkHeaderChanged;

    QRegExp m_normalHunkBodyRemoved;
    QRegExp m_normalHunkBodyAdded;
    QRegExp m_normalHunkBodyDivider;

    enum Difference::Type m_normalDiffType;

    // RegExps for rcs parsing
    QRegExp m_rcsDiffHeader;

    // Regexps for unified parsing
    QRegExp m_unifiedDiffHeader1;
    QRegExp m_unifiedDiffHeader2;

    // Regexps for renamed files
    QRegExp m_renameFromHeader;
    QRegExp m_renameToHeader;

    QRegExp m_unifiedHunkHeader;

    QRegExp m_unifiedHunkBodyAdded;
    QRegExp m_unifiedHunkBodyRemoved;
    QRegExp m_unifiedHunkBodyContext;
    QRegExp m_unifiedHunkBodyLine; // Added for convenience

protected:
    const QStringList& m_diffLines;
    DiffModel* m_currentModel;
    DiffModelMap* m_models;
    QStringList::ConstIterator m_diffIterator;

    bool m_singleFileDiff;
    bool m_malformed;

//protected:
//    const ModelList* m_list;
};

#endif
