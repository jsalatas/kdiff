//
// Created by john on 3/21/17.
//

#include <QTemporaryDir>
#include <QSaveFile>
#include <QFont>
#include <QIcon>

#include <KLocalizedString>

#include "info.h"
#include "settings.h"
#include "icons.h"
#include "linemapper.h"
#include "filenavigator.h"
#include "filenavigatoritem.h"
#include "modellist.h"
#include "diffmodel.h"

Icons* FileNavigatorItem::m_icons = new Icons();

Settings* FileNavigatorItem::m_settings = Settings::instance();

FileNavigatorItem::FileNavigatorItem(FileNavigator* view, DiffModel* model)
        : QTreeWidgetItem()
        , m_model(model)
        , m_view(view)
{
    connect(m_model, SIGNAL(modelChanged()), this, SLOT(modelChanged()));
    modelChanged();
}

FileNavigatorItem::~FileNavigatorItem() {
}

const QString FileNavigatorItem::localSource() const {
    if(m_model->isRenamed()) {
        return m_model->modelList()->tmpDir()->path() + QDir::separator() +  "a" + QDir::separator() + m_model->renamedFrom();
    }
    return m_model->source();
}

const QString FileNavigatorItem::localDestination() const {
    if(m_model->isRenamed()) {
        return m_model->modelList()->tmpDir()->path() + QDir::separator() + "b" + QDir::separator() + m_model->renamedTo();
    }
    return m_model->destination();
}

const QUrl FileNavigatorItem::sourceUrl() const {
    return m_model->sourceUrl();
}

const QUrl FileNavigatorItem::destinationUrl() const {
    return m_model->destinationUrl();
}

DiffModel *FileNavigatorItem::model() const {
    return m_model;
}

bool FileNavigatorItem::sourceMissing() const {
    return text(0).isEmpty();
}

bool FileNavigatorItem::destinationMissing() const {
    return text(1).isEmpty();
}

bool FileNavigatorItem::hasMissing() const {
    return sourceMissing() || destinationMissing();
}

bool FileNavigatorItem::isEqual() const {
    return /*m_model->isUnmodifiedMoved() ||*/ (!hasMissing() && m_model->differences()->isEmpty());
}

bool FileNavigatorItem::isDifferent() const {
    return (m_model->differences() && m_model->differences()->count() > 0) || hasMissing();
}

bool FileNavigatorItem::isDir() const {
    return m_model->isDir();
}

void FileNavigatorItem::setForeground(const QColor &color) {
    const QBrush brush(color);

    if(!text(0).isEmpty()) {
        QTreeWidgetItem::setForeground(0, brush);
    }
    if(!text(1).isEmpty()) {
        QTreeWidgetItem::setForeground(1, brush);
    }
}

void FileNavigatorItem::setIcon(const QIcon &icon) {
    if(!text(0).isEmpty()) {
        QTreeWidgetItem::setIcon(0, icon);
    } else {
        QTreeWidgetItem::setIcon(0, QIcon());
    }
    if(!text(1).isEmpty()) {
        QTreeWidgetItem::setIcon(1, icon);
    } else {
        QTreeWidgetItem::setIcon(1, QIcon());
    }
}

