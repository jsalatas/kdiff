/**************************************************************************
**                             parser.cpp
**                             ----------
**      begin                   : Sun Aug  4 15:05:35 2002
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

#include "parser.h"

#include <QLoggingCategory>

#include "cvsdiffparser.h"
#include "diffparser.h"
#include "perforceparser.h"
#include "diffmodel.h"

Parser::Parser()
{
}

Parser::~Parser()
{
}

int Parser::cleanUpCrap(QStringList& diffLines)
{
    QStringList::Iterator it = diffLines.begin();

    int nol = 0;

    QString noNewLine("\\ No newline");

    for (; it != diffLines.end(); ++it) {
        if((*it).startsWith(noNewLine)) {
            it = diffLines.erase(it);
            // correcting the advance of the iterator because of the remove
            --it;
            QString temp(*it);
            temp.truncate(temp.indexOf('\n'));
            *it = temp;
            ++nol;
        }
    }

    return nol;
}

DiffModelMap* Parser::parse(QStringList& diffLines, bool* malformed)
{
    /* Basically determine the generator then call the parse method */
    ParserBase* parser;

    m_generator = determineGenerator(diffLines);

    cleanUpCrap(diffLines);

    switch (m_generator) {
        case Compare::CVSDiff :
            parser = new CVSDiffParser(diffLines);
            break;
        case Compare::Diff :
            parser = new DiffParser(diffLines);
            break;
        case Compare::Perforce :
            parser = new PerforceParser(diffLines);
            break;
        default:
            // Nothing to delete, just leave...
            return 0L;
    }

    m_format = parser->format();
    DiffModelMap* modelList = parser->parse(malformed);

    delete parser;

    return modelList;
}

enum Compare::Generator Parser::determineGenerator(const QStringList& diffLines)
{
    // Shit have to duplicate some code with this method and the ParserBase derived classes
    QString cvsDiff("Index: ");
    QString perforceDiff("==== ");

    QStringList::ConstIterator it = diffLines.begin();
    QStringList::ConstIterator linesEnd = diffLines.end();

    while (it != linesEnd) {
        if((*it).startsWith(cvsDiff)) {
            return Compare::CVSDiff;
        } else if((*it).startsWith(perforceDiff)) {
            return Compare::Perforce;
        }
        ++it;
    }

    // For now we'll assume it is a diff file diff, later we might
    // try to really determine if it is a diff file diff.
    return Compare::Diff;
}
