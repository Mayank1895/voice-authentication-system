#include "stylemanager.h"
#include "uiconfig.h"
#include <QWidget>

void StyleManager::applyGlobalStyle(QWidget* widget,
                                    const UIConfig& config)
{
    QString style = QString(R"(

        QMainWindow {
            background-color: %1;
        }

        QLabel {
            color: %2;
            font-size: %3px;
            font-weight: bold;
        }

        QFrame {
            background-color: %4;
            border: 2px solid %5;
            border-radius: %6px;
        }

        QPushButton {
            background-color: %7;
            color: %2;
            padding: 14px;
            border: 1px solid %8;
            border-radius: 12px;
            font-size: %9px;
        }

        QPushButton:hover {
            background-color: %10;
        }

        QPushButton:pressed {
            background-color: %10;
            padding-top: 16px;
            padding-bottom: 12px;
        }

        QTextEdit {
            background-color: #000000;
            color: %11;
            border: none;
            border-radius: %6px;
            padding: 12px;
            font-family: Consolas;
            font-size: %9px;
        }

    )")
                        .arg(config.mainBackground)
                        .arg(config.textColor)
                        .arg(config.headerFontSize)
                        .arg(config.panelBackground)
                        .arg(config.panelBorder)
                        .arg(config.panelRadius)
                        .arg(config.buttonBackground)
                        .arg(config.buttonBorder)
                        .arg(config.normalFontSize)
                        .arg(config.buttonHover)
                        .arg(config.consoleTextColor);

    widget->setStyleSheet(style);
}
