/*******************************************************************************
**
** Filename   : diffmodellist.cpp
** Created on : 26 march, 2004
** Copyright 2004 Otto Bruggeman <bruggie@gmail.com>
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

#include "diffmodelmap.h"
#include "utils.h"

DiffModelMap::DiffModelMap()
{
}

DiffModelMap::~DiffModelMap()
{
}

void DiffModelMap::append(DiffModel* model) {
    if(model->key().isEmpty()) {
        // this would be be case of new or deleted files having /dev/null in
        // as destination or source respectively. We don't add them here.
        // Will pickup when collecting local files.
        //TODO: keep them somewhere else
        // TODO: override insert method and make it come through append instead
        return;
    }
    insert(model->key(), model);

}