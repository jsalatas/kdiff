/***************************************************************************
                                diffmodel.h
                                -----------
        begin                   : Sun Mar 4 2001
        Copyright 2001-2004,2009 Otto Bruggeman <bruggie@gmail.com>
        Copyright 2001-2003 John Firebaugh <jfirebaugh@kde.org>
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
***************************************************************************/

#ifndef DIFFMODEL_H
#define DIFFMODEL_H

#include <QStringList>
#include <QTemporaryDir>
#include <QList>
#include <QUrl>

#include "diffhunk.h"
#include "info.h"
#include "kdiffparser_export.h"
#include "diffprocessor.h"


class DiffHunk;

class Difference;

class DiffProcess;

class LineMapper;

class ModelList;

struct MissingRange
{
    int start;
    int end;
};
struct MissingRanges
{
    QList<MissingRange*>* sourceRanges;
    QList<MissingRange*>* destinationRanges;
};


class KDIFFPARSER_EXPORT DiffModel : public DiffProcessor
{
Q_OBJECT
public:

    DiffModel(const QString& srcBaseURL, const QString& destBaseURL, const QString& renamedFrom = QString(), const QString& renamedTo = QString(), QObject* parent = Q_NULLPTR);
    DiffModel();
    virtual ~DiffModel();

signals:
    void modelChanged();

public slots:
    void textChanged(const QString text);
    void resyncModelWithDisk();

protected slots:
    void diffProcessFinished(bool success);

protected:
    virtual Compare::ComparisonInfo* comparisonInfo() const Q_DECL_OVERRIDE;

public:
    int hunkCount() const {return m_hunks.count();}
    int differenceCount() const {return m_differences.count();}
    DiffHunk* hunkAt(int i) {return (m_hunks.at(i));}
    const Difference* differenceAt(int i) const {return (m_differences.at(i));}
    Difference* differenceAt(int i) {return (m_differences.at(i));}
    DiffHunkList* hunks() {return &m_hunks;}
    const DiffHunkList* hunks() const {return &m_hunks;}
    DifferenceList* differences() {return &m_differences;}
    const DifferenceList* differences() const {return &m_differences;}
    const QString source() const {return m_source;}
    const QString destination() const {return m_destination;}
    void swap();
    const QString sourceFile() const;
    const QString destinationFile() const;
    const QString sourcePath() const;
    const QString destinationPath() const;
    void setSourceFile(QString path);
    void setDestinationFile(QString path);
    void addHunk(DiffHunk* hunk);
    void addDiff(Difference* diff);
    int localeAwareCompareSource(const DiffModel& model);
    const QString& key() const;
    void key(const QString& m_key);
    void modelList(ModelList* modelList);
    ModelList* modelList() const;

    int sourceLines() const;
    void sourceLines(int sourceLines);
    int destinationLines() const;
    void destinationLines(int destinationLines);
    bool isReadWrite() const;
    const QUrl& sourceUrl() const;
    void sourceUrl(const QUrl& sourceUrl);
    const QUrl& destinationUrl() const;
    void destinationUrl(const QUrl& destinationUrl);
    const MissingRanges* missingRanges() const;
    bool isDir() const;
    Difference* diffContainingSourceLine(int sourceLine, bool includingMissing);
    Difference* diffContainingDestinationLine(int destinationLine, bool includingMissing);
    int sourceLineFromDestination(int destinationLine);
    int destinationLineFromSource(int sourceLine);
    void setToEqual();
    void replace(DiffModel* newModel);
    bool isSyncedWithDisk() const;
    QUrl sourceBaseUrl() const;
    QUrl destinationBaseUrl() const;
    bool isRenamed() const;
    const QString& renamedFrom() const;
    const QString& renamedTo() const;
    bool isUnmodifiedMoved() const;


private:
    int fileLinesCount(QString filename) const;
    void initInfo();
    void show();
    void splitSourceInPathAndFileName();
    void splitDestinationInPathAndFileName();
    void computeDiffStats(Difference* diff);
    void processStartMarker(Difference* diff, const QStringList& lines, QList<Marker*>::iterator& currentMarker, int& currentListLine, bool isSource);
    void updateModel(QString filename);

private:
    QString m_source;
    QString m_destination;

    QString m_sourcePath;
    QString m_destinationPath;

    QString m_sourceFile;
    QString m_destinationFile;

    QUrl m_sourceUrl;
    QUrl m_destinationUrl;

    int m_sourceLines;
    int m_destinationLines;

    DiffHunkList m_hunks;
    DifferenceList m_differences;

    QString m_key;
    MissingRanges* m_missingRanges;
    LineMapper* m_lineMapper;
    QTemporaryDir* m_tmpDir;
    QScopedPointer<DiffProcess> m_diffProcess;
    ModelList* m_modelList;
    struct Compare::ComparisonInfo* m_info;
    bool m_syncedWithDisk;

    QString m_renamedFrom;
private:
    QString m_renamedTo;

};

#endif

