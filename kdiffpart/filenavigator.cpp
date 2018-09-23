//
// Created by john on 3/21/17.
//

#include <QItemDelegate>
#include <QHeaderView>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDirIterator>
#include <QList>
#include <QSaveFile>

#include <KLocalizedString>
#include "info.h"
#include <settings.h>
#include <QtWidgets/QStyledItemDelegate>

#include "filenavigator.h"
#include "filenavigatoritem.h"
#include "settings.h"
#include "utils.h"

class FileItemDelegate : public QItemDelegate {
public:
    FileItemDelegate(QObject *parent = Q_NULLPTR) : QItemDelegate(parent) {};

    virtual void paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const override {
        if ((option.state & QStyle::State_Selected) != 0) {
            QStyleOptionViewItem viewOption(option);
            QColor foregroundColor = index.data(Qt::ForegroundRole).value<QColor>();
            viewOption.palette.setColor(QPalette::HighlightedText, foregroundColor);
            QItemDelegate::paint(painter, viewOption, index);
            return;
        }
        QItemDelegate::paint(painter, option, index);
    }
};


FileNavigator::FileNavigator(QSplitter *parent)
        : QTreeWidget(parent)
        , m_parent(parent)
        , m_info(nullptr)
        , m_files(new QMap<QString, FileNavigatorItem*>())
        , m_settings(Settings::instance())
        , m_tmpDir(nullptr)
        , m_missingRanges(new QMap<QString, MissingRanges*>())
        , m_event(nullptr)
        , m_initialized(false)
{
    setHeaderLabels(QStringList() << i18n("Source: ") << i18n("Destination: "));
    header()->setSectionResizeMode(0, QHeaderView::Fixed);
    header()->setSectionResizeMode(1, QHeaderView::Fixed);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAutoScroll(true);
    setRootIsDecorated(false);
    setItemDelegate(new FileItemDelegate(this));
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setAllColumnsShowFocus(true);

    // TODO: check also excludeFilesFile
    QStringList excludePatterns = m_settings->diffSettings()->m_excludeFilePatternList;
    for (int i = 0; i < excludePatterns.size(); ++i) {
        QRegExp rx(excludePatterns.at(i));
        rx.setPatternSyntax(QRegExp::Wildcard);
        m_excludeFiles << rx;
    }

    header()->resizeSection(0, width() / 2);
    header()->resizeSection(1, width() / 2);

    setFixedHeight(header()->height());

    setFocusPolicy(Qt::NoFocus);
    connect(m_settings, SIGNAL(navigationViewChanged()), this, SLOT(updateItemStates()));
    connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            this, SLOT(ensureSelectedAndVisible(const QItemSelection &, const QItemSelection &)));

}

void FileNavigator::ensureSelectedAndVisible(const QItemSelection& current, const QItemSelection&) {
    FileNavigatorItem *selectedItem = (FileNavigatorItem *) itemFromIndex(currentIndex());
    if(current.indexes().size() == 0) {
        selectItem(selectedItem);
    }
    scrollToItem(selectedItem, QAbstractItemView::EnsureVisible);
}

FileNavigator::~FileNavigator() {
    delete m_tmpDir;
    if (m_missingRanges) {
        qDeleteAll(m_missingRanges->begin(), m_missingRanges->end());
        delete m_missingRanges;
        m_missingRanges = nullptr;
    }
}


void FileNavigator::initialize(const DiffModelMap *modelList, Compare::ComparisonInfo *info, QString sourceBaseBath, QString destinationBaseBath) {
    m_sourceBasePath = "";
    m_destinationBasePath = "";
    m_initialized = false;
    clear();
    m_files->clear();

    qDeleteAll(m_missingRanges->begin(), m_missingRanges->end());
    m_missingRanges->clear();

    m_info = info;

    if (modelList == nullptr) {
        setHeaderLabels(QStringList() << i18n("Source: ") << i18n("Destination: "));
        return;
    }

    m_sourceBasePath = sourceBaseBath;
    m_destinationBasePath = destinationBaseBath;

    switch (info->mode) {
        case Compare::ComparingFiles:
        case Compare::ComparingDirs:
            m_diffMode = Files;
            break;
        case Compare::ShowingDiff:
            m_diffMode = Diff;
            break;
        case Compare::BlendingDir:
        case Compare::BlendingFile:
            m_diffMode = DiffAndFiles;
            break;
        default:
            qDebug() << "How did I end up here?";
            return;
    }

    fillTree(modelList);
    if(m_diffMode != Diff) {
        // A diff file may contain files that are currently excluded. We want these to appear
        excludeFiles();
    }

    // In case of Blending with files or folders, an existing file in destination without a model means an unchanged file
    // In case of comparing ?
    // TODO: Do not exclude files not even binary. Just show them without contents?
    if (m_diffMode == DiffAndFiles) {
        QMap<QString, FileNavigatorItem *>::iterator i = m_files->begin();
        while (i != m_files->end()) {
            FileNavigatorItem *item = i.value();
            if (item->model() == nullptr && !item->isDir() && item->text(0).isEmpty()) {
                i = m_files->erase(i);
            } else {
                i++;
            }
        }

        // remove empty folders
        i = m_files->begin();
        while (i != m_files->end()) {
            if (i.value()->isDir() && !containsFiles(i.key())) {
                i = m_files->erase(i);
            } else {
                i++;
            }
        }
    }


    // fill tree wideget and select first visible
    bool selected = false;
    QMap<QString, FileNavigatorItem *>::iterator i = m_files->begin();
    while (i != m_files->end()) {
        addTopLevelItem(i.value());
        i.value()->updateState();
        if (!selected && !i.value()->isHidden()) {
            selected = true;
            selectItem(i.value());
        }
        i++;
    }
    m_initialized = true;
    emit initialized();
}

