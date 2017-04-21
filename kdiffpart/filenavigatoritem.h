//
// Created by john on 3/21/17.
//

#ifndef KDIFF_FILENAVIGATORITEM_H
#define KDIFF_FILENAVIGATORITEM_H

#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QUrl>
#include "diffmodel.h"
#include <KTextEditor/Document>

class Icons;
class Settings;
class FileNavigator;

class FileNavigatorItem : public QObject, public QTreeWidgetItem
{
    Q_OBJECT
public:
    FileNavigatorItem(FileNavigator* view, DiffModel* model);
    ~FileNavigatorItem();

    const QString localSource() const;
    const QString localDestination() const;
    const QUrl sourceUrl() const;
    const QUrl destinationUrl() const;
    DiffModel *model() const;
    bool hasMissing() const;
    bool sourceMissing() const;
    bool destinationMissing() const;
    bool isEqual() const;
    bool isDifferent() const;
    bool isDir() const;
    void updateState();
    const QString &key() const;
    bool sourceExists() const;
    bool destinationExists() const;
    bool isReadWrite() const;
    bool hasPrevious() const;
    bool hasNext() const;

    Difference* diffContainingSourceLine(int sourceLine, bool includingMissing);
    Difference* diffContainingDestinationLine(int destinationLine, bool includingMissing);
    int sourceLineFromDestination(int destinationLine);
    int destinationLineFromSource(int sourceLine);

private slots:
    void modelChanged();

private:
    static Icons* m_icons;
    static Settings* m_settings;
    DiffModel* m_model;
    FileNavigator* m_view;
    QTemporaryDir* m_tmpDir;

public:
    const MissingRanges *missingRanges() const;

private:
    void setIcon(const QIcon &icon);
    void setForeground(const QColor &color);
    bool shouldHide();
};


#endif //KDIFF_FILENAVIGATORITEM_H
