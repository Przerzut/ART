/**
 * @file trammodel.h
 * @brief Plik nagłówkowy modelu danych dla tramwajów renderowanych w QML.
 * @author Kacper Rzeszut
 * @date 2026-05-21
 */

#ifndef TRAMMODEL_H
#define TRAMMODEL_H

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QList>
#include <QString>

/**
 * @brief Struktura przechowująca pojedynczy rekord o tramwaju na mapie.
 * @details Zawiera wszystkie informacje niezbędne do fizycznego i graficznego
 * odwzorowania ikony pojazdu w przestrzeni (pozycja, prędkość, zwrot).
 */
struct TramInfo {
    int id;                    ///< Unikalny identyfikator tramwaju z API MPK.
    QString line;              ///< Numer linii tramwajowej (np. "16").
    QGeoCoordinate coordinate; ///< Współrzędne GPS (Latitude, Longitude) na mapie.
    double speed;              ///< Wygładzona prędkość pojazdu w km/h.
    double heading;            ///< Kąt obrotu przodu tramwaju względem północy (w stopniach).
};

/**
 * @class TramModel
 * @brief Klasa dziedzicząca po QAbstractListModel, pełniąca rolę dostawcy danych dla QML.
 * @details Pozwala silnikowi QtQuick (QML) w sposób optymalny i płynny 
 * renderować, dodawać, usuwać i odświeżać klatki animacji wielu tramwajów jednocześnie 
 * z użyciem komponentu MapItemView.
 */
class TramModel : public QAbstractListModel {
    Q_OBJECT

public:
    /**
     * @enum TramRoles
     * @brief Definiuje wewnętrzne role (indeksy) dla poszczególnych zmiennych struktury TramInfo.
     * @details Używane wewnątrz metody `data()` oraz `roleNames()` do tłumaczenia
     * C++ na właściwości, które potrafi odczytać JavaScript/QML.
     */
    enum TramRoles {
        IdRole = Qt::UserRole + 1, ///< Rola dla ID tramwaju (mapowana na "tramId")
        LineRole,                  ///< Rola dla numeru linii (mapowana na "tramLine")
        CoordinateRole,            ///< Rola dla koordynatów (mapowana na "tramCoordinate")
        SpeedRole,                 ///< Rola dla prędkości (mapowana na "tramSpeed")
        HeadingRole                ///< Rola dla kąta obrotu (mapowana na "tramHeading")
    };

    /**
     * @brief Konstruktor modelu tramwajów.
     * @param parent Wskaźnik na obiekt nadrzędny.
     */
    explicit TramModel(QObject *parent = nullptr);

    /** @brief Zwraca liczbę aktualnie śledzonych pojazdów. */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    
    /** @brief Dostarcza QML-owi żądaną zmienną dla określonego rzędu i roli. */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    
    /** @brief Konfiguruje listę słów kluczowych (np. "tramHeading") widocznych po stronie QML. */
    QHash<int, QByteArray> roleNames() const override;

    /** @brief Dodaje nowy tramwaj lub płynnie odświeża parametry już istniejącego. */
    void updateTram(int id, const QString& line, double lat, double lon, double speed, double heading);
    
    /** @brief Bezpiecznie usuwa tramwaj z modelu (i tym samym znika on z widoku mapy). */
    void removeTram(int id);

private:
    QList<TramInfo> m_trams;   ///< Główna lista przechowująca wszystkie aktywne tramwaje w RAM.
    
    /** @brief Pomocnicza funkcja do szybkiego znalezienia indeksu tramwaju o podanym ID. */
    int findTramIndex(int id) const;
};

#endif // TRAMMODEL_H