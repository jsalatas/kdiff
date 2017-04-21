//
// Created by john on 3/11/17.
//

#include <QPainter>

#include "connectwidget.h"
#include "fileview.h"

ConnectWidget::ConnectWidget(QWidget *parent, Qt::WindowFlags f)
        : QWidget(parent, f), m_parentWidget((FileView *) parent), m_polygons(nullptr), m_penInitialized(false) {
    m_diffColor = QColor(255, 0, 0, 40);
    m_activeDiffColor = QColor(255, 0, 0, 20);
    m_activePenColor = QColor(255, 0, 0, 100);
    connect(m_parentWidget, SIGNAL(polygonsUpdated()), this, SLOT(updatePolygons()));

}

ConnectWidget::~ConnectWidget() {
    if (m_polygons != nullptr) {
        m_polygons->clear();
        delete m_polygons;
        m_polygons = nullptr;
    }
}

void ConnectWidget::updatePolygons() {
    setUpdatesEnabled(false);
    if (this->m_polygons != nullptr) {
        this->m_polygons->clear();
        delete this->m_polygons;
    }
    m_polygons = m_parentWidget->overlays();
    m_sourceTextArea = m_parentWidget->sourceTextArea();
    m_destinationTextArea = m_parentWidget->destinationTextArea();

    setUpdatesEnabled(true);
}

void ConnectWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    QPen normalPen = painter.pen();
    normalPen.setStyle(Qt::NoPen);

    QPen activePen = painter.pen();
    activePen.setStyle(Qt::SolidLine);
    activePen.setColor(m_activePenColor);
    activePen.setWidth(2);

    painter.setRenderHint(QPainter::Antialiasing);
    if (m_polygons != nullptr) {
        for (int i = 0; i < m_polygons->size(); ++i) {
            if (i == m_parentWidget->activeDiffNumber()) {
                painter.setPen(activePen);
                painter.setBrush(QBrush(m_activeDiffColor));
            } else {
                painter.setPen(normalPen);
                painter.setBrush(QBrush(m_diffColor));
            }
            painter.drawPolygon(m_polygons->at(i));
        }
    }

    // check if area is changed
    QRect sourceTextArea = m_parentWidget->sourceTextArea();
    QRect destinationTextArea = m_parentWidget->destinationTextArea();
    if (sourceTextArea.left() != m_sourceTextArea.left() ||
        sourceTextArea.top() != m_sourceTextArea.top() ||
        sourceTextArea.bottom() != m_sourceTextArea.bottom() ||
        sourceTextArea.right() != m_sourceTextArea.right() ||
        destinationTextArea.left() != m_destinationTextArea.left() ||
        destinationTextArea.top() != m_destinationTextArea.top() ||
        destinationTextArea.bottom() != m_destinationTextArea.bottom() ||
        destinationTextArea.right() != m_destinationTextArea.right()) {
        updatePolygons();
    }

}

