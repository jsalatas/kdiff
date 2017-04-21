//
// Created by john on 4/2/17.
//

#ifndef KDIFF_DIFFPROCESSOR_H
#define KDIFF_DIFFPROCESSOR_H

#include <QObject>
#include "info.h"

class DiffModelMap;

enum MessagetType
{
    Info = 0,
    Warning = 1,
    Error = 2
};

class DiffProcessor: public QObject
{
    Q_OBJECT
public:
    DiffProcessor(QObject *parent=Q_NULLPTR);
    virtual ~DiffProcessor();
    virtual bool blendOriginalIntoModelList(const QString& localURL);

signals:
    void message(QString error, MessagetType messagetType, bool stop);

protected:
    int parseDiffOutput(const QString& diff);
    QStringList split(const QString& diff);
    virtual Compare::ComparisonInfo* comparisonInfo() const = 0;
    virtual const DiffModelMap* models() const;
    virtual void models(DiffModelMap* models);

private:
    DiffModelMap* m_tmpDiffModelMap;
};


#endif //KDIFF_DIFFPROCESSOR_H
