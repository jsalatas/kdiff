//
// Created by john on 2/16/17.
//

#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <KColorScheme>
#include "diffscrollbar.h"
#include "fileview.h"
#include "filenavigatoritem.h"

DiffScrollBar::DiffScrollBar(FileView* parent)
        : QScrollBar(parent)
        , m_fileView(parent)
        , m_missing(new QList<int>())
        , m_lineColors(nullptr)
        , m_differentColor(new QColor(200, 0, 0))
        , m_missingColor(new QColor(210, 210, 210))
{
}

DiffScrollBar::~DiffScrollBar() {
    m_missing->clear();
    delete m_missing;
    m_missing = nullptr;

    if(m_lineColors != nullptr) {
        qDeleteAll(m_lineColors->begin(), m_lineColors->end());
        m_lineColors->clear();
        delete m_lineColors;
        m_lineColors = nullptr;
    }

    delete m_differentColor;
    m_differentColor = nullptr;
    delete m_missingColor;
    m_missingColor = nullptr;
}

void DiffScrollBar::setSliderPosition(int pos) {
    // account for missing lines in longest view
    int missingOffset = 0;
    for (int i = 0; i < m_missing->length(); ++i) {
        if(m_missing->at(i)< pos) {
            missingOffset++;
        } else {
            break;
        }
    }

    pos+=missingOffset;
    QScrollBar::setSliderPosition(pos);
    //update();
}

void DiffScrollBar::moveTo(QPoint point) {
    float ratio = 1. * (m_lines + m_missing->length()) / (height() - 6);

    int offset = (m_fileView->visibleEnd() - m_fileView->visibleStart()) / 2;

    int pos = point.y() * ratio - offset;

    // account for missing lines in longest view
    int missingOffset = 0;
    for (int i = 0; i < m_missing->length(); ++i) {
        if (m_missing->at(i) <= pos) {
            missingOffset++;
        } else {
            break;
        }
    }

    pos -= missingOffset;

    if (pos < 0)
        pos = 0;
    if (pos > maximum())
        pos = maximum();
    setSliderPosition(pos);

}

void DiffScrollBar::mouseMoveEvent(QMouseEvent* e) {
//    QScrollBar::mouseMoveEvent(e);
    moveTo(e->pos());
}

void DiffScrollBar::mousePressEvent(QMouseEvent* e) {
//    QScrollBar::mousePressEvent(e);
    moveTo(e->pos());
}

void DiffScrollBar::paintEvent(QPaintEvent *)
{
    if(m_lineColors != nullptr) {
        QPainter painter(this);

        QPalette p = QPalette();

        KStatefulBrush brushBackground(KColorScheme::View, KColorScheme::NormalBackground);
        KStatefulBrush brushForeGround(KColorScheme::Button, KColorScheme::NormalText);
        const QColor whiteColor = brushBackground.brush(this).color();
        const QColor blackColor = brushForeGround.brush(this).color();
        QRect fillRect = QRect(0, 0, width(), height());

        painter.setBrush(QBrush(whiteColor));

        QPen pen = painter.pen();
        pen.setColor(blackColor);
        pen.setWidth(1);
        painter.setPen(pen);

        painter.setRenderHint(QPainter::Antialiasing);

        painter.drawRect(fillRect);

        QPixmap pixbuf = QPixmap(width() - 6, m_lines);
        QPainter paint(&pixbuf);
        QPainter* pixbufPainter = &paint;


        pen = pixbufPainter->pen();
        pixbufPainter->fillRect( 0, 0, width(), m_lines + m_missing->length(), whiteColor);

        int start = pixbuf.rect().left()+3;
        int end = pixbuf.rect().right()-3;
        int middle = (end - start) /2 + start;

        QMap<int, LineColors*>::iterator i = m_lineColors->begin();
        while(i != m_lineColors->end()) {
            pen.setColor(QColor(*i.value()->sourceColor));
            pixbufPainter->setPen(pen);
            pixbufPainter->drawLine(QLine(start, i.key(), middle, i.key()));
            pen.setColor(QColor(*i.value()->destinationColor));
            pixbufPainter->setPen(pen);
            pixbufPainter->drawLine(QLine(middle + 1, i.key(), end, i.key()));
            i++;
        }
        painter.setRenderHint(QPainter::Antialiasing, false);


        int visibleStart = m_fileView->visibleStart();
        int visibleEnd = m_fileView->visibleEnd();
        pen.setColor(blackColor);
        pen.setWidth(2);
        pixbufPainter->setPen(pen);
        pixbufPainter->drawLine(QLine(start - 2, visibleStart+1, start - 2, visibleEnd));
        pixbufPainter->drawLine(QLine(end+3, visibleStart+1, end+3, visibleEnd));


        painter.drawImage(3, 3, pixbuf.toImage().scaled(QSize(pixbuf.width(), height()-6)));
    }
}

