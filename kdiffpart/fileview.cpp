//
// Created by john on 2/7/17.
//

#include <QFile>
#include <QDirIterator>
#include <algorithm>
#include <QGridLayout>
#include <QFileInfo>
#include <QPainter>
#include <KActionCollection>
#include <QPaintEvent>
#include <QThread>
#include <KLocalizedString>
#include <KXMLGUIFactory>
#include <KShortcutsDialog>
#include <KMessageBox>
#include <ktexteditor/editor.h>
#include <ktexteditor/configinterface.h>
#include <ktexteditor/movinginterface.h>
#include "diffsettings.h"

#include "fileview.h"
#include "diffscrollbar.h"
#include "kdiffpart.h"
#include "shortcutsdialog.h"
#include "connectwidget.h"
#include "filenavigator.h"
#include "filenavigatoritem.h"
#include "settings.h"
#include "utils.h"

FileView::FileView(KDiffPart *parent, QWidget* parentWidget)
        : QWidget(parentWidget)
        , m_mainWindow(parent)
        , m_updatingMode(false)
        , m_activeDiff(-1)
        , m_sourceRanges(new QList<MovingRange*>())
        , m_destinationRanges(new QList<MovingRange*>())
        , m_fileNavigatorItem(nullptr)
        , m_settings(Settings::instance())
        , m_selectDiff(First)
        , m_modelUpdating(false)
        , m_applyingDiff(false)
{
    QGridLayout *l = new QGridLayout(this);

    m_sourceText = initView();
    m_destinationText = initView();

    m_splitter = new QSplitter(this);
    m_splitter->setOrientation(Qt::Horizontal);

    m_splitter->addWidget(m_sourceText);
    m_splitter->addWidget(m_destinationText);

    m_splitter->setCollapsible(0, false);
    m_splitter->setCollapsible(1, false);


    m_columnScroll = new QScrollBar(this);
    m_columnScroll->setOrientation(Qt::Horizontal);
    QSizePolicy sp = m_columnScroll->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    m_columnScroll->setSizePolicy(sp);
    m_columnScroll->setVisible(false);
    m_columnScroll->setFocusPolicy(Qt::NoFocus);
    m_columnScroll->setMinimum(0);
    m_columnScroll->setSingleStep(1);

    m_lineScroll = new QScrollBar(this);
    m_lineScroll->setOrientation(Qt::Vertical);
    m_lineScroll->setFocusPolicy(Qt::NoFocus);
    sp = m_lineScroll->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    m_lineScroll->setSizePolicy(sp);
    m_lineScroll->setVisible(false);
    m_lineScroll->setMinimum(0);
    m_lineScroll->setSingleStep(1);

    m_diffScrollBar = new DiffScrollBar(this);
    m_diffScrollBar->setOrientation(Qt::Vertical);
    m_diffScrollBar->setFocusPolicy(Qt::NoFocus);

    sp = m_diffScrollBar->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    m_diffScrollBar->setSizePolicy(sp);
    m_diffScrollBar->setMinimum(0);
    m_diffScrollBar->setSingleStep(1);
    m_diffScrollBar->setVisible(false);

    l->addWidget(m_splitter, 0, 1);
    l->addWidget(m_diffScrollBar, 0, 0);
    l->addWidget(m_lineScroll, 0, 2);
    l->addWidget(m_columnScroll, 1, 1);

    l->setColumnStretch(0, 0);
    l->setColumnStretch(1, 2);
    l->setColumnStretch(2, 0);

    l->setRowStretch(0, 2);
    l->setRowStretch(1, 0);

    setupActions();

    m_connectWidget = new ConnectWidget(this);
    m_connectWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    m_connectWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_connectWidget->setGeometry(viewRect());

    connect(m_splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(resizeWidgets()));
    connect(m_sourceText, SIGNAL(verticalScrollPositionChanged(KTextEditor::View *,const KTextEditor::Cursor&)),
            this, SLOT(verticalScrollPositionChanged(KTextEditor::View *, const KTextEditor::Cursor&)));
    connect(m_destinationText, SIGNAL(verticalScrollPositionChanged(KTextEditor::View *, const KTextEditor::Cursor&)),
            this, SLOT(verticalScrollPositionChanged(KTextEditor::View *, const KTextEditor::Cursor&)));
    connect(m_sourceText, SIGNAL(cursorPositionChanged(KTextEditor::View *, const KTextEditor::Cursor&)),
            this, SLOT(cursorPositionChanged(KTextEditor::View *, const KTextEditor::Cursor&)));
    connect(m_destinationText, SIGNAL(cursorPositionChanged(KTextEditor::View *, const KTextEditor::Cursor&)),
            this, SLOT(cursorPositionChanged(KTextEditor::View *, const KTextEditor::Cursor&)));
    connect(m_lineScroll, SIGNAL(valueChanged(int)), this, SLOT(lineScrollMoved(int)));
    connect(m_diffScrollBar, SIGNAL(valueChanged(int)), this, SLOT(lineScrollMoved(int)));
    connect(m_columnScroll, SIGNAL(valueChanged(int)), this, SLOT(columnScrollMoved(int)));
    connect(m_sourceText->document(), SIGNAL(modeChanged(KTextEditor::Document *)),
            this, SLOT(modeChanged(KTextEditor::Document *)));
    connect(m_destinationText->document(), SIGNAL(modeChanged(KTextEditor::Document *)),
            this, SLOT(modeChanged(KTextEditor::Document *)));

    connect(m_destinationText->document(), SIGNAL(textChanged(KTextEditor::Document*)), this, SLOT(textChanged()));
    connect(m_destinationText->document(), SIGNAL(modifiedChanged(KTextEditor::Document*)), this, SLOT(modifiedChanged()));
    connect(m_destinationText->document(), SIGNAL(aboutToClose(KTextEditor::Document*)), this, SLOT(aboutToClose(KTextEditor::Document*)));

    connect(m_sourceText, SIGNAL(focusOut (KTextEditor::View*)), this, SLOT(focusOut (KTextEditor::View*)));
    connect(m_destinationText, SIGNAL(focusOut (KTextEditor::View*)), this, SLOT(focusOut (KTextEditor::View*)));
}

View *FileView::initView() {
    Document* doc = Editor::instance()->createDocument(this);

    View *view = doc->createView(this);
    view->setEnabled(false);
    view->setMinimumWidth(600);

    ConfigInterface *viewIface = qobject_cast<ConfigInterface *>(view);
    view->setStyleSheet("QScrollBar {width:0px;}");
    viewIface->setConfigValue("folding-bar", false);
    viewIface->setConfigValue("icon-bar", false);
    viewIface->setConfigValue("dynamic-word-wrap", false);
    viewIface->setConfigValue("scrollbar-minimap", false);

    ConfigInterface *docIface = qobject_cast<ConfigInterface *>(doc);
    docIface->setConfigValue("on-the-fly-spellcheck", false);

    QStringList da = Settings::disabledActions();

    for (int i = 0; i < da.length(); ++i) {
        QAction *a = view->action(da.at(i).toLatin1().data());
        if( a != NULL) {
            a->setEnabled(false);
        }

    }

    return view;
}


