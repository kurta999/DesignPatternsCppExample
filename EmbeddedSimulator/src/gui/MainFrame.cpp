/**
 * @file
 * @brief wxWidgets controls, trends, event translation, logging, and Observer updates.
 */

#include "gui/MainFrame.hpp"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dcbuffer.h>
#include <wx/gauge.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>
#include <wx/textctrl.h>

#include <algorithm>
#include <cmath>
#include <deque>
#include <filesystem>
#include <memory>

namespace simulator::gui
{
namespace
{
constexpr int kSimulationPeriodMs = 100;

wxString asWxString(const std::string& value)
{
    return wxString::FromUTF8(value.c_str());
}

wxStaticText* addReadout(wxFlexGridSizer& grid, wxWindow* parent,
                         const wxString& name, const wxString& initial)
{
    grid.Add(new wxStaticText(parent, wxID_ANY, name), 0, wxALIGN_CENTER_VERTICAL);
    auto* value = new wxStaticText(parent, wxID_ANY, initial);
    value->SetFont(value->GetFont().Bold());
    grid.Add(value, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    return value;
}
} // namespace

class TrendPanel final : public wxPanel
{
public:
    explicit TrendPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize{-1, 260});
        Bind(wxEVT_PAINT, &TrendPanel::onPaint, this);
    }

    void push(const SimulationSnapshot& snapshot)
    {
        history_.push_back(snapshot);
        while (history_.size() > 300U) history_.pop_front();
        Refresh(false);
    }

    void clear()
    {
        history_.clear();
        Refresh(false);
    }

private:
    void onPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc{this};
        dc.SetBackground(wxBrush{wxColour{24, 28, 34}});
        dc.Clear();
        const wxSize size = GetClientSize();
        if (size.x < 120 || size.y < 100) return;

        const wxRect plot{55, 20, size.x - 75, size.y - 55};
        dc.SetPen(wxPen{wxColour{70, 76, 86}, 1});
        for (int row = 0; row <= 4; ++row)
        {
            const int y = plot.y + (plot.height * row / 4);
            dc.DrawLine(plot.x, y, plot.GetRight(), y);
        }
        dc.SetTextForeground(wxColour{190, 195, 205});
        dc.DrawText("100%", 8, plot.y - 7);
        dc.DrawText("0", 32, plot.GetBottom() - 7);
        dc.DrawText("DC-link %", plot.x + 5, plot.GetBottom() + 10);
        dc.SetTextForeground(wxColour{75, 190, 255});
        dc.DrawText("Temperature / 140 C", plot.x + 95, plot.GetBottom() + 10);
        dc.SetTextForeground(wxColour{255, 170, 70});
        dc.DrawText("Current / 600 A", plot.x + 250, plot.GetBottom() + 10);

        if (history_.size() < 2U) return;
        const auto drawSeries = [&](const wxColour& colour, const auto valueOf) {
            dc.SetPen(wxPen{colour, 2});
            wxPoint previous;
            bool hasPrevious = false;
            for (std::size_t index = 0; index < history_.size(); ++index)
            {
                const float normalized = std::clamp(valueOf(history_[index]), 0.0F, 1.0F);
                const int x = plot.x + static_cast<int>(
                    (static_cast<double>(index) / static_cast<double>(history_.size() - 1U)) *
                    plot.width);
                const int y = plot.GetBottom() - static_cast<int>(normalized * plot.height);
                const wxPoint point{x, y};
                if (hasPrevious) dc.DrawLine(previous, point);
                previous = point;
                hasPrevious = true;
            }
        };
        drawSeries(wxColour{100, 225, 130}, [](const SimulationSnapshot& value) {
            const float battery = std::max(value.measurements.batteryVoltageV, 1.0F);
            return value.measurements.dcLinkVoltageV / battery;
        });
        drawSeries(wxColour{75, 190, 255}, [](const SimulationSnapshot& value) {
            return value.measurements.inverterTemperatureC / 140.0F;
        });
        drawSeries(wxColour{255, 170, 70}, [](const SimulationSnapshot& value) {
            return value.measurements.phaseCurrentA / 600.0F;
        });
    }

    std::deque<SimulationSnapshot> history_;
};

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "Embedded Design Pattern Simulator",
              wxDefaultPosition, wxSize{1380, 900}),
      engine_{logger_}, timer_{this}
{
    buildUi();
    bindEvents();

    const auto dataDirectory = std::filesystem::path{
        wxStandardPaths::Get().GetUserLocalDataDir().ToStdWstring()};
    logger_.addSink(std::make_shared<FileLogSink>(
        dataDirectory / "logs" / "embedded_simulator.log"));
    logger_.addSink(std::make_shared<CallbackLogSink>(
        [this](const LogRecord& record) { appendLog(record); }));
    engine_.attachObserver(*this);
    applySelectedConfiguration();
    logger_.log(LogLevel::Info, "Application", "GUI initialized; file and live log sinks active");
    timer_.Start(kSimulationPeriodMs);
    Centre();
}

