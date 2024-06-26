#include "mainwindow.h"
#include "networkmanager.h"
#include <QVBoxLayout>
#include <QMediaPlayer>
#include <QUrl>
#include <QPixmap>
#include <QHostAddress>
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QString>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      connectionEstablished(false)
{
    mediaPlayer = new QMediaPlayer(this);
    buttonClickSound = new QSoundEffect(this);

    stackedWidget = new QStackedWidget(this);
    mainMenuWidget = new QWidget(this);
    multiplayerMenuWidget = new QWidget(this);
    hostMenuWidget = new QWidget(this);
    joinMenuWidget = new QWidget(this);
    backgroundLabel = new QLabel(this);
    gameListWidget = new QListWidget(joinMenuWidget);
    GameoverWidget = new QWidget(this);
    networkManager = new NetworkManager(this);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();
    buttonWidth = screenWidth * 0.2;

//    QPixmap backgroundPixmap("://spaceRaceREII313/menus/mainmenuscreen2.jpg");
//    backgroundLabel->setPixmap(backgroundPixmap);
//    backgroundLabel->setFixedSize(screenWidth+1, screenHeight+1);
//    backgroundLabel->lower();



    setCentralWidget(stackedWidget);
    setupMainMenu();
    setupMultiplayerMenu();
    setupHostMenu();
    setupJoinMenu();
    setupGameover(60000,"w");

    stackedWidget->addWidget(mainMenuWidget);
    stackedWidget->addWidget(multiplayerMenuWidget);
    stackedWidget->addWidget(hostMenuWidget);
    stackedWidget->addWidget(joinMenuWidget);
    stackedWidget->addWidget(GameoverWidget);

    showMainMenu();
    //showGameover();
    //startGame();

    changeBackgroundMusic("://spaceRaceREII313/menus/background.mp3");
    buttonClickSound->setSource(QUrl("://spaceRaceREII313/menus/buttonclick.mp3"));
    buttonClickSound->setVolume(1.00);

    // Connect network manager signals
    connect(networkManager, &NetworkManager::handshakeRequestReceived, this, &MainWindow::onHandshakeRequestReceived);
    connect(networkManager, &NetworkManager::handshakeAccepted, this, &MainWindow::onHandshakeAccepted);
    connect(networkManager, &NetworkManager::handshakeRejected, this, &MainWindow::onHandshakeRejected);
    connect(networkManager, &NetworkManager::availableGamesChanged, this, &MainWindow::handleAvailableGamesChanged);
    connect(networkManager, &NetworkManager::zandersuperfunctionDataRecieved, this, &MainWindow::getszandersuperfunctionDatafromotherplayer);
    //    connect(networkManager, &NetworkManager::sendProjectileData, this, &MainWindow::receiveProjectileData);
    //connect(networkManager, &NetworkManager::connectionError, this, &MainWindow::onConnectionError);

}

MainWindow::~MainWindow()
{
}

void MainWindow::setupHostMenu()
{
    QVBoxLayout *layout = new QVBoxLayout(hostMenuWidget);
    player = 1;

    QPushButton *backButton = new QPushButton("Back", hostMenuWidget);
    backButton->setFixedWidth(buttonWidth);
    layout->addWidget(backButton);
    layout->setAlignment(Qt::AlignCenter);

    connect(backButton, &QPushButton::clicked, [this]() {
        showMultiplayerMenu();
    });
}

void MainWindow::setupJoinMenu()
{
    QPushButton *backButton = new QPushButton("Back", joinMenuWidget);
    QPushButton *refreshButton = new QPushButton("Refresh", joinMenuWidget);
    backButton->setFixedWidth(buttonWidth);
    refreshButton->setFixedWidth(buttonWidth);
    gameListWidget->setFixedWidth(buttonWidth*2);

    QVBoxLayout *layout = new QVBoxLayout(joinMenuWidget);
    layout->addWidget(gameListWidget);
    layout->addWidget(refreshButton);
    layout->addWidget(backButton);
    layout->setAlignment(Qt::AlignCenter);

    connect(backButton, &QPushButton::clicked, this, &MainWindow::showMultiplayerMenu);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::updateGameList);
}

