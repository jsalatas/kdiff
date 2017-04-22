//
// Created by john on 3/10/17.
//

#include <QAction>

#include "kdiffpart.h"
#include <QPainter>
#include <QMimeDatabase>
#include <QTemporaryDir>
#include <QPushButton>

#include <KLocalizedString>
#include <kiconloader.h>
#include <KStandardAction>
#include <KActionCollection>
#include <kpluginfactory.h>
#include <QSaveFile>
#include <KPageDialog>
#include <kjobwidgets.h>
#include <KMessageBox>
#include <applicationpage.h>

#include "diffpage.h"
#include "filenavigator.h"
#include "filenavigatoritem.h"
#include "utils.h"

K_PLUGIN_FACTORY(KdiffPartFactory, registerPlugin<KDiffPart>();)

KDiffPart::KDiffPart(QWidget *parentWidget, QObject *parent, const QVariantList &)
        : KParts::ReadWritePart(parent)
        , m_settings(Settings::instance())
        , m_diffTmpDir(nullptr)
        , m_info(new Compare::ComparisonInfo())
        , m_selectLastDiffOnOpening(false)
        , m_openFileOnSelect(true)
        , m_backupCreated(false)
{
    m_diffView = new KDiffView(parentWidget);
    m_splitter = m_diffView->splitter();

    m_filesNavigator = new FileNavigator(m_splitter);

    m_fileView = new FileView(this, m_splitter);

    m_splitter->addWidget(m_filesNavigator);
    m_splitter->addWidget(m_fileView);
    m_splitter->setCollapsible(0, false);
    m_splitter->setCollapsible(1, false);
    m_splitter->handle(0)->setCursor(Qt::ArrowCursor);
    m_splitter->handle(1)->setCursor(Qt::ArrowCursor);

    setWidget(m_splitter);

//    initIcons();
    setupActions();

    setXMLFile("kdiffpartui.rc");

    // we are read-write by default. Right ??????
    setReadWrite(true);

    // we are not modified since we haven't done anything yet
    setModified(false);

    m_filesNavigator->viewport()->installEventFilter(m_fileView);
    m_filesNavigator->installEventFilter(m_fileView);

    connect(m_fileView, SIGNAL(resized(QList<int>*)), m_filesNavigator, SLOT(resize(QList<int>*)));
    connect(m_filesNavigator, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            m_fileView, SLOT(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
    connect(m_filesNavigator, SIGNAL(initialized()), this, SLOT(unsetBusyCursor()));
    connect(m_fileView, SIGNAL(nextFile(bool, QString)), m_filesNavigator, SLOT(nextFile(bool, QString)));
    connect(m_fileView, SIGNAL(previousFile(bool, QString)), m_filesNavigator, SLOT(previousFile(bool, QString)));
    connect(m_fileView, SIGNAL(selectFileAtPosition(const QPoint)), m_filesNavigator, SLOT(selectFileAtPosition(const QPoint)));
    connect(m_fileView, SIGNAL(selectItem(FileNavigatorItem *)), m_filesNavigator, SLOT(select(FileNavigatorItem *)));
    connect(m_fileView, SIGNAL(deleteItem(QString, bool)), m_filesNavigator, SLOT(deleteItem(QString, bool)));
};

KDiffPart::~KDiffPart() {
    delete m_diffView;
    m_diffView = nullptr;

    delete m_info;
    m_info = nullptr;

    delete m_diffTmpDir;
    m_diffTmpDir = nullptr;
};

void KDiffPart::guiActivateEvent(KParts::GUIActivateEvent *) {
    m_fileView->initializeGUI();
}

void KDiffPart::setupActions() {
    m_reloadDiffs = new QAction(this);
    m_reloadDiffs->setEnabled(false);
    m_reloadDiffs->setText(i18n("&Reload Differences"));
    m_reloadDiffs->setIcon(QIcon::fromTheme("view-refresh"));
    actionCollection()->setDefaultShortcut(m_reloadDiffs, Qt::SHIFT + Qt::Key_F5);
    actionCollection()->addAction("file_reload_diffs", m_reloadDiffs);
    connect(m_reloadDiffs, SIGNAL(triggered(bool)), this, SLOT(reloadDiffs()));

    QAction *a = new QAction(this);
    a->setText(i18n("Show &Equal"));
    a->setCheckable(true);
    a->setChecked(m_settings->showEqual());
    a->setIconVisibleInMenu(false);
    a->setIcon(QIcon::fromTheme("kdiff-equal"));
    actionCollection()->setDefaultShortcut(a, Qt::ALT + Qt::CTRL + Qt::Key_E);
    actionCollection()->addAction("show_equal", a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(showEqual()));

    a = new QAction(this);
    a->setText(i18n("Show &Folders"));
    a->setCheckable(true);
    a->setChecked(m_settings->showFolders());
    a->setIconVisibleInMenu(false);
    a->setIcon(QIcon::fromTheme("kdiff-folders"));
    actionCollection()->setDefaultShortcut(a, Qt::ALT + Qt::CTRL + Qt::Key_F);
    actionCollection()->addAction("show_folders", a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(showFolders()));

    a = new QAction(this);
    a->setText(i18n("Show &Missing"));
    a->setCheckable(true);
    a->setChecked(m_settings->showMissing());
    a->setIconVisibleInMenu(false);
    a->setIcon(QIcon::fromTheme("kdiff-missing"));
    actionCollection()->setDefaultShortcut(a, Qt::ALT + Qt::CTRL + Qt::Key_M);
    actionCollection()->addAction("show_missing", a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(showMissing()));

    a = new QAction(this);
    a->setText(i18n("Focus Files &Navigator"));
    actionCollection()->addAction("focus_navigator", a);
    actionCollection()->setDefaultShortcut(a, Qt::ALT + Qt::Key_Up);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(focusNavigator()));

    a = new QAction(this);
    a->setText(i18n("Focus &Editor"));
    actionCollection()->addAction("focus_editor", a);
    actionCollection()->setDefaultShortcut(a, Qt::ALT + Qt::Key_Down);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(focusEditor()));

    a = KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());
}

