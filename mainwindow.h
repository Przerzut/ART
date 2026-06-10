/**
 * @file mainwindow.h
 * @brief Plik nagłówkowy głównego okna aplikacji Analizator Tramwajowy (ART).
 * @author Kacper Rzeszut
 * @date 2026-05-21
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QListWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox> 
#include <QTranslator>
#include <QEvent>
#include <QMap>
#include <QVector>
#include <QList>
#include <QPointF>
#include <QVariantList>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

QT_CHARTS_USE_NAMESPACE

#include "trammodel.h"

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>

/**
 * @brief Struktura przechowująca podstawowe dane pozycyjne pojazdu.
 * @details Używana do kalkulacji fizycznych (obliczanie wektorów prędkości,
 * opóźnień API oraz interpolacji).
 */
struct VehicleData {
    double lon;         ///< Długość geograficzna (Longitude)
    double lat;         ///< Szerokość geograficzna (Latitude)
    qint64 timestampMs; ///< Czas rejestracji próbki w milisekundach (Unix Epoch)
};

/**
 * @class MainWindow
 * @brief Główna klasa zarządzająca interfejsem użytkownika oraz logiką biznesową.
 * @details Spina w całość warstwę widoku (QML, QtCharts) z logiką pobierania
 * i przetwarzania danych JSON z wrocławskiego API MPK.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

    /** * @property selectedTramId
     * @brief ID obecnie podglądanego pojazdu. 
     * Udostępniane do QML w celu wizualnego wyróżnienia tramwaju na mapie.
     */
    Q_PROPERTY(int selectedTramId READ selectedTramId WRITE setSelectedTramId NOTIFY selectedTramIdChanged)
    
    /** * @property routePaths
     * @brief Lista punktów torowisk dla aktywnych linii. 
     * Przekazywana do modelu QML w celu dynamicznego rysowania szarych linii szyn.
     */
    Q_PROPERTY(QVariantList routePaths READ routePaths NOTIFY routePathsChanged)

public:
    /**
     * @brief Konstruktor klasy MainWindow.
     * @param parent Wskaźnik na widget nadrzędny.
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destruktor zwalniający zasoby okna głównego.
     */
    ~MainWindow();
    
    /** @brief Zwraca ID aktualnie śledzonego tramwaju. */
    int selectedTramId() const { return currentTrackedId; }
    
    /** * @brief Ustawia ID śledzonego tramwaju i emituje odpowiedni sygnał. 
     * @param id Nowe ID tramwaju (lub -1 dla trybu AUTO).
     */
    void setSelectedTramId(int id) {
        if (currentTrackedId != id) {
            currentTrackedId = id;
            emit selectedTramIdChanged(id);
        }
    }

    /** @brief Zwraca listę tras w formacie zrozumiałym dla silnika QML. */
    QVariantList routePaths() const;

signals:
    /** @brief Sygnał emitowany po zmianie śledzonego ID pojazdu. */
    void selectedTramIdChanged(int id);
    
    /** @brief Sygnał wymuszający przerysowanie widocznych torowisk na mapie. */
    void routePathsChanged();

protected:
    /** * @brief Nasłuchuje na globalne zdarzenia systemowe aplikacji.
     * @details Używane w celu przechwycenia momentu przełączenia języka UI.
     */
    void changeEvent(QEvent *event) override;

private slots:
    void toggleTracking();                                                           ///< Włącza/wyłącza nasłuch na dane z MPK
    void fetchTramData();                                                            ///< Inicjuje zapytanie sieciowe GET/POST do API
    void onResult(QNetworkReply* reply);                                             ///< Odbiera i parsuje surowe paczki JSON
    void toggleLanguage();                                                           ///< Zmienia dynamicznie język aplikacji (PL/EN)
    void animateTrams();                                                             ///< Pętla przeliczająca interpolację co 33ms (30 FPS)
    void onTrackedTramChanged(const QString &text);                                  ///< Odbiera zmianę z UI na temat chęci śledzenia konkretnego pojazdu
    void loadRouteFromJson(const QString& lineName, const QString& filePath);        ///< Ładuje geometrię wybranej trasy z pliku .geojson
    void updateVisibleRoutes();                                                      ///< Filtruje i czyści dane na żywo po kliknięciach użytkownika

private:
    void setupUI();                                                                  ///< Buduje układ okna (przyciski, listy)
    void setupCharts();                                                              ///< Konfiguruje moduł QtCharts
    void retranslateUi();                                                            ///< Nakłada aktualne słowniki językowe na etykiety
    double calculateSpeed(double lon1, double lat1, double lon2, double lat2, qint64 timeDiffMs); ///< Liczy fizyczną prędkość pomiędzy odczytami
    QPointF snapToRoute(double lon, double lat, const QString& lineName);            ///< Rzutuje niedokładną pozycję na trasę torów
    void cleanUpStaleTrams(qint64 currentTime);                                      ///< Usuwa z pamięci "martwe" pojazdy (GC)

    QTranslator appTranslator;                   ///< Obiekt tłumaczeń Qt
    QNetworkAccessManager* networkManager;       ///< Menedżer połączeń sieciowych
    QTimer* dataTimer;                           ///< Zegar wywołujący zapytania API (co 10s)
    bool isTracking;                             ///< Flaga stanu śledzenia
    bool isPolish;                               ///< Flaga bieżącego języka
    qint64 startTime;                            ///< Znacznik czasu rozpoczęcia pracy
    int currentTrackedId;                        ///< Bieżący śledzony tramwaj
    QMap<int, QString> tramLines;                ///< Słownik przypisujący dany ID tramwaju do nazwy linii

    TramModel* m_tramModel;                      ///< Model danych pośredniczący między C++ a QML
    QQuickWidget* m_quickWidget;                 ///< Obiekt interfejsu trzymający w sobie silnik i mapę QML

    QMap<int, VehicleData> previousPositions;    ///< Pamięć ostatnich położeń dla wyliczenia prędkości
    QMap<int, QVector<double>> speedBuffers;     ///< Bufor pomiarów do redukcji szumów uśrednianiem     
    QMap<int, QList<QPointF>> speedHistories;    ///< Pełna historia prędkości danego pojazdu do wykresu  

    QTimer* animTimer;                           ///< Zegar wyzwalający poszczególne klatki animacji
    QMap<int, QPointF> currentAnimPositions;     ///< Obecnie rysowana pozycja animowanego kółka tramwaju na mapie
    QMap<int, QPointF> targetAnimPositions;      ///< Punkt docelowy, do którego kółko tramwaju "goni"  
    QMap<QString, QVector<QPointF>> routes;      ///< Wczytane do RAMu trasy wyciągnięte z plików .geojson

    // Elementy Interfejsu (Widgety)
    QPushButton* btnToggle;
    QPushButton* btnLang;
    QLabel* statusLabel;
    QLabel* filterLabel;
    QListWidget* lineFilterList;
    QLabel* tramIdLabel;       
    QComboBox* tramIdComboBox; 
    QLabel* headerLabel;
    QTextEdit* logConsole;       
    QTabWidget* mainTabs;        
    QWidget* speedChartTab;      
    QWidget* mapChartTab;        

    // Elementy Wykresów (QtCharts)
    QChart *speedChart;
    QLineSeries *speedSeries;
    QValueAxis *speedAxisX;
    QValueAxis *speedAxisY;
};

#endif // MAINWINDOW_H