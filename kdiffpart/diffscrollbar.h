//
// Created by john on 2/16/17.
//

#ifndef KDIFF_DIFFSSCROLLBAR_H
#define KDIFF_DIFFSSCROLLBAR_H

#include <QScrollBar>
#include <QAbstractSlider>
#include "diffmodel.h"

class FileView;
class FileNavigatorItem;

struct LineColors
{
    QColor *sourceColor;
    QColor *destinationColor;
};

class DiffScrollBar : public QScrollBar
{
Q_OBJECT
public:
    DiffScrollBar(FileView* parent);
    ~DiffScrollBar();
    void setSliderPosition(int);

public slots:
    void update(FileNavigatorItem *item, int sourceLines, int destinationLines);

protected:
    void mousePressEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;

private:
    int m_lines;
    FileView* m_fileView;
    QList<int>* m_missing;
    QMap<int, LineColors*>* m_lineColors;
    QColor *m_differentColor;
    QColor *m_missingColor;
    void moveTo(QPoint point);
};

#endif //KDIFF_DIFFSSCROLLBAR_H
