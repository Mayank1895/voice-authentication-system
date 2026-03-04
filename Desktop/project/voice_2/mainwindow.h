#pragma once

#include <QMainWindow>
#include <QList>
#include <QString>

#include "uiconfig.h"
#include "voicemanager.h"

class QTextEdit;
class QPushButton;
class QLabel;
class QFrame;
class QKeyEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:

    // Enrollment
    void startEnrollment();

    // VoiceManager signals
    void updateConsole(const QString& msg);
    void updateLog(const QString& msg);
    void updateStatus(const QString& text,const QString& color);
    void updateTimer(const QString& timeText);
    void updateAdminPermissions(bool isAdmin);

    void showUsersDialog(const QList<QList<QString>>& users);

    void showUserSelectionDialog(
        const QList<QList<QString>>& users
        );

private:

    UIConfig config;
    VoiceManager* manager;

    QTextEdit* consolePanel;
    QTextEdit* logPanel;

    QPushButton* viewUsersBtn;
    QPushButton* authorizeBtn;
    QPushButton* removeAuthBtn;
    QPushButton* removeUserBtn;
    QPushButton* enrollBtn;
    QPushButton* speakBtn;
    QPushButton* commandBtn;
    QPushButton* endSessionBtn;

    QLabel* titleLabel;
    QLabel* statusIndicator;
    QLabel* statusText;
    QLabel* timerLabel;

    QLabel* controlTitle;
    QLabel* consoleTitle;
    QLabel* logTitle;

    QFrame* leftFrame;
    QFrame* centerFrame;
    QFrame* rightFrame;

    void setupUI();
    void setStatus(const QString& text,const QString& color);
    void appendConsole(const QString& msg);
    void appendLog(const QString& msg);
};
