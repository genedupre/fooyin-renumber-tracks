// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <core/track.h>
#include <QDialog>

class QEvent;
class QTreeWidget;
class QTreeWidgetItem;
class QSpinBox;
class QCheckBox;
class QPushButton;

namespace Fooyin {
class MusicLibrary;
namespace Renumber {
class RenumberDialog : public QDialog {
    Q_OBJECT
public:
    RenumberDialog(const TrackList& tracks, MusicLibrary* library, QWidget* parent);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    struct DiscGroup {
        int disc{0};
        QList<int> trackIndices;
    };

    void populateTree();
    void moveSelected(int direction);
    void moveToDisc();
    void addDisc();
    void syncFromTree();
    void updatePreview();
    void apply();

    TrackList m_tracks;
    MusicLibrary* m_library;
    QTreeWidget* m_tree;
    QSpinBox* m_startNumber;
    QSpinBox* m_discSpin;
    QPushButton* m_moveToDiscBtn;
    QCheckBox* m_padNumbers;
    QList<DiscGroup> m_groups;
    bool m_syncing{false};
};
} // namespace Renumber
} // namespace Fooyin
