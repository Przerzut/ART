/**
 * @file mainwindow.cpp
 * @brief Implementacja logiki głównego okna aplikacji Analizator Tramwajowy (ART).
 */

#include "mainwindow.h"
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QNetworkRequest>
#include <QApplication>
#include <QtMath>
#include <QSet>
#include <limits>
#include <QFile>
#include <QDebug>

/**
 * @brief Konstruktor głównego okna aplikacji.
 * @details Inicjalizuje interfejs użytkownika, ładuje domyślne trasy, podłącza silnik QML,
 * konfiguruje timery do animacji i pobierania danych oraz ustawia połączenia sygnałów.
 * @param parent Wskaźnik na widget nadrzędny (domyślnie nullptr).
 */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), isTracking(false), isPolish(false), startTime(0), currentTrackedId(-1) {

    loadRouteFromJson("1", ":/trasa1.geojson");
    lineFilterList = nullptr;

    m_tramModel = new TramModel(this);
    m_quickWidget = new QQuickWidget(this);
    
    m_quickWidget->engine()->rootContext()->setContextProperty("tramModel", m_tramModel);
    m_quickWidget->engine()->rootContext()->setContextProperty("mainWindow", this);
    
    m_quickWidget->setSource(QUrl("qrc:/map.qml"));
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    setupUI();
    setupCharts();        
    retranslateUi();

    networkManager = new QNetworkAccessManager(this);
    dataTimer = new QTimer(this);

    animTimer = new QTimer(this);
    connect(animTimer, &QTimer::timeout, this, &MainWindow::animateTrams);
    animTimer->start(33); // Ok. 30 klatek na sekundę (FPS)

    connect(btnToggle, &QPushButton::clicked, this, &MainWindow::toggleTracking);
    connect(btnLang, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::fetchTramData);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onResult);
    connect(tramIdComboBox, &QComboBox::currentTextChanged, this, &MainWindow::onTrackedTramChanged);

    updateVisibleRoutes();
}

/**
 * @brief Ładuje i parsuje plik GeoJSON zawierający geometrię torowiska.
 * @details Wyciąga koordynaty ze struktur LineString oraz MultiLineString i zapisuje
 * je jako listę punktów dla danej linii, aby algorytm dociągania miał bazę odniesienia.
 * @param lineName Nazwa/numer linii tramwajowej (np. "16").
 * @param filePath Ścieżka do zasobu GeoJSON (np. ":/trasa16.geojson").
 */
void MainWindow::loadRouteFromJson(const QString& lineName, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Nie można otworzyć pliku trasy dla linii:" << lineName;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonArray features = root["features"].toArray();
    
    QVector<QPointF> currentRoute;
    for (const QJsonValue &feature : features) {
        QJsonObject geometry = feature.toObject()["geometry"].toObject();
        QString geomType = geometry["type"].toString();
        QJsonArray coords = geometry["coordinates"].toArray();
        
        if (geomType == "LineString") {
            for (const QJsonValue &coord : coords) {
                QJsonArray point = coord.toArray();
                currentRoute.append(QPointF(point[0].toDouble(), point[1].toDouble()));
            }
        } 
        else if (geomType == "MultiLineString") {
            for (const QJsonValue &lineSegment : coords) {
                QJsonArray segmentCoords = lineSegment.toArray();
                for (const QJsonValue &coord : segmentCoords) {
                    QJsonArray point = coord.toArray();
                    currentRoute.append(QPointF(point[0].toDouble(), point[1].toDouble()));
                }
            }
        }
    }
    
    routes[lineName] = currentRoute; 
    qDebug() << "Załadowano punktów dla linii" << lineName << ":" << currentRoute.size();
}

