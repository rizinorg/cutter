#ifndef VISUALNAVBAR_H
#define VISUALNAVBAR_H

#include <QToolBar>
#include <QGraphicsScene>

#include "Cutter.h"

class MainWindow;
class QGraphicsView;

class VisualNavbar : public QToolBar
{
    Q_OBJECT

    struct xToAddress {
        double x_start;
        double x_end;
        RVA address_from;
        RVA address_to;
    };

    struct MappedSegmentMetadata {
        RVA address;
        RVA size;
    };

    struct MappedSegment {
        QList<SectionDescription> sectionDescriptions;
        QList<MappedSegmentMetadata> functions;
        QList<MappedSegmentMetadata> symbols;
        QList<MappedSegmentMetadata> strings;
        RVA address_from;
        RVA address_to;
    };

public:
    explicit VisualNavbar(MainWindow *main, QWidget *parent = nullptr);

public slots:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void fetchAndPaintData();
    void fetchData();
    void updateMetadataAndPaint();
    void updateMetadata();
    void fillData();
    void drawCursor();
    void on_seekChanged(RVA addr);

private:
    QGraphicsView     *graphicsView;
    QGraphicsScene    *graphicsScene;
    QGraphicsRectItem *cursorGraphicsItem;
    MainWindow        *main;
    RVA totalMappedSize;
    QList<SectionDescription> sections;
    QList<struct xToAddress> xToAddress;

    QList<MappedSegment> mappedSegments;

    // Used to check whether the width changed. If yes we need to re-initialize the scene (slow)
    int previousWidth;
    void drawMetadata(QList<MappedSegmentMetadata> metadata,
                      RVA offset,
                      double x,
                      double width_per_byte,
                      double h, QColor color);

    struct MappedSegment *mappedSegmentForAddress(RVA addr);
    RVA localXToAddress(double x);
    double addressToLocalX(RVA address);
    QList<QString> sectionsForAddress(RVA address);
    QString toolTipForAddress(RVA address);

    static bool sortSectionLessThan(const SectionDescription &section1,
                                    const SectionDescription &section2);


    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // VISUALNAVBAR_H
