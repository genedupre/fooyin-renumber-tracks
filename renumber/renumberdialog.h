// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <core/track.h>
#include <QDialog>

class QTreeWidget;
class QTreeWidgetItem;
class QSpinBox;
class QCheckBox;

namespace Fooyin {
class MusicLibrary;
namespace Renumber {
class RenumberDialog : public QDialog {
    Q_OBJECT
public:
    RenumberDialog(const TrackList& tracks, MusicLibrary* library, QWidget* parent);

private:
    struct DiscGroup {
        int disc{0};
        QList<int> trackIndices;
    };

    void populateTree();
    void moveSelected(int direction);
    void updatePreview();
    void apply();

    TrackList m_tracks;
    MusicLibrary* m_library;
    QTreeWidget* m_tree;
    QSpinBox* m_startNumber;
    QCheckBox* m_padNumbers;
    QList<DiscGroup> m_groups;
};
} // namespace Renumber
} // namespace Fooyin
