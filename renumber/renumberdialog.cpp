// SPDX-License-Identifier: GPL-3.0-or-later
#include "renumberdialog.h"
#include <core/library/musiclibrary.h>
#include <QCheckBox>
#include <QDropEvent>
#include <QEvent>
#include <QFileInfo>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <algorithm>

using namespace Qt::StringLiterals;
namespace Fooyin::Renumber {
namespace {
int discNumber(const Track& t)
{
    const QString d = t.discNumber().section(u'/', 0, 0).trimmed();
    bool ok = false;
    const int n = d.toInt(&ok);
    return ok && n > 0 ? n : 1;
}

int trackNumber(const Track& t)
{
    const QString d = t.trackNumber().section(u'/', 0, 0).trimmed();
    bool ok = false;
    const int n = d.toInt(&ok);
    return ok && n > 0 ? n : 0;
}

QString padded(int n, bool pad) { return pad ? QString::number(n).rightJustified(2, u'0') : QString::number(n); }
} // namespace

RenumberDialog::RenumberDialog(const TrackList& tracks, MusicLibrary* library, QWidget* parent)
    : QDialog(parent), m_tracks(tracks), m_library(library)
{
    setWindowTitle(tr("Renumber Tracks"));
    resize(750, 550);
    auto* layout = new QVBoxLayout(this);

    auto* desc = new QLabel(tr("Reorder tracks with drag-drop or the buttons. Move tracks between discs to reassign.\n"
                                "Each disc renumbers independently starting from the chosen number."), this);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    auto* optionsLayout = new QHBoxLayout;
    optionsLayout->addWidget(new QLabel(tr("Start at:"), this));
    m_startNumber = new QSpinBox(this);
    m_startNumber->setRange(0, 9999);
    m_startNumber->setValue(1);
    optionsLayout->addWidget(m_startNumber);
    m_padNumbers = new QCheckBox(tr("Pad to 2 digits"), this);
    m_padNumbers->setChecked(true);
    optionsLayout->addWidget(m_padNumbers);
    optionsLayout->addStretch();
    layout->addLayout(optionsLayout);

    connect(m_startNumber, &QSpinBox::valueChanged, this, &RenumberDialog::updatePreview);
    connect(m_padNumbers, &QCheckBox::toggled, this, &RenumberDialog::updatePreview);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({tr("Current #"), tr("New #"), tr("Title"), tr("Artist"), tr("Filename")});
    m_tree->setRootIsDecorated(true);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->header()->setStretchLastSection(true);
    m_tree->setDragEnabled(true);
    m_tree->setAcceptDrops(true);
    m_tree->setDragDropMode(QAbstractItemView::InternalMove);
    m_tree->setDefaultDropAction(Qt::MoveAction);
    m_tree->viewport()->installEventFilter(this);
    layout->addWidget(m_tree, 1);

    auto* moveLayout = new QHBoxLayout;
    auto* moveUp = new QPushButton(tr("Move Up"), this);
    auto* moveDown = new QPushButton(tr("Move Down"), this);
    moveLayout->addWidget(moveUp);
    moveLayout->addWidget(moveDown);
    moveLayout->addSpacing(20);
    moveLayout->addWidget(new QLabel(tr("Move to disc:"), this));
    m_discSpin = new QSpinBox(this);
    m_discSpin->setRange(1, 99);
    m_discSpin->setValue(1);
    moveLayout->addWidget(m_discSpin);
    m_moveToDiscBtn = new QPushButton(tr("Move"), this);
    moveLayout->addWidget(m_moveToDiscBtn);
    moveLayout->addSpacing(20);
    auto* addDiscBtn = new QPushButton(tr("Add Disc"), this);
    moveLayout->addWidget(addDiscBtn);
    moveLayout->addStretch();
    layout->addLayout(moveLayout);

    connect(moveUp, &QPushButton::clicked, this, [this] { moveSelected(-1); });
    connect(moveDown, &QPushButton::clicked, this, [this] { moveSelected(1); });
    connect(m_moveToDiscBtn, &QPushButton::clicked, this, &RenumberDialog::moveToDisc);
    connect(addDiscBtn, &QPushButton::clicked, this, &RenumberDialog::addDisc);

    auto* upShortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Up), this);
    auto* downShortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Down), this);
    connect(upShortcut, &QShortcut::activated, this, [this] { moveSelected(-1); });
    connect(downShortcut, &QShortcut::activated, this, [this] { moveSelected(1); });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto* applyBtn = buttons->addButton(tr("Apply"), QDialogButtonBox::AcceptRole);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(applyBtn, &QPushButton::clicked, this, &RenumberDialog::apply);

    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, [this] {
        const auto selected = m_tree->selectedItems();
        if(!selected.isEmpty()) {
            auto* parent = selected.first()->parent();
            if(parent) {
                int discIdx = m_tree->indexOfTopLevelItem(parent);
                if(discIdx >= 0 && discIdx < m_groups.size()) {
                    m_discSpin->setValue(m_groups[discIdx].disc);
                }
            }
        }
    });

    populateTree();
}

