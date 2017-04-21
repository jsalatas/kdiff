//
// Created by john on 1/28/17.
//

#include <QDebug>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

#include <KLocalizedString>
#include <QtCore/QMimeDatabase>

#include "opendialog.h"

OpenDialog::OpenDialog(QWidget *parent)
        : QDialog(parent)
        , m_parentWidget(parent)
        , settings(Settings::instance())
        , m_sourceListRequester(nullptr)
        , m_destinationListRequester(nullptr)
        , m_sourceListComboBox(nullptr)
        , m_destinationListComboBox(nullptr)
{
    setModal(true);


    QVBoxLayout *v = new QVBoxLayout(this);
    v->setMargin(5);
    m_layout = new QGridLayout();
    v->addLayout(m_layout);
    m_layout->setSpacing(5);
    m_layout->setColumnStretch(1, 10);


//    m_layout->addWidget(button, 4, 0);

  //  m_layout->addItem(new QSpacerItem(400, 20), 4, 2);

    QHBoxLayout *l = new QHBoxLayout();
    v->addLayout(l);
    l->setSpacing(5);

    QPushButton* button = new QPushButton(i18n("Clear &History"), this);
    button->setIcon(QIcon::fromTheme("edit-clear"));
    connect(button, SIGNAL(clicked()), this, SLOT(clearHistory()));
    l->addWidget(button, 0);

    l->addItem(new QSpacerItem(400, 20));
    l->addStretch(1);

    okButton = new QPushButton(i18n("&OK"), this);
    okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    okButton->setEnabled(false);
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    l->addWidget(okButton, 0);
    okButton->setAutoDefault(true);
    okButton->setDefault(true);

    button = new QPushButton(i18n("&Cancel"), this);
    button->setIcon(QIcon::fromTheme("dialog-cancel"));
    connect(button, SIGNAL(clicked()), this, SLOT(reject()));
    l->addWidget(button, 0);
    button->setFocus();
}

OpenDialog::~OpenDialog() {
    delete m_sourceListRequester;
    m_sourceListRequester = nullptr;
    if(m_destinationListRequester) {
        delete m_destinationListRequester;
        m_destinationListRequester = nullptr;
    }
}

void OpenDialog::init(Mode mode) {
    m_openMode = mode;
    switch (mode) {
        case Diff:
            setWindowTitle(i18n("Open Diff"));
            break;
        case Files:
            setWindowTitle(i18n("Choose Files or Folders to Compare"));
            break;
        case DiffAndFiles:
            setWindowTitle(i18n("Choose Diff and File or Folder to Compare"));
            break;
    }


    QLabel *label = new QLabel(mode == Files ? i18n("Source:") : i18n("Diff File:"), this);
    m_sourceListComboBox = new KUrlComboBox(KUrlComboBox::Both, true, this);
    m_sourceListComboBox->setObjectName("sourceUrlComboBox");
    m_sourceListComboBox->setEditable(true);
    m_sourceListComboBox->setMaxItems(10);
    m_sourceListComboBox->setUrls(mode == Files ? settings->recentSourceUrls() : settings->recentPatchUrls());
    m_sourceListRequester = new KUrlRequester(m_sourceListComboBox, nullptr);
    connect(m_sourceListRequester, SIGNAL(textChanged(const QString&)), this, SLOT(selectedFilesChanged()));
    if(mode == Files) {
        if (settings->recentSourceUrls().count() > 0) {
            m_sourceListComboBox->setUrl(QUrl::fromUserInput(settings->recentSourceUrls().at(0)));
        }
    } else {
        if (settings->recentPatchUrls().count() > 0) {
            m_sourceListComboBox->setUrl(QUrl::fromUserInput(settings->recentPatchUrls().at(0)));
        }

    }

    QPushButton *button = new QPushButton("", this);
    button->setIcon(QIcon::fromTheme("kdiff-file"));
    button->setProperty("combobox", "sourceUrlComboBox");
    button->setProperty("patch", false);
    button->setProperty("dir", false);
    button->setToolTip(i18n("Choose File"));
    if(mode == DiffAndFiles) {
        button->setProperty("patch", true);
    }
    connect(button, SIGNAL(clicked()), this, SLOT(open()));

    m_layout->addWidget(label, 0, 0);
    m_layout->addWidget(m_sourceListComboBox, 0, 1);
    m_layout->addWidget(button, 0, 2);
    QWidget::setTabOrder(m_sourceListComboBox, button);

    QPushButton *button2;
    if(mode == Files) {
        button2 = new QPushButton("", this);
        button2->setIcon(QIcon::fromTheme("kdiff-folder"));
        button2->setProperty("combobox", "sourceUrlComboBox");
        button2->setProperty("patch", false);
        button2->setProperty("dir", true);
        button2->setToolTip(i18n("Choose Folder"));
        connect(button2, SIGNAL(clicked()), this, SLOT(open()));
        m_layout->addWidget(button2, 0, 3);
        QWidget::setTabOrder(button, button2);
    }

    if(mode != Diff) {
        label = new QLabel(i18n("Destination:"), this);
        m_destinationListComboBox = new KUrlComboBox(KUrlComboBox::Both, true, this);
        m_destinationListComboBox->setObjectName("destinationUrlComboBox");
        m_destinationListComboBox->setEditable(true);
        m_destinationListComboBox->setMaxItems(10);
        m_destinationListComboBox->setUrls(settings->recentDestinationUrls());
        m_destinationListRequester = new KUrlRequester(m_destinationListComboBox,nullptr);
        connect(m_destinationListRequester, SIGNAL(textChanged(const QString&)), this, SLOT(selectedFilesChanged()));
        if (settings->recentDestinationUrls().count() > 0) {
            m_destinationListComboBox->setUrl(QUrl::fromUserInput(settings->recentDestinationUrls().at(0)));
        }
        button = new QPushButton("", this);
        button->setIcon(QIcon::fromTheme("kdiff-file"));
        button->setProperty("combobox", "destinationUrlComboBox");
        button->setProperty("patch", false);
        button->setProperty("dir", false);
        button->setToolTip(i18n("Choose File"));
        connect(button, SIGNAL(clicked()), this, SLOT(open()));

        button2 = new QPushButton("", this);
        button2->setIcon(QIcon::fromTheme("kdiff-folder"));
        button2->setProperty("combobox", "destinationUrlComboBox");
        button2->setProperty("patch", false);
        button2->setProperty("dir", true);
        button2->setToolTip(i18n("Choose Folder"));
        connect(button2, SIGNAL(clicked()), this, SLOT(open()));

        m_layout->addWidget(label, 1, 0);
        m_layout->addWidget(m_destinationListComboBox, 1, 1);
        m_layout->addWidget(button, 1, 2);
        m_layout->addWidget(button2, 1, 3);
        QWidget::setTabOrder(m_destinationListComboBox, button);
        QWidget::setTabOrder(button, button2);
    }

    selectedFilesChanged();

    QSize sh = sizeHint();
    setFixedHeight(sh.height());

    if(okButton->isEnabled()) {
        okButton->setFocus();
    }
}

