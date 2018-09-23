//
// Created by john on 2/5/17.
//
#include "settings.h"
#include <QExplicitlySharedDataPointer>

const QString Settings::COMPARE_HISTORY = "Recent Compare Files";
const QString Settings::DIFF_HISTORY = "Recent Diff Files";
const QString Settings::BLEND_HISTORY = "Recent Blend Files";

Settings* Settings::settings = NULL;

QStringList Settings::m_mappedToMainWindowActions = QStringList()
        << "edit_undo"
        << "edit_redo";

QStringList Settings::m_disabledActions = QStringList()
        << "view_dynamic_word_wrap"
        << "tools_scripts"
        << "tools_apply_wordwrap"
        << "file_save_as"
        << "file_save_as_with_encoding"
        << "view_folding_markers"
        << "view_scrollbar_marks"
        << "view_scrollbar_minimap"
        << "view_word_count"
        << "switch_to_cmd_line"
        << "folding_toplevel"
        << "folding_expandtoplevel"
        << "folding_collapselocal"
        << "folding_expandlocal"
        << "view_word_wrap_marker"
        << "dynamic_word_wrap_indicators"
        << "file_save"
        << "tools_cleanIndent"
        << "tools_align"
        << "tools_comment"
        << "Previous Editing Line"
        << "Next Editing Line"
        << "tools_uncomment"
        << "tools_toggle_comment"
        << "tools_uppercase"
        << "tools_lowercase"
        << "tools_capitalize"
        << "tools_join_lines"
        << "tools_invoke_code_completion"
        << "file_print"
        << "file_print_preview"
        << "file_save_as"
        << "file_save_as_with_encoding"
        << "go_goto_line"
        << "modified_line_up"
        << "modified_line_down"
        << "set_confdlg"
        << "tools_highlighting"
        << "view_schemas"
        << "tools_indentation"
        << "set_verticalSelect"
        << "switch_next_input_mode"
        << "view_border"
        << "view_line_numbers"
        << "view_non_printable_spaces"
        << "view_input_modes"
        << "add_bom"
        << "edit_find"
        << "edit_find_selected"
        << "edit_find_selected_backwards"
        << "edit_find_next"
        << "edit_find_prev"
        << "edit_replace"
        << "tools_toggle_automatic_spell_checking"
        << "tools_change_dictionary"
        << "tools_clear_dictionary_ranges"
        << "edit_copy_html"
        << "file_export_html"
        << "file_reload"
        << "smart_newline"
        << "tools_indent"
        << "tools_unindent"
        << "transpose_char"
        << "delete_line"
        << "to_matching_bracket"
        << "select_matching_bracket"
        << "tools_scripts_emmetDecrementNumberBy01"
        << "tools_scripts_emmetDecrementNumberBy1"
        << "tools_scripts_emmetDecrementNumberBy10"
        << "tools_scripts_emmetIncrementNumberBy01"
        << "tools_scripts_emmetIncrementNumberBy1"
        << "tools_scripts_emmetIncrementNumberBy10"
        << "tools_scripts_quickCodingExpand"
        << "tools_scripts_emmetExpand"
        << "tools_scripts_emmetWrap"
        << "tools_scripts_emmetSelectTagPairInwards"
        << "tools_scripts_emmetSelectTagPairOutwards"
        << "tools_scripts_emmetMatchingPair"
        << "tools_scripts_emmetToggleComment"
        << "tools_scripts_emmetNext"
        << "tools_scripts_emmetPrev"
        << "tools_scripts_emmetSelectNext"
        << "tools_scripts_emmetSelectPrev"
        << "tools_scripts_emmetDelete"
        << "tools_scripts_emmetSplitJoinTab"
        << "tools_scripts_emmetEvaluateMathExpression"
        << "tools_scripts_sort"
        << "tools_scripts_moveLinesDown"
        << "tools_scripts_moveLinesUp"
        << "tools_scripts_duplicateLinesUp"
        << "tools_scripts_duplicateLinesDown"
        << "tools_scripts_encodeURISelection"
        << "tools_scripts_decodeURISelection"
        << "tools_scripts_jumpIndentUp"
        << "tools_scripts_jumpIndentDown"
        << "tools_spelling_from_cursor"
        << "tools_spelling"
        << "doccomplete_bw"
        << "doccomplete_fw"
        << "bookmarks_clear"
        << "bookmarks_toggle"
        << "doccomplete_sh"
        << "tools_spelling_selection"
        << "bookmarks_next"
        << "bookmarks_previous"
        << "set_encoding"
        << "set_eol"
        << "tools_toggle_write_lock";

const QStringList Settings::disabledActions() {
    return m_disabledActions;
}

const QStringList Settings::mappedToMainWindowActions() {
    return m_mappedToMainWindowActions;
}

Settings* Settings::instance() {
    if (settings == nullptr) {
        settings = new Settings();
    }
    return settings;
};

