//
// Created by john on 3/23/17.
//


#include <QPainter>

#include <kiconloader.h>

#include "settings.h"
#include "icons.h"

Icons::Icons() {
    Settings* settings = Settings::instance();

    m_folderNormalIcon = getThemedIcon("kdiff-folder", settings->normalColor());
    m_fileNormalIcon = getThemedIcon("kdiff-file", settings->normalColor());
    m_folderDiffIcon = getThemedIcon("kdiff-folder", settings->differentColor());
    m_fileDiffIcon = getThemedIcon("kdiff-file", settings->differentColor());
    m_folderMissingIcon = getThemedIcon("kdiff-folder", settings->missingColor());
    m_fileMissingIcon = getThemedIcon("kdiff-file", settings->missingColor());
    m_folderMovedIcon = getThemedIcon("kdiff-folder", settings->unmodifiedMovedColor());
    m_fileModifiedMovedIcon = getThemedIcon("kdiff-file", settings->modifiedMovedColor());
    m_fileUnmodifiedMovedIcon = getThemedIcon("kdiff-file", settings->unmodifiedMovedColor());
}

QIcon *Icons::getThemedIcon(QString iconName, QColor color) const {
    QPixmap pixmap = SmallIcon(iconName);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    painter.end();

    return new QIcon(pixmap);
}

Icons::~Icons() {

}

const QIcon &Icons::folderNormalIcon() const {
    return *m_folderNormalIcon;
}

const QIcon &Icons::fileNormalIcon() const {
    return *m_fileNormalIcon;
}

const QIcon &Icons::folderDiffIcon() const {
    return *m_folderDiffIcon;
}

const QIcon &Icons::fileDiffIcon() const {
    return *m_fileDiffIcon;
}

const QIcon &Icons::folderMissingIcon() const {
    return *m_folderMissingIcon;
}

const QIcon &Icons::fileMissingIcon() const {
    return *m_fileMissingIcon;
}

const QIcon &Icons::folderMovedIcon() const {
    return *m_folderMovedIcon;
}

const QIcon &Icons::fileModifiedMovedIcon() const {
    return *m_fileModifiedMovedIcon;
}

const QIcon &Icons::fileUnmodifiedMovedIcon() const {
    return *m_fileUnmodifiedMovedIcon;
}
