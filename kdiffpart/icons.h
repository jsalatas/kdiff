//
// Created by john on 3/23/17.
//

#ifndef KDIFF_ICONS_H
#define KDIFF_ICONS_H

#include <QIcon>


class Icons {
public:
    Icons();
    ~Icons();
private:
    QIcon *getThemedIcon(QString iconName, QColor color) const;
public:
    const QIcon &folderNormalIcon() const;
    const QIcon &fileNormalIcon() const;
    const QIcon &folderDiffIcon() const;
    const QIcon &fileDiffIcon() const;
    const QIcon &folderMissingIcon() const;
    const QIcon &fileMissingIcon() const;

private:
    QIcon* m_folderNormalIcon;
    QIcon* m_fileNormalIcon;
    QIcon* m_folderDiffIcon;
    QIcon* m_fileDiffIcon;
    QIcon* m_folderMissingIcon;
    QIcon* m_fileMissingIcon;
};


#endif //KDIFF_ICONS_H
