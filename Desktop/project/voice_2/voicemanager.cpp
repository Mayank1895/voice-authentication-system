#include "voicemanager.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

VoiceManager::VoiceManager(QObject *parent)
    : QObject(parent),
    remainingSeconds(0),
    isAdmin(false)
{
    process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput,
            this, &VoiceManager::readBackendOutput);

    connect(process, &QProcess::readyReadStandardError,
            this, &VoiceManager::readBackendOutput);

    connect(process, &QProcess::errorOccurred,
            this, &VoiceManager::handleBackendError);

    connect(process, &QProcess::finished,
            this, &VoiceManager::backendStopped);

    connect(&sessionTimer, &QTimer::timeout, this, [=]() {

        remainingSeconds--;

        int min = remainingSeconds / 60;
        int sec = remainingSeconds % 60;

        emit sessionTimerUpdated(
            QString("%1:%2")
                .arg(min,2,10,QChar('0'))
                .arg(sec,2,10,QChar('0'))
            );

        if (remainingSeconds <= 0)
        {
            sessionTimer.stop();

            sendRequest({{"action","end_session"}});

            emit statusChanged(config.statusIdle,
                               config.statusYellow);
        }
    });

    startBackend();
}


void VoiceManager::startBackend()
{
    if(process->state() != QProcess::NotRunning)
    {
        emit consoleMessage("Backend already running.");
        return;
    }

    QString backendDir = config.backendDir;
    QString pythonExe = config.pythonExe;
    QString backendScript = backendDir + "/" + config.backendScript;

    process->setWorkingDirectory(backendDir);

    emit consoleMessage("Starting backend...");
    emit consoleMessage("Backend Dir: " + backendDir);

    process->start(pythonExe, QStringList() << backendScript);

    if(!process->waitForStarted())
    {
        emit consoleMessage("Backend failed to start.");
        emit consoleMessage(process->errorString());
        return;
    }

    emit consoleMessage("Backend process started.");
}


void VoiceManager::sendRequest(const QJsonObject &obj)
{
    if(process->state() != QProcess::Running)
    {
        emit consoleMessage("Backend not running.");
        return;
    }

    QJsonDocument doc(obj);

    QByteArray data =
        doc.toJson(QJsonDocument::Compact);

    data.append("\n");

    process->write(data);
}


void VoiceManager::readBackendOutput()
{
    while(process->canReadLine())
    {
        QString line =
            process->readLine().trimmed();

        parseLine(line);
    }
}


void VoiceManager::parseLine(const QString &line)
{
    if(line.startsWith("LOG::"))
    {
        emit consoleMessage(line.mid(5));
        return;
    }

    if(line.startsWith("AUDIT::"))
    {
        emit logMessage(line.mid(7));
        return;
    }

    if(line.startsWith("RESP::"))
    {
        QString jsonPart = line.mid(6);

        QJsonParseError error;

        QJsonDocument doc =
            QJsonDocument::fromJson(
                jsonPart.toUtf8(), &error);

        if(error.error == QJsonParseError::NoError
            && doc.isObject())
        {
            handleResponse(doc.object());
        }
        else
        {
            emit logMessage("Invalid JSON from backend.");
        }
    }
}


void VoiceManager::handleResponse(const QJsonObject &response)
{
    QString status =
        response.value("status").toString();

    if(status == "backend_ready")
    {
        emit consoleMessage("Backend Ready.");
        return;
    }

    if(status == "auth_success")
    {
        QJsonObject user =
            response["user"].toObject();

        QString role =
            user.value("role").toString();

        isAdmin = (role == "admin");

        emit adminPermissionChanged(isAdmin);

        emit statusChanged(config.statusActive,
                           config.statusGreen);

        sendRequest({{"action","start_session"}});



        return;
    }

    if(status == "session_started")
    {
        emit statusChanged("SESSION ACTIVE","#00ff88");
        startSessionTimer(120);
        return;
    }

    if(status == "session_ended")
    {
        sessionTimer.stop();

        emit statusChanged("IDLE","#ffaa00");

        emit consoleMessage("Session ended.");

        return;
    }

    if(status == "success"
        && response.contains("users"))
    {
        QJsonObject usersObj = response["users"].toObject();

        QList<QList<QString>> userList;

        for (const QString &key : usersObj.keys())
        {
            QJsonObject u = usersObj[key].toObject();

            QString role = u["role"].toString();
            QString auth = u["authorized"].toBool() ? "Yes" : "No";

            if(currentOperation == "view_users")
                userList.append({key, role, auth});

            else if(currentOperation == "authorize_user" && auth == "No")
                userList.append({key, role, auth});

            else if(currentOperation == "unauthorize_user" && auth == "Yes")
                userList.append({key, role, auth});

            else if(currentOperation == "remove_user")
                userList.append({key, role, auth});
        }

        emit userSelectionListReady(userList);
        return;
    }

    if(status == "permission_denied")
    {
        emit consoleMessage(
            "Permission denied.");
        return;
    }
}


void VoiceManager::startSessionTimer(int seconds)
{
    remainingSeconds = seconds;

    sessionTimer.start(1000);
}


void VoiceManager::handleSpeakPressed()
{
    emit consoleMessage(
        "Authentication started...");

    sendRequest({{"action","authenticate"}});
}


void VoiceManager::handleSpacePressed()
{
    sendRequest(
        {{"action","process_command"}});
}


void VoiceManager::startEnrollment(
    const QString &name)
{
    sendRequest({
        {"action","enroll"},
        {"name",name}
    });
}


void VoiceManager::requestAllUsers()
{
    sendRequest({{"action","list_users"}});
}


void VoiceManager::requestUsersForAuthorization()
{
    currentOperation = "authorize_user";

    sendRequest({{"action","list_users"}});
}


void VoiceManager::requestAuthorizedUsers()
{
    currentOperation = "unauthorize_user";

    sendRequest({{"action","list_users"}});
}


void VoiceManager::requestUsersForDeletion()
{
    currentOperation = "remove_user";

    sendRequest({{"action","list_users"}});
}


void VoiceManager::handleUserSelection(
    const QString &username)
{
    sendRequest({
        {"action",currentOperation},
        {"username",username}
    });
}
void VoiceManager::handleEndSession()
{
    sendRequest({{"action","end_session"}});
}

void VoiceManager::handleBackendError(
    QProcess::ProcessError error)
{
    Q_UNUSED(error);

    emit logMessage(
        "Backend process error.");
}


void VoiceManager::backendStopped(
    int exitCode,
    QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode)

    if(status == QProcess::CrashExit)
    {
        emit logMessage(
            "Backend process crashed.");
    }
}
