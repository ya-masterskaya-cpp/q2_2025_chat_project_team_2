#include "MainWindow.h"

class ClientApp : public wxApp {
public:
    bool OnInit() override;
    int OnExit() override;

    virtual void OnInitCmdLine(wxCmdLineParser& parser) override {}
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser) override { return true; }
};