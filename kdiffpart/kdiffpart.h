//
// Created by john on 3/10/17.
//

#ifndef KDIFF_KDIFFPART_H
#define KDIFF_KDIFFPART_H


#include <QTreeWidget>
#include <QSplitter>
#include <QDir>

#include <kparts/readwritepart.h>
#include <kparts/guiactivateevent.h>

#include "diffmodel.h"
#include "modellist.h"

#include "kdiffinterface.h"
#include "fileview.h"
#include "settings.h"
#include "kdiffview.h"

class FileNavigator;
class FileNavigatorItem;

class KDiffPart : public  KParts::ReadWritePart,
                  public KDiffInterface
{
    Q_OBJECT
    Q_INTERFACES(KDiffInterface)
public:

    KDiffPart(QWidget *parentWidget, QObject *parent, const QVariantList & );
    virtual ~KDiffPart();

private slots:
    void showFolders();
    void showEqual();
    void showMissing();
    void reloadDiffs();
    void optionsPreferences();
    void focusEditor();
    void focusNavigator();
    void unsetBusyCursor();
    void message(QString message, MessagetType messagetType, bool stop);
    void modelsChanged(const DiffModelMap* models);

public slots:
    void editKeys();


protected:
    using KParts::ReadOnlyPart::guiActivateEvent;
    void guiActivateEvent(KParts::GUIActivateEvent *) Q_DECL_OVERRIDE;

private:
    KDiffView* m_diffView;
    QSplitter* m_splitter;
    FileView* m_fileView;
    FileNavigator* m_filesNavigator;
    Settings* m_settings;

    QAction *m_reloadDiffs;
    QUrl m_sourceUrl;
    QUrl m_destinationUrl;
    QUrl m_diffUrl;

    Mode m_diffMode;
    QTemporaryDir* m_diffTmpDir;

    struct Compare::ComparisonInfo* m_info;
    QScopedPointer<ModelList> m_modelList;

    bool m_selectLastDiffOnOpening;
    bool m_openFileOnSelect;
    bool m_backupCreated;

    bool fetchURL(const QUrl& url, bool addToSource);
    bool exists(const QString &url);
    bool isDir(const QString &url) const;
    void getDiffs();
    void setupActions();
    void setBusyCursor();
    void calculateDiff();
    void showMessage(QString message, MessagetType messagetType);

// KParts::ReadWritePart methods
public:
    virtual bool queryClose();
    virtual bool saveFile() { return true; };

// KDiffInterface methods
public:
    virtual void openDiff(const QUrl &diffUrl);
    virtual void openDiff(const QString &diffOutput);
    virtual void openFiles(const QUrl &source, const QUrl &destination);
    virtual void openDiffAndFiles(const QUrl &diffFile, const QUrl &destination);
    void createBackup(QUrl destinationUrl);
};


#endif //KDIFF_KDIFFPART_H
