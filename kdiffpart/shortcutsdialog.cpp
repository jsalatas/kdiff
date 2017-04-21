//
// Created by john on 2/28/17.
//

#include <KActionCollection>
#include "shortcutsdialog.h"
#include "settings.h"

ShortcutsDialog::ShortcutsDialog(KTextEditor::View* parentView,
                                 FileView* fileView,
                                 KShortcutsEditor::ActionTypes types,
                                 KShortcutsEditor::LetterShortcuts allowLetterShortcuts,
                                 KDiffPart *parent)
        : KShortcutsDialog(types, allowLetterShortcuts, parentView)
        , m_parentView(parentView)
        , m_parent(parent)
        , m_fileView(fileView)

{
    QList<QAction *> mainActions = m_parent->actionCollection()->actions();
    KActionCollection *mainVisibleActionsCollection = new KActionCollection(m_parentView, m_parentView->actionCollection()->componentName());
    mainVisibleActionsCollection->setComponentDisplayName(m_parent->actionCollection()->componentName());
    for (int i = 0; i < mainActions.length(); ++i) {
        if(!Settings::mappedToMainWindowActions().contains(mainActions.at(i)->objectName())) {
            mainVisibleActionsCollection->addAction(mainActions.at(i)->objectName(), mainActions.at(i));
        }
    }

    addCollection(mainVisibleActionsCollection);
    QList<QAction *> kpartActions = m_parentView->actionCollection()->actions();

    KActionCollection *kpartEnabledActionsCollection = new KActionCollection(m_parentView, m_parentView->actionCollection()->componentName());
    kpartEnabledActionsCollection->setComponentDisplayName(m_parentView->actionCollection()->componentName());

    for (int i = 0; i < kpartActions.length(); ++i) {
        if(!Settings::disabledActions().contains(kpartActions.at(i)->objectName())) {
            kpartEnabledActionsCollection->addAction(kpartActions.at(i)->objectName(), kpartActions.at(i));
        }
    }

    addCollection(kpartEnabledActionsCollection );

}

void ShortcutsDialog::accept() {
    QStringList mappedActions = Settings::mappedToMainWindowActions();
    // remove mapped actions before saving
    for (int i = 0; i < mappedActions.length(); ++i) {
        m_parent->actionCollection()->takeAction(m_parent->action(mappedActions.at(i).toLatin1().data()));
    }

    m_parent->actionCollection()->writeSettings();
    m_parentView->actionCollection()->writeSettings();

    KShortcutsDialog::accept();

    // reload mapped actions after saving
    for (int i = 0; i < mappedActions.length(); ++i) {
        m_parent->actionCollection()->addAction(mappedActions.at(i), m_parentView->action(mappedActions.at(i).toLatin1().data()));
    }
    m_fileView->reloadShortcuts();
}

