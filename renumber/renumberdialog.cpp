// SPDX-License-Identifier: GPL-3.0-or-later
#include "renumberdialog.h"
#include <core/library/musiclibrary.h>
#include <QCheckBox>
#include <QFileInfo>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
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
    resize(700, 500);
    auto* layout = new QVBoxLayout(this);

    auto* desc = new QLabel(tr("Reorder tracks with the buttons, then apply to write new track numbers.\n"
                                "Tracks are grouped by disc number. Each disc renumbers independently."), this);
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
    m_tree->setDragDropMode(QAbstractItemView::InternalMove);
    m_tree->setDefaultDropAction(Qt::MoveAction);
    layout->addWidget(m_tree, 1);

    auto* moveLayout = new QHBoxLayout;
    auto* moveUp = new QPushButton(tr("Move Up"), this);
    auto* moveDown = new QPushButton(tr("Move Down"), this);
    moveLayout->addStretch();
    moveLayout->addWidget(moveUp);
    moveLayout->addWidget(moveDown);
    moveLayout->addStretch();
    layout->addLayout(moveLayout);

    connect(moveUp, &QPushButton::clicked, this, [this] { moveSelected(-1); });
    connect(moveDown, &QPushButton::clicked, this, [this] { moveSelected(1); });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto* applyBtn = buttons->addButton(tr("Apply"), QDialogButtonBox::AcceptRole);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(applyBtn, &QPushButton::clicked, this, &RenumberDialog::apply);

    populateTree();
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

void RenumberDialog::updatePreview()
{
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
            if(t.trackNumber() != newTrackStr) {
                t.setTrackNumber(newTrackStr);
                changed.push_back(t);
            }
            ++newNum;
        }
    }

    if(!changed.empty()) {
        m_library->writeTrackMetadata(changed);
    }

    accept();
}
} // namespace Fooyin::Renumber
