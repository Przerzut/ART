/**
 * @file mainwindow.cpp
 * @brief Implementacja metod klasy MainWindow.
 * * Zawiera logikę obsługi interfejsu użytkownika, komunikacji sieciowej
 * z API MPK Wrocław oraz formatowania danych sensorycznych z obsługą i18n.
 */

#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QNetworkRequest>
#include <QApplication>

/**
 * @brief Konstruktor okna głównego.
 */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), isTracking(false), isPolish(false) {
    setupUI();

    retranslateUi();

    networkManager = new QNetworkAccessManager(this);
    dataTimer = new QTimer(this);

    connect(btnToggle, &QPushButton::clicked, this, &MainWindow::toggleTracking);
    connect(btnLang, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::fetchTramData);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onResult);
}

/**
 * @brief Inicjalizacja i rozmieszczenie elementów GUI (Tylko struktura!).
 */
void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    QVBoxLayout *sidePanelLayout = new QVBoxLayout();
    
    btnLang = new QPushButton(this);
    statusLabel = new QLabel(this);
    
    btnToggle = new QPushButton(this);
    btnToggle->setMinimumHeight(45);
    btnToggle->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold;");

    filterLabel = new QLabel(this);
    lineFilterList = new QListWidget(this);
    
    QStringList linie = {"1", "16", "145", "149"};
    for(const QString& linia : linie) {
        QListWidgetItem* item = new QListWidgetItem(linia, lineFilterList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Checked);
    }

    sidePanelLayout->addWidget(btnLang);
    sidePanelLayout->addWidget(statusLabel);
    sidePanelLayout->addWidget(btnToggle);
    sidePanelLayout->addSpacing(10);
    sidePanelLayout->addWidget(filterLabel);
    sidePanelLayout->addWidget(lineFilterList);
    sidePanelLayout->addStretch();

    QVBoxLayout *rightPanelLayout = new QVBoxLayout();
    
    headerLabel = new QLabel(this);
    
    logConsole = new QTextEdit(this);
    logConsole->setReadOnly(true);
    logConsole->setMinimumHeight(250);
    logConsole->setStyleSheet("background-color: #000000; color: #00FF00; font-family: 'Courier New';  white-space: pre;");

    mainTabs = new QTabWidget(this);
    speedChartTab = new QWidget(this);
    delayChartTab = new QWidget(this);
    
    mainTabs->addTab(speedChartTab, "");
    mainTabs->addTab(delayChartTab, "");

    rightPanelLayout->addWidget(headerLabel);
    rightPanelLayout->addWidget(logConsole);
    rightPanelLayout->addWidget(mainTabs);

    mainLayout->addLayout(sidePanelLayout, 1);
    mainLayout->addLayout(rightPanelLayout, 3);

    setCentralWidget(centralWidget);
    resize(1000, 750);
}

/**
 * @brief Zbiorcze ustawianie i tłumaczenie wszystkich elementów tekstowych interfejsu.
 */
void MainWindow::retranslateUi() {
    btnLang->setText(tr("Change Language (PL)"));
    btnToggle->setText(isTracking ? tr("STOP") : tr("START"));
    statusLabel->setText(tr("Status: Active"));
    headerLabel->setText(tr("<b>ROUTE MONITOR - LIVE SENSORY DATA</b>"));
    filterLabel->setText(tr("<b>Line Filtering:</b>"));
    
    mainTabs->setTabText(0, tr("Speed [km/h]"));
    mainTabs->setTabText(1, tr("Delay [min]"));
    
    setWindowTitle(tr("ART - Tram Traffic Analyzer"));
}

/**
 * @brief Przechwytywanie zdarzeń - automatycznie wywoływane przez Qt.
 */
void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

/**
 * @brief Ładuje lub usuwa plik tłumaczenia.
 */
void MainWindow::toggleLanguage() {
    isPolish = !isPolish; 

    if (isPolish) {
        if (appTranslator.load(":/translations/app_pl.qm")) {
            qApp->installTranslator(&appTranslator);
        } else {
            logConsole->append(tr("<i>[SYSTEM] Warning: Translation file 'app_pl.qm' not found.</i>"));
            isPolish = false;
        }
    } else {
        qApp->removeTranslator(&appTranslator);
    }
}

/**
 * @brief Kontroluje stan śledzenia danych.
 */
void MainWindow::toggleTracking() {
    if(!isTracking) {
        dataTimer->start(10000); 
        btnToggle->setText(tr("STOP")); // Używamy tr() z bazowym tekstem
        btnToggle->setStyleSheet("background-color: #c62828; color: white; font-weight: bold; ");
        
        QString header = QString("<b>%1 | %2 | %3 | %4 | %5</b>")
                       .arg(tr("TIME"), -10)
                       .arg(tr("LINE"), -12)
                       .arg("ID", -14) // ID nie wymaga tłumaczenia
                       .arg(tr("POS_X"), -11)
                       .arg(tr("POS_Y"), -10);
        
        header.replace(" ", "&nbsp;");
        logConsole->append(header);
        logConsole->append(QString(70, '-')); 
    
        fetchTramData();
    } else {
        dataTimer->stop();
        btnToggle->setText(tr("START"));
        btnToggle->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold;");
        logConsole->append(tr("<i>[SYSTEM] Monitoring stopped.</i>"));
    }
    isTracking = !isTracking;
}

/**
 * @brief Wysyła żądanie HTTP POST do serwera MPK.
 */
void MainWindow::fetchTramData() {
    QUrl url("https://mpk.wroc.pl/bus_position");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QByteArray postData = "busList[bus][]=1&busList[bus][]=16&busList[bus][]=145&busList[bus][]=149";
    networkManager->post(request, postData);
}

/**
 * @brief Przetwarza otrzymane dane JSON.
 */
void MainWindow::onResult(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
        if(jsonDoc.isArray()) {
            QJsonArray records = jsonDoc.array();
            
            QStringList activeFilters;
            for(int i = 0; i < lineFilterList->count(); ++i) {
                if(lineFilterList->item(i)->checkState() == Qt::Checked)
                    activeFilters.append(lineFilterList->item(i)->text());
            }

            for (const QJsonValue& val : records) {
                QJsonObject obj = val.toObject();
                QString lineName = obj["name"].toString();
                
                if(activeFilters.contains(lineName)) {
                    QString log = QString("%1 | %2 | ID: %3 | X: %4 | Y: %5")
                        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), -10)
                        .arg(tr("Line ") + lineName, -12) // Tr() dodane dla słowa "Line"
                        .arg(QString::number(obj["k"].toInt()), -10)
                        .arg(obj["x"].toDouble(), 8, 'f', 4)
                        .arg(obj["y"].toDouble(), 8, 'f', 4);
                    logConsole->append(log);
                }
            }
        }
    }
    reply->deleteLater();
}

/**
 * @brief Destruktor klasy.
 */
MainWindow::~MainWindow() {}