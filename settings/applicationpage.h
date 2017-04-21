//
// Created by john on 3/20/17.
//

#ifndef KDIFF_APPLICATIONPAGE_H
#define KDIFF_APPLICATIONPAGE_H

#include <QFrame>
#include <QCheckBox>

#include "settings.h"

#include "kdiffsettings_export.h"

class KDIFFSETTINGS_EXPORT ApplicationPageSettings : public QFrame {
    Q_OBJECT
public:
    ApplicationPageSettings();
    ~ApplicationPageSettings();

public slots:
    virtual void restore();
    virtual void apply();
    virtual void setDefaults();

private:
    void init();

private slots:
    void slotChanged();

public:
    Settings*  m_settings;
    QCheckBox* m_autoAdvance;
    QCheckBox* m_autoSave;
    QCheckBox* m_removeEmptyFolders;
    QCheckBox* m_createBackup;

};


#endif //KDIFF_APPLICATIONPAGE_H
