//
// Created by john on 3/10/17.
//

#include "kdiffview.h"
KDiffView::KDiffView(QWidget *parent)
    : QFrame(parent)
{
    setObjectName( "KDiffFrame" );
    m_splitter = new QSplitter(this);
    m_splitter->setOrientation(Qt::Vertical);
};

KDiffView::~KDiffView()
{
};