void FileView::setupActions() {
    QAction *a = m_mainWindow->actionCollection()->addAction(KStandardAction::Back, "focus_source");
    a->setText(i18n("Focus Source"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(focusEdit()));

    a = m_mainWindow->actionCollection()->addAction(KStandardAction::Forward, "focus_destination");
    a->setText(i18n("Focus Destination"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(focusEdit()));

    m_previousDiff = new QAction(m_mainWindow);
    m_previousDiff->setEnabled(false);
    m_previousDiff->setText(i18n("&Previous Difference"));
    m_previousDiff->setToolTip(i18n("Go to previous difference"));
    m_previousDiff->setIcon(QIcon::fromTheme("arrow-up"));
    m_mainWindow->actionCollection()->addAction("previous_difference", m_previousDiff);
    m_mainWindow->actionCollection()->setDefaultShortcut(m_previousDiff, Qt::CTRL + Qt::Key_Comma);
    connect(m_previousDiff, SIGNAL(triggered(bool)), this, SLOT(gotoPreviousSlot()));

    m_nextDiff = new QAction(m_mainWindow);
    m_nextDiff->setEnabled(false);
    m_nextDiff->setText(i18n("&Next Difference"));
    m_nextDiff->setToolTip(i18n("Go to Next difference"));
    m_nextDiff->setIcon(QIcon::fromTheme("arrow-down"));
    m_mainWindow->actionCollection()->addAction("next_difference", m_nextDiff);
    m_mainWindow->actionCollection()->setDefaultShortcut(m_nextDiff, Qt::CTRL + Qt::Key_Period);
    connect(m_nextDiff, SIGNAL(triggered(bool)), this, SLOT(gotoNextSlot()));

    m_applyDiff = new QAction(m_mainWindow);
    m_applyDiff->setProperty("applyAll", false);
    m_applyDiff->setEnabled(false);
    m_applyDiff->setText(i18n("Apply &Current Difference"));
    m_applyDiff->setToolTip(i18n("Apply current difference"));
    m_applyDiff->setIcon(QIcon::fromTheme("arrow-right"));
    m_mainWindow->actionCollection()->addAction("apply_difference", m_applyDiff);
    m_mainWindow->actionCollection()->setDefaultShortcut(m_applyDiff, Qt::CTRL + Qt::Key_Space);
    connect(m_applyDiff, SIGNAL(triggered(bool)), this, SLOT(applyDiff()));

    m_applyAllDiff = new QAction(m_mainWindow);
    m_applyAllDiff->setEnabled(false);
    m_applyAllDiff->setText(i18n("&Apply All Differences"));
    m_applyAllDiff->setToolTip(i18n("Apply all differences"));
    m_applyAllDiff->setIcon(QIcon::fromTheme("arrow-right-double"));
    m_mainWindow->actionCollection()->addAction("apply_all_differences", m_applyAllDiff);
    m_mainWindow->actionCollection()->setDefaultShortcut(m_applyAllDiff, Qt::ALT + Qt::CTRL + Qt::Key_Space);
    connect(m_applyAllDiff, SIGNAL(triggered(bool)), this, SLOT(applyAllDiffs()));

    m_save = m_mainWindow->actionCollection()->addAction(KStandardAction::Save, "saveSlot");
    m_save->setEnabled(false);
    m_mainWindow->actionCollection()->setDefaultShortcut(m_save, Qt::CTRL + Qt::Key_S);
    connect(m_save, SIGNAL(triggered(bool)), this, SLOT(saveSlot()));

    m_reload = new QAction(this);
    m_reload->setEnabled(false);
    m_reload->setText(i18n("Reload &Files"));
    m_reload->setIcon(QIcon::fromTheme("view-refresh"));
    m_mainWindow->actionCollection()->setDefaultShortcut(m_reload, Qt::Key_F5);
    m_mainWindow->actionCollection()->addAction("file_reload_files", m_reload);
    connect(m_reload, SIGNAL(triggered(bool)), this, SLOT(reloadFiles()));

    a = m_sourceText->action("view_inc_font_sizes");
    connect(a, SIGNAL(triggered(bool)), this, SLOT(fontChanged()));
    a = m_destinationText->action("view_inc_font_sizes");
    connect(a, SIGNAL(triggered(bool)), this, SLOT(fontChanged()));

    a = m_sourceText->action("view_dec_font_sizes");
    connect(a, SIGNAL(triggered(bool)), this, SLOT(fontChanged()));
    a = m_destinationText->action("view_dec_font_sizes");
    connect(a, SIGNAL(triggered(bool)), this, SLOT(fontChanged()));
}

void FileView::resizeWidgets() {
    calcMaxScrollColumn();

    QList<int>* sizes = new QList<int>();

    sizes->insert(0, m_splitter->mapToParent(QPoint(m_splitter->sizes()[0], 0)).x());
    sizes->insert(1, m_splitter->mapToParent(QPoint(m_splitter->sizes()[1], 0)).x());
    emit resized(sizes);

    emit polygonsUpdated();
}

void FileView::fontChanged() {
    View* view = m_sourceText->hasFocus() ? m_sourceText : m_destinationText;
    ConfigInterface *viewIface = qobject_cast<ConfigInterface *>(view);
    QFont f = viewIface->configValue("font").value<QFont>();

    if(view == m_sourceText) {
        QFont fOther = m_destinationText->font();
        fOther.setPointSize(f.pointSize());
        ConfigInterface *iface = qobject_cast<ConfigInterface *>(m_destinationText);
        iface->setConfigValue("font", fOther);
    } else {
        QFont fOther = m_sourceText->font();
        fOther.setPointSize(f.pointSize());
        ConfigInterface *iface = qobject_cast<ConfigInterface *>(m_sourceText);
        iface->setConfigValue("font", fOther);
    }

    calcMaxScrollLine();
    calcMaxScrollColumn();
}

void FileView::modeChanged(KTextEditor::Document *document) {
    if(m_updatingMode)
        return;

    m_updatingMode = true;

    if(document == m_sourceText->document()) {
        m_destinationText->document()->setMode(m_sourceText->document()->mode());
    } else {
        m_sourceText->document()->setMode(m_destinationText->document()->mode());
    }

    m_updatingMode = false;
}

void FileView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    m_connectWidget->setGeometry(viewRect());
    emit polygonsUpdated();

    calcMaxScrollLine();
    calcMaxScrollColumn();
    resizeWidgets();
}

void FileView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    resizeWidgets();
}

bool FileView::closeDocuments(bool dontSync) {
    m_destinationText->document()->blockSignals(dontSync);
    if(!m_destinationText->document()->closeUrl()) {
        m_destinationText->document()->blockSignals(false);
        return false;
    }
    m_destinationText->document()->blockSignals(false);
    m_fileNavigatorItem = nullptr;
    m_destinationText->setEnabled(false);
    m_destinationText->update();
    m_sourceText->document()->closeUrl();
    m_sourceText->setEnabled(false);
    m_sourceText->update();
    m_lineScroll->setVisible(false);
    m_columnScroll->setVisible(false);
    m_diffScrollBar->setVisible(false);

    m_previousDiff->setEnabled(false);
    m_nextDiff->setEnabled(false);
    m_applyDiff->setEnabled(false);
    m_applyAllDiff->setEnabled(false);
    m_save->setEnabled(false);
    m_reload->setEnabled(false);

    return true;
}

void FileView::highlightMissingRanges(const MissingRanges* missingRanges) {
    if(missingRanges == nullptr) {
        return;
    }

    MovingInterface* iface = qobject_cast<KTextEditor::MovingInterface*> (m_sourceText->document());

    QBrush brush(QColor(230, 230, 230));
    for (int i = 0; i < missingRanges->sourceRanges->count(); ++i) {
        MissingRange* mr = missingRanges->sourceRanges->at(i);

        Range r = Range();
        r.setStart(Cursor(mr->start, 0));
        r.setEnd(Cursor(mr->end, 0));
        MovingRange* mrange = iface->newMovingRange(r);
        mrange->setEmptyBehavior(MovingRange::InvalidateIfEmpty);
        Attribute* attr = new Attribute();
        attr->setFontBold(true);
        attr->setBackground(brush);
        mrange->setAttribute(QExplicitlySharedDataPointer<KTextEditor::Attribute>(attr));
    }

    iface = qobject_cast<KTextEditor::MovingInterface*> (m_destinationText->document());

    for (int i = 0; i < missingRanges->destinationRanges->count(); ++i) {
        MissingRange* mr = missingRanges->destinationRanges->at(i);

        Range r = Range();
        r.setStart(Cursor(mr->start, 0));
        r.setEnd(Cursor(mr->end, 0));
        MovingRange* mrange = iface->newMovingRange(r);
        mrange->setEmptyBehavior(MovingRange::InvalidateIfEmpty);
        Attribute* attr = new Attribute();
        attr->setFontBold(true);
        attr->setBackground(brush);
        mrange->setAttribute(QExplicitlySharedDataPointer<KTextEditor::Attribute>(attr));
    }
}