bool FileNavigator::containsFiles(const QString path) {
    QMap<QString, FileNavigatorItem *>::iterator i = m_files->find(path);

    while (i != m_files->end() && i.key().contains(path)) {
        if (!i.value()->isDir()) {
            return true;
        }
        i++;
    }

    return false;
}

void FileNavigator::excludeFiles() {
    QMap<QString, FileNavigatorItem *>::iterator i = m_files->begin();
    while (i != m_files->end()) {
        FileNavigatorItem *item = i.value();
        QStringList fileParts = (!item->text(0).isEmpty() ? item->text(0) : item->text(1)).split("/");
        QString firstPart = fileParts.at(0);
        QString lastPart = fileParts.at(fileParts.length() - 1);
        bool excluded = false;
        for (int j = 0; j < m_excludeFiles.size(); ++j) {
            QRegExp rx = m_excludeFiles.at(j);
            if (firstPart == rx.pattern() || rx.exactMatch(firstPart) || lastPart == rx.pattern() || rx.exactMatch(lastPart)) {
                excluded = true;
                i = m_files->erase(i);
            }
        }
        if (!excluded)
            i++;
    }
}

void FileNavigator::select(FileNavigatorItem *newItem) {
    selectItem(newItem);
}

void FileNavigator::selectItem(FileNavigatorItem *newItem) {
    selectionModel()->setCurrentIndex(indexFromItem(newItem), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current | QItemSelectionModel::Rows);
    selectionModel()->select(indexFromItem(newItem), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current | QItemSelectionModel::Rows);
}

void FileNavigator::updateItemStates() {
    QMap<QString, FileNavigatorItem*>::iterator i = m_files->begin();
    while (i != m_files->end()) {
        i.value()->updateState();
        i++;
    }

    FileNavigatorItem *selectedItem = (FileNavigatorItem *) currentItem();//itemFromIndex(currentIndex());

    if (selectedItem && selectedItem->isHidden()) {
        QMap<QString, FileNavigatorItem*>::iterator i = m_files->find(selectedItem->key());
        while (i != m_files->end()) {
            if(!i.value()->isHidden()) {
                selectItem(i.value());
                break;
            }
            i++;
        }
    }
    scrollToItem(selectedItem, QAbstractItemView::EnsureVisible);
}

FileNavigatorItem *FileNavigator::findItem(const QString path) const {
    QMap<QString, FileNavigatorItem *>::iterator i = m_files->find(path);

    if (i != m_files->end() && i.key() == path) {
        return i.value();
    }
    return nullptr;
}

void FileNavigator::resize(QList<int> *headerSizes) {
    header()->resizeSection(0, headerSizes->at(0) > 1 ? headerSizes->at(0) - 2 : 0);
    // don't use columnWidth(int column)
    // account for scrollbar
    header()->resizeSection(1, header()->width() - columnWidth(0));
}

void FileNavigator::fillTree(const DiffModelMap *modelList) {
    auto i = modelList->begin();
    for(; i != modelList->end(); ++i) {
        DiffModel* model = i.value();
        if(model->isRenamed()) {
            if(!m_files->contains(model->key()))  {
                FileNavigatorItem* item = new FileNavigatorItem(this, model);
                m_files->insert(item->key(), item);
            }
        } else {
            FileNavigatorItem* item = new FileNavigatorItem(this, model);
            m_files->insert(item->key(), item);
        }

    }

    if (!expanded(modelList)) {
        setFixedHeight(header()->height());
        m_parent->setSizes(QList<int>() << header()->height() << m_parent->height() - header()->height());
        m_parent->handle(0)->setEnabled(false);
        m_parent->handle(0)->setCursor(Qt::ArrowCursor);
        m_parent->handle(1)->setEnabled(false);
        m_parent->handle(1)->setCursor(Qt::ArrowCursor);
        setFocusPolicy(Qt::NoFocus);
    } else {
        setMinimumHeight(header()->height());
        setMaximumHeight(m_parent->height());

        m_parent->setSizes(QList<int>() << m_parent->parentWidget()->height() * .3 << m_parent->parentWidget()->height() * .7);
        m_parent->handle(0)->setEnabled(true);
        m_parent->handle(0)->setCursor(Qt::SizeVerCursor);
        m_parent->handle(1)->setEnabled(true);
        m_parent->handle(1)->setCursor(Qt::SizeVerCursor);

        setFocusPolicy(Qt::WheelFocus);
        setFocus();
    }
};

