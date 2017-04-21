/***************************************************************************
                                diffmodel.cpp
                                -------------
        begin                   : Sun Mar 4 2001
        Copyright 2001-2009 Otto Bruggeman <bruggie@gmail.com>
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

#include <QRegExp>
#include <QSaveFile>

#include "diffmodel.h"

#include <klocalizedstring.h>

#include "difference.h"
#include "diffhunk.h"
#include "levenshteintable.h"
#include "stringlistpair.h"

#include "parserbase.h"
#include "utils.h"
#include "linemapper.h"

#include "diffprocess.h"
#include "modellist.h"


DiffModel::DiffModel(const QString& source, const QString& destination, QObject *parent)
        : DiffProcessor(parent)
        , m_source(source)
        , m_destination(destination)
        , m_sourcePath("")
        , m_destinationPath("")
        , m_sourceFile("")
        , m_destinationFile("")
        , m_sourceLines(-1)
        , m_destinationLines(-1)
        , m_hunks()
        , m_differences()
        , m_missingRanges(new MissingRanges())
        , m_lineMapper(new LineMapper(this))
        , m_tmpDir(nullptr)
        , m_diffProcess(nullptr)
        , m_info (new Compare::ComparisonInfo())
        , m_syncedWithDisk(true)
{
    splitSourceInPathAndFileName();
    splitDestinationInPathAndFileName();

    m_key = StringUtils::rightCommonPartInPaths(m_source, m_destination);

    m_missingRanges->sourceRanges = new QList<MissingRange *>();
    m_missingRanges->destinationRanges = new QList<MissingRange *>();

    initInfo();
}

DiffModel::DiffModel()
        :DiffModel("", "", nullptr)
{
}

void DiffModel::initInfo() {
    m_info->mode = Compare::ComparingFiles;
    m_info->diffMode = Compare::Default;
    m_info->format = Compare::Normal;
    m_info->generator = Compare::Diff;
}

/**  */
DiffModel::~DiffModel()
{
    delete m_tmpDir;

    delete m_missingRanges;

    DifferenceList unique;

    auto diffIt = m_differences.begin();
    auto end = m_differences.end();
    while (diffIt != end) {
        Difference* diff = *diffIt;
        unique.append(diff);
        diffIt++;
    }

    auto hunkIt = m_hunks.begin();
    auto hend = m_hunks.end();
    while (hunkIt != hend) {
        DiffHunk* hunk = *hunkIt;
        auto diffIt = hunk->differences().begin();
        auto end = hunk->differences().end();
        while (diffIt != end) {
            Difference* diff = *diffIt;
            if(!unique.contains(diff)) {
                unique.append(diff);
            }
            diffIt++;
        }
        hunkIt++;
    }


    qDeleteAll(m_hunks);
    qDeleteAll(unique);

    delete m_lineMapper;

    m_modelList = nullptr;
}

void DiffModel::splitSourceInPathAndFileName()
{
    int pos;

    if((pos = m_source.lastIndexOf("/")) >= 0) {
        m_sourcePath = m_source.mid(0, pos + 1);
    }

    if((pos = m_source.lastIndexOf("/")) >= 0) {
        m_sourceFile = m_source.mid(pos + 1, m_source.length() - pos);
    } else {
        m_sourceFile = m_source;
    }
}

void DiffModel::splitDestinationInPathAndFileName()
{
    int pos;

    if((pos = m_destination.lastIndexOf("/")) >= 0) {
        m_destinationPath = m_destination.mid(0, pos + 1);
    }

    if((pos = m_destination.lastIndexOf("/")) >= 0) {
        m_destinationFile = m_destination.mid(pos + 1, m_destination.length() - pos);
    } else {
        m_destinationFile = m_destination;
    }
}

int DiffModel::localeAwareCompareSource(const DiffModel& model)
{
    int result = m_sourcePath.localeAwareCompare(model.m_sourcePath);

    if(result == 0) {
        return m_sourceFile.localeAwareCompare(model.m_sourceFile);
    }

    return result;
}

const QString DiffModel::sourceFile() const
{
    return m_sourceFile;
}

const QString DiffModel::destinationFile() const
{
    return m_destinationFile;
}

const QString DiffModel::sourcePath() const
{
    return m_sourcePath;
}

const QString DiffModel::destinationPath() const
{
    return m_destinationPath;
}

int DiffModel::fileLinesCount(QString filename) const {
    QFile f(filename);
    if (!f.open(QFile::ReadOnly | QFile::Text))
        return  0;

    QTextStream in(&f);
    QString contents = in.readAll();

    return contents.split(QRegExp("[\r\n]")).count();
}

void DiffModel::setSourceFile(QString path)
{
    m_source = path;

    m_info->localSource = m_source;
    splitSourceInPathAndFileName();
    if(!m_destination.isEmpty()) {
        m_key = StringUtils::rightCommonPartInPaths(m_source, m_destination);
    }

    m_sourceLines = fileLinesCount(path);
    m_lineMapper->update();
}