void FileView::highlightInlineDiffs() {
    if(m_modelUpdating)
        return;

    Range empty = Range();
    empty.setStart(Cursor(0, 0));
    empty.setEnd(Cursor(0, 0));

    for (int i = 0; i < m_sourceRanges->length(); ++i) {
        m_sourceRanges->at(i)->setRange(empty);
    }
    for (int i = 0; i < m_destinationRanges->length(); ++i) {
        m_destinationRanges->at(i)->setRange(empty);
    }

    m_sourceRanges->empty();
    m_destinationRanges->empty();

    QBrush foregroundBrush(QColor(255, 210, 210));
    QBrush backgroundBrush(QColor(180, 0, 0));
    Cursor start;
    Cursor end;
    // TODO: Can I merge source and destination highlighting in one pass?
    // Destination Text
    MovingInterface* iface = qobject_cast<KTextEditor::MovingInterface*> (m_destinationText->document());
    for (int i = 0; i < m_destinationText->document()->lines(); ++i) {
        const Difference* diff = m_fileNavigatorItem->diffContainingDestinationLine(i, true);
        if(diff != nullptr) {
            for (int j = 0; j < diff->destinationLineCount(); ++j) {
                QList<Marker*> markers = diff->destinationLineAt(j)->markerList();
                for (int k = 0; k < markers.length(); ++k) {
                    start = Cursor::invalid();
                    Marker* marker = markers.at(k);
                    if (marker->type() == Marker::Start) {
                        start = Cursor(i+j, marker->offset());
                    } else {
                        int offset = marker->offset();
                        end = Cursor(i+j, offset > m_destinationText->document()->lineLength(i+j) ? m_destinationText->document()->lineLength(i+j):marker->offset());
                        if(!start.isValid()) {
                            start = Cursor(i+j, 0);
                        }
                        Range r = Range();
                        r.setStart(start);
                        r.setEnd(end);
                        MovingRange* mrange = iface->newMovingRange(r);
                        mrange->setEmptyBehavior(MovingRange::InvalidateIfEmpty);
                        Attribute* attr = new Attribute();
                        attr->setFontBold(true);
                        attr->setBackground(foregroundBrush);
                        attr->setForeground(backgroundBrush);
                        mrange->setAttribute(QExplicitlySharedDataPointer<KTextEditor::Attribute>(attr));
                        m_destinationRanges->append(mrange);
                    }
                }
            }
            i += diff->destinationLineCount();
        }
    }
    // Source Text
    iface = qobject_cast<KTextEditor::MovingInterface*> (m_sourceText->document());
    for (int i = 0; i < m_sourceText->document()->lines(); ++i) {
        const Difference* diff = m_fileNavigatorItem->diffContainingSourceLine(i, true);
        if(diff != nullptr) {
            for (int j = 0; j < diff->sourceLineCount(); ++j) {
                QList<Marker*> markers = diff->sourceLineAt(j)->markerList();
                for (int k = 0; k < markers.length(); ++k) {
                    start = Cursor::invalid();
                    Marker* marker = markers.at(k);
                    if (marker->type() == Marker::Start) {
                        start = Cursor(i+j, marker->offset());
                    } else {
                        int offset = marker->offset();
                        end = Cursor(i+j, offset > m_sourceText->document()->lineLength(i+j)? m_sourceText->document()->lineLength(i+j):marker->offset());
                        if(!start.isValid()) {
                            start = Cursor(i+j, 0);
                        }
                        Range r = Range();
                        r.setStart(start);
                        r.setEnd(end);
                        MovingRange* mrange = iface->newMovingRange(r);
                        mrange->setEmptyBehavior(MovingRange::InvalidateIfEmpty);
                        Attribute* attr = new Attribute();
                        attr->setFontBold(true);
                        attr->setBackground(foregroundBrush);
                        attr->setForeground(backgroundBrush);
                        mrange->setAttribute(QExplicitlySharedDataPointer<KTextEditor::Attribute>(attr));
                        m_sourceRanges->append(mrange);
                    }
                }
            }
            i += diff->sourceLineCount();
        }
    }
}


void FileView::focusEdit() {
    QObject *sender = QObject::sender();
    if (sender != nullptr) {
        if (m_mainWindow->actionCollection()->action("focus_source") == sender) {
            if (m_sourceText->isEnabled()) {
                m_sourceText->setFocus();
            }
        } else if (m_mainWindow->actionCollection()->action("focus_destination") == sender) {
            if (m_destinationText->isEnabled()) {
                m_destinationText->setFocus();
            }
        }

    }
}

QRect FileView::viewRect() {
    return QRect(m_sourceText->mapTo(this, m_sourceText->rect().topLeft()),
                 m_destinationText->mapTo(this, m_destinationText->textAreaRect().bottomRight()));
}

