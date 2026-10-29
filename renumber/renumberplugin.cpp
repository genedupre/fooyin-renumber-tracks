// SPDX-License-Identifier: GPL-3.0-or-later
#include "renumberplugin.h"
#include "renumberdialog.h"
#include <core/engine/audioloader.h>
#include <core/library/musiclibrary.h>
#include <core/plugins/coreplugincontext.h>
#include <gui/guiconstants.h>
#include <gui/plugins/guiplugincontext.h>
#include <gui/trackselectioncontroller.h>
#include <utils/utils.h>
#include <QMainWindow>
#include <QMenu>

namespace Fooyin::Renumber {
void RenumberPlugin::initialise(const CorePluginContext& context)
{
    m_library = context.library;
    m_loader = context.audioLoader;
}

void RenumberPlugin::initialise(const GuiPluginContext& context)
{
    m_trackSelection = context.trackSelection;

    m_trackSelection->registerTrackContextAction(
        this, TrackContextMenuArea::Track, Constants::Menus::Context::Tagging, "Renumber.RenumberTracks",
        tr("Renumber tracks..."), [this](QMenu* menu, const TrackSelection& selection) {
            if(selection.tracks.empty()) {
                return;
            }
            bool canWrite = std::ranges::all_of(selection.tracks, [this](const Track& track) {
                return track.isMetadataEditable(m_loader->canWriteMetadata(track));
            });
            auto* action = menu->addAction(tr("Renumber tracks..."));
            action->setEnabled(canWrite);
            QObject::connect(action, &QAction::triggered, this, [this, tracks = selection.tracks] {
                auto* dialog = new RenumberDialog(tracks, m_library, static_cast<QWidget*>(Utils::getMainWindow()));
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->open();
            });
        });
}
} // namespace Fooyin::Renumber