void FileNavigatorItem::modelChanged() {
    if(m_model->sourceUrl().isValid() || m_model->isRenamed()) {
        setText(0, m_model->isRenamed()? m_model->renamedFrom() : m_model->key());
    }
    if(m_model->destinationUrl().isValid() || m_model->isRenamed()) {
        setText(1, m_model->isRenamed()? m_model->renamedTo() : m_model->key());
    }

    updateState();
}
void FileNavigatorItem::updateState() {
    if(isEqual()) {
        setForeground(m_model->isRenamed()? (m_model->isUnmodifiedMoved()? m_settings->unmodifiedMovedColor(): m_settings->modifiedMovedColor()) : m_settings->normalColor());
        if(isDir()) {
            setIcon(m_model->isRenamed()? m_icons->folderMovedIcon() : m_icons->folderNormalIcon());
        } else {
            setIcon(m_model->isRenamed()? (m_model->isUnmodifiedMoved()? m_icons->fileUnmodifiedMovedIcon(): m_icons->fileModifiedMovedIcon()) : m_icons->fileNormalIcon());
        }
    } else if(hasMissing()){
        setForeground(m_model->isRenamed()? (m_model->isUnmodifiedMoved()? m_settings->unmodifiedMovedColor(): m_settings->modifiedMovedColor()) : m_settings->missingColor());
        if(isDir()) {
            setIcon(m_model->isRenamed()? m_icons->folderMovedIcon() : m_icons->folderMissingIcon());
        } else {
            setIcon(m_model->isRenamed()? (m_model->isUnmodifiedMoved()? m_icons->fileUnmodifiedMovedIcon(): m_icons->fileModifiedMovedIcon()) : m_icons->fileMissingIcon());
        }
    } else { // isDifferent()
        setForeground(m_model->isRenamed()? (m_model->isUnmodifiedMoved()? m_settings->unmodifiedMovedColor(): m_settings->modifiedMovedColor()) : m_settings->differentColor());
        if(isDir()) {
            setIcon(m_model->isRenamed()? m_icons->folderMovedIcon() : m_icons->folderDiffIcon());
        } else {
            setIcon(m_model->isRenamed()? (m_model->isUnmodifiedMoved()? m_icons->fileUnmodifiedMovedIcon(): m_icons->fileModifiedMovedIcon()) : m_icons->fileDiffIcon());
        }
    }

    setHidden(shouldHide());
}

bool FileNavigatorItem::shouldHide() {

    bool res = isDir() || (isEqual() && !m_settings->showEqual());

    res = res || isDir() || (hasMissing() && !m_settings->showMissing());

    res = res && (!isDir() || !m_settings->showFolders());

    res = res && (!isSelected() || m_model->isSyncedWithDisk());

    return res;
}

const QString &FileNavigatorItem::key() const {
    return m_model->key();
}
bool FileNavigatorItem::sourceExists() const {
    QFileInfo fileInfo(m_model->source());
    if(m_model->isRenamed()) {
        fileInfo = QFileInfo(m_model->modelList()->tmpDir()->path() + QDir::separator() + "a" + QDir::separator() + m_model->renamedFrom());
    }
    return fileInfo.exists();

}

bool FileNavigatorItem::destinationExists() const {
    QFileInfo fileInfo(m_model->destination());
    if(m_model->isRenamed()) {
        fileInfo = QFileInfo(m_model->modelList()->tmpDir()->path() + QDir::separator() + "b" + QDir::separator() + m_model->renamedTo());
    }
    return fileInfo.exists();

}

Difference* FileNavigatorItem::diffContainingSourceLine(int sourceLine, bool includingMissing) {
    return m_model->diffContainingSourceLine(sourceLine, includingMissing);
}

Difference* FileNavigatorItem::diffContainingDestinationLine(int destinationLine, bool includingMissing) {
    return m_model->diffContainingDestinationLine(destinationLine, includingMissing);
}

int FileNavigatorItem::sourceLineFromDestination(int destinationLine) {
    return m_model->sourceLineFromDestination(destinationLine);
}

int FileNavigatorItem::destinationLineFromSource(int sourceLine) {
    return m_model->destinationLineFromSource(sourceLine);
}

const MissingRanges *FileNavigatorItem::missingRanges() const {
    return m_model->missingRanges();
}


bool FileNavigatorItem::hasPrevious() const {
    return m_view->hasPrevious();
}

bool FileNavigatorItem::hasNext() const {
    return m_view->hasNext();
}

bool FileNavigatorItem::isReadWrite() const {
    return isDifferent() && m_model->isReadWrite();
}