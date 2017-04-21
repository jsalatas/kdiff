//
// Created by john on 3/21/17.
//

#ifndef KDIFF_FILENAVIGATOR_H
#define KDIFF_FILENAVIGATOR_H

#include <QTreeWidget>
#include <QTemporaryDir>
#include <QSplitter>
#include <QMap>
#include <QList>
#include <QRegExp>

#include "diffmodelmap.h"

#include "kdiffinterface.h"

class FileNavigatorItem;
class Settings;

class FileNavigator : public QTreeWidget {
Q_OBJECT
public:
    FileNavigator(QSplitter *parent);
    ~FileNavigator();
    bool hasPrevious() const;
    bool hasNext() const;

signals:
    void initialized();


public slots:
    void initialize(const DiffModelMap *modelList, Compare::ComparisonInfo *info, QString sourceBaseBath = "", QString destinationBaseBath = "");
    void resize(QList<int>* headerSizes);
    void nextFile(bool withDiffs, QString currentKey);
    void previousFile(bool withDiffs, QString currentKey);
    void select(FileNavigatorItem *newItem);
    void deleteItem(QString path, bool recursively);
    void updateItemStates();
    void selectFileAtPosition(const QPoint pos);

private slots:
    void ensureSelectedAndVisible(const QItemSelection& current, const QItemSelection&);

protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) Q_DECL_OVERRIDE;

private:
    QSplitter* m_parent;
    Mode m_diffMode;
    Compare::ComparisonInfo* m_info;
    QMap<QString, FileNavigatorItem*>* m_files;
    QString m_sourceBasePath;
    QString m_destinationBasePath;
    QList<QRegExp> m_excludeFiles;
    Settings* m_settings;
    QTemporaryDir* m_tmpDir;
    QMap<QString, MissingRanges*>* m_missingRanges;
    QMouseEvent* m_event;
    bool m_initialized;
    QString m_destinationDiffBasePath;

    void fillTree(const DiffModelMap *modelList);
    bool exists(const QString &url) const;
    bool isDir(const QString &url) const;
    FileNavigatorItem* findItem(const QString path) const;
    void excludeFiles();
    bool expanded(const DiffModelMap *modelList);
    void selectItem(FileNavigatorItem* newItem);
    bool containsFiles(const QString path);
    QModelIndex findFirstVisible();
};


#endif //KDIFF_FILENAVIGATOR_H