void DiffModel::setDestinationFile(QString path)
{
    m_destination = path;

    m_info->localDestination = m_destination;
    splitDestinationInPathAndFileName();
    if(!m_source.isEmpty()) {
        m_key = StringUtils::rightCommonPartInPaths(m_source, m_destination);
    }

    m_destinationLines = fileLinesCount(path);
    m_lineMapper->update();
}

void DiffModel::addHunk(DiffHunk* hunk)
{
    m_hunks.append(hunk);
}

void DiffModel::addDiff(Difference* diff)
{
    m_differences.append(diff);
}

// Some common computing after diff contents have been filled.
void DiffModel::computeDiffStats(Difference* diff)
{
    if(diff->sourceLineCount() > 0 && diff->destinationLineCount() > 0) {
        diff->setType(Difference::Change);
    } else if(diff->sourceLineCount() > 0) {
        diff->setType(Difference::Delete);
    } else if(diff->destinationLineCount() > 0) {
        diff->setType(Difference::Insert);
    }
    diff->determineInlineDifferences();
}

// Helper method to extract duplicate code from DiffModel::linesChanged
void DiffModel::processStartMarker(Difference* diff, const QStringList& lines, QList<Marker*>::iterator& currentMarker, int& currentListLine, bool isSource)
{
    Q_ASSERT((*currentMarker)->type() == Marker::Start);
    ++currentMarker;
    Q_ASSERT((*currentMarker)->type() == Marker::End);
    int nextDestinationListLine = (*currentMarker)->offset();
    for (; currentListLine < nextDestinationListLine; ++currentListLine) {
        if(isSource) {
            diff->addSourceLine(lines.at(currentListLine));
        } else {
            diff->addDestinationLine(lines.at(currentListLine));
        }
    }
    ++currentMarker;
    currentListLine = nextDestinationListLine;
}

void DiffModel::swap()
{
    m_source.swap(m_destination);
    m_sourcePath.swap(m_destinationPath);
    m_sourceFile.swap(m_destinationFile);

    auto hunkIt = m_hunks.begin();
    auto hEnd = m_hunks.end();
    for (; hunkIt != hEnd; ++hunkIt) {
        (*hunkIt)->swap();
    }

    auto diffIt = m_differences.begin();
    auto dEnd = m_differences.end();
    for (; diffIt != dEnd; ++diffIt) {
        (*diffIt)->swap();
    }
}
const QString& DiffModel::key() const
{
    return m_key;
}
void DiffModel::key(const QString& m_key)
{
    DiffModel::m_key = m_key;
}

const MissingRanges* DiffModel::missingRanges() const
{
    return m_missingRanges;
}

const QUrl& DiffModel::sourceUrl() const
{
    return m_sourceUrl;
}
void DiffModel::sourceUrl(const QUrl& sourceUrl)
{
    m_sourceUrl = sourceUrl;
}
const QUrl& DiffModel::destinationUrl() const
{
    return m_destinationUrl;
}
void DiffModel::destinationUrl(const QUrl& destinationUrl)
{
    m_destinationUrl = destinationUrl;
}
bool DiffModel::isReadWrite() const
{
    return m_modelList->isReadWrite();
}

bool DiffModel::isDir() const {
    QFileInfo sourceFileInfo(m_source);
    QFileInfo destinationFileInfo(m_destination);
    return (sourceFileInfo.exists() && sourceFileInfo.isDir()) ||
           (destinationFileInfo.exists() && destinationFileInfo.isDir());
}
int DiffModel::sourceLines() const
{
    return m_sourceLines;
}
void DiffModel::sourceLines(int sourceLines)
{
    m_sourceLines = sourceLines;
}
int DiffModel::destinationLines() const
{
    return m_destinationLines;
}
void DiffModel::destinationLines(int destinationLines)
{
    m_destinationLines = destinationLines;
}

Difference* DiffModel::diffContainingSourceLine(int sourceLine, bool includingMissing) {
    return m_lineMapper->diffContainingSourceLine(sourceLine, includingMissing);
}

Difference* DiffModel::diffContainingDestinationLine(int destinationLine, bool includingMissing) {
    return m_lineMapper->diffContainingDestinationLine(destinationLine, includingMissing);
}

int DiffModel::sourceLineFromDestination(int destinationLine) {
    return m_lineMapper->sourceLineFromDestination(destinationLine);
}

int DiffModel::destinationLineFromSource(int sourceLine) {
    return m_lineMapper->destinationLineFromSource(sourceLine);
}


