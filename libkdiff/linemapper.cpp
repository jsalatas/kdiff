//
// Created by john on 2/19/17.
//

#include <algorithm>
#include <QTemporaryDir>
#include <KMessageBox>
#include "diffmodel.h"
#include "linemapper.h"

LineMapper::LineMapper(DiffModel* model)
        : QObject(model)
        , m_model(model)
{
    m_sourceToDiffMap = new QMap<int, Difference*>();
    m_destinationToDiffMap = new QMap<int, Difference*>();
    m_sourceToDestinationMap = new QMap<int, int>();
    m_destinationToSourceMap = new QMap<int, int>();
    connect(m_model, SIGNAL(modelChanged()), this, SLOT(update()));
}

LineMapper::~LineMapper() {
    m_sourceToDiffMap->clear();
    delete  m_sourceToDiffMap;
    m_sourceToDiffMap = nullptr;

    m_destinationToDiffMap->clear();
    m_destinationToSourceMap->clear();
    delete m_destinationToDiffMap;
    m_destinationToDiffMap = nullptr;

    delete m_sourceToDestinationMap;
    m_sourceToDestinationMap = nullptr;

    delete m_destinationToSourceMap;
    m_destinationToSourceMap = nullptr;

}

//void LineMapper::model(DiffModel* model, int sourceLines, int destinationLines) {
void LineMapper::update() {
    m_sourceToDiffMap->clear();
    m_destinationToDiffMap->clear();
    m_sourceToDestinationMap->clear();
    m_destinationToSourceMap->clear();

    for (int i = 0; i <m_model->differenceCount(); ++i) {
        Difference* diff = m_model->differenceAt(i);
        int j = diff->sourceLineNumber();
        do {
            m_sourceToDiffMap->insert(j-1, diff);
            j++;
        } while (j < diff->sourceLineEnd());

        j = diff->destinationLineNumber();
        do {
            m_destinationToDiffMap->insert(j-1, diff);
            j++;
        } while (j < diff->destinationLineEnd());
    }

    int sourceIndex = 0;
    int destinationIndex = 0;

    bool includeMissing = true;

    while (sourceIndex < m_model->sourceLines() || destinationIndex < m_model->destinationLines()) {
        const Difference *diff = diffContainingSourceLine(sourceIndex, includeMissing);
        if (diff == nullptr) {
            includeMissing = true;
            m_sourceToDestinationMap->insert(sourceIndex, destinationIndex);
            m_destinationToSourceMap->insert(destinationIndex, sourceIndex);
            if (sourceIndex < m_model->sourceLines())
                sourceIndex++;

            if (destinationIndex < m_model->destinationLines())
                destinationIndex++;
        } else {
            int j = 0;
            int tmpSourceIndex = sourceIndex;
            int tmpDestinationIndex = destinationIndex;
            while (j < std::max(diff->sourceLineCount(), diff->destinationLineCount())) {
                if (!m_sourceToDestinationMap->contains(tmpSourceIndex))
                    m_sourceToDestinationMap->insert(tmpSourceIndex, tmpDestinationIndex);

                if (!m_destinationToSourceMap->contains(tmpDestinationIndex))
                    m_destinationToSourceMap->insert(tmpDestinationIndex, tmpSourceIndex);

                j++;
                if (j < diff->sourceLineCount()) {
                    tmpSourceIndex++;
                }
                if (j < diff->destinationLineCount()) {
                    tmpDestinationIndex++;
                }
            }
            includeMissing = diff->sourceLineCount() > 0;
            sourceIndex += diff->sourceLineCount();
            destinationIndex += diff->destinationLineCount(); //+ diff->sourceLineCount() == 0 ? 1 : 0;
        }
    }
}

Difference* LineMapper::diffContainingSourceLine(int sourceLine, bool includingMissing) {
    Difference* diff = nullptr;
    if(!m_sourceToDestinationMap)
        return diff;

    QMap<int, Difference*>::iterator i = m_sourceToDiffMap->find(sourceLine);
    if (i != m_sourceToDiffMap->end() && i.key() == sourceLine) {
        if(includingMissing || i.value()->sourceLineCount() > 0)
            diff = i.value();

    }

    return diff;
}

Difference* LineMapper::diffContainingDestinationLine(int destinationLine, bool includingMissing) {
    Difference* diff = nullptr;
    if(!m_destinationToDiffMap)
        return diff;

    QMap<int, Difference*>::iterator i = m_destinationToDiffMap->find(destinationLine);
    if (i != m_sourceToDiffMap->end() && i.key() == destinationLine) {
        if (includingMissing || i.value()->sourceLineCount() > 0)
            diff = i.value();
    }

    return diff;
}

int LineMapper::sourceLineFromDestination(int destinationLine) {
    if(!m_destinationToSourceMap)
        return -1;

    QMap<int, int>::iterator i = m_destinationToSourceMap->find(destinationLine);
    if (i != m_destinationToSourceMap->end() && i.key() == destinationLine)
        return i.value();

    // it should never reach here
    return -1;
}

int LineMapper::destinationLineFromSource(int sourceLine) {
    if(!m_sourceToDestinationMap)
        return -1;

    QMap<int, int>::iterator i = m_sourceToDestinationMap->find(sourceLine);
    if (i != m_sourceToDestinationMap->end() && i.key() == sourceLine)
        return i.value();

    // it should never reach here
    return -1;
}
