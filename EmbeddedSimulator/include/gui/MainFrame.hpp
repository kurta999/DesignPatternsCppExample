#pragma once

#include "simulator/Logging.hpp"
#include "simulator/SimulationCommands.hpp"
#include "simulator/SimulationEngine.hpp"

#include <wx/frame.h>
#include <wx/timer.h>

class wxButton;
class wxCheckBox;
class wxChoice;
class wxGauge;
class wxSlider;
class wxStaticText;
class wxTextCtrl;

namespace simulator::gui
{
class TrendPanel;

class MainFrame final : public wxFrame, public ISimulationObserver
{
public:
    MainFrame();
    ~MainFrame() override;
    void onSimulationSnapshot(const SimulationSnapshot& snapshot) override;

private:
    void buildUi();
    void bindEvents();
    void applySelectedConfiguration();
    [[nodiscard]] SimulationConfiguration selectedConfiguration() const;
    void appendLog(const LogRecord& record);
    void refreshRunState();

    void onTimer(wxTimerEvent& event);
    void onStartPause(wxCommandEvent& event);
    void onReset(wxCommandEvent& event);
    void onFaultReset(wxCommandEvent& event);
    void onApplyConfiguration(wxCommandEvent& event);

    Logger logger_;
    SimulationEngine engine_;
    SimulationCommandQueue commandQueue_;
    wxTimer timer_;

    wxChoice* profileChoice_{nullptr};
    wxChoice* boardChoice_{nullptr};
    wxChoice* frontendChoice_{nullptr};
    wxChoice* temperatureSensorChoice_{nullptr};
    wxChoice* coolingChoice_{nullptr};
    wxStaticText* driverDescription_{nullptr};

    wxCheckBox* ignitionCheck_{nullptr};
    wxCheckBox* driveRequestCheck_{nullptr};
    wxCheckBox* emergencyStopCheck_{nullptr};
    wxCheckBox* prechargeFailureCheck_{nullptr};
    wxCheckBox* overtemperatureCheck_{nullptr};
    wxCheckBox* overcurrentCheck_{nullptr};
    wxSlider* torqueSlider_{nullptr};
    wxStaticText* torqueRequestLabel_{nullptr};
    wxButton* startPauseButton_{nullptr};

    wxStaticText* stateValue_{nullptr};
    wxStaticText* faultValue_{nullptr};
    wxStaticText* batteryValue_{nullptr};
    wxStaticText* dcLinkValue_{nullptr};
    wxStaticText* temperatureValue_{nullptr};
    wxStaticText* currentValue_{nullptr};
    wxStaticText* torqueCommandValue_{nullptr};
    wxStaticText* fanValue_{nullptr};
    wxStaticText* contactorValue_{nullptr};
    wxGauge* dcLinkGauge_{nullptr};
    wxGauge* temperatureGauge_{nullptr};
    TrendPanel* trendPanel_{nullptr};
    wxTextCtrl* logText_{nullptr};
};
} // namespace simulator::gui