void KDiffPart::editKeys() {
    m_fileView->editKeys();
}

void KDiffPart::focusEditor() {
    actionCollection()->action("focus_destination")->trigger();
};

void KDiffPart::focusNavigator() {
    m_filesNavigator->setFocus();
}

void KDiffPart::showEqual() {
    m_settings->showEqual(!m_settings->showEqual());
}

void KDiffPart::showFolders() {
    m_settings->showFolders(!m_settings->showFolders());
}

void KDiffPart::showMissing() {
    m_settings->showMissing(!m_settings->showMissing());
}

void KDiffPart::openFiles(const QUrl &source, const QUrl &destination) {
    m_diffMode = Files;
    m_sourceUrl = source;
    m_destinationUrl = destination;
    m_diffUrl = nullptr;
    if (m_fileView->closeDocuments(true)) {
        m_reloadDiffs->setEnabled(true);
        m_fileView->reset();
        getDiffs();
    }
}

void KDiffPart::getDiffs() {
    setBusyCursor();
    switch (m_diffMode) {
        case Files: {
            m_info->depth = 0;
            m_info->diffMode = Compare::Default;
            m_info->format = Compare::Normal;
            m_info->generator = Compare::Diff;
            m_info->source = m_sourceUrl;
            m_info->destination = m_destinationUrl;
            if(!fetchURL(m_sourceUrl, true)) {
                unsetBusyCursor();
                return;
            }
            if(!fetchURL(m_destinationUrl, false)){
                unsetBusyCursor();
                return;
            };
            if (isDir(m_info->localSource) && isDir(m_info->localDestination)) {
                m_info->mode = Compare::ComparingDirs;
            } else {
                m_info->mode = Compare::ComparingFiles;
            }
        }
            break;

        case Diff: {
            m_info->mode = Compare::ShowingDiff;
            m_info->depth = 0;
            m_info->source = m_diffUrl;
            m_info->destination = nullptr;
            m_info->localDestination = "";
            if(!fetchURL(m_diffUrl, true)) {
                unsetBusyCursor();
                return;
            }
        }
            break;

        case DiffAndFiles: {
            m_info->destination = m_destinationUrl;
            if(!fetchURL(m_destinationUrl, true)) {
                unsetBusyCursor();
                return;
            }
            m_info->source = m_diffUrl;
            if(!fetchURL(m_diffUrl, false)) {
                unsetBusyCursor();
                return;
            }
            if (isDir(m_info->localSource)) {
                m_info->mode = Compare::BlendingDir;

            } else {
                m_info->mode = Compare::BlendingFile;
            }
        }
            break;
    }

    m_filesNavigator->initialize(nullptr, m_info);
    m_backupCreated = false;
    calculateDiff();
}

