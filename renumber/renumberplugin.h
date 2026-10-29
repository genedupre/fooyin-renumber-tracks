// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <core/plugins/coreplugin.h>
#include <core/plugins/plugin.h>
#include <gui/plugins/guiplugin.h>

namespace Fooyin::Renumber {
class RenumberPlugin : public QObject,
                       public Plugin,
                       public CorePlugin,
                       public GuiPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID FOOYIN_PLUGIN_IID FILE "renumber.json")
    Q_INTERFACES(Fooyin::Plugin Fooyin::CorePlugin Fooyin::GuiPlugin)

public:
    void initialise(const CorePluginContext& context) override;
    void initialise(const GuiPluginContext& context) override;

private:
    MusicLibrary* m_library{nullptr};
    std::shared_ptr<AudioLoader> m_loader;
    TrackSelectionController* m_trackSelection{nullptr};
};
} // namespace Fooyin::Renumber