/**
 * @brief Tworzy układ interfejsu graficznego użytkownika.
 * @details Konfiguruje panel boczny (przyciski, lista filtrowania) oraz główny panel
 * z mapą QML i wykresami analitycznymi w zakładkach.
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
    
    QStringList linie = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", 
    "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", 
    "21", "22", "23", "24"
    };
    for (const QString& linia : linie) {
        QString filePath = QString(":/trasa%1.geojson").arg(linia);
        
        if (QFile::exists(filePath)) {
            loadRouteFromJson(linia, filePath);
            
            QListWidgetItem* item = new QListWidgetItem(linia, lineFilterList);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setCheckState(Qt::Checked);
        }
    }

    tramIdLabel = new QLabel(this);
    tramIdComboBox = new QComboBox(this);
    tramIdComboBox->addItem("AUTO");

    sidePanelLayout->addWidget(btnLang);
    sidePanelLayout->addWidget(statusLabel);
    sidePanelLayout->addWidget(btnToggle);
    sidePanelLayout->addSpacing(10);
    sidePanelLayout->addWidget(filterLabel);
    sidePanelLayout->addWidget(lineFilterList);
    sidePanelLayout->addSpacing(10);
    sidePanelLayout->addWidget(tramIdLabel);
    sidePanelLayout->addWidget(tramIdComboBox);
    sidePanelLayout->addStretch();

    QVBoxLayout *rightPanelLayout = new QVBoxLayout();
    headerLabel = new QLabel(this);
    
    logConsole = new QTextEdit(this);
    logConsole->setReadOnly(true);
    logConsole->setMaximumHeight(150);
    logConsole->setStyleSheet("background-color: #000000; color: #00FF00; font-family: 'Courier New'; white-space: pre;");

    mainTabs = new QTabWidget(this);
    speedChartTab = new QWidget(this);
    mapChartTab = new QWidget(this); 
    
    mainTabs->addTab(speedChartTab, "");
    mainTabs->addTab(mapChartTab, "");

    rightPanelLayout->addWidget(headerLabel);
    rightPanelLayout->addWidget(logConsole);
    rightPanelLayout->addWidget(mainTabs);

    mainLayout->addLayout(sidePanelLayout, 1);
    mainLayout->addLayout(rightPanelLayout, 3);

    setCentralWidget(centralWidget);
    resize(1050, 750);

    QVBoxLayout *mapLayout = new QVBoxLayout(mapChartTab);
    mapLayout->setContentsMargins(0, 0, 0, 0); 
    mapLayout->addWidget(m_quickWidget);

    connect(lineFilterList, &QListWidget::itemChanged, this, &MainWindow::updateVisibleRoutes);
}

/**
 * @brief Sprawdza, czy w argumentach startowych przekazano "laptop".
 */
bool MainWindow::isLaptopMode() const {
    //qDebug() << "Tryb laptopa";
    return QCoreApplication::arguments().contains("laptop");
}

/**
 * @brief Przekazuje listę ścieżek do narysowania w warstwie widoku QML.
 * @details Zwraca listę tras (odcinków torowisk), ale tylko dla tych linii tramwajowych,
 * które w danej chwili zaznaczone są ptaszkiem na liście filtrowania.
 * @return QVariantList zawierający gotowe ścieżki QGeoCoordinate dla QML.
 */
QVariantList MainWindow::routePaths() const {
    QVariantList activePaths;
    
    if (lineFilterList) {
        for (int i = 0; i < lineFilterList->count(); ++i) {
            QListWidgetItem* item = lineFilterList->item(i);
            
            if (item->checkState() == Qt::Checked) {
                QString lineName = item->text();
                
                if (routes.contains(lineName)) {
                    QVariantList singlePath;
                    for (const QPointF& p : routes[lineName]) {
                        singlePath.append(QVariant::fromValue(QGeoCoordinate(p.y(), p.x())));
                    }
                    activePaths.append(QVariant::fromValue(singlePath));
                }
            }
        }
    }
    return activePaths; 
}

/**
 * @brief Wywoływana po zmianie stanu listy filtrowania (kliknięcie ptaszka).
 * @details Emituje powiadomienie do QML o konieczności przerysowania mapy torów oraz
 * dogłębnie czyści z pamięci aplikacji wszystkie informacje o tramwajach z odznaczonej linii.
 */
void MainWindow::updateVisibleRoutes() {
    emit routePathsChanged(); 

    if (!lineFilterList || !m_tramModel) return;

    QStringList activeFilters;
    for (int i = 0; i < lineFilterList->count(); ++i) {
        if (lineFilterList->item(i)->checkState() == Qt::Checked) {
            activeFilters.append(lineFilterList->item(i)->text());
        }
    }

    QList<int> idsToRemove;
    for (auto it = tramLines.constBegin(); it != tramLines.constEnd(); ++it) {
        if (!activeFilters.contains(it.value())) {
            idsToRemove.append(it.key()); 
        }
    }

    for (int id : idsToRemove) {
        m_tramModel->removeTram(id);       
        targetAnimPositions.remove(id);    
        currentAnimPositions.remove(id);   
        previousPositions.remove(id);      
        speedBuffers.remove(id);           
        speedHistories.remove(id);         
        tramLines.remove(id);              

        int idx = tramIdComboBox->findText(QString::number(id));
        if (idx != -1) tramIdComboBox->removeItem(idx);
        
        if (currentTrackedId == id) {
            tramIdComboBox->setCurrentIndex(0);
        }
    }
}