void DiffScrollBar::update(FileNavigatorItem *item, int sourceLines, int destinationLines) {
    setUpdatesEnabled(false);
    if(m_lineColors != nullptr) {
        qDeleteAll(m_lineColors->begin(), m_lineColors->end());
        m_lineColors->clear();
        delete m_lineColors;
        m_lineColors = nullptr;
    }

    m_lines = std::max(sourceLines, destinationLines);
    bool sourceIsLonger = sourceLines > destinationLines;

    // account for missing lines in longer view
    m_missing->clear();
    if(!item)
        return;

    for (int i = 0; i < m_lines; ++i) {
        const Difference *diff = sourceIsLonger ? item->diffContainingSourceLine(i, true) : item->diffContainingDestinationLine(
                i, true);
        if (diff != nullptr) {
            if ((sourceIsLonger && diff->sourceLineCount() == 0) || (!sourceIsLonger && diff->destinationLineCount() == 0)) {
                m_missing->append(i);
            }
        }
    }

    m_lineColors = new QMap<int, LineColors *>();
    bool includeMissing = true;
    LineColors *lineColors;
    for (int i = 0; i < m_lines + m_missing->length(); ++i) {
        const Difference *diff = sourceIsLonger ? item->diffContainingSourceLine(i, includeMissing) : item->diffContainingDestinationLine(
                i, includeMissing);
        includeMissing = true;
        if (diff != nullptr) {
            if (diff->sourceLineCount() == 0 /*&& sourceIsLonger*/) {
                lineColors = new LineColors();
                lineColors->sourceColor = m_missingColor;
                lineColors->destinationColor = m_differentColor;
                m_lineColors->insert(i, lineColors);
                if (sourceIsLonger /*&& i == diff->destinationLineCount() - 1*/) {
                    i++;
                    includeMissing = false;
                }
            } else if (diff->destinationLineCount() == 0 /*&& !sourceIsLonger*/) {
                lineColors = new LineColors();
                lineColors->sourceColor = m_differentColor;
                lineColors->destinationColor = m_missingColor;
                m_lineColors->insert(i, lineColors);
                if (!sourceIsLonger /*&& i == diff->sourceLineCount() - 1*/) {
                    i++;
                    includeMissing = false;
                }
            } else {
                for (int j = 0; j < (sourceIsLonger ? diff->sourceLineCount() : diff->destinationLineCount()); ++j) {
                    if (j < (sourceIsLonger ? diff->sourceLineCount() : diff->destinationLineCount())) {
                        lineColors = new LineColors();
                        lineColors->sourceColor = m_differentColor;
                        lineColors->destinationColor = m_differentColor;
                        m_lineColors->insert(i + j, lineColors);
                    } else {
                        lineColors->sourceColor = m_differentColor;
                        lineColors->destinationColor = m_missingColor;
                        m_lineColors->insert(i + j, lineColors);
                    }
                }
                i += sourceIsLonger ? diff->sourceLineCount() : diff->destinationLineCount();
            }
        }
    }

    setUpdatesEnabled(true);
}