void MainWindow::updateGameList()
{
    gameListWidget->clear();
    QStringList availableGames = networkManager->getAvailableGames();
    gameListWidget->addItems(availableGames);

    qDebug() << "Game list updated. Available games:" << availableGames;

    connect(gameListWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        onGameSelected(item->text());
    });
}

void MainWindow::onGameSelected(const QString &hostAddress)
{
    player = 2;
    networkManager->sendHandshakeRequest(hostAddress);
}

void MainWindow::onHandshakeRequestReceived(const QString &clientAddress)
{
    mapSeed = hostMapGenerator();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "New Player Request", "Do you want to accept the player from " + clientAddress + "?",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {

        QString codeword = QDateTime::currentDateTime().toString(Qt::ISODate);
        networkManager->sendHandshakeResponse(clientAddress, true, codeword, mapSeed);
        this->codeword = codeword;
        connectionEstablished = true; // Set the flag to true
        qDebug() << "Handshake accepted with" << clientAddress;
        startMultiplayerGame();
    } else {
        networkManager->sendHandshakeResponse(clientAddress, false, QString(), "");
        qDebug() << "Handshake rejected with" << clientAddress;
    }
}

void MainWindow::onHandshakeAccepted(const QString &codeword, const QString &mapseed)
{
    networkManager->stopBroadcasting();
    networkManager->sendMapSeedToPlayer2(mapseed);
    mapSeed = mapseed;
    this->codeword = codeword;
    connectionEstablished = true; // Set the flag to true
    QMessageBox::information(this, "Handshake", "Connection established. You are now connected.");
    startMultiplayerGame();
}

void MainWindow::onHandshakeRejected()
{
    QMessageBox::information(this, "Handshake", "Connection rejected by the host.");
}

void MainWindow::handleAvailableGamesChanged()
{
    qDebug() << "Available games changed signal received.";
    updateGameList();
}

void MainWindow::onConnectionError(const QString &message)
{
    connectionEstablished = false; // Set the flag to false
    qDebug() << "Connection Error:" << message;
}

void MainWindow::showMultiplayerMenu()
{
    stackedWidget->setCurrentWidget(multiplayerMenuWidget);
}

void MainWindow::showMainMenu()
{
    networkManager->stopBroadcasting();
    stackedWidget->setCurrentWidget(mainMenuWidget);
}

void MainWindow::showHostMenu()
{
    networkManager->startBroadcasting();
    stackedWidget->setCurrentWidget(hostMenuWidget);
}

void MainWindow::showJoinMenu()
{
    stackedWidget->setCurrentWidget(joinMenuWidget);
    updateGameList();
}

void MainWindow::startGame()
{
    removeBackgroundImage();
    stackedWidget->hide();
    QWidget *gameCentralWidget = new QWidget();
    setCentralWidget(gameCentralWidget);
    initializeApplication();

}

void MainWindow::showGameover()
{
    qDebug() << "Showing Gameover widget";
    stackedWidget->setCurrentWidget(GameoverWidget);
    qDebug() << "Gameover widget should be visible now";
}

void MainWindow::setupMainMenu()
{
    QLabel *gameNameLabel = new QLabel("Spacerace", mainMenuWidget);
    QFont font = gameNameLabel->font();
    font.setPointSize(screenWidth*0.1);
    font.setBold(true);    // Set the font to bold
    gameNameLabel->setFont(font);
    gameNameLabel->setAlignment(Qt::AlignCenter);
    QPalette palette = gameNameLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::red); // Set the text color to red
    gameNameLabel->setPalette(palette);

    QPushButton *singlePlayerButton = new QPushButton("SINGLE", mainMenuWidget);
    singlePlayerButton->setFixedWidth(buttonWidth);
    QPushButton *multiplayerButton = new QPushButton("VS MODE", mainMenuWidget);
    multiplayerButton->setFixedWidth(buttonWidth);
    QPushButton *exitButton = new QPushButton("exit game", mainMenuWidget);
    exitButton->setFixedWidth(buttonWidth);

    QVBoxLayout *layout = new QVBoxLayout(mainMenuWidget);
    layout->addWidget(gameNameLabel);
    layout->addWidget(singlePlayerButton);
    layout->addWidget(multiplayerButton);
    layout->addWidget(exitButton);
    layout->setAlignment(Qt::AlignCenter);

    connect(singlePlayerButton, &QPushButton::clicked, this, &MainWindow::showGameover);
    connect(singlePlayerButton, &QPushButton::clicked, this, &MainWindow::playButtonClickSound);
    connect(multiplayerButton, &QPushButton::clicked, this, &MainWindow::showMultiplayerMenu);
    connect(multiplayerButton, &QPushButton::clicked, this, &MainWindow::playButtonClickSound);
    connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);
}

