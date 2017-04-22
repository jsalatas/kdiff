//
// Created by john on 2/7/17.
//

#ifndef KDIFF_FILEVIEW_H
#define KDIFF_FILEVIEW_H

#include <QSplitter>
#include <QWidget>
#include <QScrollBar>
#include <QtAlgorithms>
#include <QList>
#include <ktexteditor/view.h>
#include <ktexteditor/movingrange.h>
#include "diffmodel.h"

using namespace KTextEditor;

class DiffScrollBar;
class KDiffPart;
class ConnectWidget;
class QTreeWidgetItem;
class FileNavigatorItem;
class Settings;

enum SelectDiff {
    None,
    First,
    Last
};

class FileView : public QWidget {
    Q_OBJECT

public:
    FileView(KDiffPart *parent, QWidget* parentWidget);
    ~FileView();
    void reloadShortcuts();
    void initializeGUI();
    bool eventFilter(QObject* object,QEvent* event);

signals:
    void resized(QList<int>* sizes);
    void polygonsUpdated();
    void nextFile(bool withDiffs, QString currentKey);
    void previousFile(bool withDiffs, QString currentKey);
    void textChanged(const QString text);
    void selectItem(FileNavigatorItem* item);
    void deleteItem(QString path, bool recursively);
    void createMissingFolders(const QString path);
    void resyncModelWithDisk();
    void selectFileAtPosition(const QPoint pos);

private:
    QSplitter *m_splitter;
    QScrollBar *m_lineScroll;
    QScrollBar *m_columnScroll;
    View *m_sourceText;
    View *m_destinationText;
    KDiffPart *m_mainWindow;
    ConnectWidget *m_connectWidget;
    bool m_syncingWidgets;
    View* m_changingPositionView;
    bool m_updatingMode;
    DiffScrollBar* m_diffScrollBar;
    int m_activeDiff;
    QList<MovingRange*>* m_sourceRanges;
    QList<MovingRange*>* m_destinationRanges;
    QAction *m_previousDiff;
    QAction *m_nextDiff;
    QAction *m_applyDiff;
    QAction *m_applyAllDiff;
    QAction *m_save;
    QAction *m_reload;
    FileNavigatorItem* m_fileNavigatorItem;
    Settings* m_settings;
    SelectDiff m_selectDiff;
    bool m_modelUpdating;
    bool m_applyingDiff;

    View* initView();
    void setupActions();
    void calcMaxScrollLine();
    void calcMaxScrollColumn();
    bool lineFullyVisible(View* m_view, int line);
    int longestVisibleLine(View *view);
    int lineWidth(View *view, int line);
    int rangeLength(View *view, int line, int from, int to);
    View* widestView();
    View* longestView();
    int middleLine(View* view);
    int middleToFirstLine(View* view, int middle);
    void alignView(View* view, int documentLine, int visibleLine);
    void highlightInlineDiffs();
    void highlightMissingRanges(const MissingRanges* missingRanges);
    void save();
    void gotoPrevious(bool stayInSameFile);
    void gotoNext(bool stayInSameFile);
    const Difference* activeDiff() const;
    int levelsOfEmptyParents(QString path, QString key);
    int levelsOfEmptyParents(QString key, QString sourcePath, QString destinationPath);
    bool dirHasFiles(QDir dir);

private slots:
    void resizeWidgets();
    void focusEdit();
    void verticalScrollPositionChanged(KTextEditor::View *view, const KTextEditor::Cursor &newPos);
    void cursorPositionChanged(KTextEditor::View *view, const KTextEditor::Cursor &newPos);
    void lineScrollMoved(int value);
    void columnScrollMoved(int value);
    void fontChanged();
    void modeChanged(KTextEditor::Document *document);
    void gotoPreviousSlot();
    void gotoNextSlot();
    void applyDiff();
    void applyAllDiffs();
    void textChanged();
    void modifiedChanged();
    void reloadFiles();
    void saveSlot();
    void focusOut(KTextEditor::View* view);
    void modelChanged();
    void updatingModel();
    void aboutToClose(KTextEditor::Document* doc);

public slots:
    void currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *);
    void updateActions();

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

public:
    bool closeDocuments(bool dontSync);
    QList<QPolygon>* overlays();
    QRect viewRect();
    void reset();
    int visibleEnd();
    int visibleStart();
    int activeDiffNumber() {
        return  m_activeDiff;
    }
    QRect sourceTextArea() {
        return m_sourceText->textAreaRect();
    }
    QRect destinationTextArea() {
        return m_destinationText->textAreaRect();
    }
    void editKeys();
};

#endif //KDIFF_FILEVIEW_H
