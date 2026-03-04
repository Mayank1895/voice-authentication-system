#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QJsonObject>
#include <QString>
#include <QList>

#include "uiconfig.h"

class VoiceManager : public QObject
{
    Q_OBJECT

public:
    explicit VoiceManager(QObject *parent = nullptr);

    void handleSpeakPressed();
    void handleSpacePressed();
    void handleEndSession();
    void startEnrollment(const QString &name);

    void requestAllUsers();
    void requestUsersForAuthorization();
    void requestAuthorizedUsers();
    void requestUsersForDeletion();

    void handleUserSelection(const QString &username);

signals:

    void consoleMessage(const QString &msg);
    void logMessage(const QString &msg);

    void statusChanged(const QString &text,
                       const QString &color);

    void sessionTimerUpdated(const QString &time);

    void adminPermissionChanged(bool isAdmin);

    void usersListReady(const QList<QList<QString>> &users);

    void userSelectionListReady(const QList<QList<QString>>& users);

private:

    void startBackend();
    void sendRequest(const QJsonObject &obj);

    void readBackendOutput();
    void parseLine(const QString &line);

    void handleResponse(const QJsonObject &response);

    void startSessionTimer(int seconds);

private slots:

    void handleBackendError(QProcess::ProcessError error);

    void backendStopped(int exitCode,
                        QProcess::ExitStatus status);

private:

    QProcess *process;

    QTimer sessionTimer;

    int remainingSeconds;

    bool isAdmin;

    QString currentOperation;

    UIConfig config;   // ⭐ THIS WAS MISSING
};