void MainWindow::processScoreEntry(QLineEdit *nameInput, int currentScore, QVector<ScoreEntry> &scores, QLabel *top3ScoresLabel)
{
    qDebug() << "Entering processScoreEntry function";
    QString playerName = nameInput->text();
    if (!playerName.isEmpty()) {
        qDebug() << "Player name is not empty:" << playerName;

        // Debugging ScoreEntry creation
        qDebug() << "Creating new score entry";
        ScoreEntry newScore = { playerName, currentScore, QDateTime::currentDateTime() };
        qDebug() << "New score entry created:" << newScore.name << newScore.score << newScore.date.toString();

        // Debugging scores appending
        qDebug() << "Appending new score to scores";
        scores.append(newScore);
        qDebug() << "New score appended. Scores size:" << scores.size();

        // Debugging sorting
        qDebug() << "Sorting scores";
        std::sort(scores.begin(), scores.end(), [](const ScoreEntry &a, const ScoreEntry &b) {
            return a.score > b.score;
        });
        qDebug() << "Scores sorted";

        // Debugging resizing
        if (scores.size() > 10) {
            qDebug() << "Resizing scores to top 10";
            scores.resize(10);
        }
        qDebug() << "Scores resized if needed. Scores size:" << scores.size();

        // Debugging save to file
        qDebug() << "Saving scores to file";
        saveScoresToFile("://menus/scores.txt", scores);  // Ensure writable file path
        qDebug() << "Scores saved to file";

        // Debugging label update
        qDebug() << "Updating top3ScoresLabel";
        updateTop3ScoresLabel(scores, top3ScoresLabel);
        qDebug() << "Top3ScoresLabel updated";
    } else {
        qDebug() << "Player name is empty. Skipping score entry.";
    }
    qDebug() << "Exiting processScoreEntry function";
}

