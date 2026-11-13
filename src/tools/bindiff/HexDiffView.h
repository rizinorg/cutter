#ifndef HEXDIFFVIEW_H
#define HEXDIFFVIEW_H

#include "Cutter.h"
#include "HexDiffScrollBar.h"

#include <QActionGroup>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>

#include <Configuration.h>
#include <CutterDiff.h>

enum DiffItemFormat : ut8 {
    ItemFormatHex,
    ItemFormatOct,
    ItemFormatDec,
    ItemFormatSignedDec,
    ItemFormatFloat
};

enum DiffItemSize : ut8 {
    ItemSizeByte = 1,
    ItemSizeWord = 2,
    ItemSizeDword = 4,
    ItemSizeQword = 8
};
enum AddrWidth : ut8 { AddrWidth32 = 8, AddrWidth64 = 16 };

struct HexDiffSelection
{
    RVA posA = RVA_INVALID;
    RVA posB = RVA_INVALID;

    RVA start() const { return qMin(posA, posB); }

    RVA end() const { return qMax(posA, posB); }

    bool isValid() const { return posA != UINT64_MAX && posB != UINT64_MAX && start() < end(); }

    bool contains(RVA offset) const { return isValid() && start() <= offset && offset < end(); }

    RVA length() const { return isValid() ? end() - start() + 1 : 0; }

    void clear() { posA = posB = UINT64_MAX; }
};

class HexDiffViewSyncer : public QObject
{
    Q_OBJECT
public:
    explicit HexDiffViewSyncer(CutterDiff *cutterDiff, HexDiffScrollBar *scrollbar = nullptr,
                               QObject *parent = nullptr);
    ~HexDiffViewSyncer() = default;

    DiffItemSize getItemSize();
    void setItemFormat(DiffItemFormat format);
    void setItemSize(DiffItemSize size);
    DiffItemFormat getItemFormat() const { return itemFormat; }
    int getItemLen() const;
    int getGroupedItemLen() const;
    int getGroupedItemSize() { return itemSize * (getGroupItems() ? 2 : 1); }

    ut8 getItemsPerRow() const;
    ut8 getBytesPerRow() const;
    ut8 getGroupedItemsPerRow() const { return getItemsPerRow() / (getGroupItems() ? 2 : 1); }
    void setPowBytesPerRow(ut8 pow);

    bool isLittleEndian() const;
    void setLittleEndian(bool little);

    RVA getAddressAt(quint64 byteOffset, bool orig) const;

    void setShiftA(int shift);
    void setShiftB(int shift);

    bool getGroupItems() const
    {
        return groupItems && itemSize == ItemSizeByte && itemFormat == ItemFormatHex;
    }
    void setGroupItems(bool group);

    int getAddrLen() const { return addrCharLen + (showExAddr ? 2 : 0); }
    int getAddrCharLen() const { return addrCharLen; }
    bool getShowExAddr() const { return showExAddr; }

    bool getHeaderEnabled() const { return headerEnabled; }
    QString renderItem(quint64 byteOffset, bool orig);
    QString renderAscii(quint64 byteOffset, bool orig);
    bool getShowAscii() const { return showAscii; }

    bool diffItem(quint64 byteOffset, bool orig);
    bool diffByte(quint64 byteOffset, bool orig);

    // selection
    void beginSelection(quint64 offset, bool orig);
    void updateSelection(quint64 offset, bool orig);
    void clearSelection();
    HexDiffSelection getSelectionOffsets(quint64 lastAddress, bool orig);

    // scroll and seek
    void scrollBy(int lineCount);
    void updateScrollBar();
    void onScrollBarAddressChanged(RVA value); // shall not be exposed publically
    void scrollTo(RVA addr);

    // cursor
    quint64 getCursorOffset();
    void setCursor(quint64 offset, bool orig);

private:
    CutterDiff *cutterDiff = nullptr;
    HexDiffScrollBar *scrollBar;
    DiffItemSize itemSize = ItemSizeByte;
    DiffItemFormat itemFormat = ItemFormatHex;
    bool littleEndian = true;

