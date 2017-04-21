//
// Created by john on 3/10/17.
//

#ifndef KDIFF_KDIFFVIEW_H
#define KDIFF_KDIFFVIEW_H


#include <QFrame>
#include <QSplitter>

class KDiffView: public QFrame
{
    Q_OBJECT
public:
    KDiffView(QWidget *parent);
    ~KDiffView();
    QSplitter *splitter() { return m_splitter; }

private:
    QSplitter *m_splitter;
};

#endif //KDIFF_KDIFFVIEW_H