void MainWindow::setupGameover(int currentScore, const QString &defaultName)
{
    qDebug() << "Setting up Gameover widget with current score:" << currentScore;

    // Clear any existing layout and widgets
    QLayout *existingLayout = GameoverWidget->layout();
    if (existingLayout) {
        QLayoutItem *item;
        while ((item = existingLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete existingLayout;
    }

    // Load and display the picture at the top
    QLabel *gameOverLabel = new QLabel("Game Over", GameoverWidget);
    QFont font = gameOverLabel->font();
    font.setPointSize(screenWidth*0.05); // Set the font size
    font.setBold(true);    // Set the font to bold
    gameOverLabel->setFont(font);
    gameOverLabel->setAlignment(Qt::AlignCenter);

    QPalette palette = gameOverLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::red); // Set the text color to red
    gameOverLabel->setPalette(palette);

    // Create a layout
    QVBoxLayout *layout = new QVBoxLayout(GameoverWidget);
    layout->addWidget(gameOverLabel);

    // Read scores from a text file
    QVector<ScoreEntry> scores = readScoresFromFile("://spaceRaceREII313/menus/scores.txt");  // Ensure writable file path
    std::sort(scores.begin(), scores.end(), [](const ScoreEntry &a, const ScoreEntry &b) {
        return a.score > b.score;
    });

    bool higherThanThird = false;

    if (scores.size() >= 3) {
        if (currentScore > scores[2].score) {
            higherThanThird = true;
        }
    }

    if(defaultName=="w")
    {
        QLabel *Winlabel = new QLabel("YOU WIN", GameoverWidget);
        QFont font = Winlabel->font();
        font.setPointSize(screenWidth*0.15); // Set the font size
        font.setBold(true);    // Set the font to bold
        Winlabel->setFont(font);
        Winlabel->setAlignment(Qt::AlignCenter);
        Winlabel->setPalette(palette);
        layout->addWidget(Winlabel);

        QLabel *hscorelabel = new QLabel("NEW HIGH SCORE", GameoverWidget);
        QFont font2 = hscorelabel->font();
        font2.setPointSize(screenWidth*0.05); // Set the font size
        hscorelabel->setFont(font2);
        hscorelabel->setAlignment(Qt::AlignCenter);
        hscorelabel->setPalette(palette);

        QLabel *nameLabel = new QLabel("Enter your name:", GameoverWidget);
        QFont font3 = nameLabel->font();
        font3.setPointSize(screenWidth*0.05); // Set the font size
        nameLabel->setFont(font3);
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setPalette(palette);

        if(higherThanThird){
            layout->addWidget(nameLabel);
            layout->addWidget(hscorelabel);
            QLineEdit *nameInput = new QLineEdit(GameoverWidget);
            layout->addWidget(nameInput);
            connect(nameInput, &QLineEdit::editingFinished, this, &MainWindow::saveScore); // Connect the input to saveScore slot
            nameInput->setProperty("currentScore", currentScore); // Store current score in the widget's property
        }

        // Display the top 3 highest scores
        QLabel *top3ScoresLabel = new QLabel(GameoverWidget);
        QFont font4 = top3ScoresLabel->font();
        font4.setPointSize(screenWidth*0.012); // Set the font size
        top3ScoresLabel->setFont(font4);
        top3ScoresLabel->setAlignment(Qt::AlignCenter);
        top3ScoresLabel->setPalette(palette);
        updateTop3ScoresLabel(scores, top3ScoresLabel); // Helper function to update scores label
        layout->addWidget(top3ScoresLabel);
    }
    else
    {
        QLabel *Loselabel = new QLabel("YOU LOSE", GameoverWidget);
        QFont font = Loselabel->font();
        font.setPointSize(200); // Set the font size
        font.setBold(true);    // Set the font to bold
        Loselabel->setFont(font);
        Loselabel->setAlignment(Qt::AlignCenter);
        QPalette palette2 = Loselabel->palette();
        palette2.setColor(QPalette::WindowText, Qt::darkYellow); // Set the text color to red
        Loselabel->setPalette(palette2);
        layout->addWidget(Loselabel);
    }


    // Add Play Again and Return to Main Menu buttons
    QPushButton *playAgainButton = new QPushButton("Play Again", GameoverWidget);
    playAgainButton->setFixedWidth(buttonWidth);
    QPushButton *returnToMainMenuButton = new QPushButton("Return to Main Menu", GameoverWidget);
    returnToMainMenuButton->setFixedWidth(buttonWidth);
    layout->addWidget(playAgainButton);
    layout->addWidget(returnToMainMenuButton);

    // Connect buttons to their respective slots
    connect(playAgainButton, &QPushButton::clicked, this, &MainWindow::startGame);
    connect(playAgainButton, &QPushButton::clicked, this, &MainWindow::playButtonClickSound);
    connect(returnToMainMenuButton, &QPushButton::clicked, this, &MainWindow::showMainMenu);
    connect(returnToMainMenuButton, &QPushButton::clicked, this, &MainWindow::playButtonClickSound);

    // Save score when the player enters their name


    GameoverWidget->setLayout(layout);
    qDebug() << "Gameover widget setup completed";
}

void MainWindow::updateTop3ScoresLabel(const QVector<ScoreEntry> &scores, QLabel *label)
{
    QString text = "Top 3 Scores:\n";
    for (int i = 0; i < std::min(3, scores.size()); ++i) {
        text += QString("%1. %2 - %3\n").arg(i + 1).arg(scores[i].name).arg(scores[i].score);
    }
    label->setText(text);
    qDebug() << "Updated top3ScoresLabel with text:" << text;
}

QVector<ScoreEntry> MainWindow::readScoresFromFile(const QString &filePath)
{
    QVector<ScoreEntry> scores;
    QFile file(filePath);
    if (!file.exists()) {
        qDebug() << "File does not exist:" << filePath;
        return scores;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(",");
            if (parts.size() == 3) {
                ScoreEntry entry;
                entry.name = parts[0];
                entry.score = parts[1].toInt();
                entry.date = QDateTime::fromString(parts[2], Qt::ISODate);
                scores.append(entry);
            }
        }
        file.close();
    } else {
        qDebug() << "Unable to open file for reading:" << filePath;
    }

    return scores;
}

