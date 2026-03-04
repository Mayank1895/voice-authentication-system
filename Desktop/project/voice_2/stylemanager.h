#pragma once

#include <QString>

class QWidget;
struct UIConfig;

class StyleManager
{
public:
    static void applyGlobalStyle(QWidget* widget,
                                 const UIConfig& config);
};