bool RenumberDialog::eventFilter(QObject* obj, QEvent* event)
{
    if(obj == m_tree->viewport() && event->type() == QEvent::Drop) {
        QTimer::singleShot(0, this, &RenumberDialog::syncFromTree);
    }
    return QDialog::eventFilter(obj, event);
}

void RenumberDialog::populateTree()
{
    std::stable_sort(m_tracks.begin(), m_tracks.end(), [](const Track& a, const Track& b) {
        const int da = discNumber(a), db = discNumber(b);
        if(da != db) return da < db;
        return trackNumber(a) < trackNumber(b);
    });

    m_groups.clear();
    QMap<int, int> discIndex;
    for(int i = 0; i < static_cast<int>(m_tracks.size()); ++i) {
        const int d = discNumber(m_tracks[i]);
        if(!discIndex.contains(d)) {
            discIndex[d] = m_groups.size();
            m_groups.append(DiscGroup{d, {}});
        }
        m_groups[discIndex[d]].trackIndices.append(i);
    }

    updatePreview();
}

void RenumberDialog::moveSelected(int direction)
{
    const auto selected = m_tree->selectedItems();
    if(selected.isEmpty()) return;

    for(int g = 0; g < m_groups.size(); ++g) {
        auto* groupItem = m_tree->topLevelItem(g);
        if(!groupItem) continue;

        QList<int> selectedChildIndices;
        for(int c = 0; c < groupItem->childCount(); ++c) {
            if(selected.contains(groupItem->child(c))) {
                selectedChildIndices.append(c);
            }
        }
        if(selectedChildIndices.isEmpty()) continue;

        auto& indices = m_groups[g].trackIndices;
        if(direction < 0) {
            for(int idx : selectedChildIndices) {
                if(idx <= 0) continue;
                std::swap(indices[idx], indices[idx - 1]);
            }
        }
        else {
            for(int i = selectedChildIndices.size() - 1; i >= 0; --i) {
                const int idx = selectedChildIndices[i];
                if(idx >= indices.size() - 1) continue;
                std::swap(indices[idx], indices[idx + 1]);
            }
        }
    }

    updatePreview();
}

void RenumberDialog::moveToDisc()
{
    const auto selected = m_tree->selectedItems();
    if(selected.isEmpty()) return;

    const int targetDisc = m_discSpin->value();
    int targetGroupIdx = -1;
    for(int g = 0; g < m_groups.size(); ++g) {
        if(m_groups[g].disc == targetDisc) { targetGroupIdx = g; break; }
    }
    if(targetGroupIdx < 0) {
        m_groups.append(DiscGroup{targetDisc, {}});
        targetGroupIdx = m_groups.size() - 1;
    }

    QList<int> trackIndicesToMove;
    for(auto* item : selected) {
        if(!item->parent()) continue;
        int trackIdx = item->data(0, Qt::UserRole).toInt();
        trackIndicesToMove.append(trackIdx);
    }
    if(trackIndicesToMove.isEmpty()) return;

    for(int g = 0; g < m_groups.size(); ++g) {
        if(g == targetGroupIdx) continue;
        auto& indices = m_groups[g].trackIndices;
        indices.erase(std::remove_if(indices.begin(), indices.end(),
            [&](int idx) { return trackIndicesToMove.contains(idx); }), indices.end());
    }

    for(int idx : trackIndicesToMove) {
        if(!m_groups[targetGroupIdx].trackIndices.contains(idx)) {
            m_groups[targetGroupIdx].trackIndices.append(idx);
        }
    }

    m_groups.erase(std::remove_if(m_groups.begin(), m_groups.end(),
        [](const DiscGroup& g) { return g.trackIndices.isEmpty(); }), m_groups.end());

    std::stable_sort(m_groups.begin(), m_groups.end(),
        [](const DiscGroup& a, const DiscGroup& b) { return a.disc < b.disc; });

    updatePreview();
}