void MainWindow::saveScore()
{
    QLineEdit *nameInput = qobject_cast<QLineEdit*>(sender());
    if (!nameInput) return;

    QString playerName = nameInput->text();
    if (playerName.isEmpty()) return;

    int currentScore = nameInput->property("currentScore").toInt();
    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);

    QString scoreEntry = QString("%1,%2,%3\n").arg(playerName).arg(currentScore).arg(timestamp);

    QFile file("://spaceRaceREII313/menus/scores.txt");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << scoreEntry;
        file.close();
        qDebug() << "Score saved:" << scoreEntry;
    } else {
        qDebug() << "Failed to open scores file for writing.";
    }
}

void MainWindow::saveScoresToFile(const QString &filePath, const QVector<ScoreEntry> &scores)
{
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const ScoreEntry &entry : scores) {
            out << entry.name << "," << entry.score << "," << entry.date.toString(Qt::ISODate) << "\n";
        }
        file.close();
    } else {
        qDebug() << "Unable to open file for writing:" << filePath;
    }
}

void MainWindow::setupMultiplayerMenu()
{
    QLabel *multiLabel = new QLabel("VS MODE", mainMenuWidget);
    QFont font = multiLabel->font();
    font.setPointSize(screenWidth*0.1);
    font.setBold(true);    // Set the font to bold
    multiLabel->setFont(font);
    multiLabel->setAlignment(Qt::AlignCenter);
    QPalette palette = multiLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::red); // Set the text color to red
    multiLabel->setPalette(palette);

    QPushButton *hostButton = new QPushButton("Host", multiplayerMenuWidget);
    QPushButton *joinButton = new QPushButton("Join", multiplayerMenuWidget);
    QPushButton *backButton = new QPushButton("Back", multiplayerMenuWidget);
    hostButton->setFixedWidth(buttonWidth);
    joinButton->setFixedWidth(buttonWidth);
    backButton->setFixedWidth(buttonWidth);

    QVBoxLayout *layout = new QVBoxLayout(multiplayerMenuWidget);
    layout->addWidget(hostButton);
    layout->addWidget(joinButton);
    layout->addWidget(backButton);
    layout->setAlignment(Qt::AlignCenter);

    connect(hostButton, &QPushButton::clicked, this, &MainWindow::showHostMenu);
    connect(hostButton, &QPushButton::clicked, this, &MainWindow::playButtonClickSound);
    connect(joinButton, &QPushButton::clicked, this, &MainWindow::showJoinMenu);
    connect(joinButton, &QPushButton::clicked, this, &MainWindow::playButtonClickSound);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::showMainMenu);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::playButtonClickSound);
}

void MainWindow::changeBackgroundMusic(const QString &filePath)
{
    mediaPlayer->setMedia(QUrl(filePath));
    mediaPlayer->setVolume(0);
    mediaPlayer->play();
}

void MainWindow::playButtonClickSound()
{
    buttonClickSound->play();
}

void MainWindow::removeBackgroundImage()
{
    backgroundLabel->clear();
}

QString MainWindow::hostMapGenerator()
{
    ////if player 1 run this
    ///
    QString seed = "wasdwadsddddddddddddddaaaaaaaaaaaawwwwwwwwwwwwwwwdddddddddddaaaaaaaaasssssssssssswwwwwwwwwwwwwwddddddddsadwsadwsssssssssssswwwwwddddddddddaaaaaaaaaaawwwwwwwwwwsssssssssddddddddddddddddwdssssssssssssssaaaaaaaaaaawwwwwwddddddddddwwwwwwwwwaaaaaaaawwwwwwddddddwasdwasdsssssssssssssssssssssadwwwwwwsdasssssssssssssssssaaaaaaaaaaaaaaaawdsadwad";
    return seed;
}

