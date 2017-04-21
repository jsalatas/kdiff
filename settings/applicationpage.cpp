//
// Created by john on 3/20/17.
//

#include <QDebug>
#include <QVBoxLayout>
#include <klocalizedstring.h>
#include <QtWidgets/QGroupBox>

#include "applicationpage.h"

#include "kdiffsettings_export.h"
ApplicationPageSettings::ApplicationPageSettings() : QFrame() {
    QVBoxLayout *l = new QVBoxLayout(this);

    QGroupBox* diffNavigationGroupBox = new QGroupBox(this);
    diffNavigationGroupBox->setTitle(i18n("Differences Navigation"));
    l->addWidget(diffNavigationGroupBox);
    l->setAlignment(Qt::AlignTop);

    diffNavigationGroupBox->setLayout(new QVBoxLayout());


    m_autoAdvance = new QCheckBox(this);
    m_autoAdvance->setText(i18n("Auto &Advance"));
    m_autoAdvance->setToolTip(i18n("Automatically advances to next or previous file"));
    diffNavigationGroupBox->layout()->addWidget(m_autoAdvance);

    m_autoSave = new QCheckBox(this);
    m_autoSave->setText(i18n("Auto &Save"));
    m_autoSave->setToolTip(i18n("Automatically saves the current file before advancing to a different one"));
    QHBoxLayout* h = new QHBoxLayout();
    h->addSpacing(16);
    h->addWidget(m_autoSave);
    h->setAlignment(Qt::AlignLeft);

    diffNavigationGroupBox->layout()->addItem(h);

    m_createBackup = new QCheckBox(this);
    m_createBackup->setText(i18n("Create &Backup"));
    m_createBackup->setToolTip(i18n("Automatically create backup of the original destination folder or file"));
    l->addWidget(m_createBackup);

    m_removeEmptyFolders = new QCheckBox(this);
    m_removeEmptyFolders->setText(i18n("&Remove Empty Folders"));
    m_removeEmptyFolders->setToolTip(i18n("Automatically remove folders when all files it contains are deleted"));
    l->addWidget(m_removeEmptyFolders);

    connect(m_autoAdvance, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    init();
}

ApplicationPageSettings::~ApplicationPageSettings() {
    m_settings = 0;
}

void ApplicationPageSettings::init() {
    m_settings = Settings::instance();

    m_autoAdvance->setChecked(m_settings->autoAdvance());
    m_autoSave->setChecked(m_settings->autoSave());
    m_removeEmptyFolders->setChecked(m_settings->removeEmptyFolders());
    m_createBackup->setChecked(m_settings->createBackup());
}

void ApplicationPageSettings::restore() {
    // this shouldn't do a thing...
}

void ApplicationPageSettings::apply() {
    m_settings->autoAdvance(m_autoAdvance->isChecked());
    m_settings->autoSave(m_autoSave->isChecked());
    m_settings->removeEmptyFolders(m_removeEmptyFolders->isChecked());
    m_settings->createBackup(m_createBackup->isChecked());
}

void ApplicationPageSettings::setDefaults() {
    m_autoAdvance->setChecked(true);
    m_autoSave->setChecked(true);
    m_removeEmptyFolders->setChecked(true);
    m_createBackup->setChecked(true);
}

void ApplicationPageSettings::slotChanged() {
    if(sender() == m_autoAdvance) {
        m_autoSave->setEnabled(m_autoAdvance->isChecked());
        m_autoSave->setChecked(m_autoSave->isChecked() && m_autoAdvance->isChecked());
    }
}