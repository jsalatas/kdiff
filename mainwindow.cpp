//
// Created by john on 1/28/17.
//
#include <QApplication>
#include <QAction>
#include <QDebug>
#include <QHeaderView>
#include <QBitmap>
#include <QMenuBar>
#include <QSyntaxHighlighter>
#include <QLayout>
#include <QVariantList>

#include <KLocalizedString>
#include <KActionCollection>
#include <KStandardAction>
#include <QTextEdit>
#include <KMessageBox>
#include <kshortcutsdialog.h>
#include <KXMLGUIFactory>
#include <KEditToolBar>
#include <KAboutApplicationDialog>
#include <ktexteditor/editor.h>
#include <kparts/readwritepart.h>
#include <KService/KMimeTypeTrader>

#include "mainwindow.h"
#include "opendialog.h"


KDiffMainWindow::KDiffMainWindow(QWidget *parent)
        : MainWindow(parent)
        , settings(Settings::instance())
{

    m_viewPart = KMimeTypeTrader::createInstanceFromQuery<KParts::ReadWritePart>("text/x-patch", "KDiff/ViewPart", this);
    if ( m_viewPart )
    {
        setCentralWidget( m_viewPart->widget() );

        setupActions();

        setXMLFile("kdiffui.rc");
        createShellGUI(true);

        setAutoSaveSettings();

        createGUI(m_viewPart);

    }
    else
    {
        // if we couldn't load our Part, we exit since the Shell by
        // itself can't do anything useful
        KMessageBox::error(this, i18n( "Could not load our KDiffViewPart." ) );
        exit(2);
    }
}

KDiffMainWindow::~KDiffMainWindow() {
}


void KDiffMainWindow::setupActions() {
    QAction *a = new QAction(this);
    a->setText(i18n("C&ompare Files"));
    a->setIcon(QIcon::fromTheme("document-open-folder"));
    actionCollection()->setDefaultShortcut(a, Qt::CTRL + Qt::Key_O);
    actionCollection()->addAction("compare_files", a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(openFiles()));

    a = new QAction(this);
    a->setText(i18n("Open &Diff"));
    a->setIcon(QIcon::fromTheme("document-open"));
    actionCollection()->setDefaultShortcut(a, Qt::CTRL + Qt::Key_D);
    actionCollection()->addAction("open_diff", a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(openDiff()));

    a = new QAction(this);
    a->setText(i18n("&Blend Diff and Files"));
//    a->setIcon(QIcon::fromTheme("document-open"));
    actionCollection()->setDefaultShortcut(a, Qt::CTRL + Qt::Key_B);
    actionCollection()->addAction("blend_diff", a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(openDiffAndFiles()));

    KStandardAction::quit(this, SLOT(quit()), actionCollection());

    m_paShowMenuBar = KStandardAction::showMenubar(this, SLOT(toggleMenuBar()), actionCollection());

    setStandardToolBarMenuEnabled(true);

    a = actionCollection()->addAction(KStandardAction::ConfigureToolbars, QStringLiteral("options_configure_toolbars"),
                                      this, SLOT(editToolbars()));
    a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

    a = actionCollection()->addAction(QStringLiteral("help_about_editor"));
    a->setText(i18n("&About Editor Component"));
    connect(a, SIGNAL(triggered()), this, SLOT(aboutEditor()));

    a = actionCollection()->addAction(KStandardAction::KeyBindings, m_viewPart, SLOT(editKeys()));
    a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

    swapAction = new QAction(this);
    swapAction->setEnabled(false);
    swapAction->setText(i18n("&Swap Source and Destination"));
    actionCollection()->addAction("swap", swapAction);
    connect(swapAction, SIGNAL(triggered(bool)), this, SLOT(swap()));

}

void KDiffMainWindow::openFiles() {
    OpenDialog d(this);
    int status = d.chooseFiles();

    if (status == QDialog::Accepted) {
        settings->saveRecentSourceUrls(d.sourceList());
        settings->saveRecentDestinationUrls(d.destinationList());

        QUrl source = QUrl(settings->recentSourceUrls().at(0));
        QUrl destination = QUrl(settings->recentDestinationUrls().at(0));

        viewPart()->openFiles(source, destination);
        swapAction->setEnabled(true);
    }
}

void KDiffMainWindow::openDiff() {
    OpenDialog d(this);
    int status = d.chooseDiff();

    if (status == QDialog::Accepted) {
        settings->saveRecentPatchUrls(d.patchList());
        QUrl diffUrl = QUrl(settings->recentPatchUrls().at(0));

        viewPart()->openDiff(diffUrl);
        swapAction->setEnabled(false);
    }

}

void KDiffMainWindow::openDiffAndFiles() {
    OpenDialog d(this);
    int status = d.chooseDiffAndFiles();

    if (status == QDialog::Accepted) {
        settings->saveRecentPatchUrls(d.patchList());
        settings->saveRecentDestinationUrls(d.destinationList());

        QUrl diffUrl = QUrl(settings->recentPatchUrls().at(0));
        QUrl destination = QUrl(settings->recentDestinationUrls().at(0));

        viewPart()->openDiffAndFiles(diffUrl, destination);
        swapAction->setEnabled(false);
    }

}

void KDiffMainWindow::editToolbars()
{
    KConfigGroup cfg = KSharedConfig::openConfig()->group("MainWindow");
    saveMainWindowSettings(cfg);
    KEditToolBar dlg(guiFactory(), this);

//    connect(&dlg, SIGNAL(newToolBarConfig()), this, SLOT(slotNewToolbarConfig()));
    dlg.exec();
}

void KDiffMainWindow::aboutEditor()
{
    KAboutApplicationDialog dlg(KTextEditor::Editor::instance()->aboutData(), this);
    dlg.exec();
}

void KDiffMainWindow::toggleMenuBar()
{
    if (m_paShowMenuBar->isChecked()) {
        menuBar()->show();
    } else {
        const QString accel = m_paShowMenuBar->shortcut().toString();
        if (KMessageBox::warningContinueCancel(this, i18n("This will hide the menu bar completely."
                                                                  " You can show it again by typing %1.", accel),
                                               i18n("Hide menu bar"), KStandardGuiItem::cont(),
                                               KStandardGuiItem::cancel(),
                                               QLatin1String("HideMenuBarWarning")) == KMessageBox::Continue) {
            menuBar()->hide();
        }
    }
}


void KDiffMainWindow::quit() {
    if(m_viewPart->queryClose()) {
        qApp->quit();
    }
}

void KDiffMainWindow::closeEvent(QCloseEvent *event) {
    if(m_viewPart->queryClose()) {
        KMainWindow::closeEvent(event);
    } else {
        event->ignore();
    }
}

KDiffInterface* KDiffMainWindow::viewPart() const
{
    return qobject_cast<KDiffInterface *>(m_viewPart);
}

void KDiffMainWindow::swap() {
    QUrl source = QUrl(settings->recentSourceUrls().at(0));
    QUrl destination = QUrl(settings->recentDestinationUrls().at(0));
    viewPart()->openFiles(destination, source);
}