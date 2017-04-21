/*******************************************************************************
**
** Filename   : diffmodellist.h
** Created on : 24 januari, 2004
** Copyright 2004-2005, 2009 Otto Bruggeman <bruggie@gmail.com>
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
*******************************************************************************/

#ifndef DIFFMODELLIST_H
#define DIFFMODELLIST_H

#include <QMap>

#include "diffmodel.h"
#include "kdiffparser_export.h"


class KDIFFPARSER_EXPORT DiffModelMap : public QMap<QString, DiffModel*>
{
public:
    DiffModelMap();
    virtual ~DiffModelMap();
    void append(DiffModel* model);
};

#endif