/**
 * @brief Inicjalizuje moduł QtCharts odpowiedzialny za wykres prędkości na żywo.
 * @details Ustawia osie X (czas) oraz Y (prędkość w km/h), konfiguruje wizualnie widok wykresu.
 */
void MainWindow::setupCharts() {
    speedSeries = new QLineSeries();
    QPen speedPen(Qt::blue); speedPen.setWidth(2); speedSeries->setPen(speedPen);

    speedChart = new QChart();
    speedChart->addSeries(speedSeries);
    speedChart->legend()->hide(); 
    
    speedAxisX = new QValueAxis(); speedAxisX->setRange(0, 60); speedAxisX->setLabelFormat("%d");
    speedChart->addAxis(speedAxisX, Qt::AlignBottom); speedSeries->attachAxis(speedAxisX);

    speedAxisY = new QValueAxis(); speedAxisY->setRange(0, 70); speedAxisY->setLabelFormat("%d");
    speedChart->addAxis(speedAxisY, Qt::AlignLeft); speedSeries->attachAxis(speedAxisY);

    QChartView *speedView = new QChartView(speedChart);
    speedView->setRenderHint(QPainter::Antialiasing);
    
    QVBoxLayout *speedLayout = new QVBoxLayout(speedChartTab);
    speedLayout->setContentsMargins(0, 0, 0, 0); 
    speedLayout->addWidget(speedView);
}

/**
 * @brief Odświeża wszystkie łańcuchy tekstowe w aplikacji w oparciu o aktywny język.
 * @details Przydatne w systemach wielojęzycznych (i18n), automatycznie wywoływane 
 * po pomyślnym załadowaniu nowego pliku translacji `.qm`.
 */
void MainWindow::retranslateUi() {
    btnLang->setText(tr("Change Language (PL)"));
    btnToggle->setText(isTracking ? tr("STOP") : tr("START"));
    statusLabel->setText(isTracking ? tr("Status: Active") : tr("Status: Inactive"));
    headerLabel->setText(tr("<b>ROUTE MONITOR - LIVE SENSORY DATA</b>"));
    filterLabel->setText(tr("<b>Line Filtering:</b>"));
    tramIdLabel->setText(tr("<b>Select Vehicle (ID):</b>"));
    mainTabs->setTabText(0, tr("Speed [km/h]"));
    mainTabs->setTabText(1, tr("Map 2D [GPS]"));
    setWindowTitle(tr("ART - Tram Traffic Analyzer"));
}

/**
 * @brief Przechwytuje zdarzenia zmiany języka wysyłane przez system bazowy Qt.
 * @param event Wskaźnik na przechwycone zdarzenie.
 */
void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) retranslateUi();
    QMainWindow::changeEvent(event);
}

/**
 * @brief Slot przełączający język aplikacji (Polski / Angielski).
 */
void MainWindow::toggleLanguage() {
    isPolish = !isPolish; 
    if (isPolish) {
        if (appTranslator.load(":/app_pl.qm")) qApp->installTranslator(&appTranslator);
    } else {
        qApp->removeTranslator(&appTranslator);
    }
    retranslateUi(); 
}

/**
 * @brief Przełącza globalny stan śledzenia (Włącz/Wyłącz pobieranie danych).
 * @details Resetuje timer, aktualizuje wygląd przycisku oraz loguje akcję w konsoli bocznej.
 */
void MainWindow::toggleTracking() {
    if(!isTracking) {
        startTime = QDateTime::currentMSecsSinceEpoch(); 
        dataTimer->start(10000);
        isTracking = true;
        retranslateUi();
        btnToggle->setStyleSheet("background-color: #c62828; color: white; font-weight: bold; ");
        
        QString header = QString("<b>%1 | %2 | %3 | %4 | %5</b>")
                       .arg(tr("TIME"), -10).arg(tr("LINE"), -12).arg("ID", -14).arg(tr("POS_X"), -11).arg(tr("POS_Y"), -10);
        header.replace(" ", "&nbsp;");
        logConsole->append(header);
        logConsole->append(QString(70, '-')); 
        
        fetchTramData();
    } else {
        dataTimer->stop();
        isTracking = false;
        retranslateUi();
        btnToggle->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold;");
        logConsole->append(tr("<i>[SYSTEM] Monitoring stopped.</i>"));
    }
}

