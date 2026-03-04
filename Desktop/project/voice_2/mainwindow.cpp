#include "mainwindow.h"
#include "stylemanager.h"

#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QWidget>
#include <QFrame>
#include <QScrollBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>

/* ========================================================= */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    manager = new VoiceManager(this);

    resize(config.windowWidth, config.windowHeight);
    setWindowTitle(config.windowTitle);

    setupUI();
    StyleManager::applyGlobalStyle(this, config);

    connect(manager,&VoiceManager::consoleMessage,
            this,&MainWindow::updateConsole);

    connect(manager,&VoiceManager::logMessage,
            this,&MainWindow::updateLog);

    connect(manager,&VoiceManager::statusChanged,
            this,&MainWindow::updateStatus);

    connect(manager,&VoiceManager::sessionTimerUpdated,
            this,&MainWindow::updateTimer);

    connect(manager,&VoiceManager::adminPermissionChanged,
            this,&MainWindow::updateAdminPermissions);

    connect(manager,&VoiceManager::usersListReady,
            this,&MainWindow::showUsersDialog);

    connect(manager,&VoiceManager::userSelectionListReady,
            this,&MainWindow::showUserSelectionDialog);

    setStatus("DISCONNECTED","#ff4444");
    timerLabel->setText("00:00");

    updateAdminPermissions(false);
}

/* ========================================================= */

void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    /* -------- HEADER -------- */

    QHBoxLayout *headerLayout = new QHBoxLayout();

    titleLabel = new QLabel(config.headerTitle);
    titleLabel->setAlignment(Qt::AlignCenter);

    statusIndicator = new QLabel();
    statusIndicator->setFixedSize(14,14);

    statusText = new QLabel(config.statusDisconnected);
    timerLabel = new QLabel("00:00");

    headerLayout->addStretch();
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(statusIndicator);
    headerLayout->addWidget(statusText);
    headerLayout->addSpacing(20);
    headerLayout->addWidget(timerLabel);

    mainLayout->addLayout(headerLayout);

    /* -------- BODY -------- */

    QHBoxLayout *bodyLayout = new QHBoxLayout();

    leftFrame = new QFrame();
    centerFrame = new QFrame();
    rightFrame = new QFrame();

    QVBoxLayout *leftPanel = new QVBoxLayout(leftFrame);
    QVBoxLayout *centerPanel = new QVBoxLayout(centerFrame);
    QVBoxLayout *rightPanel = new QVBoxLayout(rightFrame);

    controlTitle = new QLabel(config.sectionControl);
    consoleTitle = new QLabel(config.sectionConsole);
    logTitle = new QLabel(config.sectionLog);

    viewUsersBtn = new QPushButton(config.btnViewUsers);
    authorizeBtn = new QPushButton(config.btnAuthorize);
    removeAuthBtn = new QPushButton(config.btnRemoveAuth);
    removeUserBtn = new QPushButton(config.btnRemoveUser);
    enrollBtn = new QPushButton(config.btnEnroll);

    leftPanel->addWidget(controlTitle);
    leftPanel->addWidget(viewUsersBtn);
    leftPanel->addWidget(authorizeBtn);
    leftPanel->addWidget(removeAuthBtn);
    leftPanel->addWidget(removeUserBtn);
    leftPanel->addWidget(enrollBtn);
    leftPanel->addStretch();

    consolePanel = new QTextEdit();
    consolePanel->setReadOnly(true);

    centerPanel->addWidget(consoleTitle);
    centerPanel->addWidget(consolePanel);

    logPanel = new QTextEdit();
    logPanel->setReadOnly(true);

    rightPanel->addWidget(logTitle);
    rightPanel->addWidget(logPanel);

    bodyLayout->addWidget(leftFrame,1);
    bodyLayout->addWidget(centerFrame,3);
    bodyLayout->addWidget(rightFrame,1);

    mainLayout->addLayout(bodyLayout);

    /* -------- ACTION BUTTONS -------- */

    QHBoxLayout *actionLayout = new QHBoxLayout();

    speakBtn = new QPushButton(config.btnSpeak);
    commandBtn = new QPushButton("COMMAND");
    endSessionBtn = new QPushButton("END SESSION");

    speakBtn->setMinimumHeight(config.speakButtonHeight);
    commandBtn->setMinimumHeight(config.speakButtonHeight);
    endSessionBtn->setMinimumHeight(config.speakButtonHeight);

    speakBtn->setMinimumWidth(200);
    commandBtn->setMinimumWidth(200);
    endSessionBtn->setMinimumWidth(200);

    /* disable until session starts */

    commandBtn->setEnabled(false);
    endSessionBtn->setEnabled(false);

    actionLayout->addStretch();
    actionLayout->addWidget(speakBtn);
    actionLayout->addWidget(commandBtn);
    actionLayout->addWidget(endSessionBtn);
    actionLayout->addStretch();

    mainLayout->addLayout(actionLayout);

    /* -------- Button Connections -------- */

    connect(speakBtn,&QPushButton::clicked,
            manager,&VoiceManager::handleSpeakPressed);

    connect(commandBtn,&QPushButton::clicked,
            manager,&VoiceManager::handleSpacePressed);

    connect(endSessionBtn,&QPushButton::clicked,
            manager,&VoiceManager::handleEndSession);

    connect(viewUsersBtn,&QPushButton::clicked,
            manager,&VoiceManager::requestAllUsers);

    connect(authorizeBtn,&QPushButton::clicked,
            manager,&VoiceManager::requestUsersForAuthorization);

    connect(removeAuthBtn,&QPushButton::clicked,
            manager,&VoiceManager::requestAuthorizedUsers);

    connect(removeUserBtn,&QPushButton::clicked,
            manager,&VoiceManager::requestUsersForDeletion);

    connect(enrollBtn,&QPushButton::clicked,
            this,&MainWindow::startEnrollment);
}
/* ========================================================= */
/* -------- Manager → UI Slots -------- */

