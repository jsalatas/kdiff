/***************************************************************************
                          modellist.h
                          -------------------
    begin                : Tue Jun 26 2001
    Copyright 2001-2003 John Firebaugh <jfirebaugh@kde.org>
    Copyright 2001-2005,2009 Otto Bruggeman <bruggie@gmail.com>
    Copyright 2007-2008 Kevin Kofler   <kevin.kofler@chello.at>
    Copyright 2012      Jean-Nicolas Artaud <jeannicolasartaud@gmail.com>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MODELLIST_H
#define MODELLIST_H

#include "diffmodel.h"

#include <QObject>
#include <QDir>
#include "diffmodelmap.h"
#include "diffprocessor.h"
#include "info.h"
#include "kdiffparser_export.h"

class QTemporaryFile;

class DiffSettings;

class DiffProcess;

class KDIFFPARSER_EXPORT ModelList : public DiffProcessor
{
Q_OBJECT
public:
    ModelList(DiffSettings* diffSettings, QWidget* widgetForKIO, QObject* parent, bool isReadWrite = true);
    virtual ~ModelList();

protected:
    virtual Compare::ComparisonInfo* comparisonInfo() const Q_DECL_OVERRIDE;
    bool blendFile(DiffModel* model, const QString& lines);

signals:
    void status(Compare::Status status);
//    void message(QString error, MessagetType messagetType, bool stop);
    void modelsChanged(const DiffModelMap* models);
    void modelsUpdated();
    void modelsCleared();

protected slots:
    void diffProcessFinished(bool success);
    void slotWriteDiffOutput(bool success);

private:
    bool compare();
    bool isDirectory(const QString& url) const;
    bool isDiff(const QString& mimetype) const;
    QString readFile(const QString& fileName);
//    QStringList split(const QString& diff);
    void setBasePaths();
    void createTempFiles(const QDir rootDir, bool source);
    void collectLocalFiles();
    void accountForMovedFiles();
    void setProperties();

public slots:
    void createMissingFolders(const QString path);
    void deleteItem(QString path, bool recursively);

public:
    void swap();
    bool compare(Compare::ComparisonInfo* info);
    bool openDiff(Compare::ComparisonInfo* info);
    bool openFileAndDiff(Compare::ComparisonInfo* info);
    bool openDirAndDiff(Compare::ComparisonInfo* info);
    bool saveDiff(const QString& url, QString directory, DiffSettings* diffSettings);
    void setEncoding(const QString& encoding);
    bool parseAndOpenDiff(const QString& diff);
    void show();
    virtual bool blendOriginalIntoModelList(const QString& localURL) Q_DECL_OVERRIDE;
    enum Compare::CompareMode mode() const
    {return m_info->mode;};
    virtual const DiffModelMap* models() const Q_DECL_OVERRIDE
    {return m_models;};
    virtual void models(DiffModelMap* models) Q_DECL_OVERRIDE
    {m_models = models;};

    DiffModel* find(QString key) const;
    DiffModel* findMoved(QString key, bool source) const;
    void clear();

    DiffSettings* diffSettings() const;
    const QString& encoding() const;
    const QString& sourceBasePath() const;
    const QString& destinationBasePath() const;
    const QString& sourceDisplayBasePath() const;
    const QString& destinationDisplayBasePath() const;
    bool isReadWrite() const;
    const QTemporaryDir* tmpDir() const;

private:
    QTemporaryFile* m_diffTemp;
    QUrl m_diffURL;
    DiffProcess* m_diffProcess;
    DiffSettings* m_diffSettings;
    DiffModelMap* m_models;
    struct Compare::ComparisonInfo* m_info;
public:
    Compare::ComparisonInfo* info() const;
private:
    QString m_encoding;
    QTextCodec* m_textCodec;
    QWidget* m_widgetForKIO;
    bool m_readWrite;
    QString m_sourceBasePath;
    QString m_destinationBasePath;
    QString m_sourceDisplayBasePath;
    QString m_destinationDisplayBasePath;
    QTemporaryDir* m_tmpDir;
};

#endif
