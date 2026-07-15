#include "UiDesignerMcpServer.h"

namespace Upp {

UiDesignerMcpServer::UiDesignerMcpServer()
    : automation_(session_), endpoint_(automation_)
{
}

String UiDesignerMcpServer::Handle(const String& json_line)
{
    return endpoint_.HandleJsonLine(json_line);
}

}