void DiffModel::textChanged(const QString text) {
    m_syncedWithDisk = false;

    if(!m_tmpDir) {
        m_tmpDir = new QTemporaryDir(QDir::tempPath() + "/kdiff");
    }
    QString filename = m_tmpDir->path() + QDir::separator() + m_destinationFile;
    QSaveFile file(filename);
    file.open(QIODevice::WriteOnly);

    QByteArray outputByteArray;
    outputByteArray.append(text);
    file.write(outputByteArray);
    file.commit();

    m_destinationLines = text.split(QRegExp("[\r\n]")).count();
    updateModel(filename);
}

void DiffModel::resyncModelWithDisk() {
    m_syncedWithDisk = true;
    if(isDir())
        return;

    updateModel(m_destination);
    m_destinationLines = fileLinesCount(m_destination);
}

void DiffModel::updateModel(QString filename) {
    m_info->localDestination = filename;
    m_destinationLines = fileLinesCount(filename);

    if(m_diffProcess) {
        m_diffProcess.data()->blockSignals(true);
        m_diffProcess.data()->close();
    }
    m_diffProcess.reset(new DiffProcess(m_modelList->diffSettings(), Compare::Custom, m_info->localSource, m_info->localDestination));
    m_diffProcess->setEncoding(m_modelList->encoding());

    connect(m_diffProcess.data(), SIGNAL(diffHasFinished(bool)), this, SLOT(diffProcessFinished(bool)));

    m_diffProcess.data()->blockSignals(false);
    m_diffProcess->start();
}
void DiffModel::modelList(ModelList* modelList)
{
    m_modelList = modelList;
}

Compare::ComparisonInfo* DiffModel::comparisonInfo() const
{
    return m_info;
}


void DiffModel::diffProcessFinished(bool success)
{
    if(success) {
        if(parseDiffOutput(m_diffProcess->diffOutput()) != 0) {
            emit message(i18n("Could not parse diff output."), Error, true);
        } else {
            if(models()->size() == 0) {
                // Hmmm...... this shouldn't happen
                return;
            }

            DiffModel* newModel = models()->begin().value();
            replace(newModel);
        }
    } else if(m_diffProcess->exitStatus() == 0) {
        setToEqual();
        show();
    } else {
        emit message(m_diffProcess->stdErr(), Error, true);
    }
}

void DiffModel::replace(DiffModel* newModel) {
    qDeleteAll(m_differences.begin(), m_differences.end());
    m_differences = newModel->m_differences;

    qDeleteAll(m_hunks.begin(), m_hunks.end());
    m_hunks = newModel->m_hunks;

    qDeleteAll(m_missingRanges->sourceRanges->begin(), m_missingRanges->sourceRanges->end());
    delete m_missingRanges->sourceRanges;

    qDeleteAll(m_missingRanges->destinationRanges->begin(), m_missingRanges->destinationRanges->end());
    delete m_missingRanges->destinationRanges;
    delete  m_missingRanges;

    m_missingRanges = newModel->m_missingRanges;

    delete newModel->m_lineMapper;

    if(QFileInfo(m_source).exists()) {
        m_sourceUrl = QUrl::fromUserInput(m_modelList->sourceDisplayBasePath() + m_key);
    } else {
        m_sourceUrl = QUrl();
    }

    if(QFileInfo(m_destination).exists()) {
        m_destinationUrl = QUrl::fromUserInput(m_modelList->destinationDisplayBasePath() + m_key);
    } else {
        m_destinationUrl = QUrl();
    }

    show();

}

QUrl DiffModel::sourceBaseUrl() const {
    return QUrl::fromUserInput(m_modelList->sourceBasePath());
}

QUrl DiffModel::destinationBaseUrl() const {
    return QUrl::fromUserInput(m_modelList->destinationDisplayBasePath());
}

void DiffModel::show() {
    modelChanged();
    m_modelList->modelsUpdated();
}

void DiffModel::setToEqual()
{
    qDeleteAll(m_differences.begin(), m_differences.end());
    m_differences.clear();

    qDeleteAll(m_hunks.begin(), m_hunks.end());
    m_hunks.clear();

    if(m_missingRanges->sourceRanges) {
        qDeleteAll(m_missingRanges->sourceRanges->begin(), m_missingRanges->sourceRanges->end());
        m_missingRanges->sourceRanges->clear();
    }

    if(m_missingRanges->destinationRanges) {
        qDeleteAll(m_missingRanges->destinationRanges->begin(), m_missingRanges->destinationRanges->end());
        m_missingRanges->destinationRanges->clear();
    }

    m_sourceUrl = QUrl::fromUserInput(m_modelList->sourceDisplayBasePath()+ m_key);
    m_destinationUrl = QUrl::fromUserInput(m_modelList->destinationDisplayBasePath()+ m_key);

    m_destinationLines = m_sourceLines;

    show();
}
bool DiffModel::isSyncedWithDisk() const
{
    return m_syncedWithDisk;
}
