/**
 * @file mainwindow.h
 * @brief Plik nagłówkowy głównego okna aplikacji Analizator Tramwajowy (ART).
 * @author Kacper Rzeszut
 * @date 2026-05-12
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

// Biblioteki Qt Charts do wizualizacji danych
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

/**
 * @struct VehicleData
 * @brief Struktura przechowująca stan pojazdu w danym punkcie czasu.
 */
struct VehicleData {
    double lon;         /**< Długość geograficzna (Longitude). */
    double lat;         /**< Szerokość geograficzna (Latitude). */
    qint64 timestampMs; /**< Czas systemowy zapisu danych w ms. */
};

/**
 * @class MainWindow
 * @brief Główna klasa interfejsu i logiki biznesowej.
 * * Klasa zarządza komunikacją z API MPK Wrocław, przetwarza dane GPS, 
 * oblicza parametry ruchu (prędkość) oraz odpowiada za rendering mapy i wykresów.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor głównego okna.
     * @param parent Wskaźnik na obiekt nadrzędny (domyślnie nullptr).
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destruktor klasy MainWindow.
     */
    ~MainWindow();

protected:
    /**
     * @brief Reaguje na zmiany systemowe, w tym zmianę języka aplikacji.
     * @param event Wskaźnik na obiekt zdarzenia.
     */
    void changeEvent(QEvent *event) override;

private slots:
    /** @brief Przełącza stan monitorowania (Start/Stop). */
    void toggleTracking();
    
    /** @brief Wysyła żądanie sieciowe do API MPK. */
    void fetchTramData();
    
    /** @brief Obsługuje odpowiedź JSON z serwera. @param reply Wskaźnik na odpowiedź sieciową. */
    void onResult(QNetworkReply* reply);
    
    /** @brief Przełącza język interfejsu (PL/EN). */
    void toggleLanguage(); 
    
    /** @brief Realizuje interpolację pozycji tramwajów na mapie (animacja). */
    void animateTrams();
    
    /** @brief Obsługuje zmianę wybranego pojazdu w ComboBox. @param text Wybrany identyfikator lub "AUTO". */
    void onTrackedTramChanged(const QString &text); 

private:
    /** @brief Inicjalizuje komponenty interfejsu użytkownika. */
    void setupUI();
    
    /** @brief Konfiguruje wykresy prędkości i mapę 2D. */
    void setupCharts(); 
    
    /** @brief Aktualizuje teksty we wszystkich widżetach po zmianie języka. */
    void retranslateUi();
    
    /** * @brief Oblicza prędkość na podstawie wzoru Haversine'a.
     * @return Prędkość w jednostkach km/h.
     */
    double calculateSpeed(double lon1, double lat1, double lon2, double lat2, qint64 timeDiffMs);
    
    /** @brief Definiuje współrzędne hardkodowanej trasy linii 16. */
    void initHardcodedRoute();                
    
    /** @brief Rzutuje punkt GPS na najbliższy segment trasy. @return Punkt skorygowany. */
    QPointF snapToRoute(double lon, double lat); 
    
    /** @brief Usuwa pojazdy, które nie wysłały danych przez określony czas. */
    void cleanUpStaleTrams(qint64 currentTime);

    QTranslator appTranslator;
    QNetworkAccessManager* networkManager;
    QTimer* dataTimer;
    bool isTracking;
    bool isPolish;
    qint64 startTime;
    int currentTrackedId; 

    QMap<int, VehicleData> previousPositions;
    QMap<int, QVector<double>> speedBuffers;     
    QMap<int, QList<QPointF>> speedHistories;    

    QTimer* animTimer; 
    QMap<int, QPointF> currentAnimPositions; 
    QMap<int, QPointF> targetAnimPositions;  
    QVector<QPointF> routePoints; 

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

    QChart *speedChart;
    QLineSeries *speedSeries;
    QValueAxis *speedAxisX;
    QValueAxis *speedAxisY;

    QChart *mapChart;
    QScatterSeries *mapSeries;
    QScatterSeries *selectedMapSeries; 
    QLineSeries *routeSeries; 
    QValueAxis *mapAxisX;
    QValueAxis *mapAxisY;
};

#endif // MAINWINDOW_H