QList<QPolygon>* FileView::overlays() {

    if (!m_fileNavigatorItem || m_modelUpdating)
        return nullptr;

    QList<QPolygon>* res = new QList<QPolygon>();

    int sourceStartLineY = m_sourceText->cursorToCoordinate(Cursor(m_sourceText->firstDisplayedLine(), 0)).y();
    int destinationStartLineY = m_destinationText->cursorToCoordinate(Cursor(m_destinationText->firstDisplayedLine(), 0)).y();

    int sourceEndLineY = m_sourceText->cursorToCoordinate(Cursor(m_sourceText->lastDisplayedLine(), 0)).y();
    int destinationEndLineY = m_destinationText->cursorToCoordinate(Cursor(m_destinationText->lastDisplayedLine(), 0)).y();

    int sourceVisibleStart = m_sourceText->firstDisplayedLine();
    int sourceVisibleEnd = m_sourceText->lastDisplayedLine();
    int destinationVisibleStart = m_destinationText->firstDisplayedLine();
    int destinationVisibleEnd = m_destinationText->lastDisplayedLine();

    int sourceVisibleLines = m_sourceText->lastDisplayedLine(View::VisibleLine) - m_sourceText->firstDisplayedLine(View::VisibleLine);
    int sourceLineHeight;
    if(sourceVisibleLines > 0) {
        sourceLineHeight = (sourceEndLineY - sourceStartLineY) / sourceVisibleLines;
    } else {
        sourceLineHeight = m_sourceText->fontMetrics().height();
    };

    int destinationVisibleLines = m_destinationText->lastDisplayedLine(View::VisibleLine) - m_destinationText->firstDisplayedLine(View::VisibleLine);
    int destinationLineHeight;
    if(destinationVisibleLines > 0) {
        destinationLineHeight = (destinationEndLineY - destinationStartLineY) / destinationVisibleLines;
    } else {
        destinationLineHeight = m_destinationText->fontMetrics().height();
    };

    // recalculate extended source visible range based on the destination's visible range
    int extendedVisbleStart = m_fileNavigatorItem->sourceLineFromDestination(destinationVisibleStart);
    int extendedVisbleEnd = m_fileNavigatorItem->sourceLineFromDestination(destinationVisibleEnd);

    QPoint sourceUpperLeft = m_sourceText->rect().topLeft();
    QPoint sourceLowerRight = m_sourceText->textAreaRect().bottomRight();

    QPoint destinationUpperLeft = m_destinationText->textAreaRect().topLeft();
    QPoint destinationLowerRight = m_destinationText->rect().bottomRight();

    m_activeDiff = -1;
    for (int i = std::min(sourceVisibleStart, extendedVisbleStart) -1;
         i <= std::max(sourceVisibleEnd, extendedVisbleEnd) +1; ++i) {
        const Difference *difference = m_fileNavigatorItem->diffContainingSourceLine(i, true);
        if (difference == nullptr)
            continue;

        QPolygon polygon = QPolygon();

        int sourceStart = difference->sourceLineNumber() - 1;
        int destinationStart = difference->destinationLineNumber() - 1;

        int sourceLines = difference->sourceLineCount();
        int destinationLines = difference->destinationLineCount();

        int sourceStartY = m_sourceText->cursorToCoordinate(Cursor(sourceStart, 0)).y();
        if (sourceStartY == -1) {
            sourceStartY = sourceStartLineY - (sourceVisibleStart - sourceStart) * sourceLineHeight;
        }

        int sourceEndY = m_sourceText->cursorToCoordinate(Cursor(sourceStart + sourceLines, 0)).y();
        if (sourceEndY == -1) {
            sourceEndY = sourceEndLineY - (sourceVisibleEnd - (sourceStart + sourceLines)) * sourceLineHeight;
        }

        int destinationStartY = m_destinationText->cursorToCoordinate(Cursor(destinationStart, 0)).y();
        if (destinationStartY == -1) {
            destinationStartY = destinationStartLineY - (destinationVisibleStart - destinationStart) * destinationLineHeight;

        }

        int destinationEndY = m_destinationText->cursorToCoordinate(Cursor(destinationStart + destinationLines, 0)).y();
        if (destinationEndY == -1) {
            destinationEndY = destinationEndLineY - (destinationVisibleEnd - (destinationStart + destinationLines)) * destinationLineHeight;
        }

        QPoint point;

        point = QPoint(sourceUpperLeft.x() + 2, sourceStartY);
        polygon << m_connectWidget->mapFrom(this, m_sourceText->mapTo(this, point));

        point = QPoint(sourceLowerRight.x(), sourceStartY);
        polygon << m_connectWidget->mapFrom(this, m_sourceText->mapTo(this, point));

        point = QPoint(destinationUpperLeft.x(), destinationStartY);
        polygon << m_connectWidget->mapFrom(this, m_destinationText->mapTo(this, point));

        point = QPoint(destinationLowerRight.x(), destinationStartY);
        polygon << m_connectWidget->mapFrom(this, m_destinationText->mapTo(this, point));

        point = QPoint(destinationLowerRight.x(), destinationLines == 0 ? destinationStartY + 2 : destinationEndY);
        polygon << m_connectWidget->mapFrom(this, m_destinationText->mapTo(this, point));

        point = QPoint(destinationUpperLeft.x(), destinationLines == 0 ? destinationStartY + 2 : destinationEndY);
        polygon << m_connectWidget->mapFrom(this, m_destinationText->mapTo(this, point));

        point = QPoint(sourceLowerRight.x(), sourceLines == 0 ? sourceStartY + 2 : sourceEndY);
        polygon << m_connectWidget->mapFrom(this, m_sourceText->mapTo(this, point));

        point = QPoint(sourceUpperLeft.x() + 2, sourceLines == 0 ? sourceStartY + 2 : sourceEndY);
        polygon << m_connectWidget->mapFrom(this, m_sourceText->mapTo(this, point));

        point = QPoint(sourceUpperLeft.x() + 2, sourceStartY);
        polygon << m_connectWidget->mapFrom(this, m_sourceText->mapTo(this, point));

        res->append(polygon);

        if(difference == activeDiff()) {
            m_activeDiff = res->length() - 1;
        }

        i = difference->sourceLineNumber() - 1 + difference->sourceLineCount();

    }

    return res;
}


FileView::~FileView() {
    m_sourceRanges->clear();
    delete m_sourceRanges;
    m_sourceRanges = nullptr;
    m_destinationRanges->clear();
    delete m_destinationRanges;
    m_destinationRanges = nullptr;
}

void FileView::editKeys() {
    ShortcutsDialog dlg(m_destinationText, this, KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, m_mainWindow);
    dlg.configure(false);
}

View *FileView::widestView() {
    return lineWidth(m_sourceText, longestVisibleLine(m_sourceText)) - m_sourceText->textAreaRect().width() >
           lineWidth(m_destinationText, longestVisibleLine(m_destinationText)) - m_destinationText->textAreaRect().width() ? m_sourceText
                                                                                                        : m_destinationText;
}

View *FileView::longestView() {
    return m_sourceText->document()->lines() > m_destinationText->document()->lines() ? m_sourceText : m_destinationText;
}

void FileView::verticalScrollPositionChanged(View *view, const Cursor &newPos) {
    if (m_syncingWidgets || m_modelUpdating)
        return;

    m_changingPositionView = view;

    int line = newPos.line();
    if (line == 0) {
        // scroll to zero no matter what
        m_lineScroll->setSliderPosition(0);
        m_diffScrollBar->setSliderPosition(0);
        return;
    }

    View *longest = longestView();
    if (view != longest) {
        line = middleLine(view);
        if (newPos.line() == view->maxScrollPosition().line()) {
            line = longest->maxScrollPosition().line();
        } else {
            if (view == m_sourceText) {
                int destinationLine = m_fileNavigatorItem->destinationLineFromSource(line);
                if(destinationLine == -1) {
                    // equal files
                    destinationLine = line;
                }
                line = middleToFirstLine(m_destinationText, destinationLine);
            } else {
                int sourceLine = m_fileNavigatorItem->sourceLineFromDestination(line);
                if(sourceLine == -1) {
                    // equal files
                    sourceLine = line;
                }
                line = middleToFirstLine(m_sourceText, sourceLine);
            }
        }
    }

    m_lineScroll->setSliderPosition(line);

    m_changingPositionView = nullptr;

    emit polygonsUpdated();

}

void FileView::cursorPositionChanged(KTextEditor::View *view, const KTextEditor::Cursor &newPos) {
    if (m_syncingWidgets || m_modelUpdating)
        return;


    m_syncingWidgets = true;
    m_changingPositionView = view;

    Difference *diff = nullptr;

    int lineForScrolling = newPos.line();
    if (view == m_sourceText) {
        int line = m_fileNavigatorItem->destinationLineFromSource(newPos.line());
        if(line == -1) {
            // equal files
            line = newPos.line();
        }
        alignView(m_destinationText, line, newPos.line() - m_sourceText->firstDisplayedLine());
        diff = m_fileNavigatorItem->diffContainingSourceLine(newPos.line(), true);
        bool missing = diff != nullptr && diff->destinationLineCount() == 0;
        m_destinationText->setCursorPosition(Cursor(line, missing ? 0 : newPos.column()));
    } else {
        int line = m_fileNavigatorItem->sourceLineFromDestination(newPos.line());
        if(line == -1) {
            // equal files
            line = newPos.line();
        }
        alignView(m_sourceText, line, newPos.line() - m_destinationText->firstDisplayedLine());
        diff = m_fileNavigatorItem->diffContainingDestinationLine(newPos.line(), true);
        bool missing = diff != nullptr && diff->sourceLineCount() == 0;
        m_sourceText->setCursorPosition(Cursor(line, missing ? 0 : newPos.column()));
    }

    m_changingPositionView = nullptr;
    m_syncingWidgets = false;

    // horizontal position

    int firstVisibleColumn = 0;
    if (!lineFullyVisible(view, lineForScrolling)) {
        for (int i = 0; i < view->document()->lineLength(lineForScrolling); ++i) {
            if (view->cursorToCoordinate(Cursor(lineForScrolling, i)).x() >= view->textAreaRect().left()) {
                firstVisibleColumn = rangeLength(view, lineForScrolling, 0, i);
                break;
            }

        }
    }
    calcMaxScrollColumn();
    m_columnScroll->setSliderPosition(firstVisibleColumn);

    emit polygonsUpdated();

    updateActions();
}

int FileView::middleLine(View *view) {
    int res = (view->lastDisplayedLine() - view->firstDisplayedLine()) / 2 + view->firstDisplayedLine();
    return res;
}

int FileView::middleToFirstLine(View *view, int middle) {
    int res = middle - (view->lastDisplayedLine() - view->firstDisplayedLine()) / 2;
    if(res < 0) {
        res = middle;
    }
    return res;
}

