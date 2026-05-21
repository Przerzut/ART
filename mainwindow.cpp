/**
 * @file mainwindow.cpp
 * @brief Implementacja logiki głównego okna aplikacji Analizator Tramwajowy (ART).
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
#include <QtMath>
#include <QSet>
#include <limits>

/**
 * @file mainwindow.cpp
 * @brief Implementacja logiki głównego okna aplikacji Analizator Tramwajowy (ART).
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
#include <QtMath>
#include <QSet>
#include <limits>
#include <QFile>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), isTracking(false), isPolish(false), startTime(0), currentTrackedId(-1) {
    // 1. Ładujemy trasę z Twojego pliku GeoJSON
    loadRouteFromJson();

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
    animTimer->start(33); 

    connect(btnToggle, &QPushButton::clicked, this, &MainWindow::toggleTracking);
    connect(btnLang, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::fetchTramData);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onResult);
    connect(tramIdComboBox, &QComboBox::currentTextChanged, this, &MainWindow::onTrackedTramChanged);
}

// NOWA FUNKCJA DO ŁADOWANIA GEOJSON
void MainWindow::loadRouteFromJson() {
    routePoints.clear();
    QFile file(":/trasa16.geojson"); // Upewnij się, że plik jest w qrc
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Nie można otworzyć pliku trasy!";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonArray features = root["features"].toArray();
    
    for (const QJsonValue &feature : features) {
        QJsonObject geometry = feature.toObject()["geometry"].toObject();
        QJsonArray coords = geometry["coordinates"].toArray();
        for (const QJsonValue &coord : coords) {
            QJsonArray point = coord.toArray();
            routePoints.append(QPointF(point[0].toDouble(), point[1].toDouble()));
        }
    }
    qDebug() << "Załadowano punktów z GeoJSON:" << routePoints.size();
}

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
    
    QStringList linie = {"16"}; 
    for(const QString& linia : linie) {
        QListWidgetItem* item = new QListWidgetItem(linia, lineFilterList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Checked);
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
}

void MainWindow::initHardcodedRoute() {
    routePoints.clear();
    
    // TRASA HIGH-RESOLUTION: LINIA 16 (Tarnogaj <-> Osobowice)
    // Punkty ułożone bardzo gęsto, aby algorytm idealnie pokrywał się z ulicami i mostami.
    
    // --- Odcinek Południowy ---
    routePoints.append(QPointF(17.0425, 51.0818)); // Tarnogaj Pętla
    routePoints.append(QPointF(17.0410, 51.0855)); // Klimasa 
    routePoints.append(QPointF(17.0402, 51.0890)); // Armii Krajowej 
    routePoints.append(QPointF(17.0398, 51.0920)); // Bardzka 
    routePoints.append(QPointF(17.0395, 51.0945)); // Hubska (Prudnicka)
    routePoints.append(QPointF(17.0400, 51.0965)); // Hubska (Gliniana)
    routePoints.append(QPointF(17.0405, 51.0980)); // Hubska (Dawida)
    
    // --- Odcinek Centrum (Dworzec, Pułaskiego) ---
    routePoints.append(QPointF(17.0408, 51.0985)); // Skręt w Pułaskiego
    routePoints.append(QPointF(17.0425, 51.0988)); // Pod Wiaduktem PKP
    routePoints.append(QPointF(17.0440, 51.0995)); // Małachowskiego
    routePoints.append(QPointF(17.0485, 51.1030)); // Kościuszki
    routePoints.append(QPointF(17.0510, 51.1060)); // Plac Wróblewskiego (Start łuku)
    
    // --- Idealne wejście na Most Grunwaldzki (Brak cięcia przez wodę) ---
    routePoints.append(QPointF(17.0520, 51.1075)); // Plac Społeczny
    routePoints.append(QPointF(17.0535, 51.1085)); // Wjazd na Most Grunwaldzki
    routePoints.append(QPointF(17.0545, 51.1090)); // Środek Mostu
    routePoints.append(QPointF(17.0560, 51.1095)); // Zjazd z Mostu
    routePoints.append(QPointF(17.0600, 51.1110)); // Oś Grunwaldzka
    routePoints.append(QPointF(17.0620, 51.1118)); // Rondo Reagana
    
    // --- Odcinek Śródmieście (Piastowska, Nowowiejska) ---
    routePoints.append(QPointF(17.0610, 51.1130)); // Skręt w Piastowską
    routePoints.append(QPointF(17.0585, 51.1150)); // Piastowska / Sienkiewicza
    routePoints.append(QPointF(17.0560, 51.1170)); // Piastowska (Północ)
    routePoints.append(QPointF(17.0540, 51.1185)); // Skręt w Nowowiejską
    routePoints.append(QPointF(17.0500, 51.1195)); // Wyszyńskiego
    routePoints.append(QPointF(17.0450, 51.1210)); // Słowiańska
    routePoints.append(QPointF(17.0380, 51.1220)); // Trzebnicka
    routePoints.append(QPointF(17.0350, 51.1225)); // Dworzec Nadodrze
    
    // --- Odcinek Północny i Most Osobowicki ---
    routePoints.append(QPointF(17.0320, 51.1235)); // Łuk na Staszica
    routePoints.append(QPointF(17.0280, 51.1250)); // Plac Staszica
    routePoints.append(QPointF(17.0220, 51.1280)); // Reymonta
    routePoints.append(QPointF(17.0200, 51.1300)); // Wjazd na Most Osobowicki
    routePoints.append(QPointF(17.0180, 51.1320)); // Środek Mostu Osobowickiego
    routePoints.append(QPointF(17.0160, 51.1330)); // Zjazd z Mostu
    
    // --- Osobowicka ---
    routePoints.append(QPointF(17.0120, 51.1350)); // Bałtycka
    routePoints.append(QPointF(17.0080, 51.1360)); // Łużycka
    routePoints.append(QPointF(16.9950, 51.1390)); // Pętla Osobowice
}

QVariantList MainWindow::routePath() const {
    QVariantList path;
    for (const QPointF& p : routePoints) {
        path.append(QVariant::fromValue(QGeoCoordinate(p.y(), p.x())));
    }
    return path;
}

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

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) retranslateUi();
    QMainWindow::changeEvent(event);
}

void MainWindow::toggleLanguage() {
    isPolish = !isPolish; 
    if (isPolish) {
        if (appTranslator.load(":/app_pl.qm")) qApp->installTranslator(&appTranslator);
    } else {
        qApp->removeTranslator(&appTranslator);
    }
    retranslateUi(); 
}

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

QPointF MainWindow::snapToRoute(double lon, double lat) {
    if (routePoints.isEmpty()) return QPointF(lon, lat);
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

double MainWindow::calculateSpeed(double lon1, double lat1, double lon2, double lat2, qint64 timeDiffMs) {
    if (timeDiffMs <= 0) return 0.0;
    double R = 6371.0; 
    double dLat = qDegreesToRadians(lat2 - lat1); double dLon = qDegreesToRadians(lon2 - lon1);
    double a = qPow(qSin(dLat / 2), 2) + qCos(qDegreesToRadians(lat1)) * qCos(qDegreesToRadians(lat2)) * qPow(qSin(dLon / 2), 2);
    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
    return (R * c) / (timeDiffMs / 1000.0 / 3600.0);
}

/**
 * @brief Realizuje płynną animację przemieszczania się tramwajów (Dead Reckoning).
 * @details Silnik działa z częstotliwością ~30 FPS. Implementuje ruch jednostajny, a przy
 * braku nowych danych (ekstrapolacja) wprowadza fizyczne hamowanie tramwaju.
 */