void MainWindow::startMultiplayerGame()
{
    networkManager->stopBroadcasting();
    removeBackgroundImage();
    stackedWidget->hide();
    QWidget *gameCentralWidget = new QWidget();
    setCentralWidget(gameCentralWidget);
    initializeMultiplayerApplication();

}

void MainWindow::initializeApplication()
{
    view = new QGraphicsView(this);
    view->setParent(this);



    player1Ship = new playerShip();
    enemy1 = new enemy();
    enemy2 = new enemy();
    enemy3 = new enemy();
    enemy4 = new enemy();
    /*if(enemy1->drop == ""){


       }*/

    QImage asteroidTiles = QImage(":/spaceRaceREII313/mapElements/customAsteroidBack.png");

    if (asteroidTiles.isNull()) {
        qDebug() << "Failed to load background image file";
    }

    QBrush backgroundBrush(asteroidTiles);//asteroidTiles
    scene.setBackgroundBrush(backgroundBrush);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    scene.setSceneRect(-5000, -5000, 10000, 10000);

    // Add a few elements to the scene (for testing purposes)
    scene.addRect(500, 500, 200, 200, QPen(Qt::blue), QBrush(Qt::blue));
    scene.addEllipse(800, 800, 100, 100, QPen(Qt::red), QBrush(Qt::red));
    scene.addRect(0, 100, 50, 100, QPen(Qt::green), QBrush(Qt::green));
    scene.addRect(-300, -300, 500, 500, QPen(QColor(0, 0, 128)), QBrush(QColor(0, 0, 128)));

    // Create an instance of the playerShip class
    player1Ship->setFlag(QGraphicsItem::ItemClipsToShape, false);
    player1Ship->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
    scene.addItem(player1Ship);

    mapFeature *feature1 = new mapFeature(scene.sceneRect(), "HOST");
    scene.addItem(feature1);

    enemyTargetPos = player1Ship->getPosition();

    enemy1->setPos(300, 300);
    scene.addItem(enemy1);

    enemy2->setPos(400, 400);
    scene.addItem(enemy2);

    enemy3->setPos(500, 500);
    scene.addItem(enemy3);

    enemy4->setPos(600,600);
    scene.addItem(enemy4);

   /* enemy2->setPos(300, 300);
    scene.addItem(enemy2);

    enemy3->setPos(300, 300);
    scene.addItem(enemy3);

    enemy4->setPos(300, 300);
    scene.addItem(enemy4);*/

    // Create and add shipAugment instances
    shipAugment *augment1 = new shipAugment();
    augment1->setPos(-200, 0);
    augment1->setType("Shield");
    scene.addItem(augment1);

    shipAugment *augment2 = new shipAugment();
    augment2->setPos(-200, -100);
    augment2->setType("Blaster");
    scene.addItem(augment2);

    shipAugment *augment3 = new shipAugment();
    augment3->setPos(-200, 100);
    augment3->setType("Thruster");
    scene.addItem(augment3);

    shipAugment *augment4 = new shipAugment();
    augment4->setPos(-200, 110);
    augment4->setType("Thruster");
    scene.addItem(augment4);

    shipAugment *augment5 = new shipAugment();
    augment5->setPos(-200, 110);
    augment5->setType("Blaster");
    scene.addItem(augment5);

    // Set focus to the playerShip item
    player1Ship->setFlag(QGraphicsItem::ItemIsFocusable);
    player1Ship->setPos(0,0);
    player1Ship->setFocus();

    // Create a QGraphicsView and set the scene
    view->setScene(&scene);
    view->setWindowTitle("Space Battle Adventure Bonanza"); // Set window title

    // Set the initial view size
    const int viewWidth = screenWidth;
    const int viewHeight = screenHeight;
    view->resize(viewWidth, viewHeight); // Set window size
    view->centerOn(player1Ship->getPosition());

    timer.setInterval(5); // Update every millisecond

    connect(&timer, &QTimer::timeout, this, [this]() {
        if(enemy1->drop == "Blaster"){

            shipAugment *augment1 = new shipAugment();
            augment1->setPos(enemy1->pos());
            augment1->setType("Blaster");
            scene.addItem(augment1);
            qDebug() << "added drop : " << enemy1->drop;
            enemy1->drop = "";

        }
        scene.advance(); // Call advance to drive animation and collision detection
        enemy1->updatePosition(enemyTargetPos);
        enemy2->updatePosition(enemyTargetPos);
        enemy3->updatePosition(enemyTargetPos);
        enemy4->updatePosition(enemyTargetPos);

        enemyTargetPos = player1Ship->getPosition();
        view->centerOn(player1Ship->getPosition());
        scene.update();
        //std::cout << "playerShip Position: " << player1Ship->pos().x() << ", " << player1Ship->pos().y() << std::endl;
    });
    timer.start();

    view->show();

    /*setupGameover(int currentscore, "w"or"l")
     * showGameoveer();
*/
}

