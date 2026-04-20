#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QNetworkRequest>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), isTracking(false) {
    setupUI();

    networkManager = new QNetworkAccessManager(this);
    dataTimer = new QTimer(this);

    connect(btnToggle, &QPushButton::clicked, this, &MainWindow::toggleTracking);
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::fetchTramData);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onResult);
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // --- PANEL BOCZNY ---
    QVBoxLayout *sidePanelLayout = new QVBoxLayout();
    
    btnToggle = new QPushButton("START", this);
    btnToggle->setMinimumHeight(40);
    
    QLabel *filterLabel = new QLabel("Filtrowanie Linii:", this);
    lineFilterList = new QListWidget(this);
    // Zaktualizowano filtry pod linie z Twojego zapytania (D, 145)
    QStringList linie = {"D", "145", "17", "33"};
    for(const QString& linia : linie) {
        QListWidgetItem* item = new QListWidgetItem("Linia " + linia, lineFilterList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
    }

    sidePanelLayout->addWidget(btnToggle);
    sidePanelLayout->addWidget(filterLabel);
    sidePanelLayout->addWidget(lineFilterList);
    sidePanelLayout->addStretch();

    // --- OBSZAR GŁÓWNY ---
    mainTabs = new QTabWidget(this);
    
    logConsole = new QTextEdit(this);
    logConsole->setReadOnly(true);
    mainTabs->addTab(logConsole, "Live Tracking (Logi)");

    speedChartTab = new QWidget(this);
    mainTabs->addTab(speedChartTab, "Wykres Prędkości");

    delayChartTab = new QWidget(this);
    mainTabs->addTab(delayChartTab, "Wykres Opóźnień");

    mainLayout->addLayout(sidePanelLayout, 1);
    mainLayout->addWidget(mainTabs, 4);

    setCentralWidget(centralWidget);
    setWindowTitle("ART - Analizator Ruchu Tramwajowego (Dane Rzeczywiste)");
    resize(900, 600);
}

void MainWindow::toggleTracking() {
    if(!isTracking) {
        dataTimer->start(10000); // 10 sekund, tak jak w Twoim kodzie
        btnToggle->setText("STOP");
        logConsole->append("<b>[SYSTEM]</b> Uruchomiono strumień danych MPK...");
        fetchTramData();
    } else {
        dataTimer->stop();
        btnToggle->setText("START");
        logConsole->append("<b>[SYSTEM]</b> Zatrzymano pobieranie.");
    }
    isTracking = !isTracking;
}

void MainWindow::fetchTramData() {
    // 1. Ustawienie adresu z Twojego kodu
    QUrl url("https://mpk.wroc.pl/bus_position");
    QNetworkRequest request(url);

    // 2. Dodanie nagłówków (Headers) - odpowiednik CURLOPT_USERAGENT z curl
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
    
    // Ważne przy wysyłaniu formularzy POST (żeby serwer wiedział, jak czytać dane)
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // 3. Przygotowanie danych do wysłania (odpowiednik CURLOPT_POSTFIELDS)
    QByteArray postData = "busList[bus][]=D&busList[bus][]=145";

    // 4. Wykonanie zapytania POST (wcześniej było GET)
    networkManager->post(request, postData);
}

void MainWindow::onResult(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        
        // Zgodnie z Twoim wcześniejszym kodem, serwer zwraca tablicę obiektów
        if(jsonDoc.isArray()) {
            QJsonArray records = jsonDoc.array();
            
            logConsole->append(QString("<br><i>[%1] Odebrano pozycje %2 pojazdów</i>")
                               .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                               .arg(records.size()));

            // Przeszukiwanie paczki danych i wyciąganie tych samych parametrów co w starym kodzie
            for (int i = 0; i < records.size(); ++i) {
                QJsonObject vehicle = records[i].toObject();
                
                int id = vehicle["k"].toInt();           // Twoje "k" z JSONa
                QString name = vehicle["name"].toString(); // Twoje "name" z JSONa
                double x = vehicle["x"].toDouble();      // Długość geogr.
                double y = vehicle["y"].toDouble();      // Szerokość geogr.
                
                logConsole->append(QString("[AKTUALIZACJA] Linia: <b>%1</b> | ID: %2 | Poz -> X: %3, Y: %4")
                                   .arg(name)
                                   .arg(id)
                                   .arg(x, 0, 'f', 4) // Formatowanie do 4 miejsc po przecinku
                                   .arg(y, 0, 'f', 4));
            }
        } else {
            logConsole->append("<b>[BŁĄD]</b> Otrzymano nieprawidłowy format JSON z MPK.");
        }
    } else {
        logConsole->append("<b>[BŁĄD SIECI]</b> " + reply->errorString());
        generateSimulatedData(); // Uruchomi się, jeśli serwer MPK przestanie odpowiadać
    }
    reply->deleteLater();
}

void MainWindow::generateSimulatedData() {
    logConsole->append("[SYMULACJA] Tryb awaryjny - generowanie danych lokalnych.");
    // Pozostałość z trybu awaryjnego (uruchomi się tylko przy braku internetu)
}

MainWindow::~MainWindow() {}