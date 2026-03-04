#pragma once
#include <QString>

struct UIConfig
{
    // ---- Colors ----
    QString mainBackground = "#0f0f1a";
    QString panelBackground = "#111122";
    QString panelBorder = "#7a5cff";
    QString buttonBackground = "#1c1c2e";
    QString buttonHover = "#2a2a40";
    QString buttonBorder = "#7a5cff";
    QString textColor = "#ffffff";
    QString consoleTextColor = "#00ff88";

    QString statusRed = "#ff4444";
    QString statusGreen = "#00ff88";
    QString statusYellow = "#ffaa00";

    // ---- Sizes ----
    int windowWidth = 1350;
    int windowHeight = 820;
    int speakButtonHeight = 75;
    int headerFontSize = 22;
    int normalFontSize = 14;
    int timerFontSize = 20;
    int panelRadius = 14;
    int spacing = 20;
    int margin = 25;
    int sessionDurationSeconds = 120;

    // ---- Text ----
    QString windowTitle = "Voice Authentication Security System";
    QString headerTitle = "VOICE AUTHENTICATION SECURITY SYSTEM";

    QString sectionControl = "CONTROL PANEL";
    QString sectionConsole = "SYSTEM CONSOLE";
    QString sectionLog = "SYSTEM LOG";

    QString btnViewUsers = "View All Users";
    QString btnAuthorize = "Authorize User";
    QString btnRemoveAuth = "Remove Authorization";
    QString btnRemoveUser = "Remove User";
    QString btnEnroll = "Start Enrollment";
    QString btnSpeak = "🎤  SPEAK";

    QString statusDisconnected = "DISCONNECTED";
    QString statusActive = "SESSION ACTIVE";
    QString statusIdle = "IDLE";

    // ---- Backend Configuration ----
    QString backendDir =
        "C:/Users/MAYANK/Desktop/voice_fail_safe_v2";

    QString pythonExe =
        "C:/Users/MAYANK/Desktop/voice_fail_safe_v2/voice_2/Scripts/python.exe";

    QString backendScript =
        "backend_server.py";
};