void RenumberDialog::addDisc()
{
    int maxDisc = 0;
    for(const auto& g : m_groups) maxDisc = std::max(maxDisc, g.disc);
    const int newDisc = maxDisc + 1;
    m_groups.append(DiscGroup{newDisc, {}});
    m_discSpin->setValue(newDisc);
    updatePreview();
}

void RenumberDialog::syncFromTree()
{
    if(m_syncing) return;
    m_syncing = true;

    QList<DiscGroup> newGroups;
    for(int g = 0; g < m_tree->topLevelItemCount(); ++g) {
        auto* groupItem = m_tree->topLevelItem(g);
        if(!groupItem) continue;

        int disc = 0;
        for(const auto& old : m_groups) {
            if(tr("Disc %1").arg(old.disc) == groupItem->text(0)) { disc = old.disc; break; }
        }
        if(disc == 0 && g < m_groups.size()) disc = m_groups[g].disc;

        DiscGroup group{disc, {}};
        for(int c = 0; c < groupItem->childCount(); ++c) {
            auto* child = groupItem->child(c);
            group.trackIndices.append(child->data(0, Qt::UserRole).toInt());
        }
        if(!group.trackIndices.isEmpty()) newGroups.append(group);
    }

    m_groups = newGroups;
    updatePreview();
    m_syncing = false;
}

void RenumberDialog::updatePreview()
{
    if(m_syncing) return;

    QSet<int> selectedTrackIndices;
    for(auto* item : m_tree->selectedItems()) {
        if(item->parent()) selectedTrackIndices.insert(item->data(0, Qt::UserRole).toInt());
    }

    m_tree->clear();
    const int start = m_startNumber->value();
    const bool pad = m_padNumbers->isChecked();

    for(const auto& group : m_groups) {
        auto* groupItem = new QTreeWidgetItem(m_tree);
        groupItem->setText(0, tr("Disc %1").arg(group.disc));
        groupItem->setFlags(groupItem->flags() & ~Qt::ItemIsDragEnabled);
        groupItem->setExpanded(true);

        int newNum = start;
        for(int trackIdx : group.trackIndices) {
            const Track& t = m_tracks[trackIdx];
            auto* item = new QTreeWidgetItem(groupItem);
            item->setText(0, t.trackNumber());
            item->setText(1, padded(newNum, pad));
            item->setText(2, t.title());
            item->setText(3, t.artistsJoined(u"; "_s));
            item->setText(4, QFileInfo(t.filepath()).fileName());
            item->setData(0, Qt::UserRole, trackIdx);
            if(selectedTrackIndices.contains(trackIdx)) item->setSelected(true);
            ++newNum;
        }
    }

    m_tree->resizeColumnToContents(0);
    m_tree->resizeColumnToContents(1);
    m_tree->resizeColumnToContents(2);
}

void RenumberDialog::apply()
{
    const int start = m_startNumber->value();
    const bool pad = m_padNumbers->isChecked();
    TrackList changed;

    for(const auto& group : m_groups) {
        int newNum = start;
        for(int trackIdx : group.trackIndices) {
            Track t = m_tracks[trackIdx];
            const QString newTrackStr = padded(newNum, pad);
            const QString newDiscStr = QString::number(group.disc);
            bool modified = false;
            if(t.trackNumber() != newTrackStr) { t.setTrackNumber(newTrackStr); modified = true; }
            if(t.discNumber() != newDiscStr) { t.setDiscNumber(newDiscStr); modified = true; }
            if(modified) changed.push_back(t);
            ++newNum;
        }
    }

    if(!changed.empty()) {
        m_library->writeTrackMetadata(changed);
    }

    accept();
}
} // namespace Fooyin::Renumber