/**
 * @brief Inicjuje żądanie HTTP POST w celu pobrania lokalizacji tramwajów.
 * @details Buduje dynamiczne zapytanie bazując na aktualnie zaznaczonych liniach 
 * na liście po lewej stronie, minimalizując dzięki temu pobór niepotrzebnych danych z sieci.
 */
void MainWindow::fetchTramData() {
    QUrl url("https://mpk.wroc.pl/bus_position");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QString postString = "";
    for(int i = 0; i < lineFilterList->count(); ++i) {
        if(lineFilterList->item(i)->checkState() == Qt::Checked) {
            postString += "busList[tram][]=" + lineFilterList->item(i)->text() + "&";
        }
    }
    if (postString.endsWith("&")) postString.chop(1);
    
    networkManager->post(request, postString.toUtf8());
}

/**
 * @brief Algorytm przyciągania niedokładnych kordynatów GPS do prawdziwego śladu torowiska.
 * @details Rzutuje punkt z sensora prostopadle na odcinek wytyczony między dwoma najbliższymi
 * węzłami torowiska pobranego z GeoJSON, aby usunąć szum (błędy GPS).
 * @param lon Długość geograficzna pobrana z API.
 * @param lat Szerokość geograficzna pobrana z API.
 * @param lineName Nazwa linii (umożliwia dobór odpowiedniego szlaku z pamięci).
 * @return QPointF Precyzyjnie dociągnięta i skorygowana pozycja (longitude, latitude).
 */
QPointF MainWindow::snapToRoute(double lon, double lat, const QString& lineName) {
    if (!routes.contains(lineName) || routes[lineName].isEmpty()) {
        return QPointF(lon, lat); // Brak trasy w bazie -> rysuj surowy GPS
    }
    
    const QVector<QPointF>& routePoints = routes[lineName];
    QPointF bestPoint(lon, lat);
    double minDistanceSq = std::numeric_limits<double>::max();
    QPointF P(lon, lat);

    double cosLat = qCos(qDegreesToRadians(51.1079)); 

    for (int i = 0; i < routePoints.size() - 1; ++i) {
        QPointF A = routePoints[i]; QPointF B = routePoints[i+1];
        
        double ABx = (B.x() - A.x()) * cosLat; 
        double ABy = B.y() - A.y();
        double APx = (P.x() - A.x()) * cosLat; 
        double APy = P.y() - A.y();
        
        double dotProduct = APx * ABx + APy * ABy;
        double segLengthSq = ABx * ABx + ABy * ABy;
        
        double t = (segLengthSq != 0) ? dotProduct / segLengthSq : -1;
        QPointF C = (t < 0) ? A : ((t > 1) ? B : QPointF(A.x() + t * (B.x() - A.x()), A.y() + t * (B.y() - A.y())));
        
        double distSq = pow((P.x() - C.x()) * cosLat, 2) + pow(P.y() - C.y(), 2);
        
        if (distSq < minDistanceSq) { minDistanceSq = distSq; bestPoint = C; }
    }
    return bestPoint;
}

/**
 * @brief Wykorzystuje wzór Haversine'a do precyzyjnego oszacowania prędkości punktu na sferze (Ziemi).
 * @param lon1 Długość geo. początkowa.
 * @param lat1 Szerokość geo. początkowa.
 * @param lon2 Długość geo. końcowa.
 * @param lat2 Szerokość geo. końcowa.
 * @param timeDiffMs Różnica czasu pomiędzy zebranymi próbkami (w milisekundach).
 * @return Prędkość pojazdu w km/h.
 */
double MainWindow::calculateSpeed(double lon1, double lat1, double lon2, double lat2, qint64 timeDiffMs) {
    if (timeDiffMs <= 0) return 0.0;
    double R = 6371.0; 
    double dLat = qDegreesToRadians(lat2 - lat1); double dLon = qDegreesToRadians(lon2 - lon1);
    double a = qPow(qSin(dLat / 2), 2) + qCos(qDegreesToRadians(lat1)) * qCos(qDegreesToRadians(lat2)) * qPow(qSin(dLon / 2), 2);
    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
    return (R * c) / (timeDiffMs / 1000.0 / 3600.0);
}

