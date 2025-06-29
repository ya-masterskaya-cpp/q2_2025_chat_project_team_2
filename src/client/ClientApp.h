#pragma once
#include <wx/app.h>
#include <wx/cmdline.h>
#include "MainWindow.h"

class ClientApp : public wxApp {
public:
    virtual bool OnInit() override;
    virtual int OnExit() override;

    virtual void OnInitCmdLine(wxCmdLineParser& parser) override;
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser) override;

private:
    bool console_mode_ = false; // Флаг для режима консоли
};