void FileView::lineScrollMoved(int value) {
    if (m_syncingWidgets || m_modelUpdating)
        return;

    m_syncingWidgets = true;

    // sync scrollbar with diff map
    if(m_lineScroll->sliderPosition() != value) {
        m_lineScroll->setSliderPosition(value);
    } else if(m_diffScrollBar->sliderPosition() != value) {
        m_diffScrollBar->setSliderPosition(value);
    }
    Cursor sourceCursor;
    Cursor destinationCursor;
    if (longestView() == m_sourceText) {
        if (m_changingPositionView != m_sourceText) {
            sourceCursor = Cursor(value, 0);
            m_sourceText->setScrollPosition(sourceCursor);
        }
        if (m_changingPositionView != m_destinationText) {
            int middleLineDestination = m_fileNavigatorItem->destinationLineFromSource(middleLine(m_sourceText));
            if(middleLineDestination == -1) {
                // equal files
                middleLineDestination = middleLine(m_sourceText);
            }
            int firstLineDestination = middleLineDestination - (m_destinationText->lastDisplayedLine() - m_destinationText->firstDisplayedLine()) / 2;
            destinationCursor = Cursor(firstLineDestination, 0);
            m_destinationText->setScrollPosition(destinationCursor);
        }
    } else {
        if (m_changingPositionView != m_destinationText) {
            destinationCursor = Cursor(value, 0);
            m_destinationText->setScrollPosition(destinationCursor);
        }
        if (m_changingPositionView != m_sourceText) {
            int middleLineSource = m_fileNavigatorItem->sourceLineFromDestination(middleLine(m_destinationText));
            if(middleLineSource == -1) {
                // equal files
                middleLineSource = middleLine(m_destinationText);
            }
            int firstLineSource = middleLineSource - (m_sourceText->lastDisplayedLine() - m_sourceText->firstDisplayedLine()) / 2;
            sourceCursor = Cursor(firstLineSource, 0);
            m_sourceText->setScrollPosition(sourceCursor);
        }
    }

    calcMaxScrollColumn();
    emit polygonsUpdated();

    m_syncingWidgets = false;
}

void FileView::columnScrollMoved(int value) {
    if (m_syncingWidgets)
        return;

    m_syncingWidgets = true;

    int longestVisible = longestVisibleLine(m_sourceText);
    int w = lineWidth(m_sourceText, longestVisible);
    if (!lineFullyVisible(m_sourceText, longestVisible)) {
        m_sourceText->setHorizontalScrollPosition(value < w ? value : w);
    } else {
        m_sourceText->setHorizontalScrollPosition(0);
    }

    longestVisible = longestVisibleLine(m_destinationText);
    w = lineWidth(m_destinationText, longestVisible);
    if (!lineFullyVisible(m_destinationText, longestVisible)) {
        m_destinationText->setHorizontalScrollPosition(value < w ? value : w);
    } else {
        m_destinationText->setHorizontalScrollPosition(0);
    }

    m_syncingWidgets = false;
}

int FileView::longestVisibleLine(View *view) {
    int longestLineLength = 0;
    int longestLine = 0;
    for (int i = view->firstDisplayedLine(); i <= view->lastDisplayedLine(); ++i) {
        if (view->document()->lineLength(i) > longestLineLength) {
            longestLineLength = view->document()->lineLength(i);
            longestLine = i;
        }
    }

    return longestLine;
}

bool FileView::lineFullyVisible(View *view, int line) {
    int lineLength = view->document()->lineLength(line);

    QPoint startLine = view->cursorToCoordinate(Cursor(line, 0));
    QPoint endLine = view->cursorToCoordinate(Cursor(line, lineLength));

    return view->textAreaRect().left() <= startLine.x() && view->textAreaRect().right() >= endLine.x();

}

int FileView::rangeLength(View *view, int line, int from, int to) {
    QPoint startColumn = view->cursorToCoordinate(Cursor(line, from));
    QPoint endColumn = view->cursorToCoordinate(Cursor(line, to));
    return endColumn.x() - startColumn.x();
}

int FileView::lineWidth(View *view, int line) {
    int lineLength = view->document()->lineLength(line);

    QPoint startColumn = view->cursorToCoordinate(Cursor(line, 0));
    QPoint endColumn = view->cursorToCoordinate(Cursor(line, lineLength));

    return endColumn.x() - startColumn.x() + view->fontMetrics().width(QLatin1Char(' '));
}

void FileView::calcMaxScrollLine() {
    int maxScrollLine = std::max(m_sourceText->maxScrollPosition().line(), m_destinationText->maxScrollPosition().line());

    m_lineScroll->setVisible(maxScrollLine > 1);
    m_diffScrollBar->setVisible(maxScrollLine > 1);
    m_lineScroll->setMaximum(maxScrollLine);
    m_diffScrollBar->setMaximum(maxScrollLine);
}

void FileView::calcMaxScrollColumn() {
    View *view = widestView();
    int pageStep = lineWidth(view, longestVisibleLine(view));
    int singleStep = view->fontMetrics().averageCharWidth();

    int maximum = pageStep - view->textAreaRect().width();
    if (maximum < 0)
        maximum = 0;

    m_columnScroll->setVisible(maximum > 0);
    m_columnScroll->setSingleStep(singleStep);
    m_columnScroll->setPageStep(pageStep);
    m_columnScroll->setMaximum(maximum);
}

void FileView::alignView(View *view, int documentLine, int visibleLine) {
    int start = documentLine - visibleLine;
    if (start < 0)
        start = 0;

    if(view->firstDisplayedLine() != start) {
        Cursor c = Cursor(start, 0);
        view->setScrollPosition(c);
    }

    if (view == longestView()) {
        if(m_lineScroll->sliderPosition() != start)
            m_lineScroll->setSliderPosition(start);
        if(m_diffScrollBar->sliderPosition() != start)
            m_diffScrollBar->setSliderPosition(start);
    }
}

void FileView::reset() {
    if(m_fileNavigatorItem) {
        disconnect(this, 0, m_fileNavigatorItem->model(), 0);
        disconnect(m_fileNavigatorItem->model(), 0, this, 0);
        m_fileNavigatorItem = nullptr;
    }

    m_reload->setEnabled(false);
    m_applyDiff->setEnabled(false);
    m_nextDiff->setEnabled(false);
    m_previousDiff->setEnabled(false);
    m_applyAllDiff->setEnabled(false);

    emit polygonsUpdated();
    m_destinationText->document()->setMode("Normal");
    update();
}

int FileView::visibleEnd() {
    return longestView() == m_sourceText ? m_sourceText->lastDisplayedLine() : m_destinationText->lastDisplayedLine();
}

int FileView::visibleStart() {
    return longestView() == m_sourceText ? m_sourceText->firstDisplayedLine() : m_destinationText->firstDisplayedLine();
}

void FileView::gotoPreviousSlot() {
    gotoPrevious(false);
}

void FileView::gotoPrevious(bool stayInSameFile) {
    View *view = longestView();
    bool isSourceView = view == m_sourceText;
    bool found = false;
    Cursor pos = view->cursorPosition();
    //const Difference* currentDiff = isSourceView? m_fileNavigatorItem->diffContainingSourceLine(pos.line(), true) : m_fileNavigatorItem->diffContainingDestinationLine(pos.line(), true);
    for (int i = pos.line()-1; i >= 0; --i) {
        const Difference* diff = isSourceView? m_fileNavigatorItem->diffContainingSourceLine(i, true) : m_fileNavigatorItem->diffContainingDestinationLine(i, true);
        if(diff != nullptr && diff != activeDiff()) {
            int linesCount = isSourceView? diff->sourceLineCount() : diff->destinationLineCount();
            if(linesCount> 0) {
                int start = isSourceView ? diff->sourceLineNumber() - 1 : diff->destinationLineNumber() - 1;
                alignView(view, start, middleLine(view) + 1 - view->firstDisplayedLine() - linesCount/ 2);
                view->setCursorPosition(Cursor(start, 0));
            } else {
                // use the other view
                linesCount = isSourceView? diff->destinationLineCount() : diff->sourceLineCount();
                int start = isSourceView ? diff->destinationLineNumber() - 1 : diff->sourceLineNumber() - 1;
                View* v = isSourceView ? m_destinationText : m_sourceText;
                alignView(v, start, middleLine(v) + 1 - v->firstDisplayedLine()- linesCount/ 2);
                v->setCursorPosition(Cursor(start, 0));
            }
            found = true;
            break;
        }
    }
    if(!found && !stayInSameFile && m_settings->autoAdvance()) {
        m_selectDiff = Last;
        if(m_destinationText->document()->closeUrl()) {
            emit previousFile(true, m_fileNavigatorItem->key());
        }
    }
}