void OpenDialog::open() {
    QPushButton* button = (QPushButton*) sender();
    KUrlComboBox* urlComboBox = findChild<KUrlComboBox*>( button->property( "combobox" ).toString() );
    bool isDir = button->property("dir").toBool();
    bool isPatch = button->property("patch").toBool();
    openURL(urlComboBox, isDir, isPatch);
}

int OpenDialog::chooseFiles() {
    settings->loadRecentUrls(Settings::COMPARE_HISTORY);
    init(Files);
    return exec();
}

int OpenDialog::chooseDiff() {
    settings->loadRecentUrls(Settings::DIFF_HISTORY);
    init(Diff);
    return exec();
}

int OpenDialog::chooseDiffAndFiles() {
    settings->loadRecentUrls(Settings::BLEND_HISTORY);
    init(DiffAndFiles);
    return exec();
}

void OpenDialog::selectedFilesChanged() {

    okButton->setEnabled((m_sourceListRequester == nullptr || !m_sourceListRequester->url().isEmpty())
                         && (m_destinationListRequester == nullptr || !m_destinationListRequester->url().isEmpty()));
}


void OpenDialog::openURL(KUrlComboBox *urlComboBox, bool bDir, bool bPatch) {
    QUrl current = QUrl::fromUserInput(urlComboBox->currentText());
    if (!current.isValid()) {
        current = QUrl("");
    }

    QString mime = bPatch ? QMimeDatabase().mimeTypeForName("text/x-patch").filterString() : QString("");
    QUrl newURL = bDir ? QFileDialog::getExistingDirectoryUrl(this, i18n("Select Folder"),
                                                              current,
                                                              QFileDialog::ReadOnly)
                       : QFileDialog::getOpenFileUrl(this, bPatch ? i18n("Select Diff File") : i18n("Select File"),
                                                     current, mime);
    if (!newURL.isEmpty()) {
        urlComboBox->setUrl(newURL);
    }
    // newURL won't be modified if nothing was selected.
}


void OpenDialog::accept() {
    m_sourceListComboBox->insertUrl(0, QUrl(m_sourceListComboBox->currentText()));

    if (m_destinationListComboBox != nullptr) {
        m_destinationListComboBox->insertUrl(0, QUrl(m_destinationListComboBox->currentText()));
    }

    QDialog::accept();
}

void OpenDialog::reject() {
    QDialog::reject();
}

QStringList OpenDialog::sourceList() {
    return m_sourceListComboBox->urls();
}

QStringList OpenDialog::destinationList() {
    return m_destinationListComboBox->urls();
}

QStringList OpenDialog::patchList() {
    return m_sourceListComboBox->urls();
}

void OpenDialog::clearHistory() {
    m_sourceListComboBox->clearEditText();
    m_sourceListComboBox->clear();

    if(m_openMode != Diff) {
        m_destinationListComboBox->clearEditText();
        m_destinationListComboBox->clear();
    }

    switch (m_openMode) {
        case Files:
            settings->saveRecentSourceUrls(QStringList());
            settings->saveRecentDestinationUrls(QStringList());
            break;
        case Diff:
            settings->saveRecentPatchUrls(QStringList());
            break;
        case DiffAndFiles:
            settings->saveRecentPatchUrls(QStringList());
            settings->saveRecentDestinationUrls(QStringList());
            break;

    }
}