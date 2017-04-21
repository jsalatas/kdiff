/***************************************************************************
                                difference.cpp
                                --------------
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

#include "difference.h"
#include "differencestringpair.h"
#include "levenshteintable.h"

Difference::Difference( int sourceLineNo, int destinationLineNo, int type ) :
	QObject(),
	m_type( type ),
	m_sourceLineNo( sourceLineNo ),
	m_destinationLineNo( destinationLineNo ),
	m_conflicts( false ),
	m_swapped(false)
{
}

Difference::~Difference()
{
	qDeleteAll( m_sourceLines );
	qDeleteAll( m_destinationLines );
}

void Difference::addSourceLine( QString line )
{
	m_sourceLines.append( new DifferenceString( line ) );
}

void Difference::addDestinationLine( QString line )
{
	m_destinationLines.append( new DifferenceString( line ) );
}

int Difference::sourceLineCount() const
{
	return m_sourceLines.count();
}

int Difference::destinationLineCount() const
{
	return m_destinationLines.count();
}

int Difference::sourceLineEnd() const
{
	return m_sourceLineNo + m_sourceLines.count();
}

int Difference::destinationLineEnd() const
{
	return m_destinationLineNo + m_destinationLines.count();
}

void Difference::determineInlineDifferences()
{
	if ( m_type != Difference::Change )
		return;

	// Do nothing for now when the slc != dlc
	// One could try to find the closest matching destination string for any
	// of the source strings but this is compute intensive
	int slc = sourceLineCount();

	if ( slc != destinationLineCount() )
		return;

	LevenshteinTable<DifferenceStringPair> table;

	for ( int i = 0; i < slc; ++i )
	{
		DifferenceString* sl = sourceLineAt( i );
		DifferenceString* dl = destinationLineAt( i );
		DifferenceStringPair* pair = new DifferenceStringPair(sl, dl);

		// return value 0 means something went wrong creating the table so dont bother finding markers
		if ( table.createTable( pair ) != 0 )
			table.createListsOfMarkers();
	}
}

void Difference::swap() {
	if(m_swapped)
		return;

	m_swapped = true;
    int tmp = m_sourceLineNo;
    m_sourceLineNo = m_destinationLineNo;
    m_destinationLineNo = tmp;

    DifferenceStringList tmpList = m_sourceLines;
    m_sourceLines = m_destinationLines;
    m_destinationLines = tmpList;

}