bool KDiffPart::fetchURL(const QUrl &url, bool addToSource) {
    // Default value if there is an error is "", we rely on it!
    QString tempFileName("");
    // Only in case of error do we set result to false, don't forget!!
    bool result = true;
    QTemporaryDir *tmpDir = 0;

    if (!url.isLocalFile()) {
        if (FileUtils::isDir(url)) {
            tmpDir = new QTemporaryDir(QDir::tempPath() + "/kdiff");
            tmpDir->setAutoRemove(true);
            tempFileName = tmpDir->path() + QLatin1Char('/') + url.fileName();
            if (!FileUtils::copyFile(url, QUrl::fromLocalFile(tempFileName), true)) {
                return false;
            }
        } else {
            tmpDir = new QTemporaryDir(QDir::tempPath() + "/kdiff");
            tmpDir->setAutoRemove(true); // Yes this is the default but just to make sure

            if (!FileUtils::copyDir(url, QUrl::fromLocalFile(tmpDir->path()), true)) {
                delete tmpDir;
                tmpDir = nullptr;
                return false;
            } else {
                tempFileName = tmpDir->path();
                QDir dir(tempFileName);
                QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                if (entries.size() == 1) // More than 1 entry in here means big problems!!!
                {
                    if (!tempFileName.endsWith('/'))
                        tempFileName += '/';
                    tempFileName += entries.at(0);
                    tempFileName += '/';
                } else {
                    delete tmpDir;
                    tmpDir = nullptr;
                    tempFileName = "";
                    result = false;
                }
            }
        }
    } else {
        // is Local already, check if exists
        if (exists(url.toLocalFile()))
            tempFileName = url.toLocalFile();
        else {
            KMessageBox::error(0, i18n("<qt>File <b>%1</b> does not exist.</qt>", url.toDisplayString()));
            result = false;
        }
    }

    if (addToSource) {
        m_info->localSource = tempFileName;
        m_info->sourceQTempDir = tmpDir;
    } else {
        m_info->localDestination = tempFileName;
        m_info->destinationQTempDir = tmpDir;
    }

    return result;
}

bool KDiffPart::exists(const QString &url) {
    QFileInfo fi(url);
    return fi.exists();
}

bool KDiffPart::isDir(const QString &url) const {
    QFileInfo fi(url);
    return fi.isDir();
}

void KDiffPart::reloadDiffs() {
    m_fileView->closeDocuments(true);
    m_fileView->reset();
    getDiffs();
}

bool KDiffPart::queryClose() {
    // FIXME: this might be closeDocuments(false)
    return m_fileView->closeDocuments(true);
}

void KDiffPart::openDiff(const QUrl &diffUrl) {
    m_diffMode = Diff;
    m_sourceUrl = nullptr;
    m_destinationUrl = nullptr;
    m_diffUrl = diffUrl;
    if (m_fileView->closeDocuments(true)) {
        m_reloadDiffs->setEnabled(true);
        m_fileView->reset();
        getDiffs();
    }
}

void KDiffPart::openDiff(const QString &diffOutput) {
    //create a temp file and open it
    // TODO: This is untested

    delete m_diffTmpDir;

    m_diffTmpDir = new QTemporaryDir(QDir::tempPath() + "/kdiff");
    QDir dir = QDir(m_diffTmpDir->path());
    QString diffFileName = dir.path() + QDir::separator() + "tmp.diff";
    QSaveFile diffFile(diffFileName);
    diffFile.open(QIODevice::WriteOnly);
    QByteArray outputByteArray;
    outputByteArray.append(diffOutput);
    diffFile.write(outputByteArray);
    diffFile.commit();
    const QUrl diffFileUrl = QUrl(diffFileName);
    openDiff(diffFileUrl);
}

void KDiffPart::openDiffAndFiles(const QUrl &diffFile, const QUrl &destination) {
    m_diffMode = DiffAndFiles;
    m_sourceUrl = nullptr;
    m_destinationUrl = destination;
    m_diffUrl = diffFile;

    if (m_fileView->closeDocuments(true)) {
        m_reloadDiffs->setEnabled(true);
        m_fileView->reset();
        getDiffs();
    }

}

void KDiffPart::setBusyCursor() {
    QCursor c;
    c.setShape(Qt::BusyCursor);
    m_filesNavigator->setCursor(c);
    m_splitter->setCursor(c);
    m_fileView->setCursor(c);
}

void KDiffPart::unsetBusyCursor() {
    m_filesNavigator->unsetCursor();
    m_splitter->unsetCursor();
    m_fileView->unsetCursor();
}

