#ifndef _Utilities_UiDesigner_MCP_UiDesignerMcpServer_h_
#define _Utilities_UiDesigner_MCP_UiDesignerMcpServer_h_

#include <Utilities/UiDesigner/Services/UiDesignerServices.h>

namespace Upp {

class UiDesignerMcpServer {
public:
    UiDesignerMcpServer();

    String Handle(const String& json_line);
    UiDesignerSession& Session() { return session_; }

private:
    UiDesignerSession session_;
    UiDesignerAutomationService automation_;
    UiDesignerMcpEndpoint endpoint_;
};

}

#endif
