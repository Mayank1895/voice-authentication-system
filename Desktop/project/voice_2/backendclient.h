#pragma once

#include <QObject>
#include <QProcess>
#include <QJsonObject>

class BackendClient : public QObject
{
    Q_OBJECT

public:
    explicit BackendClient(QObject *parent = nullptr);

    void startBackend();
    void sendRequest(const QJsonObject &obj);

signals:
    void logReceived(const QString &message);
    void responseReceived(const QJsonObject &response);
    void backendReady();
    void backendError(const QString &error);

private slots:
    void readBackendOutput();
    void handleError(QProcess::ProcessError error);

private:
    QProcess *process;
    void parseLine(const QString &line);
};