void MainWindow::initializeMultiplayerApplication()
{
    view = new QGraphicsView(this);
    view->setParent(this);

    player1Ship = new playerShip();
    player2Ship = new playerShip(); // Create a second player ship

    QImage asteroidTiles = QImage(":/spaceRaceREII313/mapElements/customAsteroidBack.png");

    if (asteroidTiles.isNull()) {
        qDebug() << "Failed to load background image file";
    }

    QBrush backgroundBrush(asteroidTiles); // asteroidTiles
    scene.setBackgroundBrush(backgroundBrush);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    scene.setSceneRect(-5000, -5000, 10000, 10000);

    // Add a few elements to the scene (for testing purposes)
    scene.addRect(500, 500, 200, 200, QPen(Qt::blue), QBrush(Qt::blue));
    scene.addEllipse(800, 800, 100, 100, QPen(Qt::red), QBrush(Qt::red));
    scene.addRect(0, 100, 50, 100, QPen(Qt::green), QBrush(Qt::green));
    scene.addRect(-300, -300, 500, 500, QPen(QColor(0, 0, 128)), QBrush(QColor(0, 0, 128)));



    player2Ship->setFlag(QGraphicsItem::ItemClipsToShape, false);
    player2Ship->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
    scene.addItem(player2Ship);

    // Initialize player ships
    player1Ship->setFlag(QGraphicsItem::ItemClipsToShape, false);
    player1Ship->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
    scene.addItem(player1Ship);

    // Position the ships
    player1Ship->setPos(0, 0);
    player2Ship->setPos(0, 0); // Example position, adjust as needed

    mapFeature *feature1 = new mapFeature(scene.sceneRect(), mapSeed);
    scene.addItem(feature1);

    // Create and add shipAugment instances
    shipAugment *augment1 = new shipAugment();
    augment1->setPos(-200, 0);
    augment1->setType("Shield");
    scene.addItem(augment1);

    shipAugment *augment2 = new shipAugment();
    augment2->setPos(-200, -100);
    augment2->setType("Blaster");
    scene.addItem(augment2);

    shipAugment *augment3 = new shipAugment();
    augment3->setPos(-200, 100);
    augment3->setType("Thruster");
    scene.addItem(augment3);

    shipAugment *augment4 = new shipAugment();
    augment4->setPos(-200, 110);
    augment4->setType("Thruster");
    scene.addItem(augment4);

    shipAugment *augment5 = new shipAugment();
    augment5->setPos(-200, 110);
    augment5->setType("Blaster");
    scene.addItem(augment5);

    // Set focus to the player ship based on the player variable
    if(player == 1){
    player1Ship->setFlag(QGraphicsItem::ItemIsFocusable);} else if(player==2){
    player2Ship->setFlag(QGraphicsItem::ItemIsFocusable);}

    // Debugging output
    qDebug() << "Player value: " << player;

    if (player == 1) {
        player1Ship->setFocus();
        qDebug() << "Player 1 ship focused.";
    } else if (player == 2) {
        player2Ship->setFocus();
        qDebug() << "Player 2 ship focused.";
    } else {
        qDebug() << "Unknown player value.";
    }

    // Create a QGraphicsView and set the scene
    view->setScene(&scene);
    view->setWindowTitle("Space Battle Adventure Bonanza"); // Set window title

    // Set the initial view size
    const int viewWidth = screenWidth;
    const int viewHeight = screenHeight;
    view->resize(viewWidth, viewHeight);

    // Center the view on the appropriate player ship

    timer.setInterval(5); // Update every millisecond //5

    connect(&timer, &QTimer::timeout, this, [this]() {
        scene.advance(); // Call advance to drive animation and collision detection
        // Center the view on the appropriate player ship
        //preparezandersuperfunctionDatatosendtherplayer();
        if (player == 1) {
            view->centerOn(player1Ship);
        } else if (player == 2) {
            view->centerOn(player2Ship);
        }
        scene.update();
        preparezandersuperfunctionDatatosendtherplayer();
    });


    timer.start();
    view->show();
}