    qint64 shiftA = 0;
    qint64 shiftB = 0;
    RVA startAddress = 0;
    RVA cursorAddr = RVA_INVALID;
    ut8 powBytesPerRow = 5;

    bool groupItems = false;
    bool showExAddr = true;
    int addrCharLen = AddrWidth64;
    bool headerEnabled = true;
    bool showAscii = true;

    HexDiffSelection selection;
    // cursor
    bool cursorOwnedByA = true;
    RVA shifted(RVA addr, qint64 shift) const;
signals:
    void refreshView();
    void selectionChanged();
};

class HexDiffView : public QScrollArea
{
    Q_OBJECT
public:
    explicit HexDiffView(HexDiffViewSyncer *syncer, bool orig, QWidget *parent);
    ~HexDiffView() = default;

private:
    HexDiffViewSyncer *syncer = nullptr;
    bool orig = true;

private:
    enum CursorArea : ut8 { ItemArea, AsciiArea };
    // Colors
    QColor backgroundColor;
    QColor borderColor;
    QColor addrColor;
    QColor defColor;
    QColor diffColor;
    void updateColors();

    // Metrics determined by the font
    QFont monospaceFont;
    void setMonospaceFont();
    qreal charWidth;
    qreal lineHeight;
    int visibleLines = 10;
    // Gaps
    double rowGap = .2;
    double columnGap = 1;
    int areaGap = 3;
    qreal getAreaSpacing() const { return areaGap * charWidth; }
    qreal getRowSpacing() const { return rowGap * lineHeight; }
    qreal getColumnSpacing() const { return columnGap * charWidth; }
    // Area
    QRectF addrArea;
    void drawAddressItem(QPainter &painter, const QRectF &rect, RVA addr);
    void drawAddressArea(QPainter &painter);

    QRectF itemArea;
    void drawItem(QPainter &painter, QRectF rect, quint64 byteOffset,
                  const QPolygonF &clip = QPolygonF());
    void drawItemArea(QPainter &painter);
    void drawItemSelectionBackground(QPainter &painter, const QPolygonF &poly);
    QPolygonF getItemSelectionPolygon();
    QRectF getItemRectAtOffset(quint64 offset);
    quint64 getItemOffset(QPointF pos);
    RVA getItemAddrAtPos(QPointF pos) { return syncer->getAddressAt(getItemOffset(pos), orig); }

    QRectF asciiArea;
    void drawAsciiArea(QPainter &painter);
    void drawAsciiItem(QPainter &painter, QRectF rect, quint64 byteOffset);
    void drawAsciiSelectionBackground(QPainter &painter);
    QRectF getAsciiItemRectAtOffset(quint64 offset);
    quint64 getAsciiOffset(QPointF pos);
    RVA getAsciiAddrAtPos(QPointF pos) { return syncer->getAddressAt(getAsciiOffset(pos), orig); }

    void updateAreaGeometry();
    // HeaderArea
    void drawHeaderArea(QPainter &painter);

    // Updates
    void updateViewport();

    // scrollbars
    void updateScrollbarWidth();

    // Mouse
    quint64 cursorOffset = 0;
    CursorArea cursorArea = ItemArea;
    QRectF cursorRect;

    // Address
    RVA lastVisibleOffset() { return syncer->getBytesPerRow() * visibleLines; }
    bool isOffsetVisible(quint64 offset) { return lastVisibleOffset() >= offset; }

    // Selection
    bool selecting = false;

    // context menu
    void showContextMenu(const QPoint &pos);

    // Scroll and seek
    int accumScrollWheelDeltaY = 0;
    QTimer *autoScrollTimer;
    void autoScroll();
    // cursor
    void drawItemCursor(QPainter &painter);
    void drawAsciiCursor(QPainter &painter);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
};

#endif // HEXDIFFVIEW_H