bool FileNavigator::expanded(const DiffModelMap *modelList) {
    return (exists(m_sourceBasePath) && isDir(m_sourceBasePath) &&
            exists(m_destinationBasePath) && isDir(m_destinationBasePath)) ||
           (modelList && modelList->size() > 1);
}

bool FileNavigator::exists(const QString &url) const {
    QFileInfo fi(url);
    return fi.exists();
};

bool FileNavigator::isDir(const QString &url) const {
    QFileInfo fi(url);
    return fi.isDir();
};

QModelIndex FileNavigator::findFirstVisible() {
    QMap<QString, FileNavigatorItem *>::iterator i = m_files->begin();
    while (i != m_files->end()) {
        if(!i.value()->isHidden()) {
            return indexFromItem(i.value());
        }
        i++;
    }

    return QModelIndex();
}

bool FileNavigator::hasPrevious() const {
    FileNavigatorItem *selectedItem = (FileNavigatorItem *) currentItem();

    if(!selectedItem) {
        return false;
    }

    QMap<QString, FileNavigatorItem *>::iterator i = m_files->find(selectedItem->key());
    while (i != m_files->begin()) {
        --i;
        if(!i.value()->isHidden() && i.key() != selectedItem->key() && i.value()->isDifferent()) {
            return true;
        }
    }

    return false;
}

bool FileNavigator::hasNext() const {
    FileNavigatorItem *selectedItem = (FileNavigatorItem *) currentItem();
    if(!selectedItem) {
        return false;
    }

    QMap<QString, FileNavigatorItem *>::iterator i = m_files->find(selectedItem->key());
    while (i != m_files->end()) {
        if(!i.value()->isHidden() && i.key() != selectedItem->key() && i.value()->isDifferent()) {
            return true;
        }
        i++;
    }

    return false;
}

QModelIndex FileNavigator::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers){
    QModelIndex current = QTreeWidget:: moveCursor(cursorAction, modifiers);

    if(current.row() == -1)
        return current;

    FileNavigatorItem* item = m_files->values().at(current.row());
    if(!item->isHidden())
        return current;

    if(cursorAction == MoveHome) {
        current = findFirstVisible();
    }

    return current;
}


void FileNavigator::nextFile(bool withDiffs, QString currentKey) {
    FileNavigatorItem *selectedItem = currentKey.isEmpty() ? (FileNavigatorItem *) currentItem() : findItem(currentKey);
    if(!selectedItem) {
        return;
    }

    QMap<QString, FileNavigatorItem *>::iterator i = m_files->find(selectedItem->key());
    while (i != m_files->end()) {
        if(!i.value()->isHidden() && i.key() != selectedItem->key()) {
            if(!withDiffs || i.value()->isDifferent()) {
                selectItem(i.value());
                return;
            }
        }
        i++;
    }

}

void FileNavigator::previousFile(bool withDiffs, QString currentKey) {
    FileNavigatorItem *selectedItem = currentKey.isEmpty() ? (FileNavigatorItem *) currentItem() : findItem(currentKey);

    if(!selectedItem) {
        return;
    }

    QMap<QString, FileNavigatorItem *>::iterator i = m_files->find(selectedItem->key());
    while (i != m_files->begin()) {
        --i;
        if(!i.value()->isHidden() && i.key() != selectedItem->key()) {
            if(!withDiffs || i.value()->isDifferent()) {
                selectItem(i.value());
                return;
            }
        }
    }

}

void FileNavigator::deleteItem(QString path, bool recursively) {
    const QString itemKey = path;

    auto i = m_files->find(itemKey);

    while(i != m_files->end() && (i.key() == itemKey || (recursively && i.key().startsWith(itemKey)))) {
        model()->removeRow(indexFromItem(i.value()).row());
        i = m_files->erase(i);
    }

    while(i != m_files->end()) {
        if(i.value()->isDifferent() && !i.value()->isHidden()) {
            selectItem(i.value());
            break;
        }
        i++;
    }
}

void FileNavigator::selectFileAtPosition(const QPoint pos) {
    QPoint localPoint = viewport()->mapFromGlobal(pos);
    selectItem((FileNavigatorItem*)itemAt(localPoint));

}