//void MainWindow::getszandersuperfunctionDatafromotherplayer(const QString &data)
//{
////    QDataStream stream(data);
//    QString datatype;
////    stream >> dataType;

//    if (dataType == "zandersuperfunctionDataRecieved") {
//        QString posX, posY;
//        QString angle;
////        stream >> posX >> posY >> angle;

//        qDebug() << "Projectile data received - posX:" << posX << "posY:" << posY << "angle:" << angle;
//    }
//}

void MainWindow::getszandersuperfunctionDatafromotherplayer(const QString &data)
{
    QString dataType = "zandersuperfunctionDataRecieved";
    if (data.startsWith(dataType)) {
        QStringList parts = data.mid(dataType.length()).trimmed().split(' ');
        if (parts.size() == 6) {
            QString shipPosX = parts[0];
            QString shipPosY = parts[1];
            QString shipAngle = parts[2];
            QString posX = parts[3];
            QString posY = parts[4];
            QString angle = parts[5];

            QPointF shipPos(shipPosX.toInt(),shipPosY.toInt());
            int intShipAngle = shipAngle.toInt();
            qDebug() << "Printing this sexy data: " << shipPos;

            if(player == 1){
            player2Ship->setPos(shipPos);
            player2Ship->angle=intShipAngle;
            } else {
            player1Ship->setPos(shipPos);
            player1Ship->angle=intShipAngle;
            }

            qDebug() << "Ship data received - posX:" << shipPosX << "posY:" << shipPosY << "angle:" << shipAngle;

            QPointF projPos(posX.toInt(),posY.toInt());
            int intAngle = angle.toInt();

            projectile *proj = new projectile(projPos, intAngle);
            scene.addItem(proj);

            qDebug() << "Projectile data received - posX:" << posX << "posY:" << posY << "angle:" << angle;
        } else if(parts.size() == 3) {
                QString shipPosX = parts[0];
                QString shipPosY = parts[1];
                QString shipAngle = parts[2];

                QPointF shipPos(shipPosX.toInt(),shipPosY.toInt());
                int intShipAngle = shipAngle.toInt();
                qDebug() << "Printing this sexy data: " << shipPos;

                if(player == 1){
                player2Ship->setPos(shipPos);
                player2Ship->angle=intShipAngle;
                } else {
                player1Ship->setPos(shipPos);
                player1Ship->angle=intShipAngle;
                }

                qDebug() << "Ship data received - posX:" << shipPosX << "posY:" << shipPosY << "angle:" << shipAngle;

        }
    }

}

int alternator = 0;
void MainWindow::preparezandersuperfunctionDatatosendtherplayer()
{
    QString data;

    if(player ==1)data = "zandersuperfunctionDataRecieved " + player1Ship->shipLog;
    if(player ==2)data = "zandersuperfunctionDataRecieved " + player2Ship->shipLog;

    //qDebug() << "DATA prepared to send : " << data;
    networkManager->sendzandersuperfunctionDatatootherplayer(data);
    player1Ship->shipLog="";
    player2Ship->shipLog="";
}