Settings::Settings()
        : config(KSharedConfig::openConfig())
        , generalSettings(KConfigGroup(config, "General"))
        , m_diffSettings(new DiffSettings(nullptr))
{
    m_showFolders = generalSettings.readEntry("Show Folders", false);
    m_showEqual = generalSettings.readEntry("Show Equal", false);
    m_showMissing = generalSettings.readEntry("Show Missing", false);

    m_autoAdvance = generalSettings.readEntry("Auto Advance", true);
    m_autoSave = generalSettings.readEntry("Auto Save", true);
    m_removeEmptyFolders = generalSettings.readEntry("Remove Empty Folders", true);
    m_createBackup = generalSettings.readEntry("Create Backup", true);
    m_diffSettings->loadSettings(config.data());

    m_normalColor = QPalette().color(QPalette::Active, QPalette::Text);
    m_missingColor = Qt::blue;
    m_differentColor = Qt::red;
    m_modifiedMovedColor = QColor(0, 168, 0, 255);
    m_unmodifiedMovedColor = QColor(128, 128, 128, 255);
}

Settings::~Settings() {
    delete m_diffSettings;
    m_diffSettings = nullptr;
}
const QStringList &Settings::recentSourceUrls() const {
    return m_recentSourceUrls;
}

void Settings::loadRecentUrls(QString configGroupName) {
    m_configGroupName = configGroupName;
    KConfigGroup group(config, m_configGroupName);

    m_recentSourceUrls = group.readEntry("Recent Source Urls", QStringList());
    m_recentDestinationUrls = group.readEntry("Recent Destination Urls", QStringList());
    m_recentPatchUrls = group.readEntry("Recent Patch Urls", QStringList());
}

void Settings::saveRecentSourceUrls(const QStringList &recentSourceUrls) {
    m_recentSourceUrls = recentSourceUrls;

    KConfigGroup group(config, m_configGroupName);
    group.writeEntry("Recent Source Urls", recentSourceUrls);
}

const QStringList &Settings::recentDestinationUrls() const {
    return m_recentDestinationUrls;
}

void Settings::saveRecentDestinationUrls(const QStringList &recentDestinationUrls) {
    m_recentDestinationUrls = recentDestinationUrls;
    KConfigGroup group(config, m_configGroupName);
    group.writeEntry("Recent Destination Urls", recentDestinationUrls);
}

const QStringList &Settings::recentPatchUrls() const {
    return m_recentPatchUrls;
}

void Settings::saveRecentPatchUrls(const QStringList &recentPatchUrls) {
    m_recentPatchUrls = recentPatchUrls;
    KConfigGroup group(config, m_configGroupName);
    group.writeEntry("Recent Patch Urls", recentPatchUrls);
}

bool Settings::showFolders() const {
    return m_showFolders;
}

void Settings::showFolders(bool showFolders) {
    m_showFolders = showFolders;
    generalSettings.writeEntry("Show Folders", showFolders);
    emit navigationViewChanged();
}

bool Settings::showEqual() const {
    return m_showEqual;
}

void Settings::showEqual(bool showEqual) {
    m_showEqual = showEqual;
    generalSettings.writeEntry("Show Equal", showEqual);
    emit navigationViewChanged();
}

bool Settings::showMissing() const {
    return m_showMissing;
}

void Settings::showMissing(bool showMissing) {
    m_showMissing = showMissing;
    generalSettings.writeEntry("Show Missing", showMissing);
    emit navigationViewChanged();
}

DiffSettings* Settings::diffSettings() {
    return m_diffSettings;
}

void Settings::diffSettings(DiffSettings* diffSettings) {
    m_diffSettings = diffSettings;
    diffSettings->saveSettings(config.data());
}

bool Settings::autoAdvance() const {
    return m_autoAdvance;
}

void Settings::autoAdvance(bool autoAdvance) {
    m_autoAdvance = autoAdvance;
    generalSettings.writeEntry("Auto Advance", autoAdvance);
}


bool Settings::autoSave() const {
    return m_autoSave;
}

void Settings::autoSave(bool autoSave) {
    m_autoSave = autoSave;
    generalSettings.writeEntry("Auto Save", autoSave);
}

bool Settings::removeEmptyFolders() const {
    return m_removeEmptyFolders;
}

void Settings::removeEmptyFolders(bool removeEmptyFolders) {
    m_removeEmptyFolders = removeEmptyFolders;
    generalSettings.writeEntry("Remove Empty Folders", removeEmptyFolders);
}

bool Settings::createBackup() const {
    return m_createBackup;
}

void Settings::createBackup(bool createBackup) {
    m_createBackup = createBackup;
    generalSettings.writeEntry("Create Backup", createBackup);
}

const QColor &Settings::normalColor() const {
    return m_normalColor;
}

const QColor &Settings::missingColor() const {
    return m_missingColor;
}
const QColor &Settings::differentColor() const {
    return m_differentColor;
}

const QColor &Settings::modifiedMovedColor() const {
    return m_modifiedMovedColor;
}

const QColor &Settings::unmodifiedMovedColor() const {
    return m_unmodifiedMovedColor;
}