void FileView::gotoNextSlot() {
    gotoNext(false);
}

void FileView::gotoNext(bool stayInSameFile) {
    View *view = longestView();
    bool found = false;
    bool isSourceView = view == m_sourceText;
    Cursor pos = view->cursorPosition();
    const Difference* currentDiff = activeDiff();//isSourceView? m_fileNavigatorItem->diffContainingSourceLine(pos.line(), true) : m_fileNavigatorItem->diffContainingDestinationLine(pos.line(), true);
    for (int i = pos.line(); i < view->document()->lines(); ++i) {
        const Difference* diff = isSourceView? m_fileNavigatorItem->diffContainingSourceLine(i, true) : m_fileNavigatorItem->diffContainingDestinationLine(i, true);
        if(diff != nullptr && diff != currentDiff) {
            int linesCount = isSourceView? diff->sourceLineCount() : diff->destinationLineCount();
            if(linesCount> 0) {
                int start = isSourceView ? diff->sourceLineNumber() - 1 : diff->destinationLineNumber() - 1;
                alignView(view, start, middleLine(view) + 1 - view->firstDisplayedLine() - linesCount/ 2);
                view->setCursorPosition(Cursor(start, 0));
            } else {
                // use the other view
                linesCount = isSourceView? diff->destinationLineCount() : diff->sourceLineCount();
                int start = isSourceView ? diff->destinationLineNumber() - 1 : diff->sourceLineNumber() - 1;
                View* v = isSourceView ? m_destinationText : m_sourceText;
                alignView(v, start, middleLine(v) + 1 - v->firstDisplayedLine() - linesCount/ 2);
                v->setCursorPosition(Cursor(start, 0));
            }
            found = true;
            break;
        }
    }
    if(!found && !stayInSameFile && m_settings->autoAdvance()) {
        m_selectDiff = First;
        QString key = m_fileNavigatorItem->key();
        if(m_settings->autoSave() && m_applyingDiff && !m_fileNavigatorItem->isDifferent() && !m_fileNavigatorItem->isDir()) {
            save();
            m_applyingDiff = false;
            emit resyncModelWithDisk();
            emit nextFile(true, key);
        } else if(!m_applyingDiff) {
            if(m_destinationText->document()->closeUrl()) {
                emit nextFile(true, key);
            }
        }
    }

}

const Difference* FileView::activeDiff() const {
    if(m_modelUpdating)
        return nullptr;

    const Difference *diff = m_fileNavigatorItem->diffContainingSourceLine(m_sourceText->cursorPosition().line(), true);

    if(diff == nullptr) {
        diff = m_fileNavigatorItem->diffContainingDestinationLine(m_destinationText->cursorPosition().line(), true);
    }
    return diff;
}

void FileView::applyDiff() {
    if(m_applyDiff->property("applyAll").toBool()) {
        m_applyAllDiff->trigger();
        return;
    }
    const Difference* diff = activeDiff();
    if(!diff)
        return;

    m_mainWindow->createBackup(m_fileNavigatorItem->model()->destinationBaseUrl());

    updatingModel();

    Cursor sourceStart = Cursor(diff->sourceLineNumber() - 1, 0);
    bool isLastLine = diff->sourceLineNumber() + diff->sourceLineCount() >= m_sourceText->document()->lines();
    Cursor sourceEnd = Cursor(
            isLastLine ? m_sourceText->document()->lines() - 1 : diff->sourceLineNumber() + diff->sourceLineCount(),
            isLastLine ? m_sourceText->document()->lineLength(m_sourceText->document()->lines() - 1) : 0);
    Cursor destinationStart = Cursor(diff->destinationLineNumber() - 1, 0);
    isLastLine = diff->destinationLineNumber() + diff->destinationLineCount() >= m_destinationText->document()->lines();
    Cursor destinationEnd = Cursor(isLastLine ? m_destinationText->document()->lines() - 1 : diff->destinationLineNumber() +
                                                                                             diff->destinationLineCount(),
                                   isLastLine ? m_destinationText->document()->lineLength(
                                           m_destinationText->document()->lines() - 1) : 0);
    Range sourceRange = Range(sourceStart, sourceEnd);
    Range destinationRange = Range(destinationStart, destinationEnd);
    QString sourceText = m_sourceText->document()->text(sourceRange);

    m_applyingDiff = true;
    m_destinationText->document()->replaceText(destinationRange, sourceText);
}

void FileView::applyAllDiffs() {
    m_mainWindow->createBackup(m_fileNavigatorItem->model()->destinationBaseUrl());
    if(m_fileNavigatorItem->isDir() && m_fileNavigatorItem->destinationMissing()) {
        QUrl source = QUrl::fromLocalFile(m_fileNavigatorItem->localSource());
        QStringList localDestinationParts = m_fileNavigatorItem->localDestination().split(QDir::separator(), QString::SkipEmptyParts);
        QUrl localDestination = QUrl::fromLocalFile(m_fileNavigatorItem->localDestination());
        localDestination.setPath(StringUtils::rightStrip(localDestination.path(), localDestinationParts[localDestinationParts.size() - 1]));

        if(!FileUtils::makePath(localDestination))
            return;

        if(!FileUtils::copyDir(source, localDestination)) {
            return;
        }

        // checking source here as destination doesn't exists yet
        if(m_fileNavigatorItem->sourceUrl().isValid() && !m_fileNavigatorItem->sourceUrl().isLocalFile()) {
            QUrl source = m_fileNavigatorItem->sourceUrl();
            QUrl destination = QUrl(m_fileNavigatorItem->model()->destinationBaseUrl());
            destination.setPath(destination.path() + m_fileNavigatorItem->key());
            destination.setPath(StringUtils::rightStrip(destination.path(), localDestinationParts[localDestinationParts.size() - 1]));
            if(!FileUtils::makePath(destination)) {
                return;
            }

            if(!FileUtils::copyDir(source, destination)) {
                return;
            }
        }
        emit createMissingFolders(m_fileNavigatorItem->key());
        emit nextFile(true, m_fileNavigatorItem->key());
    } else if(m_fileNavigatorItem->isDir() && m_fileNavigatorItem->sourceMissing()) {
        QDir dir(m_fileNavigatorItem->localDestination());
        dir.removeRecursively();
        if(m_fileNavigatorItem->destinationUrl().isValid() && !m_fileNavigatorItem->destinationUrl().isLocalFile()) {
            FileUtils::deleteFileOrDir(m_fileNavigatorItem->destinationUrl());
        }
        emit deleteItem(m_fileNavigatorItem->key(), true);
    } else if(m_fileNavigatorItem->sourceMissing()) {
        QString destinationFileName = m_destinationText->document()->url().toLocalFile();
        if(!m_destinationText->document()->closeUrl()) {
            return;
        }
        QFile::remove(destinationFileName);
        if(m_fileNavigatorItem->destinationUrl().isValid() && !m_fileNavigatorItem->destinationUrl().isLocalFile()) {
            FileUtils::deleteFileOrDir(m_fileNavigatorItem->destinationUrl());
        }

        int levelsUp = levelsOfEmptyParents(m_fileNavigatorItem->key(), m_fileNavigatorItem->model()->source(), m_fileNavigatorItem->model()->destination());
        if(levelsUp > 0) {
            QString path = m_fileNavigatorItem->model()->destination();
            QString commonPart = StringUtils::rightStrip(m_fileNavigatorItem->model()->destination(), m_fileNavigatorItem->key());
            QUrl remotePath = QUrl(m_fileNavigatorItem->destinationUrl());
            QString remotePathString = remotePath.path();
            for (int i = 0; i < levelsUp; ++i) {
                path = path.mid(0, path.lastIndexOf(QDir::separator()));
                if(path.endsWith(QDir::separator())) {
                    path = path.mid(0, path.size() - 1);
                }

                remotePathString = remotePathString.mid(0, remotePathString.lastIndexOf(QDir::separator()));
                if(remotePathString.endsWith(QDir::separator())) {
                    remotePathString = remotePathString.mid(0, remotePathString.size() - 1);
                }
            }
            const QString str = QString(remotePathString);
            remotePath.setPath(str);

            path = StringUtils::leftStrip(path, commonPart);

            QDir dir(path);
            dir.removeRecursively();
            if(remotePath.isValid() && !remotePath.isLocalFile()) {
                FileUtils::deleteFileOrDir(remotePath);
            }
            emit deleteItem(path, true);
        } else {
            emit deleteItem(m_fileNavigatorItem->key(), false);
        }
    } else {
        m_applyingDiff = true;
        m_destinationText->document()->setText(m_sourceText->document()->text());
    }
}

