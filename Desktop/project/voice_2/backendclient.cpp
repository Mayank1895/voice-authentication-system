#include "backendclient.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

BackendClient::BackendClient(QObject *parent)
    : QObject(parent)
{
    process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput,
            this, &BackendClient::readBackendOutput);

    connect(process, &QProcess::errorOccurred,
            this, &BackendClient::handleError);
}

void BackendClient::startBackend()
{
    process->start("py", QStringList() << "backend_server.py");
}

void BackendClient::sendRequest(const QJsonObject &obj)
{
    if (process->state() != QProcess::Running)
        return;

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append("\n");

    process->write(data);

}

void BackendClient::readBackendOutput()
{
    while (process->canReadLine())
    {
        QString line = process->readLine().trimmed();
        parseLine(line);
    }
}

void BackendClient::parseLine(const QString &line)
{
    if (line.startsWith("LOG::"))
    {
        QString msg = line.mid(5);
        emit logReceived(msg);
        return;
    }

    if (line.startsWith("RESP::"))
    {
        QString jsonPart = line.mid(6);

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonPart.toUtf8(), &error);

        if (error.error == QJsonParseError::NoError && doc.isObject())
        {
            emit responseReceived(doc.object());

            if (doc.object().value("status") == "backend_ready")
                emit backendReady();
        }
        else
        {
            emit backendError("Invalid JSON from backend");
        }

        return;
    }
}

void BackendClient::handleError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    emit backendError("Backend process error");
}
