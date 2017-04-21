//
// Created by john on 1/28/17.
//

#ifndef KDIFF_OPENDIALOG_H
#define KDIFF_OPENDIALOG_H

#include <QGridLayout>
#include <QDialog>
#include <QSpinBox>

#include <kurlrequester.h>
#include <kurlcombobox.h>

#include "settings.h"
#include "kdiffinterface.h"

class OpenDialog : public QDialog
{
Q_OBJECT
public:
    OpenDialog(QWidget* parent);
    ~OpenDialog();

    QPushButton* okButton;
    virtual void accept();
    virtual void reject();
    int chooseFiles();
    int chooseDiff();
    int chooseDiffAndFiles();
    QStringList sourceList();
    QStringList destinationList();
    QStringList patchList();

private slots:
    void open();
    void selectedFilesChanged();
    void clearHistory();

private:
    void openURL(KUrlComboBox *urlComboBox, bool bDir, bool bPatch);
    void init(Mode mode);
    // KUrlRequester are only needed for autopcomplete in KUrlComboBox!!!!
    QWidget* m_parentWidget;
    Settings* settings;
    KUrlRequester* m_sourceListRequester;
    KUrlRequester* m_destinationListRequester;
    KUrlComboBox* m_sourceListComboBox;
    KUrlComboBox* m_destinationListComboBox;
    QGridLayout *m_layout;
    Mode m_openMode;
};
#endif //KDIFF_OPENDIALOG_H