bool FileView::dirHasFiles(QDir dir) {
    QDirIterator sourceIt(dir.path(),
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks,
            QDirIterator::Subdirectories);
    while (sourceIt.hasNext()) {
        QString filename = sourceIt.next();
        QFileInfo fi = sourceIt.fileInfo();
        if(!fi.isDir()) {
            return true;
        }
    }

    return false;
}

int FileView::levelsOfEmptyParents(QString key, QString sourcePath, QString destinationPath) {
    int sourceLevels = levelsOfEmptyParents(sourcePath, key);
    int destinationLevels = levelsOfEmptyParents(destinationPath, key);
    return std::min(sourceLevels, destinationLevels);
}

int FileView::levelsOfEmptyParents(QString path, QString key) {
    if(!m_settings->removeEmptyFolders())
        return 0;

    QString commonPart = StringUtils::rightStrip(path, key);
    QStringList keyParts = key.split(QDir::separator(), QString::SkipEmptyParts);

    QDir dir(commonPart);
    int i=0;
    while(dirHasFiles(dir) && i < keyParts.size() && dir.path() != m_fileNavigatorItem->model()->destinationPath()) {
        dir.cd(keyParts[i]);
        i++;
    }

    return keyParts.size() - i;
}

void FileView::textChanged() {
    emit textChanged(m_destinationText->document()->text());
}

void FileView::modifiedChanged() {
    m_save->setEnabled(m_destinationText->document()->isModified());
}
void FileView::updatingModel() {
    m_modelUpdating = true;
}

void FileView::modelChanged() {
    m_modelUpdating = false;
    if(m_applyingDiff) {
        if(!activeDiff()) {
            gotoNext(false);
        }
        m_applyingDiff = false;
    } else {
        cursorPositionChanged(m_destinationText, m_destinationText->cursorPosition());
    }
    m_diffScrollBar->update(m_fileNavigatorItem, m_sourceText->document()->lines(), m_destinationText->document()->lines());
    highlightInlineDiffs();
    highlightMissingRanges(m_fileNavigatorItem->missingRanges());
    emit polygonsUpdated();
    updateActions();
}

void FileView::initializeGUI() {
    // HACK: force katepart to read any modified shortcut
    m_mainWindow->factory()->addClient(m_sourceText);
    m_mainWindow->factory()->removeClient(m_sourceText);
    m_mainWindow->factory()->addClient(m_destinationText);
    m_mainWindow->factory()->removeClient(m_destinationText);
    QStringList mappedActions = Settings::mappedToMainWindowActions();
    for (int i = 0; i < mappedActions.length(); ++i) {
        m_mainWindow->actionCollection()->addAction(mappedActions.at(i), m_destinationText->action(mappedActions.at(i).toLatin1().data()));
    }
}

void FileView::reloadShortcuts() {
    m_sourceText->actionCollection()->readSettings();
    m_destinationText->actionCollection()->readSettings();
}

void FileView::reloadFiles() {
    if(!m_destinationText->document()->isModified())
        return;

    m_selectDiff = None;

    View *focusedView = m_sourceText->hasFocus() ? m_sourceText : m_destinationText;
    Cursor cursorPosition = focusedView->cursorPosition();
    int scrollLine = m_lineScroll->sliderPosition();
    int scrollColumn = m_columnScroll->sliderPosition();

    m_destinationText->document()->documentReload();

    focusedView->setCursorPosition(cursorPosition);
    m_lineScroll->setSliderPosition(scrollLine);
    m_columnScroll->setSliderPosition(scrollColumn);
}

void FileView::saveSlot() {
    save();
}

void FileView::save() {
    QString dirName = StringUtils::rightStrip(m_destinationText->document()->url().path(), m_destinationText->document()->url().fileName());
    QDir dir = QDir(dirName);
    if(!dir.exists()) {
        QDir root = QDir();
        root.mkpath(dir.path());
        emit createMissingFolders(dir.path());
    }
    m_destinationText->document()->save();

    if(m_fileNavigatorItem->destinationUrl().isValid() && !m_fileNavigatorItem->destinationUrl().isLocalFile()) {
        QUrl destinationFile(m_fileNavigatorItem->destinationUrl());
        QUrl destinationPathUrl(m_fileNavigatorItem->destinationUrl());
        destinationPathUrl.setPath(StringUtils::rightStrip(destinationPathUrl.path(), m_destinationText->document()->url().fileName()));
        if(!FileUtils::makePath(destinationPathUrl))
            return;

        if(!FileUtils::copyFile(m_destinationText->document()->url(), destinationFile)) {
            return;
        }
    }
}

