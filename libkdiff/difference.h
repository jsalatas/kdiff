/***************************************************************************
                                difference.h
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

#ifndef DIFFERENCE_H
#define DIFFERENCE_H

#include <qvector.h>

#include <QLoggingCategory>
#include "kdiffparser_export.h"
#include "marker.h"

class QString;

class KDIFFPARSER_EXPORT DifferenceString
{
public:
    DifferenceString()
    {
    }
    explicit DifferenceString(const QString& string, const MarkerList& markerList = MarkerList())
            :
            m_string(string)
            , m_markerList(markerList)
    {
        calculateHash();
    }
    DifferenceString(const DifferenceString& ds)
            :
            m_string(ds.m_string)
            , m_conflict(ds.m_conflict)
            , m_hash(ds.m_hash)
            , m_markerList(ds.m_markerList)
    {
    }
    ~DifferenceString()
    {
        qDeleteAll(m_markerList);
    }

public:
    const QString& string() const
    {
        return m_string;
    }
    const QString& conflictString() const
    {
        return m_conflict;
    }
    const MarkerList& markerList()
    {
        return m_markerList;
    }
    void setString(const QString& string)
    {
        m_string = string;
        calculateHash();
    }
    void setConflictString(const QString& conflict)
    {
        m_conflict = conflict;
    }
    void setMarkerList(const MarkerList& markerList)
    {
        m_markerList = markerList;
    }
    void prepend(Marker* marker)
    {
        m_markerList.prepend(marker);
    }
    bool operator==(const DifferenceString& ks)
    {
        if(m_hash != ks.m_hash) {
            return false;
        }
        return m_string == ks.m_string;
    }

protected:
    void calculateHash()
    {
        unsigned short const* str = reinterpret_cast<unsigned short const*>( m_string.unicode());
        const unsigned int len = m_string.length();

        m_hash = 1315423911;

        for (unsigned int i = 0; i < len; i++) {
            m_hash ^= (m_hash << 5) + str[i] + (m_hash >> 2);
        }
    }

private:
    QString m_string;
    QString m_conflict;
    unsigned int m_hash;
    MarkerList m_markerList;
};

typedef QVector<DifferenceString*> DifferenceStringList;

class KDIFFPARSER_EXPORT Difference : public QObject
{
Q_OBJECT
public:
    enum Type
    {
        Change, Insert, Delete, Unchanged
    };

public:
    Difference(int sourceLineNo, int destinationLineNo, int type = Difference::Unchanged);
    ~Difference();

public:
    int type() const
    {return m_type;};

    int sourceLineNumber() const
    {return m_sourceLineNo;}
    int destinationLineNumber() const
    {return m_destinationLineNo;}

    int sourceLineCount() const;
    int destinationLineCount() const;

    int sourceLineEnd() const;
    int destinationLineEnd() const;

    DifferenceString* sourceLineAt(int i) const
    {return m_sourceLines[i];}
    DifferenceString* destinationLineAt(int i) const
    {return m_destinationLines[i];}

    const DifferenceStringList sourceLines() const
    {return m_sourceLines;}
    const DifferenceStringList destinationLines() const
    {return m_destinationLines;}

    bool hasConflict() const
    {
        return m_conflicts;
    }
    void setConflict(bool conflicts)
    {
        m_conflicts = conflicts;
    }

    void setType(int type)
    {m_type = type;}

    void addSourceLine(QString line);
    void addDestinationLine(QString line);

    /** This method will calculate the differences between the individual strings and store them as Markers */
    void determineInlineDifferences();

    void swap();

signals:
    void differenceApplied(Difference*);

private:
    int m_type;

    int m_sourceLineNo;
    int m_destinationLineNo;

    DifferenceStringList m_sourceLines;
    DifferenceStringList m_destinationLines;

    bool m_conflicts;
    bool m_swapped;
};

typedef QList<Difference*> DifferenceList;

#endif

