//
// Created by john on 3/11/17.
//

#ifndef KDIFF_CONNECTWIDGET_H
#define KDIFF_CONNECTWIDGET_H

#include <QWidget>
#include <QPen>

class FileView;

class ConnectWidget : public QWidget {
    Q_OBJECT
public:
    ConnectWidget(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    ~ConnectWidget();

protected:
    void paintEvent(QPaintEvent* );

private slots:
    void updatePolygons();
private:
    FileView *m_parentWidget;
    QColor m_diffColor;
    QColor m_activeDiffColor;
    QColor m_activePenColor;
    QList<QPolygon>* m_polygons;
    bool m_penInitialized;
    QRect m_sourceTextArea;
    QRect m_destinationTextArea;
};


#endif //KDIFF_CONNECTWIDGET_H