void MainWindow::animateTrams() {
    if (!isTracking || targetAnimPositions.isEmpty()) return;

    double dt = 0.033; // 33ms na klatkę (dla pętli 30fps)

    for (int id : targetAnimPositions.keys()) {
        QPointF target = targetAnimPositions[id];
        // Jeśli kropka nie istnieje w systemie animacji, zaczyna w punkcie target
        QPointF current = currentAnimPositions.value(id, target); 

        // 1. Obliczanie średniej wygładzonej prędkości (z bufora)
        double speedKmh = 0.0;
        if (speedBuffers.contains(id) && !speedBuffers[id].isEmpty()) {
            for (double s : speedBuffers[id]) speedKmh += s;
            speedKmh /= speedBuffers[id].size();
        }

        // 2. Wyliczanie kąta obrotu (Heading) w stronę celu
        double dx = target.x() - current.x();
        double dy = target.y() - current.y();
        double distanceDegrees = qSqrt(dx*dx + dy*dy);
        double heading = qRadiansToDegrees(qAtan2(dy, dx)) * -1 + 90;

        // Jeśli tramwaj stoi w korku / na przystanku
        if (speedKmh < 1.0) {
            currentAnimPositions[id] = target;
            m_tramModel->updateTram(id, "16", current.y(), current.x(), speedKmh, heading); 
            continue; 
        }

        // 3. FIZYKA: Przemieszczanie (Dead Reckoning)
        double speedMs = speedKmh / 3.6;
        double distanceMeters = speedMs * dt; 
        
        // Zgrubny przelicznik we Wrocławiu: 1 stopień to ok. 111 320 metrów
        double maxDegreesPerFrame = distanceMeters / 111320.0;

        // Scenariusz A: Tramwaj goni punkt z API
        if (distanceDegrees > 0.00001) {
            double ratio = qMin(1.0, maxDegreesPerFrame / distanceDegrees);
            current.setX(current.x() + dx * ratio);
            current.setY(current.y() + dy * ratio);
        } 
        // Scenariusz B: Tramwaj dojechał do celu, a API milczy -> Przewidywanie w przyszłość z hamowaniem
        else {
            if (previousPositions.contains(id)) {
                // Skąd przyjechał ostatnio tramwaj (poprzedni pakiet)
                QPointF prev = QPointF(previousPositions[id].lon, previousPositions[id].lat);
                
                // Jaki wektor kierunkowy ma zachować w przewidywaniu
                double dirX = target.x() - prev.x();
                double dirY = target.y() - prev.y();
                double dirLen = qSqrt(dirX*dirX + dirY*dirY);
                
                if (dirLen > 0) {
                    // Mnożymy ostatnią prędkość w buforze przez wartość mniejszą niż 1 (hamowanie)
                    // Obniżamy ją w samej tablicy, żeby za chwilę wejść w warunek (speedKmh < 1.0)
                    for (int i=0; i<speedBuffers[id].size(); ++i) {
                         speedBuffers[id][i] *= 0.95; // Wytraca 5% prędkości z każdą klatką
                    }
                    
                    // Jeśli wciąż jedzie szybciej niż spacer, przesuwamy go (wirtualne hamowanie)
                    if (speedKmh > 2.0) {
                        double slowdownMaxDegrees = (speedKmh / 3.6 * dt) / 111320.0;
                        current.setX(current.x() + (dirX / dirLen) * slowdownMaxDegrees);
                        current.setY(current.y() + (dirY / dirLen) * slowdownMaxDegrees);
                    }
                }
            }
        }

        // 4. Zapisujemy pozycję do kolejnej klatki
        currentAnimPositions[id] = current;
        
        // Przeliczamy kąt obrotu na podstawie faktycznego ruchu
        heading = qRadiansToDegrees(qAtan2(target.y() - current.y(), target.x() - current.x())) * -1 + 90;
        
        // 5. Rysujemy w QML
        m_tramModel->updateTram(id, "16", current.y(), current.x(), speedKmh, heading);
    }
}

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
                double currentLat = obj["x"].toDouble(); 
                double currentLon = obj["y"].toDouble(); 
                
                QPointF snappedPos = snapToRoute(currentLon, currentLat);
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

MainWindow::~MainWindow() {}