/***************************************************************************
                          modellist.cpp
                          --------------------
    begin                : Tue Jun 26 2001
    Copyright 2001-2005,2009 Otto Bruggeman <otto.bruggeman@home.nl>
    Copyright 2001-2003 John Firebaugh <jfirebaugh@kde.org>
    Copyright 2007-2010 Kevin Kofler   <kevin.kofler@chello.at>
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

#include <QAction>

#include "modellist.h"
#include <QFile>
#include <QDir>
#include <QRegExp>
#include <QTextCodec>
#include <QTextStream>
#include <QLinkedList>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QSaveFile>
#include <QDirIterator>
#include <KSharedConfig>

#include <kcharsets.h>
#include <kdirwatch.h>
#include <kio/udsentry.h>
#include <kio/statjob.h>
#include <kio/mkdirjob.h>
#include <kio/filecopyjob.h>

#include <klocalizedstring.h>
#include <QMimeType>
#include <QMimeDatabase>
#include <kstandardaction.h>

#include "diffprocess.h"
#include "parser.h"
#include "utils.h"
#include "info.h"
#include "diffsettings.h"

ModelList::ModelList(DiffSettings* diffSettings, QWidget* widgetForKIO, QObject* parent, bool isReadWrite)
        : DiffProcessor(parent)
        , m_diffProcess(nullptr)
        , m_diffSettings(diffSettings)
        , m_models(nullptr)
        , m_info(nullptr)
        , m_textCodec(nullptr)
        , m_widgetForKIO(widgetForKIO)
        , m_readWrite(isReadWrite)
        , m_tmpDir(nullptr)
{
}

ModelList::~ModelList()
{
    m_info = nullptr;
    if(m_models) {
        qDeleteAll(m_models->begin(), m_models->end());
        m_models->clear();
        delete m_models;
        m_models = nullptr;
    }
    delete m_diffProcess;
    m_diffProcess = nullptr;
    delete m_tmpDir;
    m_tmpDir = nullptr;
}

bool ModelList::isDirectory(const QString& url) const
{
    QFileInfo fi(url);
    if(fi.isDir()) {
        return true;
    } else {
        return false;
    }
}

bool ModelList::isDiff(const QString& mimeType) const
{
    if(mimeType == "text/x-patch") {
        return true;
    } else {
        return false;
    }
}

bool ModelList::compare(Compare::ComparisonInfo* info)
{
    m_info = info;
    bool result = false;

    bool sourceIsDirectory = isDirectory(m_info->localSource);
    bool destinationIsDirectory = isDirectory(m_info->localDestination);

    if(sourceIsDirectory && destinationIsDirectory) {
        m_info->mode = Compare::ComparingDirs;
        result = compare();
    } else if(!sourceIsDirectory && !destinationIsDirectory) {
        QFile sourceFile(m_info->localSource);
        QMimeDatabase db;
        QString sourceMimeType = "application/x-zerosize";
        if(sourceFile.exists()) {
            sourceFile.open(QIODevice::ReadOnly);
            sourceMimeType = (db.mimeTypeForData(sourceFile.readAll())).name();
            sourceFile.close();
        }

        QFile destinationFile(m_info->localDestination);
        QString destinationMimeType = "application/x-zerosize";
        if(destinationFile.exists()) {
            destinationFile.open(QIODevice::ReadOnly);
            destinationMimeType = (db.mimeTypeForData(destinationFile.readAll())).name();
            destinationFile.close();
        }
        // Not checking if it is a text file/something diff can even compare, we'll let diff handle that
        if(!isDiff(sourceMimeType) && isDiff(destinationMimeType)) {
            m_info->mode = Compare::BlendingFile;
            result = openFileAndDiff(info);
        } else if(isDiff(sourceMimeType) && !isDiff(destinationMimeType)) {
            m_info->mode = Compare::BlendingFile;
            // Swap source and destination before calling this
            m_info->swapSourceWithDestination();
            // Do we need to notify anyone we swapped source and destination?
            // No we do not need to notify anyone about swapping source with destination
            result = openFileAndDiff(m_info);
        } else {
            m_info->mode = Compare::ComparingFiles;
            result = compare();
        }
    } else if(sourceIsDirectory && !destinationIsDirectory) {
        m_info->mode = Compare::BlendingDir;
        result = openDirAndDiff(m_info);
    } else {
        m_info->mode = Compare::BlendingDir;
        // Swap source and destination first in m_info
        m_info->swapSourceWithDestination();
        // Do we need to notify anyone we swapped source and destination?
        // No we do not need to notify anyone about swapping source with destination
        result = openDirAndDiff(m_info);
    }

    return result;
}

bool ModelList::compare()
{
    clear(); // Destroy the old models...

    delete m_diffProcess;

    m_diffProcess = new DiffProcess(m_diffSettings, Compare::Custom, m_info->localSource, m_info->localDestination, QString());
    m_diffProcess->setEncoding(m_encoding);

    connect(m_diffProcess, SIGNAL(diffHasFinished(bool)),
            this, SLOT(diffProcessFinished(bool)));

    emit status(Compare::RunningDiff);
    m_diffProcess->start();

    return true;
}

QString lstripSeparators(const QString& from, uint count)
{
    int position = 0;
    for (uint i = 0; i < count; ++i) {
        position = from.indexOf('/', position);
        if(position == -1) {
            break;
        }
    }
    if(position == -1) {
        return "";
    } else {
        return from.mid(position);
    }
}

void ModelList::swap()
{
    auto modelIt = m_models->begin();
    auto mEnd = m_models->end();
    for (; modelIt != mEnd; ++modelIt) {
        (*modelIt)->swap();
    }
}

bool ModelList::openFileAndDiff(Compare::ComparisonInfo* info)
{
    m_info = info;

    clear();

    if(parseDiffOutput(readFile(m_info->localDestination)) != 0) {
        emit message(i18n("<qt>No models or no differences. File: <b>%1</b>, is not a valid diff file.</qt>", m_info->destination.toDisplayString()), Error, true);
        return false;
    }

    if(!blendOriginalIntoModelList(m_info->localSource)) {
        emit message(i18n("<qt>There were problems applying the diff <b>%1</b> to the file <b>%2</b>.</qt>", m_info->source.toDisplayString(), m_info->destination.toDisplayString()), Error, true);
        return false;
    }

    swap();
    show();

    return true;
}

bool ModelList::openDirAndDiff(Compare::ComparisonInfo* info)
{
    m_info = info;

    clear();

    if(parseDiffOutput(readFile(m_info->localDestination)) != 0) {
        emit message(i18n("<qt>No models or no differences, this file: <b>%1</b>, is not a valid diff file.</qt>", m_info->destination.toDisplayString()), Error, true);
        return false;
    }

    // Do our thing :)
    if(!blendOriginalIntoModelList(m_info->localSource)) {
        // Trouble blending the original into the model
        emit message(i18n("<qt>There were problems applying the diff <b>%1</b> to the folder <b>%2</b>.</qt>", m_info->source.toDisplayString(), m_info->destination.toDisplayString()), Error, true);
        return false;
    }

    swap();
    show();

    return true;
}

void ModelList::setEncoding(const QString& encoding)
{
    m_encoding = encoding;
    if(!encoding.compare("default", Qt::CaseInsensitive)) {
        m_textCodec = QTextCodec::codecForLocale();
    } else {
        m_textCodec = KCharsets::charsets()->codecForName(encoding.toLatin1());
        if(!m_textCodec) {
            m_textCodec = QTextCodec::codecForLocale();
        }
    }
}

void ModelList::diffProcessFinished(bool success)
{
    if(success) {
        emit status(Compare::Parsing);
        if(parseDiffOutput(m_diffProcess->diffOutput()) != 0) {
            emit message(i18n("Could not parse diff output."), Error, true);
        } else {
            if(m_info->mode != Compare::ShowingDiff) {
                blendOriginalIntoModelList(m_info->localSource);
            }
            show();
        }
        emit status(Compare::FinishedParsing);
    } else if(m_diffProcess->exitStatus() == 0) {
        emit message(i18n("The files are identical."), Info, true);
    } else {
        emit message(m_diffProcess->stdErr(), Error, true);
    }
}


//QStringList ModelList::split(const QString& fileContents)
//{
//    QString contents = fileContents;
//    QStringList list;
//
//    int pos = 0;
//    int oldpos = 0;
//    // split that does not strip the split char
//#ifdef QT_OS_MAC
//    const char split = '\r';
//#else
//    const char split = '\n';
//#endif
//    while ((pos = contents.indexOf(split, oldpos)) >= 0) {
//        list.append(contents.mid(oldpos, pos - oldpos + 1));
//        oldpos = pos + 1;
//    }
//
//    if(contents.length() > oldpos) {
//        list.append(contents.right(contents.length() - oldpos));
//    }
//
//    return list;
//}

QString ModelList::readFile(const QString& fileName)
{
    QStringList list;

    QFile file(fileName);
    file.open(QIODevice::ReadOnly);

    QTextStream stream(&file);

    if(!m_textCodec) {
        m_textCodec = QTextCodec::codecForLocale();
    }

    stream.setCodec(m_textCodec);

    QString contents = stream.readAll();

    file.close();

    return contents;
}

bool ModelList::openDiff(Compare::ComparisonInfo* info)
{
    m_info = info;


    if(m_info->localSource.isEmpty()) {
        return false;
    }

    QString diff = readFile(m_info->localSource);

    clear(); // Clear the current models

    emit status(Compare::Parsing);

    if(parseDiffOutput(diff) != 0) {
        emit message(i18n("Could not parse diff output."), Error, true);
        return false;
    }

    show();

    emit status(Compare::FinishedParsing);

    return true;
}

bool ModelList::parseAndOpenDiff(const QString& diff)
{
    clear(); // Clear the current models

    emit status(Compare::Parsing);

    if(parseDiffOutput(diff) != 0) {
        emit message(i18n("Could not parse diff output."), Error, true);
        return false;
    }

    show();

    emit status(Compare::FinishedParsing);
    return true;
}

bool ModelList::saveDiff(const QString& url, QString directory, DiffSettings* diffSettings)
{

    m_diffTemp = new QTemporaryFile();
    m_diffURL = QUrl(url); // ### TODO the "url" argument should be a QUrl

    if(!m_diffTemp->open()) {
        emit message(i18n("Could not open a temporary file."), Error, true);
        m_diffTemp->remove();
        delete m_diffTemp;
        m_diffTemp = 0;
        return false;
    }
    delete m_diffProcess;

    m_diffProcess = new DiffProcess(diffSettings, Compare::Custom, m_info->localSource, m_info->localDestination, directory);
    m_diffProcess->setEncoding(m_encoding);

    connect(m_diffProcess, SIGNAL(diffHasFinished(bool)),
            this, SLOT(slotWriteDiffOutput(bool)));

    emit status(Compare::RunningDiff);
    m_diffProcess->start();
    return true;
}

void ModelList::slotWriteDiffOutput(bool success)
{
    if(success) {
        QTextStream stream(m_diffTemp);

        stream << m_diffProcess->diffOutput();

        m_diffTemp->close();

        KIO::FileCopyJob* copyJob = KIO::file_copy(QUrl::fromLocalFile(m_diffTemp->fileName()), m_diffURL);
        copyJob->exec();

        emit status(Compare::FinishedWritingDiff);
    }

    m_diffURL = QUrl();
    m_diffTemp->remove();

    delete m_diffTemp;
    m_diffTemp = 0;

    delete m_diffProcess;
    m_diffProcess = 0;
}

bool ModelList::blendOriginalIntoModelList(const QString& localURL)
{
    QFileInfo fi(localURL);

    bool result = true;
    DiffModel* model;

    QString fileContents;

    if(fi.isDir()) { // is a dir
        auto modelIt = m_models->begin();
        auto mEnd = m_models->end();
        for (; modelIt != mEnd; ++modelIt) {
            model = *modelIt;
            if(model->source() == "/dev/null") {
                continue;
            }
            QString filename = model->destination();
            if(!filename.startsWith(localURL)) {
                QString commonPath = StringUtils::rightCommonPartInPaths(model->source(), model->destination());
                filename = QDir(localURL).filePath(commonPath);
            }
            QFileInfo fi2(filename);
            if(fi2.exists() && fi2.isFile()) {
                fileContents = readFile(filename);
                result = result && blendFile(model, fileContents);
            } else {
                fileContents.truncate(0);
                result = result && blendFile(model, fileContents);
            }
        }
    } else if(fi.isFile()) { // is a file
        fileContents = readFile(localURL);

        result = result && blendFile(m_models->begin().value(), fileContents);
    }

    return result;
}

bool ModelList::blendFile(DiffModel* model, const QString& fileContents)
{
    if(!model) {
        return false;
    }

    bool hasConflicts = false;

    int srcLineNo = 1, destLineNo = 1;

    QStringList list = split(fileContents);
    QStringList lines;
            foreach (const QString& str, list) {
            lines.append(str);
        }

    int lineIndex = 0;

    DiffHunkList* hunks = model->hunks();
    auto hunkIt = hunks->begin();

    DiffHunk* newHunk = 0;
    Difference* newDiff = 0;

    // FIXME: this approach is not very good, we should first check if the hunk applies cleanly
    // and without offset and if not use that new linenumber with offset to compare against
    // This will only work for files we just diffed but not for blending where
    // file(s) to patch has/have potentially changed

    for (; hunkIt != hunks->end(); ++hunkIt) {
        // Do we need to insert a new hunk before this one ?
        DiffHunk* hunk = *hunkIt;

        // In case we are blending diff with files, check for conflicts
        if(m_info->mode == Compare::BlendingDir || m_info->mode == Compare::BlendingFile) {
            for (int i = 0; i < hunk->differences().count(); ++i) {
                Difference* diff = hunk->differences().at(i);
                for (int j = 0; j < diff->sourceLineCount(); ++j) {
                    if(lines.size() > diff->sourceLineNumber() + j - 1 &&
                       lines.at(diff->sourceLineNumber() + j - 1) != diff->sourceLineAt(j)->string()) {
                        diff->setConflict(true);
                        hasConflicts = true;
                    }
                }
            }
        }

        if(srcLineNo < hunk->sourceLineNumber()) {
            newHunk = new DiffHunk(srcLineNo, destLineNo, "", DiffHunk::AddedByBlend);

            hunkIt = ++hunks->insert(hunkIt, newHunk);

            newDiff = new Difference(srcLineNo, destLineNo,
                    Difference::Unchanged);

            newHunk->add(newDiff);

            while (srcLineNo < hunk->sourceLineNumber() && lineIndex < lines.size()) {
                newDiff->addSourceLine(lines.at(lineIndex));
                newDiff->addDestinationLine(lines.at(lineIndex));
                srcLineNo++;
                destLineNo++;
                ++lineIndex;
            }
        }

        // Now we add the linecount difference for the hunk that follows
        int size = hunk->sourceLineCount();

        for (int i = 0; i < size; ++i) {
            ++lineIndex;
        }

        srcLineNo += size;
        destLineNo += hunk->destinationLineCount();
    }

    if(lineIndex < lines.size()) {
        newHunk = new DiffHunk(srcLineNo, destLineNo, "", DiffHunk::AddedByBlend);

        model->addHunk(newHunk);

        newDiff = new Difference(srcLineNo, destLineNo, Difference::Unchanged);

        newHunk->add(newDiff);

        while (lineIndex < lines.size()) {
            newDiff->addSourceLine(lines.at(lineIndex));
            newDiff->addDestinationLine(lines.at(lineIndex));
            ++lineIndex;
        }
    }

    return !hasConflicts;
}

void ModelList::show()
{
    setBasePaths();
    accountForMovedFiles();
    collectLocalFiles();
    setProperties();

    emit modelsChanged(m_models);
}

void ModelList::clear()
{
    if(m_models) {
        m_models->clear();
    }

    emit modelsCleared();
}


DiffModel* ModelList::find(QString key) const {
    auto i = m_models->find(key);

    if (i != m_models->end() && i.key() == key) {
        return i.value();
    }
    return nullptr;
}

DiffModel* ModelList::findMoved(QString key, bool source) const {
    auto i = m_models->begin();
    for (; i != m_models->end(); ++i) {
        DiffModel* model = i.value();
        if(model->isRenamed()) {
            if(source) {
                if(model->renamedFrom().compare(key)) {
                    return i.value();
                }
            } else {
                if(model->renamedTo().compare(key)) {
                    return i.value();
                }
            }
        }
    }

    return nullptr;
}

void ModelList::setBasePaths() {
    switch (m_info->mode) {
        case Compare::ComparingFiles:
        case Compare::ComparingDirs:
            m_sourceDisplayBasePath = m_info->source.isLocalFile() ? m_info->localSource : m_info->source.url();
            m_destinationDisplayBasePath = m_info->destination.isLocalFile() ? m_info->localDestination : m_info->destination.url();
            m_sourceBasePath = m_info->localSource;
            m_destinationBasePath = m_info->localDestination;
            m_readWrite = true;
            break;
        case Compare::ShowingDiff: {
            auto i = m_models->begin();
            for (; i != m_models->end(); ++i) {
                DiffModel *model = i.value();
                if(model->source() != "/dev/null" && model->destination() != "/dev/null") {
                    QString commonPath = StringUtils::rightCommonPartInPaths(model->source(), model->destination());
                    m_sourceDisplayBasePath = StringUtils::rightStrip(model->source(), commonPath);
                    m_destinationDisplayBasePath = StringUtils::rightStrip(model->destination(), commonPath);
                    m_readWrite = false;
                    break;
                }
            }
            m_sourceBasePath = "";
            m_destinationBasePath = "";
        }
            break;
        case Compare::BlendingFile:
        case Compare::BlendingDir: {
            m_destinationDisplayBasePath = m_info->source.isLocalFile() ? m_info->localSource : m_info->source.url();
            m_destinationBasePath = m_info->localSource;
            auto i = m_models->begin();
            for (; i != m_models->end(); ++i) {
                DiffModel* model = i.value();
                if(model->source() != "/dev/null" && model->destination() != "/dev/null") {
                    m_sourceDisplayBasePath = model->source();
                    QString commonPath = StringUtils::rightCommonPartInPaths(model->source(), model->destination());
                    m_sourceDisplayBasePath.remove(commonPath);
                    m_readWrite = true;
                    break;
                }
            }

            m_sourceBasePath = "";
        }
            break;
        case Compare::UnknownMode:
            // TODO: What do I need to here???
            break;
    }

    if(m_info->mode != Compare::ComparingFiles && m_info->mode != Compare::BlendingFile) {
        if(!m_sourceDisplayBasePath.endsWith(QDir::separator())) {
            m_sourceDisplayBasePath += QDir::separator();
        }

        if(!m_destinationDisplayBasePath.endsWith(QDir::separator())) {
            m_destinationDisplayBasePath += QDir::separator();
        }
    }

    if (m_sourceBasePath.isEmpty() || m_destinationBasePath.isEmpty()) {
        delete m_tmpDir;
        m_tmpDir = new QTemporaryDir(QDir::tempPath() + "/kdiff");
        QDir dir = QDir(m_tmpDir->path());
        if (m_sourceBasePath.isEmpty()) {
            createTempFiles(dir, true);
        }
        if (m_destinationBasePath.isEmpty()) {
            createTempFiles(dir, false);
        }
    }

    if (!m_sourceBasePath.endsWith(QDir::separator())) {
        m_sourceBasePath += QDir::separator();
    }
    if (!m_destinationBasePath.endsWith(QDir::separator())) {
        m_destinationBasePath += QDir::separator();
    }
}

void ModelList::setProperties() {
    auto i = m_models->begin();
    for (; i != m_models->end(); ++i) {
        i.value()->modelList(this);
    }
}

void ModelList::createTempFiles(const QDir rootDir, bool source) {
    bool basePathsSet = false;
    auto i = m_models->begin();
    for (; i != m_models->end(); ++i) {
        DiffModel *model = i.value();

        QString path = source ? model->source() : model->destination();
        if(path == "/dev/null") {
            continue;
        }

        path = StringUtils::leftStrip(path, source ? m_sourceDisplayBasePath : m_destinationDisplayBasePath);

        if (!basePathsSet && model->source() != "/dev/null" &&  model->destination() != "/dev/null") {
            basePathsSet = true;
            QString commonPath = StringUtils::rightCommonPartInPaths(model->source(), model->destination());
            if (source) {
                m_sourceBasePath = rootDir.path() + QDir::separator() + StringUtils::rightStrip(model->source(), commonPath);
            } else {
                m_destinationBasePath = rootDir.path() + QDir::separator() + StringUtils::rightStrip(model->destination(), commonPath);
            }
        }

        int fileLine = 0;
        QByteArray outputByteArray;
        for (int k = 0; k < model->hunks()->count(); ++k) {
            DiffHunk *hunk = model->hunks()->at(k);

            for (int i = 0; i < hunk->differences().count(); ++i) {

                Difference *diff = hunk->differences().at(i);

                int diffLineNumber = source ? diff->sourceLineNumber() : diff->destinationLineNumber();
                int diffLineCount = source ? diff->sourceLineCount() : diff->destinationLineCount();
                if (diffLineNumber - 1 >= fileLine) {
                    int start = fileLine;
                    bool hasMissing = false;

                    for (int j = start; j < diffLineNumber - 1; ++j) {
                        outputByteArray.append("\n");
                        fileLine++;
                        hasMissing = true;
                    }

                    if (hasMissing) {
                        MissingRange *range = new MissingRange();
                        range->start = start;
                        range->end = diffLineNumber - 1;
                        if(source) {
                            model->missingRanges()->sourceRanges->append(range);
                        } else {
                            model->missingRanges()->destinationRanges->append(range);
                        }
                    }
                }

                for (int j = 0; j < diffLineCount; ++j) {
                    QString line = source ? diff->sourceLineAt(j)->string() : diff->destinationLineAt(j)->string();
                    outputByteArray.append(line);
                    fileLine++;
                }

            }
        }

        QString fileName = rootDir.path() + QDir::separator() + (source ? model->source() : model->destination());

        if (outputByteArray.size() != 0) {
            //  don't save empty files
            rootDir.mkpath(source ? model->sourcePath() : model->destinationPath());
            QSaveFile tmpFile(fileName);
            tmpFile.open(QIODevice::WriteOnly);
            tmpFile.write(outputByteArray);
            tmpFile.commit();
        }
    }

}


void ModelList::accountForMovedFiles() {
    auto i = m_models->begin();
    for (; i != m_models->end(); ++i) {
        DiffModel* model = i.value();
    }
}

void ModelList::collectLocalFiles() {
    QMimeDatabase db;

    QDirIterator sourceIt(m_sourceBasePath,
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks,
            QDirIterator::Subdirectories);
    while (sourceIt.hasNext()) {
        QString filename = sourceIt.next();
        QFileInfo fileinfo = sourceIt.fileInfo();

        QMimeType mimetype = db.mimeTypeForFile(filename);

        if (fileinfo.isDir() || mimetype.inherits("text/plain")) {
            QString path = StringUtils::leftStrip(filename, m_sourceBasePath);
            if (path.startsWith(QDir::separator())) {
                path = path.mid(1);
            }
            DiffModel* model = find(path);
            if(!model) {
                // Check for moved
                model = findMoved(path, true);
                if(!model) {
                    model = new DiffModel();
                    m_models->insert(path, model);
                    model->key(path);
                }
            }
            model->setSourceFile(filename);
            model ->sourceUrl(QUrl::fromUserInput(m_tmpDir->path() + QDir::separator() + "a" + QDir::separator() + m_sourceDisplayBasePath + path));
            if(!QFileInfo(m_destinationBasePath+path).exists()) {
                model->setDestinationFile(m_destinationBasePath+path);
            }
        }
    }

    QDirIterator destinationIt(m_destinationBasePath,
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks,
            QDirIterator::Subdirectories);
    while (destinationIt.hasNext()) {
        QString filename = destinationIt.next();
        QFileInfo fileinfo = destinationIt.fileInfo();
        QMimeType mimetype = db.mimeTypeForFile(fileinfo);
        if (fileinfo.isDir() || mimetype.inherits("text/plain")) {
            QString path = StringUtils::leftStrip(filename, m_destinationBasePath);
            if (path.startsWith(QDir::separator())) {
                path = path.mid(1);
            }
            DiffModel* model = find(path);
            if(!model) {
                // Check for moved
                model = findMoved(path, false);
                if(!model) {
                    model = new DiffModel();
                    m_models->insert(path, model);
                    model->key(path);
                }
            }
            model->setDestinationFile(filename);
            model ->destinationUrl(QUrl::fromUserInput(m_tmpDir->path() + QDir::separator() + "b" + QDir::separator() + m_destinationDisplayBasePath + path));
            if(!QFileInfo(m_sourceBasePath+path).exists()) {
                model->setSourceFile(m_sourceBasePath+path);
                if(m_info->mode == Compare::BlendingDir || m_info->mode == Compare::BlendingFile) {
                    model ->sourceUrl(QUrl::fromUserInput(m_tmpDir->path() + QDir::separator() + "a" + QDir::separator() + m_sourceDisplayBasePath + path));
                }
            }
        }
    }

    if(m_info->mode == Compare::BlendingFile || m_info->mode == Compare::ComparingFiles) {
        DiffModel *model = m_models->begin().value();
        model->setSourceFile(m_sourceDisplayBasePath);
        model ->sourceUrl(QUrl::fromUserInput(m_sourceDisplayBasePath));
        model->setDestinationFile(m_destinationDisplayBasePath);
        model ->destinationUrl(QUrl::fromUserInput(m_destinationDisplayBasePath));
    }
}
const QString& ModelList::sourceBasePath() const
{
    return m_sourceBasePath;
}
const QString& ModelList::destinationBasePath() const
{
    return m_destinationBasePath;
}
const QString& ModelList::sourceDisplayBasePath() const
{
    return m_sourceDisplayBasePath;
}
const QString& ModelList::destinationDisplayBasePath() const
{
    return m_destinationDisplayBasePath;
}
DiffSettings* ModelList::diffSettings() const
{
    return m_diffSettings;
}
const QString& ModelList::encoding() const
{
    return m_encoding;
}
Compare::ComparisonInfo* ModelList::comparisonInfo() const
{
    return m_info;
}
bool ModelList::isReadWrite() const
{
    return m_readWrite;
}


void ModelList::createMissingFolders(const QString path) {
    QString relPath = StringUtils::leftStrip(path, m_destinationBasePath);

    const QStringList pathParts = relPath.split(QDir::separator(), QString::SkipEmptyParts);
    QString key;
    // first find any missing parent folder
    for (int i = 0; i < pathParts.size(); ++i) {
        if(!key.isEmpty()) {
            key += QDir::separator();
        }

        key += pathParts[i];

        DiffModel* model = find(key);
        // It would always find and item. Checking anyway...
        if(model && model->destinationUrl().isValid()) {
            QUrl destination = QUrl::fromUserInput(m_destinationDisplayBasePath + model->key());
            model->destinationUrl(destination);
            model->setToEqual();
        }
    }

    // Then all subfolders and files in folder
    auto i = m_models->find(path);

    while(i != m_models->end() && (i.key() == path || i.key().startsWith(path))) {
        DiffModel* model = i.value();
        QUrl destination = QUrl::fromUserInput(m_destinationDisplayBasePath + model->key());
        model->destinationUrl(destination);
        model->setToEqual();
        i++;
    }
}

void ModelList::deleteItem(QString path, bool recursively) {
    const QString itemKey = path;

    auto i = m_models->find(itemKey);

    while(i != m_models->end() && (i.key() == itemKey || (recursively && i.key().startsWith(itemKey)))) {
        i = m_models->erase(i);
    }
}
const QTemporaryDir* ModelList::tmpDir() const
{
    return m_tmpDir;
}