MainFrame::~MainFrame()
{
    timer_.Stop();
}

void MainFrame::buildUi()
{
    auto* fileMenu = new wxMenu();
    const auto exitId = wxWindow::NewControlId();
    fileMenu->Append(exitId, "E&xit\tAlt+F4");
    auto* simulationMenu = new wxMenu();
    const auto startId = wxWindow::NewControlId();
    const auto resetId = wxWindow::NewControlId();
    simulationMenu->Append(startId, "Start / Pause\tF5");
    simulationMenu->Append(resetId, "Reset\tCtrl+R");
    auto* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, "&About");
    auto* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(simulationMenu, "&Simulation");
    menuBar->Append(helpMenu, "&Help");
    SetMenuBar(menuBar);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, exitId);
    Bind(wxEVT_MENU, &MainFrame::onStartPause, this, startId);
    Bind(wxEVT_MENU, &MainFrame::onReset, this, resetId);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxMessageBox("Connected simulation of embedded design patterns\n"
                     "wxWidgets + vcpkg + MSVC C++17",
                     "About", wxOK | wxICON_INFORMATION, this);
    }, wxID_ABOUT);

    auto* root = new wxPanel(this);
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* configurationBox = new wxStaticBoxSizer(wxVERTICAL, root, "Device and driver configuration");
    auto* configurationParent = configurationBox->GetStaticBox();
    auto* configurationGrid = new wxFlexGridSizer(2, 7, 9);
    configurationGrid->AddGrowableCol(1, 1);
    const auto addChoice = [&](const wxString& label, wxChoice*& choice,
                               const std::initializer_list<wxString>& values) {
        configurationGrid->Add(new wxStaticText(configurationParent, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
        choice = new wxChoice(configurationParent, wxID_ANY);
        for (const auto& value : values) choice->Append(value);
        choice->SetSelection(0);
        configurationGrid->Add(choice, 1, wxEXPAND);
    };
    addChoice("Device profile", profileChoice_, {"400 V traction inverter", "800 V traction inverter"});
    addChoice("Control board", boardChoice_, {"STM32H7", "NXP S32K3"});
    addChoice("Analog frontend", frontendChoice_, {"On-chip ADC", "Isolated sigma-delta ADC"});
    addChoice("Temperature device", temperatureSensorChoice_, {"10 kOhm NTC", "PT100 RTD"});
    addChoice("Cooling strategy", coolingChoice_, {"Quiet", "Performance"});
    configurationBox->Add(configurationGrid, 0, wxEXPAND | wxALL, 8);
    auto* applyButton = new wxButton(configurationParent, wxID_ANY, "Apply configuration and reset");
    configurationBox->Add(applyButton, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    driverDescription_ = new wxStaticText(configurationParent, wxID_ANY, "Driver: -", wxDefaultPosition,
                                          wxDefaultSize, wxST_ELLIPSIZE_END);
    configurationBox->Add(driverDescription_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    applyButton->Bind(wxEVT_BUTTON, &MainFrame::onApplyConfiguration, this);
    topSizer->Add(configurationBox, 1, wxEXPAND | wxRIGHT, 8);

    auto* controlsBox = new wxStaticBoxSizer(wxVERTICAL, root, "Operator controls and fault injection");
    auto* controlsParent = controlsBox->GetStaticBox();
    auto* controlGrid = new wxFlexGridSizer(2, 5, 8);
    ignitionCheck_ = new wxCheckBox(controlsParent, wxID_ANY, "Ignition on");
    driveRequestCheck_ = new wxCheckBox(controlsParent, wxID_ANY, "Drive request");
    emergencyStopCheck_ = new wxCheckBox(controlsParent, wxID_ANY, "Emergency stop");
    prechargeFailureCheck_ = new wxCheckBox(controlsParent, wxID_ANY, "Precharge circuit open");
    overtemperatureCheck_ = new wxCheckBox(controlsParent, wxID_ANY, "Force overtemperature");
    overcurrentCheck_ = new wxCheckBox(controlsParent, wxID_ANY, "Force phase overcurrent");
    controlGrid->Add(ignitionCheck_); controlGrid->Add(driveRequestCheck_);
    controlGrid->Add(emergencyStopCheck_); controlGrid->Add(prechargeFailureCheck_);
    controlGrid->Add(overtemperatureCheck_); controlGrid->Add(overcurrentCheck_);
    controlsBox->Add(controlGrid, 0, wxALL, 8);
    torqueRequestLabel_ = new wxStaticText(controlsParent, wxID_ANY, "Torque request: 0 Nm");
    torqueSlider_ = new wxSlider(controlsParent, wxID_ANY, 0, -250, 250,
                                 wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    controlsBox->Add(torqueRequestLabel_, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    controlsBox->Add(torqueSlider_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    auto* controlButtons = new wxBoxSizer(wxHORIZONTAL);
    startPauseButton_ = new wxButton(controlsParent, wxID_ANY, "Start simulation");
    auto* resetButton = new wxButton(controlsParent, wxID_ANY, "Reset plant");
    auto* faultResetButton = new wxButton(controlsParent, wxID_ANY, "Request fault reset");
    controlButtons->Add(startPauseButton_, 1, wxRIGHT, 5);
    controlButtons->Add(resetButton, 1, wxRIGHT, 5);
    controlButtons->Add(faultResetButton, 1);
    controlsBox->Add(controlButtons, 0, wxEXPAND | wxALL, 8);
    startPauseButton_->Bind(wxEVT_BUTTON, &MainFrame::onStartPause, this);
    resetButton->Bind(wxEVT_BUTTON, &MainFrame::onReset, this);
    faultResetButton->Bind(wxEVT_BUTTON, &MainFrame::onFaultReset, this);
    topSizer->Add(controlsBox, 1, wxEXPAND | wxRIGHT, 8);

    auto* measurementsBox = new wxStaticBoxSizer(wxVERTICAL, root, "Live measurements");
    auto* measurementsParent = measurementsBox->GetStaticBox();
    auto* measurementsGrid = new wxFlexGridSizer(2, 4, 8);
    measurementsGrid->AddGrowableCol(1, 1);
    stateValue_ = addReadout(*measurementsGrid, measurementsParent, "State", "Standby");
    faultValue_ = addReadout(*measurementsGrid, measurementsParent, "Fault", "None");
    batteryValue_ = addReadout(*measurementsGrid, measurementsParent, "Battery", "0 V");
    dcLinkValue_ = addReadout(*measurementsGrid, measurementsParent, "DC link", "0 V");
    temperatureValue_ = addReadout(*measurementsGrid, measurementsParent, "Temperature", "25 C");
    currentValue_ = addReadout(*measurementsGrid, measurementsParent, "Phase current", "0 A");
    torqueCommandValue_ = addReadout(*measurementsGrid, measurementsParent, "Torque command", "0 Nm");
    fanValue_ = addReadout(*measurementsGrid, measurementsParent, "Fan duty", "0 %");
    contactorValue_ = addReadout(*measurementsGrid, measurementsParent, "Power path", "Open");
    measurementsBox->Add(measurementsGrid, 0, wxEXPAND | wxALL, 8);
    measurementsBox->Add(new wxStaticText(measurementsParent, wxID_ANY, "DC-link charge"), 0, wxLEFT | wxRIGHT, 8);
    dcLinkGauge_ = new wxGauge(measurementsParent, wxID_ANY, 100);
    measurementsBox->Add(dcLinkGauge_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    measurementsBox->Add(new wxStaticText(measurementsParent, wxID_ANY, "Temperature / 140 C"), 0, wxLEFT | wxRIGHT, 8);
    temperatureGauge_ = new wxGauge(measurementsParent, wxID_ANY, 140);
    measurementsBox->Add(temperatureGauge_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    topSizer->Add(measurementsBox, 1, wxEXPAND);
    mainSizer->Add(topSizer, 0, wxEXPAND | wxALL, 10);

    trendPanel_ = new TrendPanel(root);
    mainSizer->Add(trendPanel_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    auto* logBox = new wxStaticBoxSizer(wxVERTICAL, root, "Timestamped event and measurement log");
    auto* logParent = logBox->GetStaticBox();
    logText_ = new wxTextCtrl(logParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize{-1, 180},
                              wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxHSCROLL);
    logText_->SetFont(wxFontInfo{9}.Family(wxFONTFAMILY_TELETYPE));
    logBox->Add(logText_, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(logBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    root->SetSizer(mainSizer);
    CreateStatusBar(2);
    SetStatusText("Paused", 0);
    SetStatusText("100 ms simulation task", 1);
}

void MainFrame::bindEvents()
{
    Bind(wxEVT_TIMER, &MainFrame::onTimer, this, timer_.GetId());
    torqueSlider_->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
        torqueRequestLabel_->SetLabel(wxString::Format("Torque request: %d Nm", torqueSlider_->GetValue()));
    });
}

SimulationConfiguration MainFrame::selectedConfiguration() const
{
    SimulationConfiguration configuration;
    configuration.profile = profileChoice_->GetSelection() == 0
                                ? DeviceProfile::Traction400V : DeviceProfile::Traction800V;
    configuration.board = boardChoice_->GetSelection() == 0
                              ? BoardFamily::Stm32H7 : BoardFamily::NxpS32K3;
    configuration.analogFrontend = frontendChoice_->GetSelection() == 0
                                       ? AnalogFrontend::OnChipAdc : AnalogFrontend::IsolatedAdc;
    configuration.temperatureSensor = temperatureSensorChoice_->GetSelection() == 0
                                          ? TemperatureSensor::Ntc10K : TemperatureSensor::Pt100;
    configuration.coolingMode = coolingChoice_->GetSelection() == 0
                                    ? CoolingMode::Quiet : CoolingMode::Performance;
    return configuration;
}

void MainFrame::applySelectedConfiguration()
{
    engine_.applyConfiguration(selectedConfiguration());
    driverDescription_->SetLabel("Driver: " + asWxString(engine_.driverDescription()));
    trendPanel_->clear();
    refreshRunState();
}

void MainFrame::onSimulationSnapshot(const SimulationSnapshot& value)
{
    stateValue_->SetLabel(inverter::toString(value.operatingState));
    faultValue_->SetLabel(inverter::toString(value.fault));
    faultValue_->SetForegroundColour(value.fault == inverter::FaultCode::None
                                         ? wxColour{30, 145, 70} : wxColour{210, 45, 45});
    batteryValue_->SetLabel(wxString::Format("%.1f V", value.measurements.batteryVoltageV));
    dcLinkValue_->SetLabel(wxString::Format("%.1f V", value.measurements.dcLinkVoltageV));
    temperatureValue_->SetLabel(wxString::Format("%.1f C", value.measurements.inverterTemperatureC));
    currentValue_->SetLabel(wxString::Format("%.1f A", value.measurements.phaseCurrentA));
    torqueCommandValue_->SetLabel(wxString::Format("%.1f Nm", value.torqueCommandNm));
    fanValue_->SetLabel(wxString::Format("%.0f %%", value.fanDutyPercent));
    contactorValue_->SetLabel(wxString::Format("Precharge=%s  Main=%s  PWM=%s",
        value.prechargeRelayClosed ? "ON" : "OFF",
        value.mainContactorClosed ? "ON" : "OFF",
        value.pwmEnabled ? "ON" : "OFF"));
    const float battery = std::max(value.measurements.batteryVoltageV, 1.0F);
    dcLinkGauge_->SetValue(static_cast<int>(std::clamp(
        (value.measurements.dcLinkVoltageV / battery) * 100.0F, 0.0F, 100.0F)));
    temperatureGauge_->SetValue(static_cast<int>(std::clamp(
        value.measurements.inverterTemperatureC, 0.0F, 140.0F)));
    trendPanel_->push(value);
}

void MainFrame::appendLog(const LogRecord& record)
{
    if (logText_->GetLastPosition() > 200'000) logText_->Remove(0, 50'000);
    logText_->AppendText(asWxString(formatLogRecord(record)) + "\n");
}

void MainFrame::refreshRunState()
{
    startPauseButton_->SetLabel(engine_.isRunning() ? "Pause simulation" : "Start simulation");
    SetStatusText(engine_.isRunning() ? "Running" : "Paused", 0);
}

void MainFrame::onTimer(wxTimerEvent&)
{
    SimulationControls controls;
    controls.ignitionOn = ignitionCheck_->GetValue();
    controls.driveRequest = driveRequestCheck_->GetValue();
    controls.emergencyStopActive = emergencyStopCheck_->GetValue();
    controls.forcePrechargeOpenCircuit = prechargeFailureCheck_->GetValue();
    controls.forceOvertemperature = overtemperatureCheck_->GetValue();
    controls.forcePhaseOvercurrent = overcurrentCheck_->GetValue();
    controls.requestedTorqueNm = static_cast<float>(torqueSlider_->GetValue());
    engine_.setControls(controls);
    commandQueue_.dispatchAll(engine_);
    engine_.step(kSimulationPeriodMs);
}

void MainFrame::onStartPause(wxCommandEvent&)
{
    if (engine_.isRunning())
        commandQueue_.enqueue(std::make_unique<PauseCommand>());
    else
        commandQueue_.enqueue(std::make_unique<StartCommand>());
    commandQueue_.dispatchAll(engine_);
    refreshRunState();
}

void MainFrame::onReset(wxCommandEvent&)
{
    commandQueue_.enqueue(std::make_unique<ResetCommand>());
    commandQueue_.dispatchAll(engine_);
    ignitionCheck_->SetValue(false);
    driveRequestCheck_->SetValue(false);
    emergencyStopCheck_->SetValue(false);
    prechargeFailureCheck_->SetValue(false);
    overtemperatureCheck_->SetValue(false);
    overcurrentCheck_->SetValue(false);
    torqueSlider_->SetValue(0);
    torqueRequestLabel_->SetLabel("Torque request: 0 Nm");
    trendPanel_->clear();
    refreshRunState();
}

void MainFrame::onFaultReset(wxCommandEvent&)
{
    commandQueue_.enqueue(std::make_unique<FaultResetCommand>());
    commandQueue_.dispatchAll(engine_);
}

void MainFrame::onApplyConfiguration(wxCommandEvent&)
{
    applySelectedConfiguration();
}
} // namespace simulator::gui
