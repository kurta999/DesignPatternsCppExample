/**
 * @file
 * @brief wxWidgets application entry point, including automated GUI smoke-test mode.
 */

#include "gui/MainFrame.hpp"

#include <wx/app.h>

namespace simulator::gui
{
class SimulatorApp final : public wxApp
{
public:
    bool OnInit() override
    {
        SetAppName("Embedded Pattern Simulator");
        SetVendorName("DesignPatterns");
        auto* frame = new MainFrame();

        bool smokeTest = false;
        for (int index = 1; index < argc; ++index)
        {
            if (wxString{argv[index]} == "--smoke-test") smokeTest = true;
        }
        if (smokeTest)
        {
            frame->Show(false);
            CallAfter([this, frame] {
                frame->Destroy();
                ExitMainLoop();
            });
        }
        else
        {
            frame->Show(true);
        }
        return true;
    }
};
} // namespace simulator::gui

wxIMPLEMENT_APP(simulator::gui::SimulatorApp);
