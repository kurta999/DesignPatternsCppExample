#pragma once

/**
 * @file MainFrame.hpp
 * @brief wxWidgets composition root and primary simulator window.
 */

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
/** @addtogroup connected_simulator
 *  @{
 */
/** @brief Custom time-series panel implemented privately by the GUI source. */
class TrendPanel;

/**
 * @brief Main desktop window and Observer of simulation snapshots.
 *
 * The frame is the application composition root: it owns the Logger,
 * SimulationEngine, Command queue, wx timer, and all widgets. wx event handlers
 * translate user intent into core types and never implement controller policy.
 */
class MainFrame final : public wxFrame, public ISimulationObserver
{
public:
    /** @brief Construct collaborators, build controls, bind events, and attach sinks. */
    MainFrame();
    ~MainFrame() override;
    /** @brief Refresh readouts, gauges, and trends from one immutable snapshot. */
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
/** @} */
} // namespace simulator::gui
