//
// Created by john on 2/5/17.
//

#ifndef KDIFF_SETTINGS_H
#define KDIFF_SETTINGS_H

#include <KSharedConfig>
#include <KConfigGroup>
#include "diffsettings.h"
#include "kdiffsettings_export.h"

class KDIFFSETTINGS_EXPORT Settings : public QObject
{
Q_OBJECT
public:
    static Settings* instance();
    static const QString COMPARE_HISTORY;
    static const QString DIFF_HISTORY;
    static const QString BLEND_HISTORY;

private:
    Settings();
    ~Settings();
    KSharedConfigPtr config;
    KConfigGroup generalSettings;

signals:
    void navigationViewChanged();

public:
    void loadRecentUrls(QString configGroupName);
    const QStringList &recentPatchUrls() const;
    void saveRecentPatchUrls(const QStringList &recentPatchUrls);
    const QStringList &recentSourceUrls() const;
    void saveRecentSourceUrls(const QStringList &recentSourceUrls);
    const QStringList &recentDestinationUrls() const;
    void saveRecentDestinationUrls(const QStringList &recentDestinationUrls);
    bool showFolders() const;
    void showFolders(bool showFolders);
    bool showEqual() const;
    void showEqual(bool showEqual);
    bool showMissing() const;
    void showMissing(bool showMissing);
    const QColor &normalColor() const;
    const QColor &missingColor() const;
    const QColor &differentColor() const;
    bool autoAdvance() const;
    void autoAdvance(bool autoAdvance);
    bool autoSave() const;
    void autoSave(bool autoSave);
    bool removeEmptyFolders() const;
    void removeEmptyFolders(bool removeEmptyFolders);
    bool createBackup() const;
    void createBackup(bool createBackup);

    DiffSettings* diffSettings();
    void diffSettings(DiffSettings* diffSettings);
    static const QStringList disabledActions();
    static const QStringList mappedToMainWindowActions();

private:
    static Settings* settings;
    QStringList m_recentPatchUrls;
    QStringList m_recentSourceUrls;
    QStringList m_recentDestinationUrls;
    bool m_showFolders;
    bool m_showEqual;
    bool m_showMissing;
    DiffSettings* m_diffSettings;
    QString m_configGroupName;
    static QStringList m_disabledActions;
    static QStringList m_mappedToMainWindowActions;
    bool m_autoAdvance;
    bool m_autoSave;
    bool m_removeEmptyFolders;
    bool m_createBackup;
    QColor m_normalColor;
    QColor m_missingColor;
    QColor m_differentColor;


};

#endif //KDIFF_SETTINGS_H