void FileView::updateActions() {
    if(m_modelUpdating || !m_fileNavigatorItem) {
        m_applyAllDiff->setEnabled(false);
        m_previousDiff->setEnabled(false);
        m_nextDiff->setEnabled(false);
        m_applyDiff->setEnabled(false);
        m_applyAllDiff->setEnabled(false);
        m_save->setEnabled(false);
        m_reload->setEnabled(false);

        return;
    }

    Cursor sourceCursor = m_sourceText->cursorPosition();
    Cursor destinationCursor = m_destinationText->cursorPosition();

    m_applyAllDiff->setEnabled(m_fileNavigatorItem->isDifferent() && m_fileNavigatorItem->isReadWrite());

    m_applyDiff->setProperty("applyAll", false);


    if(m_fileNavigatorItem->sourceMissing()) {
        if (m_fileNavigatorItem->isDir()) {
            m_applyDiff->setEnabled(m_fileNavigatorItem->isReadWrite());
            m_applyDiff->setProperty("applyAll", true);
            m_applyDiff->setToolTip(i18n("Delete selected folder and all of its contents"));
            m_applyAllDiff->setToolTip(i18n("Delete selected folder and all of its contents"));
        } else {
            m_applyDiff->setEnabled(m_fileNavigatorItem->isReadWrite());
            m_applyDiff->setProperty("applyAll", true);
            m_applyDiff->setToolTip(i18n("Delete selected file"));
            m_applyAllDiff->setToolTip(i18n("Delete selected file"));
        }
    } else if (!m_fileNavigatorItem->isDir() && !m_fileNavigatorItem->destinationExists()) {
        m_applyDiff->setEnabled(m_fileNavigatorItem->isReadWrite());
        m_applyDiff->setProperty("applyAll", true);
        m_applyDiff->setToolTip(i18n("Create missing file"));
        m_applyAllDiff->setToolTip(i18n("Create missing file"));
    } else if (m_fileNavigatorItem->isDir() && !m_fileNavigatorItem->destinationExists()) {
        m_applyDiff->setEnabled(m_fileNavigatorItem->isReadWrite());
        m_applyDiff->setProperty("applyAll", true);
        m_applyDiff->setToolTip(i18n("Create current folder and all of its contents"));
        m_applyAllDiff->setText(i18n("&Create Folder"));
        m_applyAllDiff->setToolTip(i18n("Create current folder and all of its contents"));
    } else {
        m_applyDiff->setEnabled(m_applyAllDiff->isEnabled() && activeDiff());
        m_applyDiff->setToolTip(i18n("Apply current difference"));
        m_applyAllDiff->setToolTip(i18n("Apply all differences"));
        m_applyAllDiff->setText(i18n("&Apply All Differences"));
        m_applyAllDiff->setIcon(QIcon::fromTheme("arrow-right-double"));
    }


    bool hasPreviousDiff = false;
    bool hasNextDiff = false;

    for (int i = 0; i < m_sourceText->document()->lines(); ++i) {
        const Difference* diff = m_fileNavigatorItem->diffContainingSourceLine(i, true);
        if(diff != nullptr) {
            if(diff->sourceLineNumber()+ diff->sourceLineCount() <= sourceCursor.line()) {
                hasPreviousDiff = true;
            } else if(diff->sourceLineNumber() - 1 > sourceCursor.line()) {
                hasNextDiff = true;
                break;
            }
        }
    }
    if(!hasPreviousDiff || !hasNextDiff) {
        for (int i = 0; i < m_destinationText->document()->lines(); ++i) {
            const Difference* diff = m_fileNavigatorItem->diffContainingDestinationLine(i, true);
            if (diff != nullptr) {
                if (diff->destinationLineNumber()+ diff->destinationLineCount() <= destinationCursor.line()) {
                    hasPreviousDiff = true;
                } else if (diff->destinationLineNumber() - 1 > destinationCursor.line()) {
                    hasNextDiff = true;
                    break;
                }
            }
        }
    }

    m_previousDiff->setEnabled(hasPreviousDiff || (m_settings->autoAdvance() && m_fileNavigatorItem && m_fileNavigatorItem->hasPrevious()));
    m_nextDiff->setEnabled(hasNextDiff || (m_settings->autoAdvance() && m_fileNavigatorItem && m_fileNavigatorItem->hasNext()));
    m_reload->setEnabled(m_fileNavigatorItem->isReadWrite());
}

void FileView::focusOut(KTextEditor::View* view) {
    if (view->isEnabled()) {
        view->removeSelection();
    }

}

void FileView::currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *) {
    if(!current || m_fileNavigatorItem == current) {
        if(!current) {
            if(closeDocuments(false)) {
                m_diffScrollBar->update(nullptr, 0, 0);
                if(m_fileNavigatorItem != nullptr) {
                    disconnect(this, 0, m_fileNavigatorItem, 0);
                    disconnect(m_fileNavigatorItem, 0, this, 0);
                }
            } else {
                emit selectItem(m_fileNavigatorItem);

            }
        }
        return;
    }

    if(m_fileNavigatorItem != nullptr) {
        disconnect(this, 0, m_fileNavigatorItem->model(), 0);
        disconnect(m_fileNavigatorItem->model(), 0, this, 0);
//        m_fileNavigatorItem->modified(false);
    }

    if(!current) {
        if(!closeDocuments(false)) {
            emit selectItem(m_fileNavigatorItem);
        }
        return;
    }

    m_fileNavigatorItem = (FileNavigatorItem*) current;
    m_modelUpdating = false;
    connect(this, SIGNAL(textChanged(const QString)), m_fileNavigatorItem->model(), SLOT(textChanged(const QString)));
    connect(this, SIGNAL(resyncModelWithDisk()), m_fileNavigatorItem->model(), SLOT(resyncModelWithDisk()));
    connect(m_fileNavigatorItem->model(), SIGNAL(modelChanged()), this, SLOT(modelChanged()));

    setUpdatesEnabled(false);
    m_destinationText->document()->setMode("Normal");

    if(m_fileNavigatorItem->model()->isRenamed() && !m_fileNavigatorItem->model()->sourceUrl().isValid() && !m_fileNavigatorItem->model()->destinationUrl().isValid()) {
        m_sourceText->document()->closeUrl();
        m_sourceText->setEnabled(false);
        m_sourceText->document()->setReadWrite(false);
        m_destinationText->document()->closeUrl();
        m_destinationText->setEnabled(false);
        m_destinationText->document()->setReadWrite(false);
    } else {
        // open destination first
        if(m_fileNavigatorItem->isDir()) {
            m_destinationText->document()->closeUrl();
            if(m_fileNavigatorItem->isDir()) {
                m_destinationText->setEnabled(false);
            }
        } else {
            m_destinationText->document()->openUrl(QUrl::fromLocalFile(m_fileNavigatorItem->localDestination()));
            m_destinationText->setEnabled(true);
            m_destinationText->document()->setReadWrite(false);
        }

        if(!m_fileNavigatorItem->sourceExists() || m_fileNavigatorItem->isDir()) {
            m_sourceText->document()->closeUrl();
            if(m_fileNavigatorItem->isDir()) {
                m_sourceText->setEnabled(false);
            }
        } else {
            m_sourceText->document()->openUrl(QUrl::fromLocalFile(m_fileNavigatorItem->localSource()));
            m_sourceText->setEnabled(true);
            m_sourceText->document()->setReadWrite(false);
            m_destinationText->document()->setEncoding(m_sourceText->document()->encoding());
        }

        m_destinationText->document()->setReadWrite(m_fileNavigatorItem->isReadWrite());
    }

    setUpdatesEnabled(true);
    m_syncingWidgets = false;
    m_lineScroll->setValue(0);
    m_diffScrollBar->setValue(0);
    calcMaxScrollLine();
    m_columnScroll->setValue(0);
    calcMaxScrollColumn();

    modelChanged();

    updateActions();
    if(m_selectDiff == First) {
        Cursor pos(0, 0);
        if(m_fileNavigatorItem->destinationMissing()) {
            m_sourceText->setCursorPosition(pos);
        } else {
            m_destinationText->setCursorPosition(pos);
        }
        if(!activeDiff()) {
            gotoNext(true);
        }
    } else if(m_selectDiff == Last) {
        Cursor pos(m_destinationText->document()->lines() - 1, 0);
        m_destinationText->setCursorPosition(pos);
        if(!activeDiff()) {
            gotoPrevious(true);
        }
    }
    m_selectDiff = First;
}

bool FileView::eventFilter(QObject* object, QEvent* event) {
    if (event->type() == QEvent::KeyPress && object->inherits("FileNavigator")) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        FileNavigator* fn = static_cast<FileNavigator*>(object);
        if((ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Home || ke->key() == Qt::Key_PageUp) && fn->hasPrevious()) {
            bool status = !m_destinationText->document()->closeUrl();
            return status;
        } else if((ke->key() == Qt::Key_Down || ke->key() == Qt::Key_End  || ke->key() == Qt::Key_PageDown) && fn->hasNext()) {
            bool status = !m_destinationText->document()->closeUrl();
            return status;
        }
    } else if (event->type() == QEvent::MouseButtonPress && !object->inherits("FileNavigator")) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        QWidget* viewport = static_cast<QWidget*>(object);
        FileNavigator* fn = (FileNavigator*) viewport->parentWidget();
        FileNavigatorItem* item = (FileNavigatorItem*)fn->itemAt(viewport->mapFromGlobal(me->globalPos()));
        if(item && m_fileNavigatorItem->key() != item->key()) {
            bool status = !m_destinationText->document()->closeUrl();
            return status;
        }
    }


    return false;
}

void FileView::aboutToClose(KTextEditor::Document* doc) {
    if(m_fileNavigatorItem && m_fileNavigatorItem->model()->isReadWrite() && doc->url().path().endsWith(m_fileNavigatorItem->key())) {
        emit resyncModelWithDisk();
    }
}