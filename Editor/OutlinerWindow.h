#pragma once

#include "UI.h"

namespace ToolKit
{
  namespace Editor
  {
    class OutlinerWindow : public Window
    {
    public:
      OutlinerWindow(XmlNode* node);
      OutlinerWindow();
      virtual ~OutlinerWindow();
      virtual void Show() override;
      virtual Type GetType() const override;
      virtual void DispatchSignals() const override;
    };
  }
}
