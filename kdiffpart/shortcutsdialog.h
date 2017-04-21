//
// Created by john on 2/28/17.
//

#ifndef KDIFF_SHORTCUTSDIALOG_H
#define KDIFF_SHORTCUTSDIALOG_H

#include <KShortcutsDialog>
#include <KShortcutsEditor>
#include <ktexteditor/view.h>
#include "kdiffpart.h"
#include "fileview.h"

class ShortcutsDialog : public KShortcutsDialog {
Q_OBJECT
public:
    explicit ShortcutsDialog(KTextEditor::View *parentView, FileView* fileView, KShortcutsEditor::ActionTypes types = KShortcutsEditor::AllActions,
                             KShortcutsEditor::LetterShortcuts allowLetterShortcuts = KShortcutsEditor::LetterShortcutsAllowed,
                             KDiffPart *parent = nullptr);

    virtual void accept();

private:
    KTextEditor::View *m_parentView;
    KDiffPart *m_parent;
    FileView* m_fileView;
};


#endif //KDIFF_SHORTCUTSDIALOG_H