void MainWindow::updateConsole(const QString &msg)
{
    appendConsole(msg);
}

void MainWindow::updateLog(const QString &msg)
{
    appendLog(msg);
}

void MainWindow::updateStatus(const QString &text,const QString &color)
{
    setStatus(text,color);

    /* SESSION ACTIVE */

    if(text == config.statusActive)
    {
        speakBtn->setEnabled(false);
        commandBtn->setEnabled(true);
        endSessionBtn->setEnabled(true);

        appendConsole("Session active. Press COMMAND to issue commands.");
    }

    /* SESSION ENDED / IDLE */

    if(text == config.statusIdle)
    {
        speakBtn->setEnabled(true);
        commandBtn->setEnabled(false);
        endSessionBtn->setEnabled(false);

        appendConsole("Session ended. Press SPEAK to authenticate.");
    }
}

void MainWindow::updateTimer(const QString &timeText)
{
    timerLabel->setText(timeText);
}

void MainWindow::updateAdminPermissions(bool isAdmin)
{
    authorizeBtn->setEnabled(isAdmin);
    removeAuthBtn->setEnabled(isAdmin);
    removeUserBtn->setEnabled(isAdmin);
    enrollBtn->setEnabled(true);
}

/* ========================================================= */
/* -------- Enrollment -------- */

void MainWindow::startEnrollment()
{
    bool ok;

    QString name = QInputDialog::getText(
        this,
        "Enrollment",
        "Enter New User Name:",
        QLineEdit::Normal,
        "",
        &ok
        );

    if(ok && !name.isEmpty())
    {
        appendConsole("Enrollment request sent for: " + name);
        manager->startEnrollment(name);
    }
}

/* ========================================================= */
/* -------- Users Dialog -------- */
void MainWindow::showUsersDialog(const QList<QList<QString>>& users)
{
    if(users.isEmpty())
    {
        appendLog("No users available.");
        return;
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Users");
    dialog->resize(500,350);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    QTableWidget *table = new QTableWidget(users.size(),3,dialog);

    table->setHorizontalHeaderLabels({"Name","Role","Authorized"});
    table->horizontalHeader()->setStretchLastSection(true);

    table->verticalHeader()->setDefaultSectionSize(30);
    table->verticalHeader()->setMinimumWidth(40);

    table->setColumnWidth(0,160);
    table->setColumnWidth(1,120);

    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for(int i=0;i<users.size();i++)
    {
        table->setItem(i,0,new QTableWidgetItem(users[i][0]));
        table->setItem(i,1,new QTableWidgetItem(users[i][1]));
        table->setItem(i,2,new QTableWidgetItem(users[i][2]));
    }

    layout->addWidget(table);

    dialog->exec();
}

void MainWindow::showUserSelectionDialog(const QList<QList<QString>>& users)
{
    if(users.isEmpty())
    {
        appendLog("No users available.");
        return;
    }

    // Create dialog as pointer
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("select user");
    dialog->resize(500,350);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(15,15,15,15);
    layout->setSpacing(10);

    QTableWidget *table = new QTableWidget(users.size(),3,dialog);

    table->setHorizontalHeaderLabels({"Name","Role","Authorized"});
    table->horizontalHeader()->setStretchLastSection(true);

    table->verticalHeader()->setDefaultSectionSize(30);
    table->verticalHeader()->setMinimumWidth(40);

    table->setColumnWidth(0,160);
    table->setColumnWidth(1,120);

    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for(int i=0;i<users.size();i++)
    {
        table->setItem(i,0,new QTableWidgetItem(users[i][0]));
        table->setItem(i,1,new QTableWidgetItem(users[i][1]));
        table->setItem(i,2,new QTableWidgetItem(users[i][2]));
    }

    QPushButton *actionBtn = new QPushButton("confirm",dialog);
    actionBtn->setMinimumHeight(35);
    actionBtn->setMinimumWidth(120);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(actionBtn);
    btnLayout->addStretch();

    layout->addWidget(table);
    layout->addLayout(btnLayout);

    // Button logic
    connect(actionBtn,&QPushButton::clicked,this,[=](){

        int row = table->currentRow();

        if(row < 0)
        {
            QMessageBox::warning(this,"Selection","Please select a user.");
            return;
        }

        QString username = table->item(row,0)->text();

        manager->handleUserSelection(username);

        dialog->accept();   // Works now because dialog is pointer
    });

    dialog->exec();
}
/* ========================================================= */

void MainWindow::setStatus(const QString &text,const QString &color)
{
    statusText->setText(text);

    statusIndicator->setStyleSheet(
        QString("background-color:%1;border-radius:7px;").arg(color)
        );
}

void MainWindow::appendConsole(const QString &msg)
{
    consolePanel->append(msg);
    consolePanel->verticalScrollBar()->setValue(
        consolePanel->verticalScrollBar()->maximum()
        );
}

void MainWindow::appendLog(const QString &msg)
{
    logPanel->append(msg);
    logPanel->verticalScrollBar()->setValue(
        logPanel->verticalScrollBar()->maximum()
        );
}

/* ========================================================= */

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Space)
    {
        if(statusText->text() == config.statusActive)
        {
            manager->handleSpacePressed();
        }
        else
        {
            appendConsole("Authenticate first using SPEAK.");
        }
    }

    QMainWindow::keyPressEvent(event);
}