void KDiffPart::optionsPreferences()
{
    KPageDialog pref(0);
    pref.setWindowTitle(i18n("Preferences"));
    pref.setStandardButtons(QDialogButtonBox::Reset | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    pref.button(QDialogButtonBox::Ok)->setDefault(true);

    ApplicationPageSettings* appPage = new ApplicationPageSettings();
    KPageWidgetItem *item = pref.addPage(appPage, i18n("KDiff Settings"));
    item->setIcon(QIcon::fromTheme("kompare"));
    item->setHeader(i18n("KDiff Settings"));


    DiffPageSettings* diffPage = new DiffPageSettings();
    item = pref.addPage(diffPage, i18n("Diff Settings"));
    item->setIcon(QIcon::fromTheme("text-x-patch"));
    item->setHeader(i18n("Diff Settings"));

    QPushButton *restoreDefaults = pref.button(QDialogButtonBox::Reset);
    connect(restoreDefaults, SIGNAL(clicked()), appPage, SLOT(setDefaults()));
    connect(restoreDefaults, SIGNAL(clicked()), diffPage, SLOT(setDefaults()));

    QPushButton *ok = pref.button(QDialogButtonBox::Ok);
    connect(ok, SIGNAL(clicked()), appPage, SLOT(apply()));
    connect(ok, SIGNAL(clicked()), diffPage, SLOT(apply()));
    connect(ok, SIGNAL(clicked()), m_fileView, SLOT(updateActions()));

    pref.exec();

}

void KDiffPart::calculateDiff()
{
    m_modelList.reset(new ModelList(Settings::instance()->diffSettings(), m_splitter, this));
    connect(m_fileView, SIGNAL(createMissingFolders(const QString)),  m_modelList.data(), SLOT(createMissingFolders(const QString)));
    connect(m_modelList.data(), SIGNAL(modelsUpdated()), m_filesNavigator, SLOT(updateItemStates()));
    connect(m_modelList.data(), SIGNAL(modelsChanged(const DiffModelMap*)),
            this, SLOT(modelsChanged(const DiffModelMap*)));
    connect(m_modelList.data(), SIGNAL(message(QString, MessagetType, bool)), this, SLOT(message(QString, MessagetType, bool)));
    connect(m_fileView, SIGNAL(deleteItem(QString, bool)), m_modelList.data(), SLOT(deleteItem(QString, bool)));

    if(m_info->mode == Compare::ShowingDiff) {
        m_modelList->openDiff(m_info);
    } else if(m_info->mode == Compare::BlendingDir) {
        m_modelList->openDirAndDiff(m_info);
    } else if(m_info->mode == Compare::BlendingFile) {
        m_modelList->openFileAndDiff(m_info);
    } else {
        m_modelList->compare(m_info);
    }
}

void KDiffPart::showMessage(QString message, MessagetType messagetType) {
    switch (messagetType) {
        case Info:
            KMessageBox::information(m_splitter, message);
            break;
        case Warning: {
            auto dialog = new QDialog(m_splitter);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setWindowTitle(i18n("Warning"));
            dialog->setObjectName(QStringLiteral("warningYesNoList"));
            auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, dialog);
            KMessageBox::createKMessageBox(dialog, buttonBox, QMessageBox::Warning,
                    message, QStringList(), QString(), nullptr, KMessageBox::NoExec | KMessageBox::Notify);
            dialog->show();
            break;
        }
        case Error:
            KMessageBox::error(m_splitter, message);
            break;
    }
}

void KDiffPart::message(QString message, MessagetType messagetType, bool stop) {
    showMessage(message, messagetType);
    if(stop) {
        unsetBusyCursor();
    }
}
void KDiffPart::modelsChanged(const DiffModelMap* models) {
    m_filesNavigator->setHeaderLabels(QStringList() << i18n("Source: ") + m_modelList->sourceDisplayBasePath()
                                                    << i18n("Destination: ") + m_modelList->destinationDisplayBasePath());

    m_filesNavigator->initialize(models, m_info, m_modelList->sourceBasePath(), m_modelList->destinationBasePath());
}


void KDiffPart::createBackup(QUrl destinationUrl) {
    if(m_backupCreated || !m_settings->createBackup())
        return;


    QUrl backupUrl = QUrl(destinationUrl);
    QString backupPath = backupUrl.path();
    if(backupPath.endsWith(QDir::separator())) {
        backupPath = backupPath.mid(0, backupPath.length() - 1);
    }
    backupUrl.setPath(backupPath +"_"+QDateTime::currentDateTime().toString("yyyyMMddhhmmss") +"_kdiff");

    m_backupCreated = FileUtils::copyDir(destinationUrl, backupUrl);
}
#include "kdiffpart.moc"

