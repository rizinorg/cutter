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

    bool isValid() const { return posA != RVA_INVALID && posB != RVA_INVALID && start() < end(); }

    bool isEmpty() const {return posA == posB;}

    bool contains(RVA address) const { return isValid() && start() <= address && address < end(); }

    RVA length() const { return isValid() ? end() - start() + 1 : 0; }

    void clear() { posA = posB = UINT64_MAX; }
};

class HexDiffCursor : public QObject{
    Q_OBJECT
public:
    HexDiffCursor(QObject *parent = nullptr):QObject(parent){}
    ~HexDiffCursor() = default;
    void setCursor(RVA addr){
        posA = addr;
        posB = addr;
        emit cursorChanged();
    }
    void startSelection(){
        posA = posB;
    }
    void updateSelection(RVA addr)
    {
        if(posB== addr){
            return;
        }
        if (posA == RVA_INVALID) {
            posA = addr;
        }
        posB = addr;
        emit cursorChanged();
    }
    void extendSelection(qint64 by){
        posB = by<0 ? posB-qAbs(by)
                      : posB + qAbs(by);
    }
    bool contains(RVA addr) const {
        const RVA start = qMin(posA, posB);
        const RVA end = qMax(posA,posB);
        return start<=addr &&
                end>=addr &&
                start!=end;
    }
    bool hasSelection() const{
        return posA != posB;
    }
    void clearSelection(){
        posA = posB;
        emit cursorChanged();
    }
    int hiddenIn(RVA a, RVA b) const {
        if(a>b){
            const RVA temp = a;
            a=b;
            b=temp;
        }
        if(a<=posB && posB<=b){
            return 0;
        }else if(posB>b){
            return 1;
        }else{
            return -1;
        }
    }

    RVA addr() const{
        return posB;
    }
    HexDiffSelection getSelection(){
        HexDiffSelection selection;
        selection.posA = posA;
        selection.posB = posB;
        return selection;
    }

private:
    //selection
    RVA posA = RVA_INVALID;
    RVA posB = RVA_INVALID;//cursorPosition
signals:
    void cursorChanged();
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

    RVA getAddressAt(qint64 byteOffset, bool orig) const;

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
    QString renderItem(qint64 byteOffset, bool orig);
    QString renderAscii(qint64 byteOffset, bool orig);
    bool getShowAscii() const { return showAscii; }

    bool diffItem(qint64 byteOffset, bool orig);
    bool diffByte(qint64 byteOffset, bool orig);

    // selection
    void beginSelection(bool orig);
    void updateSelection(qint64 byteOffset, bool orig);
    void updateSelectionBy(qint64 by, bool orig);
    void clearSelection();
    HexDiffSelection getSelectionOffsets(qint64 lastOffset, bool orig);
    HexDiffSelection getSelectionAddr(bool orig);

    // scroll and seek
    void scrollBy(int lineCount);
    void updateScrollBar();
    void onScrollBarAddressChanged(RVA value); // shall not be exposed publically
    void scrollTo(RVA addr);
    void moveCursorBy(qint64 shift, bool orig);
    void showCursor(qint64 byteOffset, bool orig);

    // cursor
    qint64 getCursorOffset() const;
    void setCursorAtOffset(qint64 offset, bool orig);
    void setCursor(RVA addr);
    //keyboard move
private:
    CutterDiff *cutterDiff = nullptr;
    HexDiffScrollBar *scrollBar;
    DiffItemSize itemSize = ItemSizeByte;
    DiffItemFormat itemFormat = ItemFormatHex;
    HexDiffCursor *cursor;
    bool littleEndian = true;

    qint64 shiftA = 0;
    qint64 shiftB = 0;
    RVA startAddress = 0;
    RVA cursorAddr = RVA_INVALID;
    ut8 powBytesPerRow = 3;

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
    void cursorChanged();
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
    void drawItem(QPainter &painter, QRectF rect, qint64 byteOffset,
                  const QPolygonF &clip = QPolygonF());
    void drawItemArea(QPainter &painter);
    void drawItemSelectionBackground(QPainter &painter, const QPolygonF &poly);
    QPolygonF getItemSelectionPolygon();
    QRectF getItemRectAtOffset(qint64 offset);
    qint64 getItemOffset(QPointF pos);
    RVA getItemAddrAtPos(QPointF pos) { return syncer->getAddressAt(getItemOffset(pos), orig); }

    QRectF asciiArea;
    void drawAsciiArea(QPainter &painter);
    void drawAsciiItem(QPainter &painter, QRectF rect, qint64 byteOffset);
    void drawAsciiSelectionBackground(QPainter &painter);
    QRectF getAsciiItemRectAtOffset(qint64 offset);
    qint64 getAsciiOffset(QPointF pos);

    void updateAreaGeometry();
    // HeaderArea
    void drawHeaderArea(QPainter &painter);

    // Updates
    void updateViewport();

    // scrollbars
    void updateScrollbarWidth();

    // Mouse
    qint64 cursorOffset = 0;
    CursorArea cursorArea = ItemArea;
    QRectF cursorRect;

    // Address
    qint64 lastVisibleOffset() { return syncer->getBytesPerRow() * visibleLines -1;}
    bool isOffsetVisible(qint64 offset) { return offset>=0 && lastVisibleOffset() >= offset; }

    // Selection
    bool mouseSelecting = false;
    bool keyboardSelecting = false;

    bool selecting() const {return mouseSelecting || keyboardSelecting;}

    // context menu
    void showContextMenu(const QPoint &pos);

    // Scroll and seek
    int accumScrollWheelDeltaY = 0;
    QTimer *autoScrollTimer;
    void autoScroll();
    // cursor
    void drawItemCursor(QPainter &painter);
    void drawAsciiCursor(QPainter &painter);
private slots:
    void onCursorUpdated();
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
};

#endif // HEXDIFFVIEW_H