/**
 * @brief Realizuje płynną interpolację oraz animację przemieszczania się tramwajów.
 * @details Opiera się na kinematycznym wektorze prędkości. W każdej klatce (33ms) 
 * przesuwa tramwaj zgodnie z jego zwrotem w stronę najnowszego punktu pobranego z API.
 * Następnie dla uzyskanej klatki ponawia dociąganie do krzywizny torów.
 */
void MainWindow::animateTrams() {
    if (!isTracking || targetAnimPositions.isEmpty()) return;

    double dt = 0.033;

    for (int id : targetAnimPositions.keys()) {
        QPointF target = targetAnimPositions[id];
        QPointF current = currentAnimPositions.value(id, target); 
        QPointF oldCurrent = current; // Zapisujemy pozycję przed ruchem do obliczenia kąta!

        QString myLine = tramLines.value(id, "");
        
        double speedKmh = 0.0;
        if (speedBuffers.contains(id) && !speedBuffers[id].isEmpty()) {
            for (double s : speedBuffers[id]) speedKmh += s;
            speedKmh /= speedBuffers[id].size();
        }

        if (speedKmh < 1.0) {
            currentAnimPositions[id] = target;
            m_tramModel->updateTram(id, myLine, current.y(), current.x(), speedKmh, 0); 
            continue; 
        }

        double speedMs = speedKmh / 3.6;
        double stepDegrees = (speedMs * dt) / 111320.0; 

        double dx = target.x() - current.x();
        double dy = target.y() - current.y();
        double distanceToTarget = qSqrt(dx*dx + dy*dy);

        double moveX = 0;
        double moveY = 0;

        if (distanceToTarget > stepDegrees) {
            moveX = (dx / distanceToTarget) * stepDegrees;
            moveY = (dy / distanceToTarget) * stepDegrees;
        } 
        else {
            if (previousPositions.contains(id)) {
                QPointF prev = QPointF(previousPositions[id].lon, previousPositions[id].lat);
                double dirX = target.x() - prev.x();
                double dirY = target.y() - prev.y();
                double dirLen = qSqrt(dirX*dirX + dirY*dirY);

                if (dirLen > 0) {
                    for (int i=0; i<speedBuffers[id].size(); ++i) {
                        speedBuffers[id][i] *= 0.98; 
                    }
                    if (speedKmh > 2.0) {
                        moveX = (dirX / dirLen) * stepDegrees;
                        moveY = (dirY / dirLen) * stepDegrees;
                    }
                }
            }
        }

        QPointF newPos(current.x() + moveX, current.y() + moveY);

        current = snapToRoute(newPos.x(), newPos.y(), myLine);
        currentAnimPositions[id] = current;

        double actualDx = current.x() - oldCurrent.x();
        double actualDy = current.y() - oldCurrent.y();
        double heading = qRadiansToDegrees(qAtan2(actualDy, actualDx)) * -1 + 90;

        m_tramModel->updateTram(id, myLine, current.y(), current.x(), speedKmh, heading);
    }
}

/**
 * @brief Obsługuje zmianę wartości w menu wyboru śledzonego tramwaju (ComboBox).
 * @details Zmienia podświetlenie na mapie QML oraz podmienia dane prędkości renderowane na wykresie.
 * @param text Nowo wybrana opcja ("AUTO" lub ID konkretnego tramwaju).
 */
void MainWindow::onTrackedTramChanged(const QString &text) {
    if (text == "AUTO") {
        setSelectedTramId(-1); 
    } else {
        setSelectedTramId(text.toInt()); 
    }
    
    speedSeries->clear();
    
    int id = selectedTramId();
    if (id != -1 && speedHistories.contains(id)) {
        speedSeries->replace(speedHistories[id]); 
    } 
    
    double currentSecs = (QDateTime::currentMSecsSinceEpoch() - startTime) / 1000.0;
    speedAxisX->setRange(qMax(0.0, currentSecs - 60.0), qMax(60.0, currentSecs));
    
    logConsole->append(tr("<i>[SYSTEM] Switched view to vehicle ID: %1</i>").arg(text));
}

/**
 * @brief Proces "odśmiecający" (Garbage Collector) - zwalnia z pamięci pojazdy niekatywne.
 * @details Jeśli API MPK przestało nadawać sygnał dla jakiegoś ID (np. tramwaj zjechał do zajezdni
 * i wyłączył nadajnik GPS) przez ponad 60 sekund, aplikacja bezpiecznie usuwa go z mapy oraz pamięci.
 * @param currentTime Obecny znacznik czasu (Unix Epoch ms).
 */
