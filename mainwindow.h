// Created by john on 1/28/17.
//

#ifndef KDIFF_MAINWINDOW_H
#define KDIFF_MAINWINDOW_H

#include <KToggleAction>

#include <QSplitter>
#include <QTreeWidget>

#include <kparts/mainwindow.h>
#include "kdiffinterface.h"
#include "settings.h"

namespace KParts {
    class ReadOnlyPart;
    class ReadWritePart;
}

class KDiffMainWindow : public KParts::MainWindow
{
    Q_OBJECT
public:
    KDiffMainWindow(QWidget *parent=0);
    ~KDiffMainWindow();

private slots:
    void openFiles();
    void openDiff();
    void openDiffAndFiles();
    void quit();
    void editToolbars();
    void aboutEditor();
    void toggleMenuBar();
    void swap();

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    Settings* settings;
    KToggleAction* m_paShowMenuBar;
    KParts::ReadWritePart* m_viewPart;
    KDiffInterface* viewPart() const;
    void setupActions();
    QAction* swapAction;
};

#endif //KDIFF_MAINWINDOW_H
