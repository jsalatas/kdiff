//
// Created by john on 2/19/17.
//

#ifndef KDIFF_LINEMAPPER_H
#define KDIFF_LINEMAPPER_H

#include <ktexteditor/document.h>
//#include "modellist.h"
//#include "difference.h"


class DiffModel;
class Difference;

class LineMapper: public QObject {
Q_OBJECT
public:
    friend class DiffModel;
    LineMapper(DiffModel* model);
    ~LineMapper();

public slots:
    void update();//DiffModel* model, int sourceLines, int destinationLines);

private:
    DiffModel* m_model;
    QMap<int, Difference*>* m_sourceToDiffMap;
    QMap<int, Difference*>* m_destinationToDiffMap;
    QMap<int, int>* m_sourceToDestinationMap;
    QMap<int, int>* m_destinationToSourceMap;

    Difference* diffContainingSourceLine(int sourceLine, bool includingMissing);
    Difference* diffContainingDestinationLine(int destinationLine, bool includingMissing);
    int sourceLineFromDestination(int destinationLine);
    int destinationLineFromSource(int sourceLine);

};


#endif //KDIFF_LINEMAPPER_H