void MainWindow::cleanUpStaleTrams(qint64 currentTime) {
    QList<int> toRemove;
    for (auto it = previousPositions.constBegin(); it != previousPositions.constEnd(); ++it) {
        if (currentTime - it.value().timestampMs > 60000) { 
            toRemove.append(it.key());
        }
    }
    
    for (int id : toRemove) {
        int idx = tramIdComboBox->findText(QString::number(id));
        if (idx != -1) tramIdComboBox->removeItem(idx);
        
        previousPositions.remove(id);
        speedBuffers.remove(id);
        speedHistories.remove(id);
        currentAnimPositions.remove(id);
        targetAnimPositions.remove(id);
        
        m_tramModel->removeTram(id); 

        if (currentTrackedId == id) {
            tramIdComboBox->setCurrentIndex(0); 
        }
    }
}

/**
 * @brief Przetwarza odpowiedź zwrotną od serwera MPK.
 * @details Parauje otrzymane od API paczki danych JSON. Odnajduje współrzędne tramwajów,
 * wylicza ich prędkość na podstawie odległości miedzy pakietami i zarządza archiwum pozycji.
 * Aktualizuje stan wizualny mapy (punkty docelowe) dla silnika rysowania QML.
 * @param reply Obiekt QNetworkReply zawierający odpowiedź z serwera.
 */
void MainWindow::onResult(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        logConsole->append(tr("<i>[ERROR] API Failure: %1</i>").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
    if(jsonDoc.isArray()) {
        QJsonArray records = jsonDoc.array();
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        double currentSecs = (currentTime - startTime) / 1000.0;
        
        QStringList activeFilters;
        for(int i = 0; i < lineFilterList->count(); ++i) {
            if(lineFilterList->item(i)->checkState() == Qt::Checked) activeFilters.append(lineFilterList->item(i)->text());
        }

        int activeTargetId = -1;

        for (const QJsonValue& val : records) {
            QJsonObject obj = val.toObject();
            QString lineName = obj["name"].toString();
            
            if(activeFilters.contains(lineName)) {
                int id = obj["k"].toInt();

                tramLines[id] = lineName;

                double currentLat = obj["x"].toDouble(); 
                double currentLon = obj["y"].toDouble(); 
                
                QPointF snappedPos = snapToRoute(currentLon, currentLat, lineName);
                targetAnimPositions[id] = snappedPos; 
                if (!currentAnimPositions.contains(id)) currentAnimPositions[id] = snappedPos;

                QString log = QString("%1 | %2 | ID: %3 | X: %4 | Y: %5")
                    .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), -10)
                    .arg(tr("Line ") + lineName, -12) 
                    .arg(QString::number(id), -10)
                    .arg(snappedPos.x(), 8, 'f', 4)
                    .arg(snappedPos.y(), 8, 'f', 4);
                logConsole->append(log);

                if (previousPositions.contains(id)) {
                    VehicleData prev = previousPositions[id];
                    if (prev.lon != currentLon || prev.lat != currentLat) {
                        double rawSpeed = calculateSpeed(prev.lon, prev.lat, currentLon, currentLat, currentTime - prev.timestampMs);
                        
                        if (rawSpeed <= 75.0) {
                            speedBuffers[id].append(rawSpeed);
                            if (speedBuffers[id].size() > 3) speedBuffers[id].removeFirst(); 

                            double smoothedSpeed = 0.0;
                            for (double s : speedBuffers[id]) smoothedSpeed += s;
                            smoothedSpeed /= speedBuffers[id].size(); 

                            speedHistories[id].append(QPointF(currentSecs, smoothedSpeed));
                            if (speedHistories[id].size() > 50) speedHistories[id].removeFirst();

                            if (id == currentTrackedId || (currentTrackedId == -1 && activeTargetId == -1)) {
                                speedSeries->append(currentSecs, smoothedSpeed);
                            }
                        }
                    }
                }
                previousPositions[id] = {currentLon, currentLat, currentTime};

                if (tramIdComboBox->findText(QString::number(id)) == -1) {
                    tramIdComboBox->addItem(QString::number(id));
                }
                
                if (activeTargetId == -1) activeTargetId = id; 
            }
        }

        cleanUpStaleTrams(currentTime); 
        speedAxisX->setRange(qMax(0.0, currentSecs - 60.0), qMax(60.0, currentSecs));
    }
    reply->deleteLater();
}

/**
 * @brief Destruktor domyślny głównego okna aplikacji.
 */
MainWindow::~MainWindow() {}