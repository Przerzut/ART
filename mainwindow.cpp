#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), isTracking(false) {
    setupUI();

    // Inicjalizacja modułów sieci i czasu
    networkManager = new QNetworkAccessManager(this);
    dataTimer = new QTimer(this);

    // Połączenie sygnałów i slotów
    connect(btnToggle, &QPushButton::clicked, this, &MainWindow::toggleTracking);
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::fetchTramData);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onResult);
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // --- PANEL BOCZNY (Nawigacja i Filtrowanie) ---
    QVBoxLayout *sidePanelLayout = new QVBoxLayout();
    
    btnToggle = new QPushButton("START", this);
    btnToggle->setMinimumHeight(40);
    
    QLabel *filterLabel = new QLabel("Filtrowanie Linii:", this);
    lineFilterList = new QListWidget(this);
    // Dodanie przykładowych linii do filtra
    QStringList linie = {"1", "2", "4", "10", "16", "17", "33"};
    for(const QString& linia : linie) {
        QListWidgetItem* item = new QListWidgetItem("Linia " + linia, lineFilterList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked); // Domyślnie zaznaczone
    }

    sidePanelLayout->addWidget(btnToggle);
    sidePanelLayout->addWidget(filterLabel);
    sidePanelLayout->addWidget(lineFilterList);
    sidePanelLayout->addStretch(); // Wypycha elementy do góry

    // --- OBSZAR GŁÓWNY (Zakładki) ---
    mainTabs = new QTabWidget(this);
    
    // Zakładka 1: Logi / Mapa (Live Tracking)
    logConsole = new QTextEdit(this);
    logConsole->setReadOnly(true);
    mainTabs->addTab(logConsole, "Live Tracking (Logi)");

    // Zakładka 2: Prędkość (Pusta na razie - przygotowana pod QtCharts)
    speedChartTab = new QWidget(this);
    mainTabs->addTab(speedChartTab, "Wykres Prędkości");

    // Zakładka 3: Opóźnienia
    delayChartTab = new QWidget(this);
    mainTabs->addTab(delayChartTab, "Wykres Opóźnień");

    // Złożenie wszystkiego w oknie
    mainLayout->addLayout(sidePanelLayout, 1); // Proporcja szerokości 1
    mainLayout->addWidget(mainTabs, 4);        // Proporcja szerokości 4

    setCentralWidget(centralWidget);
    setWindowTitle("ART - Analizator Ruchu Tramwajowego");
    resize(900, 600);
}

void MainWindow::toggleTracking() {
    if(!isTracking) {
        dataTimer->start(10000); // Pobieranie co 10 sekund
        btnToggle->setText("STOP");
        logConsole->append("<b>[SYSTEM]</b> Uruchomiono strumień danych...");
        fetchTramData(); // Natychmiastowe pobranie po kliknięciu
    } else {
        dataTimer->stop();
        btnToggle->setText("START");
        logConsole->append("<b>[SYSTEM]</b> Zatrzymano pobieranie.");
    }
    isTracking = !isTracking;
}

void MainWindow::fetchTramData() {
    // Zapytanie do Otwartego Wrocławia
    QString url = "https://www.wroclaw.pl/open-data/api/action/datastore_search_sql?sql=SELECT%20*%20from%20\"17308285-3973-42f3-91c2-4022239c064e\"";
    networkManager->get(QNetworkRequest(QUrl(url)));
}

void MainWindow::onResult(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject root = jsonDoc.object();
        
        QJsonArray records = root["result"].toObject()["records"].toArray();
        
        logConsole->append(QString("<i>[%1] Odebrano poprawnie dane z API. Liczba pojazdów: %2</i>")
                           .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                           .arg(records.size()));

        // Przetworzenie przykładowych 3 pojazdów, aby nie zalać logów
        for (int i = 0; i < qMin(records.size(), 3); ++i) {
            QJsonObject tram = records[i].toObject();
            QString line = tram["Line"].toString();
            double lat = tram["Y"].toDouble();
            double lon = tram["X"].toDouble();
            
            logConsole->append(QString("Linia %1 -> X: %2, Y: %3").arg(line).arg(lon).arg(lat));
        }
    } else {
        // Jeśli nie ma internetu lub API leży, generujemy symulowane dane sensoryczne
        logConsole->append("<b>[BŁĄD SIECI]</b> Brak połączenia. Generowanie danych symulowanych...");
        generateSimulatedData();
    }
    reply->deleteLater();
}

void MainWindow::generateSimulatedData() {
    // Symulacja pakietu JSON z odczytami z czujników GPS
    QString mockJson = R"({
        "result": {
            "records": [
                {"Line": "17", "X": 17.0345, "Y": 51.1078, "Opoznienie": 2},
                {"Line": "33", "X": 17.0512, "Y": 51.1123, "Opoznienie": -1},
                {"Line": "4",  "X": 17.0211, "Y": 51.0988, "Opoznienie": 5}
            ]
        }
    })";

    QJsonDocument doc = QJsonDocument::fromJson(mockJson.toUtf8());
    QJsonArray records = doc.object()["result"].toObject()["records"].toArray();

    for (int i = 0; i < records.size(); ++i) {
        QJsonObject tram = records[i].toObject();
        logConsole->append(QString("[SYMULACJA] Linia %1 -> X: %2, Y: %3")
                           .arg(tram["Line"].toString())
                           .arg(tram["X"].toDouble())
                           .arg(tram["Y"].toDouble()));
    }
}

MainWindow::~MainWindow() {}